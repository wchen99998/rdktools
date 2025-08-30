#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "molecular_ops.hpp"

namespace py = pybind11;

PYBIND11_MODULE(_rdtools_core, m) {
    m.doc() = "High-performance molecular operations using RDKit C++";
    
    // Molecular weight calculation
    m.def("calculate_molecular_weights", &rdtools::calculate_molecular_weights,
          "Calculate molecular weights for SMILES strings",
          py::arg("smiles_list"));
    
    // LogP calculation
    m.def("calculate_logp", &rdtools::calculate_logp,
          "Calculate LogP values for SMILES strings",
          py::arg("smiles_list"));
    
    // TPSA calculation
    m.def("calculate_tpsa", &rdtools::calculate_tpsa,
          "Calculate TPSA values for SMILES strings",
          py::arg("smiles_list"));
    
    // SMILES validation
    m.def("validate_smiles", &rdtools::validate_smiles,
          "Validate SMILES strings and return boolean array",
          py::arg("smiles_list"));
    
    // Multiple descriptors calculation
    m.def("calculate_multiple_descriptors", &rdtools::calculate_multiple_descriptors,
          "Calculate multiple descriptors efficiently for SMILES strings",
          py::arg("smiles_list"));
    
    // SMILES canonicalization
    m.def("canonicalize_smiles", &rdtools::canonicalize_smiles,
          "Convert SMILES to canonical form",
          py::arg("smiles_list"));
    
    // Morgan fingerprints
    m.def("calculate_morgan_fingerprints", &rdtools::calculate_morgan_fingerprints,
          "Calculate Morgan fingerprints as bit vectors",
          py::arg("smiles_list"),
          py::arg("radius") = 2,
          py::arg("nbits") = 2048);
    
    // Module version
    m.attr("__version__") = "0.1.0";
}