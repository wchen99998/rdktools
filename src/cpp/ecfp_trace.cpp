#include "ecfp_trace.hpp"

#include <DataStructs/ExplicitBitVect.h>
#include <GraphMol/Descriptors/MolDescriptors.h>
#include <GraphMol/Fingerprints/FingerprintGenerator.h>
#include <GraphMol/Fingerprints/MorganFingerprints.h>
#include <GraphMol/Fingerprints/MorganGenerator.h>
#include <GraphMol/MolOps.h>
#include <GraphMol/RingInfo.h>
#include <GraphMol/SmilesParse/SmilesParse.h>
#include <GraphMol/SmilesParse/SmartsWrite.h>
#include <GraphMol/Subgraphs/Subgraphs.h>
#include <RDGeneral/RDLog.h>
#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace {

using BitInfoMap = RDKit::AdditionalOutput::bitInfoMapType;
using CenterRadiusPair = std::pair<unsigned int, unsigned int>;

struct TokenMetrics {
    unsigned int radius = 0;
    unsigned int numAtoms = 0;
    unsigned int numBonds = 0;
    unsigned int hasRing = 0;
    unsigned int numHetero = 0;
    unsigned int hasUnsat = 0;
    std::string token;
};

constexpr const char* kCountSeparator = "\xC3\x97";
constexpr const char* kChainArrow = " \xE2\x86\x92 ";

unsigned int token_radius(const std::string& token) {
    if (token.size() < 3 || token[0] != 'r') {
        return 0;
    }
    const auto pos = token.find(':');
    if (pos == std::string::npos || pos <= 1) {
        return 0;
    }
    try {
        return static_cast<unsigned int>(std::stoul(token.substr(1, pos - 1)));
    } catch (const std::exception&) {
        return 0;
    }
}

std::string token_smarts(const std::string& token) {
    const auto pos = token.find(':');
    if (pos == std::string::npos) {
        return token;
    }
    return token.substr(pos + 1);
}

TokenMetrics compute_metrics(const std::string& token) {
    TokenMetrics metrics;
    metrics.radius = token_radius(token);
    metrics.token = token;

    const std::string smarts = token_smarts(token);
    std::unique_ptr<RDKit::RWMol> query(RDKit::SmartsToMol(smarts));
    if (!query) {
        return metrics;
    }

    try {
        RDKit::MolOps::fastFindRings(*query);
    } catch (const std::exception&) {
    }

    metrics.numAtoms = query->getNumAtoms();
    metrics.numBonds = query->getNumBonds();
    metrics.hasRing = query->getRingInfo()->numRings() > 0 ? 1 : 0;

    unsigned int hetero = 0;
    for (const auto atom : query->atoms()) {
        const auto atomicNum = atom->getAtomicNum();
        if (atomicNum != 6 && atomicNum != 1) {
            ++hetero;
        }
    }
    metrics.numHetero = hetero;

    bool unsaturated = false;
    for (const auto bond : query->bonds()) {
        const auto btype = bond->getBondType();
        if (btype == RDKit::Bond::DOUBLE || btype == RDKit::Bond::TRIPLE ||
            btype == RDKit::Bond::AROMATIC) {
            unsaturated = true;
            break;
        }
    }
    metrics.hasUnsat = unsaturated ? 1U : 0U;

    return metrics;
}

TokenMetrics token_metrics(const std::string& token) {
    static std::mutex cache_mutex;
    static std::unordered_map<std::string, TokenMetrics> cache;

    {
        std::lock_guard<std::mutex> guard(cache_mutex);
        auto it = cache.find(token);
        if (it != cache.end()) {
            return it->second;
        }
    }

    TokenMetrics computed = compute_metrics(token);
    {
        std::lock_guard<std::mutex> guard(cache_mutex);
        auto [it, _] = cache.emplace(token, computed);
        return it->second;
    }
}

auto complexity_key(const std::string& token) {
    const TokenMetrics metrics = token_metrics(token);
    return std::make_tuple(metrics.radius, metrics.numAtoms, metrics.numBonds,
                           metrics.hasRing, metrics.numHetero,
                           metrics.hasUnsat, metrics.token);
}

