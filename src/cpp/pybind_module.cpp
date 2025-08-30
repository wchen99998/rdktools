#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/string.h>
#include "molecular_ops.hpp"

namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(_rdtools_core, m) {
    m.doc() = "High-performance molecular operations using RDKit C++";
    
    // Molecular weight calculation
    m.def("calculate_molecular_weights", &rdtools::calculate_molecular_weights,
          "Calculate molecular weights for SMILES strings",
          "smiles_list"_a);
    
    // LogP calculation
    m.def("calculate_logp", &rdtools::calculate_logp,
          "Calculate LogP values for SMILES strings",
          "smiles_list"_a);
    
    // TPSA calculation
    m.def("calculate_tpsa", &rdtools::calculate_tpsa,
          "Calculate TPSA values for SMILES strings",
          "smiles_list"_a);
    
    // SMILES validation
    m.def("validate_smiles", &rdtools::validate_smiles,
          "Validate SMILES strings and return boolean array",
          "smiles_list"_a);
    
    // Multiple descriptors calculation
    m.def("calculate_multiple_descriptors", &rdtools::calculate_multiple_descriptors,
          "Calculate multiple descriptors efficiently for SMILES strings",
          "smiles_list"_a);
    
    // SMILES canonicalization
    m.def("canonicalize_smiles", &rdtools::canonicalize_smiles,
          "Convert SMILES to canonical form",
          "smiles_list"_a);
    
    // Morgan fingerprints
    m.def("calculate_morgan_fingerprints", &rdtools::calculate_morgan_fingerprints,
          "Calculate Morgan fingerprints as bit vectors",
          "smiles_list"_a,
          "radius"_a = 2,
          "nbits"_a = 2048);
    
    // Module version
    m.attr("__version__") = "0.1.0";
}