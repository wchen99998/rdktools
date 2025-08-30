#include "molecular_ops.hpp"
#include <GraphMol/SmilesParse/SmilesParse.h>
#include <GraphMol/SmilesParse/SmilesWrite.h>
#include <GraphMol/Descriptors/MolDescriptors.h>
#include <GraphMol/Fingerprints/MorganFingerprints.h>
#include <GraphMol/FileParsers/MolSupplier.h>
#include <RDGeneral/RDLog.h>
#include <iostream>
#include <stdexcept>

namespace rdtools {

namespace py = pybind11;

// Helper function to create molecule from SMILES
std::unique_ptr<RDKit::ROMol> smiles_to_mol(const std::string& smiles) {
    try {
        std::unique_ptr<RDKit::ROMol> mol(RDKit::SmilesToMol(smiles));
        return mol;
    } catch (const std::exception& e) {
        return nullptr;
    }
}

py::array_t<double> calculate_molecular_weights(const std::vector<std::string>& smiles_list) {
    size_t size = smiles_list.size();
    
    // Create output array
    auto result = py::array_t<double>(size);
    auto result_buf = result.request();
    auto* result_ptr = static_cast<double*>(result_buf.ptr);
    
    // Process each SMILES
    for (size_t i = 0; i < size; ++i) {
        auto mol = smiles_to_mol(smiles_list[i]);
        if (mol) {
            result_ptr[i] = RDKit::Descriptors::calcAMW(*mol);
        } else {
            result_ptr[i] = std::numeric_limits<double>::quiet_NaN();
        }
    }
    
    return result;
}

py::array_t<double> calculate_logp(const std::vector<std::string>& smiles_list) {
    size_t size = smiles_list.size();
    
    auto result = py::array_t<double>(size);
    auto result_buf = result.request();
    auto* result_ptr = static_cast<double*>(result_buf.ptr);
    
    for (size_t i = 0; i < size; ++i) {
        auto mol = smiles_to_mol(smiles_list[i]);
        if (mol) {
            result_ptr[i] = RDKit::Descriptors::calcClogP(*mol);
        } else {
            result_ptr[i] = std::numeric_limits<double>::quiet_NaN();
        }
    }
    
    return result;
}

py::array_t<double> calculate_tpsa(const std::vector<std::string>& smiles_list) {
    size_t size = smiles_list.size();
    
    auto result = py::array_t<double>(size);
    auto result_buf = result.request();
    auto* result_ptr = static_cast<double*>(result_buf.ptr);
    
    for (size_t i = 0; i < size; ++i) {
        auto mol = smiles_to_mol(smiles_list[i]);
        if (mol) {
            result_ptr[i] = RDKit::Descriptors::calcTPSA(*mol);
        } else {
            result_ptr[i] = std::numeric_limits<double>::quiet_NaN();
        }
    }
    
    return result;
}

py::array_t<bool> validate_smiles(const std::vector<std::string>& smiles_list) {
    size_t size = smiles_list.size();
    
    auto result = py::array_t<bool>(size);
    auto result_buf = result.request();
    auto* result_ptr = static_cast<bool*>(result_buf.ptr);
    
    for (size_t i = 0; i < size; ++i) {
        auto mol = smiles_to_mol(smiles_list[i]);
        result_ptr[i] = (mol != nullptr);
    }
    
    return result;
}

py::dict calculate_multiple_descriptors(const std::vector<std::string>& smiles_list) {
    size_t size = smiles_list.size();
    
    // Create output arrays
    auto mw_result = py::array_t<double>(size);
    auto logp_result = py::array_t<double>(size);
    auto tpsa_result = py::array_t<double>(size);
    
    auto mw_buf = mw_result.request();
    auto logp_buf = logp_result.request();
    auto tpsa_buf = tpsa_result.request();
    
    auto* mw_ptr = static_cast<double*>(mw_buf.ptr);
    auto* logp_ptr = static_cast<double*>(logp_buf.ptr);
    auto* tpsa_ptr = static_cast<double*>(tpsa_buf.ptr);
    
    // Process each SMILES once and calculate all descriptors
    for (size_t i = 0; i < size; ++i) {
        auto mol = smiles_to_mol(smiles_list[i]);
        if (mol) {
            mw_ptr[i] = RDKit::Descriptors::calcAMW(*mol);
            logp_ptr[i] = RDKit::Descriptors::calcClogP(*mol);
            tpsa_ptr[i] = RDKit::Descriptors::calcTPSA(*mol);
        } else {
            mw_ptr[i] = std::numeric_limits<double>::quiet_NaN();
            logp_ptr[i] = std::numeric_limits<double>::quiet_NaN();
            tpsa_ptr[i] = std::numeric_limits<double>::quiet_NaN();
        }
    }
    
    py::dict result;
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

py::array_t<uint8_t> calculate_morgan_fingerprints(
    const std::vector<std::string>& smiles_list,
    int radius,
    int nbits
) {
    size_t size = smiles_list.size();
    
    // Create 2D output array (size x nbits)
    auto result = py::array_t<uint8_t>({size, static_cast<size_t>(nbits)});
    auto result_buf = result.request();
    auto* result_ptr = static_cast<uint8_t*>(result_buf.ptr);
    
    for (size_t i = 0; i < size; ++i) {
        auto mol = smiles_to_mol(smiles_list[i]);
        if (mol) {
            try {
                auto fp = RDKit::MorganFingerprints::getFingerprintAsBitVect(*mol, radius, nbits);
                
                // Copy bits to result array
                for (int j = 0; j < nbits; ++j) {
                    result_ptr[i * nbits + j] = fp->getBit(j) ? 1 : 0;
                }
            } catch (const std::exception& e) {
                // On error, set all bits to 0
                for (int j = 0; j < nbits; ++j) {
                    result_ptr[i * nbits + j] = 0;
                }
            }
        } else {
            // Invalid SMILES, set all bits to 0
            for (int j = 0; j < nbits; ++j) {
                result_ptr[i * nbits + j] = 0;
            }
        }
    }
    
    return result;
}

} // namespace rdtools