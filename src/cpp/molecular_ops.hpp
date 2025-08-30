#pragma once

#include <vector>
#include <string>
#include <memory>
#include <GraphMol/GraphMol.h>
#include <GraphMol/SmilesParse/SmilesParse.h>
#include <GraphMol/Descriptors/MolDescriptors.h>
#include <GraphMol/FileParsers/MolSupplier.h>
#include <GraphMol/MolPickler.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace rdtools {

/**
 * @brief Process SMILES strings from list and return molecular weights
 * @param smiles_list list of SMILES strings
 * @return numpy array of molecular weights
 */
pybind11::array_t<double> calculate_molecular_weights(
    const std::vector<std::string>& smiles_list
);

/**
 * @brief Calculate LogP values for SMILES strings
 * @param smiles_list list of SMILES strings
 * @return numpy array of LogP values
 */
pybind11::array_t<double> calculate_logp(
    const std::vector<std::string>& smiles_list
);

/**
 * @brief Calculate TPSA (Topological Polar Surface Area) values
 * @param smiles_list list of SMILES strings
 * @return numpy array of TPSA values
 */
pybind11::array_t<double> calculate_tpsa(
    const std::vector<std::string>& smiles_list
);

/**
 * @brief Validate SMILES strings and return boolean array
 * @param smiles_list list of SMILES strings
 * @return numpy array of boolean values (true for valid SMILES)
 */
pybind11::array_t<bool> validate_smiles(
    const std::vector<std::string>& smiles_list
);

/**
 * @brief Calculate multiple descriptors at once for efficiency
 * @param smiles_list list of SMILES strings
 * @return dictionary with arrays of molecular weights, LogP, and TPSA
 */
pybind11::dict calculate_multiple_descriptors(
    const std::vector<std::string>& smiles_list
);

/**
 * @brief Convert SMILES to canonical SMILES
 * @param smiles_list list of SMILES strings
 * @return list of canonical SMILES strings
 */
std::vector<std::string> canonicalize_smiles(
    const std::vector<std::string>& smiles_list
);

/**
 * @brief Calculate Morgan fingerprints as bit vectors
 * @param smiles_list list of SMILES strings
 * @param radius fingerprint radius (default: 2)
 * @param nbits number of bits in fingerprint (default: 2048)
 * @return 2D numpy array where each row is a fingerprint bit vector
 */
pybind11::array_t<uint8_t> calculate_morgan_fingerprints(
    const std::vector<std::string>& smiles_list,
    int radius = 2,
    int nbits = 2048
);

} // namespace rdtools