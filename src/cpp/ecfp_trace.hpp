#pragma once

#include <GraphMol/GraphMol.h>
#include <string>

namespace rdktools {

std::string ecfp_reasoning_trace_from_smiles(
    const std::string& smiles,
    unsigned int radius,
    bool isomeric,
    bool kekulize,
    bool include_per_center);

} // namespace rdktools
