#!/usr/bin/env python3
"""
Pytest suite for TensorFlow custom ops integration.
"""

import pytest

tf = pytest.importorskip("tensorflow")
tf_ops = pytest.importorskip("rdktools.tf_ops")

if not tf_ops.is_tf_ops_available():
    pytest.skip(
        "rdktools TensorFlow custom ops not available; build the extension first",
        allow_module_level=True,
    )


def test_string_process_passthrough():
    inputs = tf.constant(["CCO", "c1ccccc1", "CC(=O)O"])

    outputs = tf_ops.string_process(inputs)

    assert outputs.shape == inputs.shape
    assert tf.reduce_all(tf.equal(inputs, outputs)).numpy()


def test_create_tf_dataset_batches():
    smiles = ["CCO", "c1ccccc1", "CC(=O)O", "CCC"]
    dataset = tf_ops.create_tf_dataset_op(smiles, batch_size=2, prefetch=False)

    batches = list(dataset)
    assert len(batches) == 2

    flattened = tf.concat(batches, axis=0).numpy()
    assert list(flattened) == [s.encode() for s in smiles]