std::string join_lines(const std::vector<std::string>& lines) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) {
            oss << '\n';
        }
        oss << lines[i];
    }
    return oss.str();
}

std::string join_with(const std::vector<std::string>& items,
                      const std::string& separator) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i != 0) {
            oss << separator;
        }
        oss << items[i];
    }
    return oss.str();
}

BitInfoMap collect_morgan_bitinfo(const RDKit::ROMol& mol, unsigned int radius,
                                  bool includeChirality) {
    RDKit::AdditionalOutput additionalOutput;
    additionalOutput.allocateBitInfoMap();

    auto generator = std::unique_ptr<RDKit::FingerprintGenerator<std::uint64_t>>(
        RDKit::MorganFingerprint::getMorganGenerator<std::uint64_t>(
            radius, true, includeChirality, true));

    auto fingerprint =
        generator->getFingerprint(mol, nullptr, nullptr, -1, &additionalOutput);

    (void)fingerprint;
    return additionalOutput.bitInfoMap ? *additionalOutput.bitInfoMap
                                       : BitInfoMap{};
}

std::vector<std::uint8_t> compute_morgan_fingerprint_bits(
    const RDKit::ROMol& mol,
    unsigned int radius,
    bool includeChirality) {
    std::vector<std::uint8_t> bits(
        rdktools::kECFPReasoningFingerprintSize, 0);
    try {
        std::unique_ptr<::ExplicitBitVect> fp(
            RDKit::MorganFingerprints::getFingerprintAsBitVect(
                mol,
                radius,
                static_cast<unsigned int>(
                    rdktools::kECFPReasoningFingerprintSize),
                nullptr,
                nullptr,
                includeChirality,
                true,
                false,
                nullptr));
        if (!fp) {
            return bits;
        }

        for (unsigned int idx = 0;
             idx < static_cast<unsigned int>(
                       rdktools::kECFPReasoningFingerprintSize);
             ++idx) {
            bits[idx] = fp->getBit(idx) ? 1U : 0U;
        }
    } catch (const std::exception&) {
        // Leave bits zeroed on failure.
    }
    return bits;
}

std::map<unsigned int, std::map<unsigned int, std::string>>
ecfp_env_tokens_by_center(const RDKit::ROMol& source, unsigned int radius,
                          bool isomeric, bool kekulize,
                          bool include_radius_tag, bool mark_root) {
    RDKit::RWMol mol(source);
    if (kekulize) {
        try {
            RDKit::MolOps::Kekulize(mol);
        } catch (const RDKit::MolSanitizeException&) {
        } catch (const RDKit::KekulizeException&) {
        }
    }

    BitInfoMap bitInfo = collect_morgan_bitinfo(mol, radius, isomeric);
    std::set<CenterRadiusPair> pairs;
    for (const auto& entry : bitInfo) {
        for (const auto& occurrence : entry.second) {
            if (occurrence.second <= radius) {
                pairs.emplace(occurrence.first, occurrence.second);
            }
        }
    }

    std::vector<int> originalMapNums;
    originalMapNums.reserve(mol.getNumAtoms());
    for (const auto atom : mol.atoms()) {
        originalMapNums.push_back(atom->getAtomMapNum());
    }

    std::map<unsigned int, std::map<unsigned int, std::string>> perCenter;
    for (const auto& pr : pairs) {
        const unsigned int center = pr.first;
        const unsigned int layer = pr.second;

        std::vector<int> bondIndices =
            RDKit::findAtomEnvironmentOfRadiusN(mol, layer, center);

        std::set<int> atomSet;
        atomSet.insert(static_cast<int>(center));
        for (const int bidx : bondIndices) {
            const auto bond = mol.getBondWithIdx(bidx);
            atomSet.insert(static_cast<int>(bond->getBeginAtomIdx()));
            atomSet.insert(static_cast<int>(bond->getEndAtomIdx()));
        }

        std::vector<int> atomList(atomSet.begin(), atomSet.end());
        std::sort(atomList.begin(), atomList.end());

        if (mark_root) {
            for (std::size_t idx = 0; idx < originalMapNums.size(); ++idx) {
                mol.getAtomWithIdx(static_cast<unsigned int>(idx))
                    ->setAtomMapNum(0);
            }
            mol.getAtomWithIdx(center)->setAtomMapNum(1);
        }

        const std::vector<int>* bondPtr =
            bondIndices.empty() ? nullptr : &bondIndices;

        std::string smarts =
            RDKit::MolFragmentToSmarts(mol, atomList, bondPtr, isomeric);

        if (mark_root) {
            for (std::size_t idx = 0; idx < originalMapNums.size(); ++idx) {
                mol.getAtomWithIdx(static_cast<unsigned int>(idx))
                    ->setAtomMapNum(originalMapNums[idx]);
            }
        }

        std::ostringstream oss;
        if (include_radius_tag) {
            oss << 'r' << layer << ':' << smarts;
        } else {
            oss << smarts;
        }

        perCenter[center][layer] = oss.str();
    }

    return perCenter;
}

