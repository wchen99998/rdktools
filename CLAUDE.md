# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

RDTools is a Python library that provides high-performance molecular operations using RDKit's C++ core through pybind11 bindings. It enables fast molecular descriptor calculations, SMILES validation, and fingerprint generation by exposing RDKit's optimized C++ implementation with native numpy array support.

## Architecture

### Core Components

1. **C++ Extension Module** (`src/cpp/`):
   - `molecular_ops.hpp/cpp`: Core molecular operations implementation using RDKit C++
   - `pybind_module.cpp`: pybind11 bindings that expose C++ functions to Python
   - Functions process numpy arrays of SMILES strings in parallel for high throughput

2. **Python API** (`src/rdtools/__init__.py`):
   - Provides user-friendly interface with input validation and type conversion
   - Wraps C++ functions with numpy array handling
   - Includes batch processing utilities for large datasets

3. **Build System**:
   - Uses scikit-build-core with CMake for C++ compilation
   - CMakeLists.txt handles RDKit dependency detection and linking
   - Supports multiple RDKit installation methods (apt, conda, homebrew)

### Data Flow

1. Python numpy arrays of SMILES strings → input validation
2. Convert to std::vector<std::string> for C++ processing  
3. RDKit C++ operations process molecules in parallel
4. Results returned as numpy arrays with proper error handling (NaN for invalid SMILES)

## Essential Commands

### Prerequisites Setup
```bash
# Install RDKit (required before building)
sudo apt-get install python3-rdkit librdkit-dev librdkit1 rdkit-data
```

### Development Workflow
```bash
# Setup development environment
uv sync

# Build C++ extension (required for functionality)
uv run python -m pip install -e .

# Alternative: Force rebuild with verbose output
SKBUILD_EDITABLE_VERBOSE=1 uv sync --reinstall

# Run tests
uv run pytest tests/

# Run specific test
uv run pytest tests/test_rdtools.py::TestBasicFunctions::test_molecular_weights

# Code formatting
uv run black src/
uv run isort src/
```

### Build Troubleshooting
```bash
# Check RDKit installation
python -c "import rdkit; print(rdkit.__version__)"

# Clean build (if build issues occur)
rm -rf build/ && uv run python -m pip install -e .

# Debug build with CMake directly
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make VERBOSE=1
```

## Key Implementation Details

### Function Categories

1. **Core Descriptors**: `molecular_weights()`, `logp()`, `tpsa()` - single descriptor calculations
2. **Batch Operations**: `descriptors()` - multiple descriptors in one pass (more efficient)
3. **Validation**: `is_valid()`, `canonical_smiles()` - SMILES processing utilities  
4. **Fingerprints**: `morgan_fingerprints()` - bit vector generation for similarity calculations
5. **Convenience**: `filter_valid()`, `batch_process()` - high-level data processing

### Error Handling Strategy

- Invalid SMILES return NaN for numeric calculations, False for validation, empty string for canonicalization
- All functions handle mixed valid/invalid input gracefully
- Input validation converts lists to numpy arrays automatically
- Type conversion ensures string arrays regardless of input dtype

### Performance Considerations

- Batch processing functions (`descriptors()`, `batch_process()`) are more efficient than individual calls
- C++ implementation processes molecules in parallel where possible
- Functions designed for high-throughput processing (10K-50K molecules/second)
- Memory efficient with direct numpy array access, minimal Python overhead

## Testing Requirements

- Tests require the C++ extension to be built first
- Use `pytest.skip` if extension not available (graceful degradation)
- Test coverage includes: basic functionality, error handling, input validation, consistency between batch/individual operations
- Performance benchmarks available in `examples/performance_benchmark.py`

## Dependencies

- **Build-time**: scikit-build-core, pybind11, CMake, RDKit C++ libraries
- **Runtime**: numpy, RDKit Python bindings (optional but recommended for validation)
- **Python**: >=3.12 (specified in pyproject.toml)

## Development Notes

- Always test both individual and batch operations for consistency
- Ensure proper handling of edge cases (empty arrays, all invalid SMILES, mixed validity)
- Performance-critical code should be implemented in C++ layer
- Python layer focuses on user experience and numpy integration