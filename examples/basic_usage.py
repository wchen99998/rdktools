#!/usr/bin/env python3
"""
Example script demonstrating rdktools library usage with numpy arrays.

This script shows how to use rdktools for high-performance molecular operations
on large arrays of SMILES strings using RDKit's C++ core.
"""

import numpy as np
import time
from typing import List

# Note: In a real installation, you would import like this:
# import rdktools

# For this example, we'll handle the import gracefully
try:
    import rdktools
    RDTOOLS_AVAILABLE = True
except ImportError:
    RDTOOLS_AVAILABLE = False
    print("rdktools C++ extension not built yet. Please build it first.")
    print("Run: uv run python setup.py build_ext --inplace")

def create_sample_data() -> np.ndarray:
    """Create sample SMILES data for testing."""
    sample_smiles = [
        'CCO',                    # ethanol
        'c1ccccc1',              # benzene
        'CC(C)O',                # isopropanol
        'c1ccc(O)cc1',           # phenol
        'CCN(CC)CC',             # triethylamine
        'c1cnc(C)nc1',           # methylpyrimidine
        'CCCCCCCC',              # octane
        'C(C(C(C(C(C(F)(F)F)(F)F)(F)F)(F)F)(F)F)(F)F', # perfluorohexane
        'c1cc(Cl)ccc1O',         # 4-chlorophenol
        'invalid_smiles',        # invalid SMILES for testing
        'CC(=O)O',               # acetic acid
        'c1ccc2c(c1)cccc2',      # naphthalene
    ]
    
    # Create larger dataset by repeating
    large_dataset = sample_smiles * 100  # 1200 molecules
    
    return np.array(large_dataset, dtype=str)

def demonstrate_basic_functions():
    """Demonstrate basic molecular descriptor calculations."""
    print("=== Basic Molecular Descriptor Calculations ===")
    
    # Sample data
    smiles = np.array(['CCO', 'c1ccccc1', 'CC(=O)O', 'invalid'])
    print(f"Input SMILES: {smiles}")
    
    if not RDTOOLS_AVAILABLE:
        print("rdktools not available - skipping demonstrations")
        return
    
    # Validate SMILES
    valid = rdktools.is_valid(smiles)
    print(f"Valid SMILES: {valid}")
    
    # Calculate molecular weights
    mw = rdktools.molecular_weights(smiles)
    print(f"Molecular weights: {mw}")
    
    # Calculate LogP
    logp_vals = rdktools.logp(smiles)
    print(f"LogP values: {logp_vals}")
    
    # Calculate TPSA
    tpsa_vals = rdktools.tpsa(smiles)
    print(f"TPSA values: {tpsa_vals}")
    
    # Get canonical SMILES
    canonical = rdktools.canonical_smiles(smiles)
    print(f"Canonical SMILES: {canonical}")
    
    print()

def demonstrate_batch_processing():
    """Demonstrate efficient batch processing of large datasets."""
    print("=== Batch Processing Demonstration ===")
    
    if not RDTOOLS_AVAILABLE:
        print("rdktools not available - skipping demonstrations")
        return
    
    # Create large dataset
    large_smiles = create_sample_data()
    print(f"Processing {len(large_smiles)} SMILES strings...")
    
    # Time the batch processing
    start_time = time.time()
    
    # Method 1: Calculate all descriptors at once (most efficient)
    results = rdktools.descriptors(large_smiles)
    
    end_time = time.time()
    processing_time = end_time - start_time
    
    print(f"Processed {len(large_smiles)} molecules in {processing_time:.3f} seconds")
    print(f"Rate: {len(large_smiles)/processing_time:.0f} molecules/second")
    
    # Show some statistics
    valid_mask = ~np.isnan(results['molecular_weight'])
    n_valid = np.sum(valid_mask)
    
    print(f"\nResults summary:")
    print(f"Valid molecules: {n_valid}/{len(large_smiles)}")
    print(f"Average molecular weight: {np.nanmean(results['molecular_weight']):.2f}")
    print(f"Average LogP: {np.nanmean(results['logp']):.2f}")
    print(f"Average TPSA: {np.nanmean(results['tpsa']):.2f}")
    
    print()

