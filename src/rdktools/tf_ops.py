"""
TensorFlow custom operations for RDK-Tools.

This module provides TensorFlow custom operations for use in TF data pipelines.
These operations can be used to process molecular data efficiently within
TensorFlow graphs and tf.data pipelines.
"""

import os
from typing import Optional, Tuple

import numpy as np
import tensorflow as tf


def _load_tf_ops():
    """Load the TensorFlow custom ops library."""
    try:
        # Get the directory where this module is located
        module_dir = os.path.dirname(os.path.abspath(__file__))
        
        # Look for the shared library
        lib_path = os.path.join(module_dir, 'rdktools_tf_ops.so')
        
        if not os.path.exists(lib_path):
            raise ImportError(f"TensorFlow ops library not found at {lib_path}")
        
        # Load the custom ops
        tf_ops_module = tf.load_op_library(lib_path)
        return tf_ops_module
        
    except Exception as e:
        raise ImportError(f"Failed to load TensorFlow custom ops: {e}")


# Load the ops module
try:
    _tf_ops_module = _load_tf_ops()
    _TF_OPS_AVAILABLE = True
except ImportError as e:
    _tf_ops_module = None
    _TF_OPS_AVAILABLE = False
    _import_error = e


def _check_tf_ops():
    """Check if TensorFlow custom ops are available."""
    if not _TF_OPS_AVAILABLE:
        raise ImportError(
            f"RDK-Tools TensorFlow ops not available: {_import_error}. "
            "Please ensure TensorFlow is installed and the library was built with TensorFlow support."
        )


def string_process(
    input_strings: tf.Tensor,
    name: Optional[str] = None,
    fingerprint_size: int = 2048,
) -> Tuple[tf.Tensor, tf.Tensor]:
    """
    Generate reasoning traces and Morgan fingerprints for SMILES tensors.
    
    Args:
        input_strings: A string tensor to process.
        name: Optional name for the operation.
        fingerprint_size: Desired fingerprint length in bits. Non-positive
            values fall back to the default of 2048.
        
    Returns:
        Tuple `(traces, fingerprints)` where `traces` matches the input shape
        with string tensors containing the reasoning trace, and `fingerprints`
        appends a trailing dimension of size `fingerprint_size` (default 2048)
        with the uint8 fingerprint.
        
    Raises:
        ImportError: If TensorFlow custom ops are not available.
        
    Example:
        >>> import tensorflow as tf
        >>> from rdktools.tf_ops import string_process
        >>> 
        >>> # Create a dataset with SMILES strings
        >>> smiles = tf.constant(["CCO", "c1ccccc1", "CC(=O)O"])
        >>> traces, fps = string_process(smiles)
        >>> print(traces.numpy()[0])
        >>> print(fps.numpy()[0].shape)
        
        # Use in a tf.data pipeline
        >>> dataset = tf.data.Dataset.from_tensor_slices(["CCO", "c1ccccc1"])
        >>> dataset = dataset.map(string_process)
        >>> for traces, fps in dataset.take(1):
        ...     print(traces.numpy()[0], fps.numpy()[0].sum())
    """
    _check_tf_ops()

    if not isinstance(fingerprint_size, int):
        raise TypeError("fingerprint_size must be an int")
    size = fingerprint_size if fingerprint_size > 0 else 2048

    return _tf_ops_module.string_process(
        input_strings, fingerprint_size=size, name=name
    )


def create_tf_dataset_op(
    smiles,
    *,
    batch_size: int = 32,
    shuffle: bool = False,
    shuffle_buffer_size: Optional[int] = None,
    prefetch: bool = True,
    fingerprint_size: int = 2048,
) -> tf.data.Dataset:
    """
    Convenience helper that builds a tf.data pipeline backed by the custom op.

    Args:
        smiles: Iterable or tensor of SMILES strings.
        batch_size: Number of elements per batch.
        shuffle: Whether to shuffle the dataset.
        shuffle_buffer_size: Buffer size for shuffling. Defaults to dataset size.
        prefetch: Whether to add a `prefetch(tf.data.AUTOTUNE)` stage.
        fingerprint_size: Desired fingerprint length for the op. Non-positive
            values fall back to 2048.

    Returns:
        Configured tf.data.Dataset yielding batches of `(traces, fingerprints)`
        tensors produced by the custom op.
    """
    _check_tf_ops()

    if isinstance(smiles, (list, tuple)):
        smiles = tf.constant(smiles, dtype=tf.string)
    elif isinstance(smiles, np.ndarray):
        smiles = tf.constant(smiles.astype(str), dtype=tf.string)

    if isinstance(smiles, tf.Tensor):
        dataset = tf.data.Dataset.from_tensor_slices(smiles)
    else:
        dataset = tf.data.Dataset.from_generator(
            lambda: smiles,
            output_signature=tf.TensorSpec(shape=(), dtype=tf.string),
        )

    dataset = dataset.map(
        lambda value: string_process(
            value, fingerprint_size=fingerprint_size
        ),
        num_parallel_calls=tf.data.AUTOTUNE,
    )

    if shuffle:
        if shuffle_buffer_size is None:
            try:
                shuffle_buffer_size = len(smiles)  # type: ignore[arg-type]
            except TypeError:
                raise ValueError(
                    "shuffle_buffer_size must be provided when using generators"
                ) from None
        dataset = dataset.shuffle(shuffle_buffer_size)

    dataset = dataset.batch(batch_size)

    if prefetch:
        dataset = dataset.prefetch(tf.data.AUTOTUNE)

    return dataset

# Export public API
__all__ = [
    "string_process",
    "create_tf_dataset_op",
]


# Utility function to check if TensorFlow ops are available
def is_tf_ops_available() -> bool:
    """Check if TensorFlow custom ops are available."""
    return _TF_OPS_AVAILABLE
