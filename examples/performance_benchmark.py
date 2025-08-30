#!/usr/bin/env python3
"""
Performance benchmark for rdtools library.

This script compares the performance of rdtools against pure RDKit Python
for common molecular operations.
"""

import time
import numpy as np
from typing import List, Tuple

# Try to import both libraries
try:
    import rdtools
    RDTOOLS_AVAILABLE = True
except ImportError:
    RDTOOLS_AVAILABLE = False

try:
    from rdkit import Chem
    from rdkit.Chem import Descriptors
    RDKIT_AVAILABLE = True
except ImportError:
    RDKIT_AVAILABLE = False

def create_benchmark_data(n_molecules: int = 1000) -> np.ndarray:
    """Create benchmark dataset with diverse SMILES."""
    base_smiles = [
        'CCO',                    # ethanol
        'c1ccccc1',              # benzene
        'CC(C)O',                # isopropanol
        'c1ccc(O)cc1',           # phenol
        'CCN(CC)CC',             # triethylamine
        'c1cnc(C)nc1',           # methylpyrimidine
        'CCCCCCCC',              # octane
        'c1cc(Cl)ccc1O',         # 4-chlorophenol
        'CC(=O)O',               # acetic acid
        'c1ccc2c(c1)cccc2',      # naphthalene
        'c1ccc(cc1)C(=O)O',      # benzoic acid
        'CCN',                   # ethylamine
        'c1ccc(cc1)N',           # aniline
        'CCCCO',                 # butanol
        'c1ccncc1',              # pyridine
    ]
    
    # Replicate to reach desired size
    cycles = (n_molecules // len(base_smiles)) + 1
    dataset = (base_smiles * cycles)[:n_molecules]
    
    return np.array(dataset, dtype=str)

def benchmark_rdkit_python(smiles_array: np.ndarray) -> Tuple[float, dict]:
    """Benchmark pure RDKit Python implementation."""
    if not RDKIT_AVAILABLE:
        return float('inf'), {}
    
    start_time = time.time()
    
    results = {
        'molecular_weight': [],
        'logp': [],
        'tpsa': [],
        'valid': []
    }
    
    for smiles in smiles_array:
        try:
            mol = Chem.MolFromSmiles(smiles)
            if mol:
                results['molecular_weight'].append(Descriptors.MolWt(mol))
                results['logp'].append(Descriptors.MolLogP(mol))
                results['tpsa'].append(Descriptors.TPSA(mol))
                results['valid'].append(True)
            else:
                results['molecular_weight'].append(float('nan'))
                results['logp'].append(float('nan'))
                results['tpsa'].append(float('nan'))
                results['valid'].append(False)
        except:
            results['molecular_weight'].append(float('nan'))
            results['logp'].append(float('nan'))
            results['tpsa'].append(float('nan'))
            results['valid'].append(False)
    
    end_time = time.time()
    
    # Convert to numpy arrays
    for key in results:
        results[key] = np.array(results[key])
    
    return end_time - start_time, results

def benchmark_rdtools(smiles_array: np.ndarray) -> Tuple[float, dict]:
    """Benchmark rdtools implementation."""
    if not RDTOOLS_AVAILABLE:
        return float('inf'), {}
    
    start_time = time.time()
    
    # Use the batch descriptor function for maximum efficiency
    results = rdtools.descriptors(smiles_array)
    results['valid'] = rdtools.is_valid(smiles_array)
    
    end_time = time.time()
    
    return end_time - start_time, results

def run_benchmark(n_molecules: int = 1000) -> None:
    """Run complete benchmark comparison."""
    print(f"\nBenchmarking with {n_molecules:,} molecules...")
    print("-" * 50)
    
    # Create test data
    smiles_array = create_benchmark_data(n_molecules)
    print(f"Generated {len(smiles_array)} SMILES strings")
    
    # Benchmark RDKit Python
    rdkit_time, rdkit_results = benchmark_rdkit_python(smiles_array)
    
    # Benchmark rdtools
    rdtools_time, rdtools_results = benchmark_rdtools(smiles_array)
    
    # Display results
    print(f"\nTiming Results:")
    if RDKIT_AVAILABLE:
        print(f"RDKit Python: {rdkit_time:.3f} seconds ({n_molecules/rdkit_time:.0f} mol/sec)")
    else:
        print("RDKit Python: Not available")
    
    if RDTOOLS_AVAILABLE:
        print(f"RDTools:      {rdtools_time:.3f} seconds ({n_molecules/rdtools_time:.0f} mol/sec)")
        
        if RDKIT_AVAILABLE and rdkit_time > 0:
            speedup = rdkit_time / rdtools_time
            print(f"Speedup:      {speedup:.1f}x faster")
    else:
        print("RDTools:      Not available")
    
    # Verify results match (if both available)
    if RDKIT_AVAILABLE and RDTOOLS_AVAILABLE:
        print(f"\nAccuracy Check:")
        
        # Check molecular weights (allowing for small numerical differences)
        mw_match = np.allclose(rdkit_results['molecular_weight'], 
                              rdtools_results['molecular_weight'], 
                              equal_nan=True, rtol=1e-6)
        print(f"Molecular weights match: {mw_match}")
        
        # Check LogP
        logp_match = np.allclose(rdkit_results['logp'], 
                                rdtools_results['logp'], 
                                equal_nan=True, rtol=1e-6)
        print(f"LogP values match: {logp_match}")
        
        # Check TPSA
        tpsa_match = np.allclose(rdkit_results['tpsa'], 
                                rdtools_results['tpsa'], 
                                equal_nan=True, rtol=1e-6)
        print(f"TPSA values match: {tpsa_match}")
        
        # Check validity
        valid_match = np.array_equal(rdkit_results['valid'], rdtools_results['valid'])
        print(f"Validity flags match: {valid_match}")

def benchmark_fingerprints(n_molecules: int = 1000) -> None:
    """Benchmark fingerprint calculations."""
    print(f"\nFingerprint Benchmark with {n_molecules:,} molecules...")
    print("-" * 50)
    
    if not RDTOOLS_AVAILABLE:
        print("RDTools not available - cannot benchmark fingerprints")
        return
    
    smiles_array = create_benchmark_data(n_molecules)
    
    # Benchmark fingerprint calculation
    start_time = time.time()
    fingerprints = rdtools.morgan_fingerprints(smiles_array, radius=2, nbits=2048)
    end_time = time.time()
    
    fp_time = end_time - start_time
    
    print(f"Fingerprint calculation: {fp_time:.3f} seconds ({n_molecules/fp_time:.0f} mol/sec)")
    print(f"Generated fingerprints shape: {fingerprints.shape}")
    print(f"Memory usage: {fingerprints.nbytes / 1024 / 1024:.1f} MB")

def main():
    """Run all benchmarks."""
    print("RDTools Performance Benchmark")
    print("=" * 50)
    
    # Check availability
    if not RDKIT_AVAILABLE:
        print("⚠️  RDKit not available - limited benchmarking")
    
    if not RDTOOLS_AVAILABLE:
        print("⚠️  RDTools not available - cannot run benchmarks")
        print("Please build the extension first:")
        print("  uv run python build.py")
        return
    
    # Run benchmarks with different sizes
    sizes = [100, 1000, 5000]
    
    for size in sizes:
        run_benchmark(size)
    
    # Benchmark fingerprints
    benchmark_fingerprints(1000)
    
    print(f"\nBenchmark completed!")
    print(f"\nNote: Performance depends on:")
    print(f"- CPU architecture and clock speed")
    print(f"- Memory bandwidth")
    print(f"- Compiler optimizations")
    print(f"- RDKit version and build options")

if __name__ == "__main__":
    main()