# RDKTools

High-performance molecular operations using RDKit C++ with numpy arrays.

RDKTools provides fast molecular descriptor calculations, SMILES validation, and fingerprint generation by exposing RDKit's C++ core through nanobind bindings. This allows for efficient processing of large numpy arrays of SMILES strings.

## Features

- **Fast Molecular Descriptors**: Calculate molecular weight, LogP, TPSA in parallel
- **SMILES Validation**: Efficiently validate large arrays of SMILES strings
- **Fingerprint Generation**: Morgan fingerprints as numpy bit vectors
- **TensorFlow Integration**: Custom TensorFlow operations for data pipelines (tf.data compatible)
- **Batch Processing**: Optimized for large datasets with batch processing utilities
- **NumPy Integration**: Native support for numpy arrays with proper error handling

## Installation

There's no wheel built yet.

### Dependencies

#### 1. System Dependencies

**Ubuntu/Debian (Recommended)**:
```bash
# Update package manager
sudo apt-get update

# Install RDKit C++ libraries and development headers
sudo apt-get install -y python3-rdkit librdkit-dev librdkit1 rdkit-data

# Install build essentials (if not already installed)
sudo apt-get install -y build-essential cmake pkg-config
```

**macOS with Homebrew**:
```bash
# Install RDKit
brew install rdkit

# Install build tools (usually pre-installed with Xcode)
xcode-select --install
```

**Windows**:
```bash
# Using conda (recommended for Windows)
conda install -c conda-forge rdkit-dev rdkit
# You'll also need Visual Studio Build Tools
```

#### 2. Python Dependencies

The build system automatically handles Python dependencies, but here's what gets installed:

**Build-time dependencies** (handled by pyproject.toml):
- `scikit-build-core>=0.10` - Modern CMake-based Python build backend
- `nanobind>=2.0.0` - Fast C++/Python binding library
- `tensorflow==2.19.0` - For TensorFlow custom operations (built automatically when available)

**Runtime dependencies**:
- `numpy>=1.20.0` - Core numerical operations
- `tensorflow==2.19.0` - Required for TensorFlow custom operations

**Development dependencies** (optional):
- `pytest>=6.0` - Testing framework
- `black` - Code formatting
- `isort` - Import sorting

### Build and Installation

```bash
# 1. Clone and navigate
git clone <repository-url>
cd rdtools

# Build Rdkit
./scripts/build_rdkit.sh

# Set up build env for linking
conda create -n rdtools -p 3.12
conda install boost-cpp=1.84 boost=1.84 eigen cairo

# 4. Build and install in editable mode
RDKIT_ROOT=./rdtools/build/rdkit/rdkit pip install -e .
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

### TensorFlow Operations

#### `rdtools.tf_ops.string_process(smiles_tensor)`
TensorFlow custom operation for string processing in data pipelines.

**Parameters:**
- `smiles_tensor`: TensorFlow string tensor

**Returns:**
- TensorFlow string tensor with processed SMILES

**Example:**
```python
import tensorflow as tf
import rdtools.tf_ops

# Use in tf.data pipeline
dataset = tf.data.Dataset.from_tensor_slices(["CCO", "c1ccccc1"])
dataset = dataset.map(rdtools.tf_ops.string_process)
```

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

## Dependencies

### Runtime Dependencies
- **Python**: >= 3.12 (as specified in pyproject.toml)
- **NumPy**: >= 1.20.0 (for array operations)

### System Dependencies
- **RDKit**: >= 2022.9.1 (C++ libraries and headers required)
- **Boost**: ==1.84.0 


### Build Dependencies
- **scikit-build-core**: >= 0.10 (modern CMake-based build backend)
- **nanobind**: >= 2.0.0 (fast C++/Python binding library)
- **tensorflow**: == 2.19.0 (for TensorFlow custom operations)
- **CMake**: >= 3.15 (build system)
- **C++ Compiler**: Supporting C++17 standard (GCC 7+, Clang 5+, MSVC 2017+)

## License

MIT

