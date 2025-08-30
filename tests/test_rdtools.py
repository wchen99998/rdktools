#!/usr/bin/env python3
"""
Tests for rdktools library.

Note: These tests require the C++ extension to be built.
Run: uv run python setup.py build_ext --inplace
"""

import pytest
import numpy as np
import numpy.testing as npt

# Handle import gracefully for testing without built extension
try:
    import rdktools
    RDTOOLS_AVAILABLE = True
except ImportError:
    RDTOOLS_AVAILABLE = False
    pytest.skip("rdktools C++ extension not available", allow_module_level=True)


class TestBasicFunctions:
    """Test basic molecular descriptor functions."""
    
    def test_molecular_weights(self):
        """Test molecular weight calculations."""
        smiles = np.array(['CCO', 'c1ccccc1', 'C'])
        weights = rdktools.molecular_weights(smiles)
        
        assert len(weights) == 3
        assert weights.dtype == np.float64
        
        # Test known values (approximate)
        assert abs(weights[0] - 46.07) < 0.1  # ethanol
        assert abs(weights[1] - 78.11) < 0.1  # benzene
        assert abs(weights[2] - 16.04) < 0.1  # methane
    
    def test_invalid_smiles(self):
        """Test handling of invalid SMILES."""
        smiles = np.array(['CCO', 'invalid_smiles', 'c1ccccc1'])
        weights = rdktools.molecular_weights(smiles)
        
        assert len(weights) == 3
        assert not np.isnan(weights[0])  # valid
        assert np.isnan(weights[1])      # invalid
        assert not np.isnan(weights[2])  # valid
    
    def test_logp_calculation(self):
        """Test LogP calculations."""
        smiles = np.array(['CCO', 'CCCCCCCC'])  # polar vs non-polar
        logp_vals = rdktools.logp(smiles)
        
        assert len(logp_vals) == 2
        assert logp_vals.dtype == np.float64
        
        # Octane should have higher LogP than ethanol
        assert logp_vals[1] > logp_vals[0]
    
    def test_tpsa_calculation(self):
        """Test TPSA calculations."""
        smiles = np.array(['CCO', 'CCCC'])  # with and without polar groups
        tpsa_vals = rdktools.tpsa(smiles)
        
        assert len(tpsa_vals) == 2
        assert tpsa_vals.dtype == np.float64
        
        # Ethanol should have higher TPSA than butane
        assert tpsa_vals[0] > tpsa_vals[1]
    
    def test_smiles_validation(self):
        """Test SMILES validation."""
        smiles = np.array(['CCO', 'invalid', 'c1ccccc1', 'bad_smiles'])
        valid = rdktools.is_valid(smiles)
        
        assert len(valid) == 4
        assert valid.dtype == bool
        assert valid[0] == True   # CCO
        assert valid[1] == False  # invalid
        assert valid[2] == True   # benzene
        assert valid[3] == False  # bad_smiles
    
    def test_canonical_smiles(self):
        """Test SMILES canonicalization."""
        smiles = np.array(['CCO', 'OCC'])  # different representations of ethanol
        canonical = rdktools.canonical_smiles(smiles)
        
        assert len(canonical) == 2
        assert canonical.dtype.kind in ['U', 'S', 'O']  # string type
        
        # Both should give the same canonical form
        assert canonical[0] == canonical[1]


class TestBatchOperations:
    """Test batch processing functions."""
    
    def test_multiple_descriptors(self):
        """Test calculating multiple descriptors at once."""
        smiles = np.array(['CCO', 'c1ccccc1', 'CC(=O)O'])
        results = rdktools.descriptors(smiles)
        
        assert isinstance(results, dict)
        assert 'molecular_weight' in results
        assert 'logp' in results
        assert 'tpsa' in results
        
        # Check array shapes and types
        for key in ['molecular_weight', 'logp', 'tpsa']:
            assert len(results[key]) == 3
            assert results[key].dtype == np.float64
    
    def test_filter_valid(self):
        """Test filtering to valid SMILES only."""
        smiles = np.array(['CCO', 'invalid', 'c1ccccc1'])
        valid_smiles = rdktools.filter_valid(smiles)
        
        assert len(valid_smiles) == 2
        assert 'CCO' in valid_smiles
        assert 'c1ccccc1' in valid_smiles
        assert 'invalid' not in valid_smiles
    
    def test_batch_process(self):
        """Test comprehensive batch processing."""
        smiles = np.array(['CCO', 'c1ccccc1', 'invalid', 'CC(=O)O'])
        
        results = rdktools.batch_process(
            smiles,
            batch_size=2,
            include_descriptors=True,
            include_fingerprints=True,
            radius=2,
            nbits=512
        )
        
        # Check all expected keys are present
        expected_keys = ['molecular_weight', 'logp', 'tpsa', 'valid', 
                        'canonical_smiles', 'fingerprints']
        for key in expected_keys:
            assert key in results
        
        # Check array dimensions
        assert len(results['valid']) == 4
        assert results['fingerprints'].shape == (4, 512)
        
        # Check validity flags
        assert results['valid'][0] == True   # CCO
        assert results['valid'][1] == True   # benzene  
        assert results['valid'][2] == False  # invalid
        assert results['valid'][3] == True   # acetic acid


