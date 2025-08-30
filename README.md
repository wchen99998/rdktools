# RDTools

High-performance molecular operations using RDKit C++ with numpy arrays.

RDTools provides fast molecular descriptor calculations, SMILES validation, and fingerprint generation by exposing RDKit's C++ core through pybind11 bindings. This allows for efficient processing of large numpy arrays of SMILES strings.

## Features

- **Fast Molecular Descriptors**: Calculate molecular weight, LogP, TPSA in parallel
- **SMILES Validation**: Efficiently validate large arrays of SMILES strings
- **Fingerprint Generation**: Morgan fingerprints as numpy bit vectors
- **Batch Processing**: Optimized for large datasets with batch processing utilities
- **NumPy Integration**: Native support for numpy arrays with proper error handling

## Installation

### Prerequisites

1. **RDKit**: Install RDKit (C++ libraries and headers required)
   ```bash
   # Ubuntu/Debian (recommended):
   sudo apt-get install python3-rdkit librdkit-dev librdkit1 rdkit-data
   
   # macOS with Homebrew:
   brew install rdkit
   
   # Alternative: Using conda
   conda install -c conda-forge rdkit
   ```

2. **Build Dependencies**: 
   ```bash
   uv add --dev cmake pybind11
   ```

### Build and Install

1. **Clone and setup**:
   ```bash
   git clone <repository>
   cd rdtools
   uv sync
   ```

2. **Build the C++ extension**:
   ```bash
   # Method 1: Using setup.py
   uv run python setup.py build_ext --inplace
   
   # Method 2: Using CMake
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ```

3. **Install in development mode**:
   ```bash
   uv run pip install -e .
   ```

## Quick Start

```python
import numpy as np
import rdtools

# Sample SMILES strings
smiles = np.array(['CCO', 'c1ccccc1', 'CC(=O)O', 'invalid_smiles'])

# Validate SMILES
valid = rdtools.is_valid(smiles)
print(f"Valid: {valid}")  # [True, True, True, False]

# Calculate molecular descriptors
descriptors = rdtools.descriptors(smiles)
print(f"Molecular weights: {descriptors['molecular_weight']}")
print(f"LogP values: {descriptors['logp']}")
print(f"TPSA values: {descriptors['tpsa']}")

# Generate fingerprints
fingerprints = rdtools.morgan_fingerprints(smiles, radius=2, nbits=2048)
print(f"Fingerprint shape: {fingerprints.shape}")  # (4, 2048)
```

## API Reference

### Core Functions

#### `rdtools.molecular_weights(smiles_array)`
Calculate molecular weights for SMILES strings.

**Parameters:**
- `smiles_array`: numpy array or list of SMILES strings

**Returns:**
- numpy array of molecular weights (float64), NaN for invalid SMILES

#### `rdtools.logp(smiles_array)`
Calculate LogP (octanol-water partition coefficient) values.

#### `rdtools.tpsa(smiles_array)`
Calculate TPSA (Topological Polar Surface Area) values.

#### `rdtools.is_valid(smiles_array)`
Validate SMILES strings.

**Returns:**
- boolean numpy array indicating validity

#### `rdtools.descriptors(smiles_array)`
Calculate multiple descriptors efficiently in a single pass.

**Returns:**
- Dictionary with keys: 'molecular_weight', 'logp', 'tpsa'

#### `rdtools.canonical_smiles(smiles_array)`
Convert SMILES to canonical form.

#### `rdtools.morgan_fingerprints(smiles_array, radius=2, nbits=2048)`
Calculate Morgan fingerprints as bit vectors.

**Parameters:**
- `radius`: fingerprint radius (default: 2)
- `nbits`: number of bits (default: 2048)

**Returns:**
- 2D numpy array of shape (n_molecules, nbits) with dtype uint8

### Utility Functions

#### `rdtools.filter_valid(smiles_array)`
Filter array to keep only valid SMILES.

#### `rdtools.batch_process(smiles_array, batch_size=1000, **kwargs)`
Process large arrays in batches with comprehensive results.

## Performance

RDTools is optimized for high-throughput molecular processing:

- **Batch Processing**: Calculate multiple descriptors in a single pass
- **C++ Core**: Uses RDKit's optimized C++ implementation
- **Memory Efficient**: Minimal Python overhead with direct numpy array access
- **Parallel Ready**: Functions are designed to work well with joblib/multiprocessing

### Benchmarks

On a modern CPU, RDTools can process:
- ~10,000-50,000 molecules/second for descriptor calculations
- ~5,000-20,000 molecules/second for fingerprint generation

Performance varies by molecule complexity and system specifications.

## Examples

See the `examples/` directory for detailed usage examples:

- `basic_usage.py`: Comprehensive demonstration of all features
- `performance_comparison.py`: Benchmarking against pure Python approaches

## Development

### Building from Source

1. **Install dependencies**:
   ```bash
   uv sync --group dev
   ```

2. **Run tests**:
   ```bash
   uv run pytest tests/
   ```

3. **Code formatting**:
   ```bash
   uv run black src/
   uv run isort src/
   ```

### Project Structure

```
rdtools/
├── src/
│   ├── rdtools/           # Python package
│   │   └── __init__.py    # Main API
│   └── cpp/               # C++ source code
│       ├── molecular_ops.hpp
│       ├── molecular_ops.cpp
│       └── pybind_module.cpp
├── examples/              # Usage examples
├── tests/                 # Test suite
├── CMakeLists.txt         # CMake build config
├── setup.py              # Python build config
└── pyproject.toml        # Project metadata
```

## Dependencies

- **Python**: >= 3.8
- **NumPy**: >= 1.20.0
- **RDKit**: >= 2023.9.1 (C++ libraries required)
- **pybind11**: >= 2.10.0 (build time)

## License

[Add your license here]

## Contributing

[Add contributing guidelines here]

## Citation

If you use RDTools in your research, please cite:
[Add citation information here]