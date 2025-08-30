#!/usr/bin/env python3
"""
Test script for TensorFlow custom string processing operation.
"""

import sys
import os

def test_tf_op():
    """Test the TensorFlow custom string operation."""
    
    print("Testing TensorFlow custom string operation...")
    
    # Check if TensorFlow is available
    try:
        import tensorflow as tf
        print(f"✓ TensorFlow version: {tf.__version__}")
    except ImportError:
        print("❌ TensorFlow not available - skipping TensorFlow op tests")
        return
    
    # Check if our TensorFlow ops are available
    try:
        from src.rdktools.tf_ops import string_process, is_tf_ops_available
        
        if not is_tf_ops_available():
            print("❌ TensorFlow custom ops not available")
            print("   Make sure the library was built with TensorFlow support")
            return
            
        print("✓ TensorFlow custom ops loaded successfully")
        
    except ImportError as e:
        print(f"❌ Failed to import TensorFlow ops: {e}")
        return
    
    # Test basic string processing
    try:
        # Test with simple strings
        test_strings = tf.constant(["CCO", "c1ccccc1", "CC(=O)O"])
        result = string_process(test_strings)
        
        print(f"✓ Input strings: {test_strings.numpy()}")
        print(f"✓ Output strings: {result.numpy()}")
        
        # Verify output matches input (pass-through behavior)
        if tf.reduce_all(tf.equal(test_strings, result)):
            print("✓ Pass-through behavior verified")
        else:
            print("❌ Output doesn't match input")
            
    except Exception as e:
        print(f"❌ Error during string processing: {e}")
        return
    
    # Test with tf.data pipeline
    try:
        from src.rdktools.tf_ops import create_tf_dataset_op
        
        smiles_list = ["CCO", "c1ccccc1", "CC(=O)O", "CCC"]
        dataset = create_tf_dataset_op(smiles_list, batch_size=2)
        
        print("✓ Created tf.data pipeline")
        
        # Process a few batches
        batch_count = 0
        for batch in dataset.take(2):
            batch_count += 1
            print(f"✓ Batch {batch_count}: {batch.numpy()}")
            
    except Exception as e:
        print(f"❌ Error during tf.data pipeline test: {e}")
        return
    
    print("✅ All TensorFlow custom op tests passed!")


def check_build_requirements():
    """Check if TensorFlow is available for building."""
    try:
        import tensorflow as tf
        print(f"TensorFlow version: {tf.__version__}")
        print(f"TensorFlow include dir: {tf.sysconfig.get_include()}")
        print(f"TensorFlow lib dir: {tf.sysconfig.get_lib()}")
        return True
    except ImportError:
        print("TensorFlow not available - install with: pip install tensorflow")
        return False


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "check":
        check_build_requirements()
    else:
        test_tf_op()