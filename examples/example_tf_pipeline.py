#!/usr/bin/env python3
"""
Example demonstrating TensorFlow custom string operation in a data pipeline.

This shows how to use the custom C++ string processing operation within
TensorFlow's tf.data API for high-performance data processing.
"""

import tensorflow as tf
from src.rdktools.tf_ops import string_process, create_tf_dataset_op

def main():
    print("🧪 TensorFlow Data Pipeline Example with Custom String Operation")
    print("=" * 60)
    
    # Example SMILES strings (molecular representations)
    smiles_data = [
        "CCO",           # Ethanol
        "c1ccccc1",      # Benzene
        "CC(=O)O",       # Acetic acid
        "CCC",           # Propane
        "CCCO",          # Propanol
        "c1ccc(O)cc1",   # Phenol
        "CC(C)O",        # Isopropanol
        "CCCCO",         # Butanol
    ]
    
    print(f"📊 Processing {len(smiles_data)} SMILES strings...")
    print(f"Input data: {smiles_data}")
    print()
    
    # Method 1: Direct use of string_process operation
    print("🔧 Method 1: Direct string processing")
    input_tensor = tf.constant(smiles_data)
    processed_tensor = string_process(input_tensor)
    
    print(f"Original: {input_tensor.numpy()}")
    print(f"Processed: {processed_tensor.numpy()}")
    print("✅ Direct processing completed\n")
    
    # Method 2: Using tf.data pipeline with custom operation
    print("🔧 Method 2: tf.data pipeline with custom operation")
    
    # Create dataset
    dataset = tf.data.Dataset.from_tensor_slices(smiles_data)
    
    # Apply custom string processing operation
    dataset = dataset.map(
        lambda x: string_process(x),
        num_parallel_calls=tf.data.AUTOTUNE
    )
    
    # Batch the data
    dataset = dataset.batch(3)
    
    # Prefetch for performance
    dataset = dataset.prefetch(tf.data.AUTOTUNE)
    
    print("Processing batches:")
    for i, batch in enumerate(dataset):
        print(f"  Batch {i+1}: {batch.numpy()}")
    
    print("✅ Pipeline processing completed\n")
    
    # Method 3: Using convenience function
    print("🔧 Method 3: Using convenience function")
    
    dataset = create_tf_dataset_op(smiles_data, batch_size=4)
    
    print("Processing with convenience function:")
    for i, batch in enumerate(dataset):
        print(f"  Batch {i+1}: {batch.numpy()}")
    
    print("✅ Convenience function completed\n")
    
    # Method 4: Simulating a real data pipeline
    print("🔧 Method 4: Realistic data pipeline simulation")
    
    # Simulate reading from files or database
    def simulate_data_source():
        """Simulate a data source that yields SMILES strings."""
        for smiles in smiles_data:
            yield smiles
    
    # Create dataset from generator
    dataset = tf.data.Dataset.from_generator(
        simulate_data_source,
        output_signature=tf.TensorSpec(shape=(), dtype=tf.string)
    )
    
    # Apply transformations
    dataset = (dataset
        .map(string_process, num_parallel_calls=tf.data.AUTOTUNE)  # Custom C++ op
        .batch(2)
        .prefetch(tf.data.AUTOTUNE)
    )
    
    print("Realistic pipeline processing:")
    for i, batch in enumerate(dataset):
        print(f"  Batch {i+1}: {batch.numpy()}")
    
    print("✅ Realistic pipeline completed\n")
    
    print("🎉 All examples completed successfully!")
    print("\n📝 Notes:")
    print("- The string_process operation currently implements pass-through behavior")
    print("- You can extend the C++ implementation to perform custom string processing")
    print("- This pattern works efficiently with large datasets in production")
    print("- The operation integrates seamlessly with TensorFlow's execution graph")


if __name__ == "__main__":
    main()