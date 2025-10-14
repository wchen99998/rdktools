#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/string.h>
#include "molecular_ops.hpp"
 
// Helper macros to stringify VERSION_INFO passed from CMake
#ifndef STRINGIFY
#  define STRINGIFY(x) #x
#endif
#ifndef MACRO_STRINGIFY
#  define MACRO_STRINGIFY(x) STRINGIFY(x)
#endif

namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(_rdktools_core, m) {
    m.doc() = "High-performance molecular operations using RDKit C++";
    
    // Molecular weight calculation
    m.def("calculate_molecular_weights", &rdktools::calculate_molecular_weights,
          "Calculate molecular weights for SMILES strings",
          "smiles_list"_a);
    
    // LogP calculation
    m.def("calculate_logp", &rdktools::calculate_logp,
          "Calculate LogP values for SMILES strings",
          "smiles_list"_a);
    
    // TPSA calculation
    m.def("calculate_tpsa", &rdktools::calculate_tpsa,
          "Calculate TPSA values for SMILES strings",
          "smiles_list"_a);
    
    // SMILES validation
    m.def("validate_smiles", &rdktools::validate_smiles,
          "Validate SMILES strings and return boolean array",
          "smiles_list"_a);
    
    // Multiple descriptors calculation
    m.def("calculate_multiple_descriptors", &rdktools::calculate_multiple_descriptors,
          "Calculate multiple descriptors efficiently for SMILES strings",
          "smiles_list"_a);
    
    // SMILES canonicalization
    m.def("canonicalize_smiles", &rdktools::canonicalize_smiles,
          "Convert SMILES to canonical form",
          "smiles_list"_a);
    
    // Morgan fingerprints
    m.def("calculate_morgan_fingerprints", &rdktools::calculate_morgan_fingerprints,
          "Calculate Morgan fingerprints as bit vectors",
          "smiles_list"_a,
          "radius"_a = 2,
          "nbits"_a = 2048);
    
    // ECFP reasoning trace
    m.def("ecfp_reasoning_trace", &rdktools::ecfp_reasoning_trace,
          "Generate an ECFP reasoning trace and fingerprint for a SMILES string",
          "smiles"_a,
          "radius"_a = 2,
          "isomeric"_a = true,
          "kekulize"_a = false,
          "include_per_center"_a = true);
    
    // Module version
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
}