std::unique_ptr<RDKit::ROMol> smiles_to_mol(const std::string& smiles) {
    try {
        return std::unique_ptr<RDKit::ROMol>(RDKit::SmilesToMol(smiles));
    } catch (const std::exception&) {
        return nullptr;
    }
}

} // namespace

namespace rdktools {

ReasoningTraceResult ecfp_reasoning_trace_from_smiles(
    const std::string& smiles,
    unsigned int radius,
    bool isomeric,
    bool kekulize,
    bool include_per_center) {
    std::vector<std::uint8_t> fingerprint(
        rdktools::kECFPReasoningFingerprintSize,
        static_cast<std::uint8_t>(0));

    auto mol = smiles_to_mol(smiles);
    if (!mol) {
        return {std::string(), std::move(fingerprint)};
    }

    const auto per_center = ecfp_env_tokens_by_center(
        *mol, radius, isomeric, kekulize, true, true);
    fingerprint = compute_morgan_fingerprint_bits(
        *mol, radius, isomeric);

    std::map<unsigned int, std::map<std::string, unsigned int>> by_radius;
    for (const auto& center_entry : per_center) {
        for (const auto& layer_entry : center_entry.second) {
            by_radius[layer_entry.first][layer_entry.second] += 1;
        }
    }

    std::vector<std::string> lines;
    for (const auto& radius_entry : by_radius) {
        std::vector<std::pair<std::string, unsigned int>> tokens(
            radius_entry.second.begin(), radius_entry.second.end());
        std::sort(tokens.begin(), tokens.end(),
                  [](const auto& lhs, const auto& rhs) {
                      return complexity_key(lhs.first) <
                             complexity_key(rhs.first);
                  });

        std::vector<std::string> pieces;
        pieces.reserve(tokens.size());
        for (const auto& token_count : tokens) {
            std::ostringstream oss;
            oss << token_count.first << kCountSeparator << token_count.second;
            pieces.push_back(oss.str());
        }

        std::ostringstream line;
        line << 'r' << radius_entry.first << ": " << join_with(pieces, ", ");
        lines.push_back(line.str());
    }

    if (include_per_center && !per_center.empty()) {
        lines.emplace_back("");
        lines.emplace_back("# per-center chains");

        for (const auto& center_entry : per_center) {
            const unsigned int atom_idx = center_entry.first;
            const auto atom = mol->getAtomWithIdx(atom_idx);

            std::vector<std::pair<unsigned int, std::string>> chain(
                center_entry.second.begin(), center_entry.second.end());
            std::sort(chain.begin(), chain.end(),
                      [](const auto& lhs, const auto& rhs) {
                          return lhs.first < rhs.first;
                      });

            std::vector<std::string> tokens;
            tokens.reserve(chain.size());
            for (const auto& item : chain) {
                tokens.push_back(item.second);
            }

            std::ostringstream line;
            line << atom->getSymbol() << atom_idx << ": "
                 << join_with(tokens, kChainArrow);
            lines.push_back(line.str());
        }
    }

    return {join_lines(lines), std::move(fingerprint)};
}

} // namespace rdktools
