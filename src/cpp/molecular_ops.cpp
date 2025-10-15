#include "molecular_ops.hpp"
#include "ecfp_trace.hpp"
#include <GraphMol/Descriptors/MolDescriptors.h>
#include <GraphMol/Fingerprints/MorganFingerprints.h>
#include <GraphMol/FileParsers/MolSupplier.h>
#include <GraphMol/MolOps.h>
#include <GraphMol/SmilesParse/SmilesParse.h>
#include <GraphMol/SmilesParse/SmilesWrite.h>
#include <RDGeneral/RDLog.h>
#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>

namespace rdktools {

namespace nb = nanobind;

// Helper function to create molecule from SMILES
std::unique_ptr<RDKit::ROMol> smiles_to_mol(const std::string& smiles) {
    try {
        std::unique_ptr<RDKit::ROMol> mol(RDKit::SmilesToMol(smiles));
        return mol;
    } catch (const std::exception& e) {
        return nullptr;
    }
}

nb::ndarray<nb::numpy, double> calculate_molecular_weights(const std::vector<std::string>& smiles_list) {
    size_t size = smiles_list.size();
    
    // Allocate memory for result
    double* data = new double[size];
    
    // Process each SMILES
    for (size_t i = 0; i < size; ++i) {
        auto mol = smiles_to_mol(smiles_list[i]);
        if (mol) {
            data[i] = RDKit::Descriptors::calcAMW(*mol);
        } else {
            data[i] = std::numeric_limits<double>::quiet_NaN();
        }
    }
    
    // Create nanobind capsule for memory management
    nb::capsule owner(data, [](void *p) noexcept {
        delete[] static_cast<double*>(p);
    });
    
    // Create nanobind ndarray
    return nb::ndarray<nb::numpy, double>(data, {size}, owner);
}

nb::ndarray<nb::numpy, double> calculate_logp(const std::vector<std::string>& smiles_list) {
    size_t size = smiles_list.size();
    
    double* data = new double[size];
    
    for (size_t i = 0; i < size; ++i) {
        auto mol = smiles_to_mol(smiles_list[i]);
        if (mol) {
            data[i] = RDKit::Descriptors::calcClogP(*mol);
        } else {
            data[i] = std::numeric_limits<double>::quiet_NaN();
        }
    }
    
    nb::capsule owner(data, [](void *p) noexcept {
        delete[] static_cast<double*>(p);
    });
    
    return nb::ndarray<nb::numpy, double>(data, {size}, owner);
}

nb::ndarray<nb::numpy, double> calculate_tpsa(const std::vector<std::string>& smiles_list) {
    size_t size = smiles_list.size();
    
    double* data = new double[size];
    
    for (size_t i = 0; i < size; ++i) {
        auto mol = smiles_to_mol(smiles_list[i]);
        if (mol) {
            data[i] = RDKit::Descriptors::calcTPSA(*mol);
        } else {
            data[i] = std::numeric_limits<double>::quiet_NaN();
        }
    }
    
    nb::capsule owner(data, [](void *p) noexcept {
        delete[] static_cast<double*>(p);
    });
    
    return nb::ndarray<nb::numpy, double>(data, {size}, owner);
}

nb::ndarray<nb::numpy, bool> validate_smiles(const std::vector<std::string>& smiles_list) {
    size_t size = smiles_list.size();
    
    bool* data = new bool[size];
    
    for (size_t i = 0; i < size; ++i) {
        auto mol = smiles_to_mol(smiles_list[i]);
        data[i] = (mol != nullptr);
    }
    
    nb::capsule owner(data, [](void *p) noexcept {
        delete[] static_cast<bool*>(p);
    });
    
    return nb::ndarray<nb::numpy, bool>(data, {size}, owner);
}

nb::dict calculate_multiple_descriptors(const std::vector<std::string>& smiles_list) {
    size_t size = smiles_list.size();
    
    // Allocate memory for arrays
    double* mw_data = new double[size];
    double* logp_data = new double[size];
    double* tpsa_data = new double[size];
    
    // Process each SMILES once and calculate all descriptors
    for (size_t i = 0; i < size; ++i) {
        auto mol = smiles_to_mol(smiles_list[i]);
        if (mol) {
            mw_data[i] = RDKit::Descriptors::calcAMW(*mol);
            logp_data[i] = RDKit::Descriptors::calcClogP(*mol);
            tpsa_data[i] = RDKit::Descriptors::calcTPSA(*mol);
        } else {
            mw_data[i] = std::numeric_limits<double>::quiet_NaN();
            logp_data[i] = std::numeric_limits<double>::quiet_NaN();
            tpsa_data[i] = std::numeric_limits<double>::quiet_NaN();
        }
    }
    
    // Create capsules for memory management
    nb::capsule mw_owner(mw_data, [](void *p) noexcept { delete[] static_cast<double*>(p); });
    nb::capsule logp_owner(logp_data, [](void *p) noexcept { delete[] static_cast<double*>(p); });
    nb::capsule tpsa_owner(tpsa_data, [](void *p) noexcept { delete[] static_cast<double*>(p); });
    
    // Create arrays
    auto mw_result = nb::ndarray<nb::numpy, double>(mw_data, {size}, mw_owner);
    auto logp_result = nb::ndarray<nb::numpy, double>(logp_data, {size}, logp_owner);
    auto tpsa_result = nb::ndarray<nb::numpy, double>(tpsa_data, {size}, tpsa_owner);
    
    nb::dict result;
    result["molecular_weight"] = mw_result;
    result["logp"] = logp_result;
    result["tpsa"] = tpsa_result;
    
    return result;
}

std::vector<std::string> canonicalize_smiles(const std::vector<std::string>& smiles_list) {
    std::vector<std::string> result;
    result.reserve(smiles_list.size());
    
    for (const auto& smiles : smiles_list) {
        auto mol = smiles_to_mol(smiles);
        if (mol) {
            result.push_back(RDKit::MolToSmiles(*mol));
        } else {
            result.push_back("");
        }
    }
    
    return result;
}

nb::ndarray<nb::numpy, uint8_t> calculate_morgan_fingerprints(
    const std::vector<std::string>& smiles_list,
    int radius,
    int nbits
) {
    size_t size = smiles_list.size();
    
    // Allocate memory for 2D array (size x nbits)
    uint8_t* data = new uint8_t[size * nbits];
    
    for (size_t i = 0; i < size; ++i) {
        auto mol = smiles_to_mol(smiles_list[i]);
        if (mol) {
            try {
                auto fp = RDKit::MorganFingerprints::getFingerprintAsBitVect(*mol, radius, nbits);
                
                // Copy bits to result array
                for (int j = 0; j < nbits; ++j) {
                    data[i * nbits + j] = fp->getBit(j) ? 1 : 0;
                }
            } catch (const std::exception& e) {
                // On error, set all bits to 0
                for (int j = 0; j < nbits; ++j) {
                    data[i * nbits + j] = 0;
                }
            }
        } else {
            // Invalid SMILES, set all bits to 0
            for (int j = 0; j < nbits; ++j) {
                data[i * nbits + j] = 0;
            }
        }
    }
    
    nb::capsule owner(data, [](void *p) noexcept {
        delete[] static_cast<uint8_t*>(p);
    });
    
    // Create 2D nanobind ndarray
    return nb::ndarray<nb::numpy, uint8_t>(data, {size, static_cast<size_t>(nbits)}, owner);
}

nb::tuple ecfp_reasoning_trace(const std::string& smiles,
                               int radius,
                               bool isomeric,
                               bool kekulize,
                               bool include_per_center,
                               int fingerprint_size) {
    const unsigned int fp_radius =
        radius < 0 ? 0U : static_cast<unsigned int>(radius);
    const std::size_t fp_bits =
        fingerprint_size <= 0
            ? kECFPReasoningFingerprintSize
            : static_cast<std::size_t>(fingerprint_size);
    auto trace_result = ecfp_reasoning_trace_from_smiles(
        smiles, fp_radius, isomeric, kekulize, include_per_center, fp_bits);
    std::string trace = std::move(std::get<0>(trace_result));
    std::vector<std::uint8_t> fingerprint =
        std::move(std::get<1>(trace_result));

    const std::size_t fp_size = fingerprint.size();
    uint8_t* data = new uint8_t[fp_size];
    std::copy(fingerprint.begin(), fingerprint.end(), data);
    nb::capsule owner(data, [](void* p) noexcept {
        delete[] static_cast<uint8_t*>(p);
    });
    auto fingerprint_array =
        nb::ndarray<nb::numpy, uint8_t>(data, {fp_size}, owner);

    return nb::make_tuple(std::move(trace), std::move(fingerprint_array));
}

} // namespace rdktools
