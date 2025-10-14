#pragma once

#include <GraphMol/GraphMol.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace rdktools {

inline constexpr std::size_t kECFPReasoningFingerprintSize = 2048;

using ReasoningTraceResult =
    std::tuple<std::string, std::vector<std::uint8_t>>;

ReasoningTraceResult ecfp_reasoning_trace_from_smiles(
    const std::string& smiles,
    unsigned int radius,
    bool isomeric,
    bool kekulize,
    bool include_per_center);

} // namespace rdktools
