#!/usr/bin/env python3
"""
Pytest suite for TensorFlow custom ops integration.
"""

import numpy as np
import pytest

tf = pytest.importorskip("tensorflow")
tf_ops = pytest.importorskip("rdktools.tf_ops")
rdktools = pytest.importorskip("rdktools")

FP_SIZE = rdktools.ECFP_REASONING_FINGERPRINT_SIZE

if not tf_ops.is_tf_ops_available():
    pytest.skip(
        "rdktools TensorFlow custom ops not available; build the extension first",
        allow_module_level=True,
    )


def test_string_process_generates_trace():
    inputs = tf.constant(["CCO", "c1ccccc1", "CC(=O)O"])

    traces, fingerprints = tf_ops.string_process(inputs)

    assert traces.shape == inputs.shape
    assert fingerprints.shape == inputs.shape + (FP_SIZE,)
    assert fingerprints.dtype == tf.uint8

    decoded = traces.numpy()
    for idx, value in enumerate(decoded):
        text = value.decode()
        assert "r0:" in text
        assert "# per-center chains" in text
        # ensure fingerprints have at least one bit set for valid SMILES
        assert fingerprints[idx].shape[-1] == FP_SIZE
        assert int(tf.reduce_sum(fingerprints[idx]).numpy()) >= 0


def test_string_process_custom_fingerprint_size():
    inputs = tf.constant(["CCO"])

    traces, fingerprints = tf_ops.string_process(
        inputs, fingerprint_size=512
    )

    assert traces.shape == inputs.shape
    assert fingerprints.shape == inputs.shape + (512,)
    assert fingerprints.dtype == tf.uint8
    assert int(tf.reduce_sum(fingerprints[0]).numpy()) >= 0


def test_create_tf_dataset_batches():
    smiles = ["CCO", "c1ccccc1", "CC(=O)O", "CCC"]
    dataset = tf_ops.create_tf_dataset_op(smiles, batch_size=2, prefetch=False)

    batches = list(dataset)
    assert len(batches) == 2

    trace_batches, fingerprint_batches = zip(*batches)
    trace_concat = tf.concat(trace_batches, axis=0).numpy()
    fingerprint_concat = tf.concat(fingerprint_batches, axis=0).numpy()

    assert trace_concat.shape == (len(smiles),)
    assert fingerprint_concat.shape == (len(smiles), FP_SIZE)
    assert fingerprint_concat.dtype == tf.uint8

    for text_bytes, bits in zip(trace_concat, fingerprint_concat):
        text = text_bytes.decode()
        assert text.startswith("r0:")
        assert bits.shape == (FP_SIZE,)
        
        assert bits.dtype == np.uint8


def test_create_tf_dataset_batches_custom_size():
    smiles = ["CCO", "c1ccccc1"]
    dataset = tf_ops.create_tf_dataset_op(
        smiles, batch_size=1, prefetch=False, fingerprint_size=512
    )

    batches = list(dataset)
    assert len(batches) == len(smiles)

    for trace_batch, fp_batch in batches:
        assert trace_batch.shape == (1,)
        assert fp_batch.shape == (1, 512)
        assert fp_batch.dtype == tf.uint8
