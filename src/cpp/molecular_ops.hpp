#pragma once

#include <vector>
#include <string>
#include <memory>
#include <GraphMol/GraphMol.h>
#include <GraphMol/SmilesParse/SmilesParse.h>
#include <GraphMol/Descriptors/MolDescriptors.h>
#include <GraphMol/FileParsers/MolSupplier.h>
#include <GraphMol/MolPickler.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/string.h>

namespace rdktools {

/**
 * @brief Process SMILES strings from list and return molecular weights
 * @param smiles_list list of SMILES strings
 * @return numpy array of molecular weights
 */
nanobind::ndarray<nanobind::numpy, double> calculate_molecular_weights(
    const std::vector<std::string>& smiles_list
);

/**
 * @brief Calculate LogP values for SMILES strings
 * @param smiles_list list of SMILES strings
 * @return numpy array of LogP values
 */
nanobind::ndarray<nanobind::numpy, double> calculate_logp(
    const std::vector<std::string>& smiles_list
);

/**
 * @brief Calculate TPSA (Topological Polar Surface Area) values
 * @param smiles_list list of SMILES strings
 * @return numpy array of TPSA values
 */
nanobind::ndarray<nanobind::numpy, double> calculate_tpsa(
    const std::vector<std::string>& smiles_list
);

/**
 * @brief Validate SMILES strings and return boolean array
 * @param smiles_list list of SMILES strings
 * @return numpy array of boolean values (true for valid SMILES)
 */
nanobind::ndarray<nanobind::numpy, bool> validate_smiles(
    const std::vector<std::string>& smiles_list
);

/**
 * @brief Calculate multiple descriptors at once for efficiency
 * @param smiles_list list of SMILES strings
 * @return dictionary with arrays of molecular weights, LogP, and TPSA
 */
nanobind::dict calculate_multiple_descriptors(
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
nanobind::ndarray<nanobind::numpy, uint8_t> calculate_morgan_fingerprints(
    const std::vector<std::string>& smiles_list,
    int radius = 2,
    int nbits = 2048
);

/**
 * @brief Generate an ECFP-style reasoning trace for a SMILES string.
 * @param smiles SMILES string to analyse
 * @param radius Morgan fingerprint radius (default: 2)
 * @param isomeric Whether to include stereochemistry when generating SMARTS
 * @param kekulize Whether to kekulize the molecule before generating fragments
 * @param include_per_center Whether to include per-atom chains in the trace
 * @return Multi-line reasoning trace text. Invalid SMILES yield an empty string.
 */
std::string ecfp_reasoning_trace(
    const std::string& smiles,
    int radius = 2,
    bool isomeric = true,
    bool kekulize = false,
    bool include_per_center = true
);

} // namespace rdktools