class TestFingerprints:
    """Test fingerprint calculations."""
    
    def test_morgan_fingerprints_basic(self):
        """Test basic Morgan fingerprint calculation."""
        smiles = np.array(['CCO', 'c1ccccc1'])
        fps = rdktools.morgan_fingerprints(smiles, radius=2, nbits=1024)
        
        assert fps.shape == (2, 1024)
        assert fps.dtype == np.uint8
        
        # Check that fingerprints contain only 0s and 1s
        assert np.all((fps == 0) | (fps == 1))
        
        # Different molecules should have different fingerprints
        assert not np.array_equal(fps[0], fps[1])
    
    def test_morgan_fingerprints_parameters(self):
        """Test fingerprint parameter variations."""
        smiles = np.array(['CCO'])
        
        # Test different radii
        fp_r1 = rdktools.morgan_fingerprints(smiles, radius=1, nbits=512)
        fp_r2 = rdktools.morgan_fingerprints(smiles, radius=2, nbits=512)
        
        assert fp_r1.shape == (1, 512)
        assert fp_r2.shape == (1, 512)
        
        # Different radii should give different fingerprints
        assert not np.array_equal(fp_r1[0], fp_r2[0])
        
        # Test different bit counts
        fp_512 = rdktools.morgan_fingerprints(smiles, radius=2, nbits=512)
        fp_1024 = rdktools.morgan_fingerprints(smiles, radius=2, nbits=1024)
        
        assert fp_512.shape == (1, 512)
        assert fp_1024.shape == (1, 1024)
    
    def test_fingerprint_similarity(self):
        """Test fingerprint similarity calculations."""
        # Similar molecules should have similar fingerprints
        smiles = np.array(['CCO', 'CCC'])  # ethanol vs propane
        fps = rdktools.morgan_fingerprints(smiles, radius=2, nbits=1024)
        
        # Calculate Tanimoto similarity
        fp1, fp2 = fps[0], fps[1]
        intersection = np.sum(fp1 & fp2)
        union = np.sum(fp1 | fp2)
        tanimoto = intersection / union if union > 0 else 0.0
        
        # Should have some similarity but not be identical
        assert 0.0 < tanimoto < 1.0


class TestInputValidation:
    """Test input validation and error handling."""
    
    def test_empty_array(self):
        """Test handling of empty arrays."""
        empty_smiles = np.array([], dtype=str)
        
        weights = rdktools.molecular_weights(empty_smiles)
        assert len(weights) == 0
        assert weights.dtype == np.float64
    
    def test_list_input(self):
        """Test that lists are accepted and converted to arrays."""
        smiles_list = ['CCO', 'c1ccccc1']
        weights = rdktools.molecular_weights(smiles_list)
        
        assert len(weights) == 2
        assert weights.dtype == np.float64
    
    def test_invalid_array_dimensions(self):
        """Test error handling for invalid array dimensions."""
        # 2D array should raise error
        smiles_2d = np.array([['CCO', 'CCC'], ['c1ccccc1', 'CC(=O)O']])
        
        with pytest.raises(ValueError, match="1-dimensional"):
            rdktools.molecular_weights(smiles_2d)
    
    def test_type_conversion(self):
        """Test automatic type conversion for different input types."""
        # Test with different numpy dtypes
        smiles_object = np.array(['CCO', 'c1ccccc1'], dtype=object)
        smiles_unicode = np.array(['CCO', 'c1ccccc1'], dtype='U10')
        
        weights_obj = rdktools.molecular_weights(smiles_object)
        weights_uni = rdktools.molecular_weights(smiles_unicode)
        
        # Results should be the same regardless of input string type
        npt.assert_array_equal(weights_obj, weights_uni)


class TestConsistency:
    """Test consistency between different calculation methods."""
    
    def test_individual_vs_batch_descriptors(self):
        """Test that individual and batch calculations give same results."""
        smiles = np.array(['CCO', 'c1ccccc1', 'CC(=O)O'])
        
        # Individual calculations
        mw_individual = rdktools.molecular_weights(smiles)
        logp_individual = rdktools.logp(smiles)
        tpsa_individual = rdktools.tpsa(smiles)
        
        # Batch calculation
        batch_results = rdktools.descriptors(smiles)
        
        # Results should be identical
        npt.assert_array_equal(mw_individual, batch_results['molecular_weight'])
        npt.assert_array_equal(logp_individual, batch_results['logp'])
        npt.assert_array_equal(tpsa_individual, batch_results['tpsa'])
    
    def test_canonical_smiles_consistency(self):
        """Test canonical SMILES consistency."""
        # Different representations of the same molecule
        smiles = np.array(['CCO', 'OCC', 'C(O)C'])
        canonical = rdktools.canonical_smiles(smiles)
        
        # All should give the same canonical form
        assert canonical[0] == canonical[1] == canonical[2]


if __name__ == "__main__":
    pytest.main([__file__])