def demonstrate_fingerprints():
    """Demonstrate fingerprint calculations."""
    print("=== Morgan Fingerprint Demonstration ===")
    
    if not RDTOOLS_AVAILABLE:
        print("rdktools not available - skipping demonstrations")
        return
    
    # Sample molecules
    smiles = np.array(['CCO', 'c1ccccc1', 'CC(=O)O'])
    print(f"Calculating fingerprints for: {smiles}")
    
    # Calculate fingerprints
    fps = rdktools.morgan_fingerprints(smiles, radius=2, nbits=1024)
    
    print(f"Fingerprint matrix shape: {fps.shape}")
    print(f"Fingerprint data type: {fps.dtype}")
    
    # Show fingerprint statistics
    for i, smi in enumerate(smiles):
        n_bits_set = np.sum(fps[i])
        print(f"  {smi}: {n_bits_set} bits set")
    
    # Calculate Tanimoto similarity between first two molecules
    fp1, fp2 = fps[0], fps[1]
    intersection = np.sum(fp1 & fp2)
    union = np.sum(fp1 | fp2)
    tanimoto = intersection / union if union > 0 else 0.0
    
    print(f"Tanimoto similarity between '{smiles[0]}' and '{smiles[1]}': {tanimoto:.3f}")
    
    print()

def demonstrate_advanced_batch():
    """Demonstrate advanced batch processing with filtering."""
    print("=== Advanced Batch Processing ===")
    
    if not RDTOOLS_AVAILABLE:
        print("rdktools not available - skipping demonstrations")
        return
    
    # Create test data with some invalid SMILES
    test_smiles = np.array([
        'CCO', 'CCC', 'c1ccccc1', 'invalid1', 'CC(=O)O', 
        'bad_smiles', 'c1cnc(C)nc1', 'CCCCCCCC'
    ])
    
    print(f"Input: {test_smiles}")
    
    # Filter to valid SMILES only
    valid_smiles = rdktools.filter_valid(test_smiles)
    print(f"Valid SMILES: {valid_smiles}")
    
    # Use the comprehensive batch processing function
    batch_results = rdktools.batch_process(
        test_smiles,
        batch_size=4,  # Small batch for demonstration
        include_descriptors=True,
        include_fingerprints=True,
        radius=2,
        nbits=512
    )
    
    print(f"\nBatch processing results:")
    print(f"Valid molecules: {np.sum(batch_results['valid'])}/{len(test_smiles)}")
    
    # Show results for valid molecules only
    valid_indices = batch_results['valid']
    if np.any(valid_indices):
        print(f"Valid molecular weights: {batch_results['molecular_weight'][valid_indices]}")
        print(f"Fingerprint shape: {batch_results['fingerprints'][valid_indices].shape}")
    
    print()

def compare_performance():
    """Compare performance of different approaches."""
    print("=== Performance Comparison ===")
    
    if not RDTOOLS_AVAILABLE:
        print("rdktools not available - skipping demonstrations")
        return
    
    # Create moderately sized dataset
    smiles = create_sample_data()[:100]  # 100 molecules
    
    print(f"Comparing performance on {len(smiles)} molecules:")
    
    # Method 1: Individual function calls
    start = time.time()
    mw1 = rdktools.molecular_weights(smiles)
    logp1 = rdktools.logp(smiles)
    tpsa1 = rdktools.tpsa(smiles)
    time1 = time.time() - start
    
    # Method 2: Batch descriptors function
    start = time.time()
    results2 = rdktools.descriptors(smiles)
    time2 = time.time() - start
    
    print(f"Individual calls: {time1:.4f} seconds")
    print(f"Batch calculation: {time2:.4f} seconds")
    print(f"Speedup: {time1/time2:.1f}x")
    
    # Verify results are the same
    if np.allclose(mw1, results2['molecular_weight'], equal_nan=True):
        print("✓ Results are identical")
    else:
        print("✗ Results differ!")
    
    print()

def main():
    """Run all demonstrations."""
    print("RDTools Demonstration Script")
    print("=" * 40)
    
    if not RDTOOLS_AVAILABLE:
        print("\nTo use this library, first build the C++ extension:")
        print("1. Make sure RDKit is installed (conda install rdkit)")
        print("2. Build the extension: uv run python setup.py build_ext --inplace")
        print("3. Or use CMake: mkdir build && cd build && cmake .. && make")
        return
    
    demonstrate_basic_functions()
    demonstrate_batch_processing()
    demonstrate_fingerprints()
    demonstrate_advanced_batch()
    compare_performance()
    
    print("All demonstrations completed!")

if __name__ == "__main__":
    main()