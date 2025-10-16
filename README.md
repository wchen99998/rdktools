# RDKTools

High-performance molecular operations using RDKit C++ with numpy arrays.

RDKTools provides fast molecular descriptor calculations, SMILES validation, and fingerprint generation by exposing RDKit's C++ core through nanobind bindings. This allows for efficient processing of large numpy arrays of SMILES strings.

## Features

- **Fast Molecular Descriptors**: Calculate molecular weight, LogP, TPSA in parallel
- **SMILES Validation**: Efficiently validate large arrays of SMILES strings
- **Fingerprint Generation**: Morgan fingerprints as numpy bit vectors
- **Fingerprint Reasoning**: Generate human-readable ECFP traces explaining fingerprint environments
- **TensorFlow Integration**: Custom TensorFlow operations for data pipelines (tf.data compatible)
- **Batch Processing**: Optimized for large datasets with batch processing utilities
- **NumPy Integration**: Native support for numpy arrays with proper error handling

## Installation

There's no wheel built yet.

## Local Development Workflow

The project vendors Boost and RDKit automatically during the first configure, so day-to-day C++ iteration can stay fast. A typical loop looks like this:

1. Create a development environment:
   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   pip install --upgrade pip
   ```

2. Install the package in editable mode without build isolation so the build uses the Python from your venv (and its TensorFlow install):
   ```bash
   pip install --no-build-isolation -e .
   ```
   The first run takes longer because CMake downloads and builds Boost and RDKit into `build/editable-*`. Keep that directory around to avoid rebuilding the dependencies.

3. Rebuild after editing C++ sources without touching the vendored dependencies:
   ```bash
   cmake --build build/editable-<tag> --target _rdktools_core
   cmake --build build/editable-<tag> --target tf_ops  # only if you changed TensorFlow op code
   ```
   Replace `<tag>` with the directory suffix CMake chose (for example, `cp312-cp312-macosx_12_0_arm64`).

4. Run tests directly from the same venv:
   ```bash
   pytest tests
   ```

5. Optional: generate a debug build directory once if you need symbols or sanitizers:
   ```bash
   cmake -S . -B build/editable-debug -GNinja \
     -DCMAKE_BUILD_TYPE=Debug \
     -DPython_EXECUTABLE="$(which python)"
   cmake --build build/editable-debug --target _rdktools_core
   ```

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
conda install boost-cpp=1.88 boost=1.88 eigen cairo

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

#### `rdtools.ecfp_reasoning_trace(smiles, radius=2, *, isomeric=True, kekulize=False, include_per_center=True, fingerprint_size=2048)`
Generate a human-readable explanation of the environments that contribute to the ECFP (Morgan) fingerprint for a single SMILES string.

**Returns:**
- Tuple `(trace, fingerprint)` where `trace` is a multi-line string with
  aggregated tokens and optional per-atom environment chains, and `fingerprint`
  is a NumPy array of shape `(fingerprint_size,)` with dtype `uint8`.

**Example:**
```python
import rdtools

trace, fingerprint = rdtools.ecfp_reasoning_trace("CCO", fingerprint_size=512)
print(trace)
print(fingerprint.shape)  # (512,)
# r0: r0:[#6]×2, r0:[#8]×1
# r1: r1:[#6]-[#6]×2
#
# per-center chains
# C0: r0:[#6] → r1:[#6]-[#6]
# C1: r0:[#6] → r1:[#6]-[#6]
# O2: r0:[#8]
```

Invalid SMILES return an empty string and an all-zero fingerprint, allowing the
caller to decide how to surface errors.

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
- `fingerprint_size`: Positive integer bit length (default: 2048)

**Returns:**
- Tuple `(traces, fingerprints)` where `traces` mirrors the input shape with
  string tensors of reasoning traces and `fingerprints` appends a trailing
  dimension of length `fingerprint_size` (default 2048) containing the uint8 bit
  vectors. Invalid inputs yield `[invalid]` traces and zeroed fingerprints.

**Example:**
```python
import tensorflow as tf
import rdtools.tf_ops

# Use in tf.data pipeline
dataset = tf.data.Dataset.from_tensor_slices(["CCO", "c1ccccc1"])
dataset = dataset.map(
    lambda value: rdtools.tf_ops.string_process(
        value, fingerprint_size=1024
    )
)

for traces, fingerprints in dataset.take(1):
    print(traces.numpy()[0].decode())
    print(fingerprints.numpy()[0].shape)  # (1024,)
    # r0: ...
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
- **Boost**: ==1.88.0 


### Build Dependencies
- **scikit-build-core**: >= 0.10 (modern CMake-based build backend)
- **nanobind**: >= 2.0.0 (fast C++/Python binding library)
- **tensorflow**: == 2.19.0 (for TensorFlow custom operations)
- **CMake**: >= 3.15 (build system)
- **C++ Compiler**: Supporting C++17 standard (GCC 7+, Clang 5+, MSVC 2017+)

## License

MIT
