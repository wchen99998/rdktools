#include "tf_string_op.hpp"
#include "ecfp_trace.hpp"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/shape_inference.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_shape.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/logging.h"
#include <exception>
#include <string>
#include <vector>

namespace tensorflow {

// Register the custom op
REGISTER_OP("StringProcess")
    .Input("input_strings: string")
    .Output("output_strings: string")
    .Output("output_fingerprints: uint8")
    .SetShapeFn([](::tensorflow::shape_inference::InferenceContext* c) {
      // Output shape for traces matches input
      const auto input_shape = c->input(0);
      c->set_output(0, input_shape);

      // Fingerprint output has an extra trailing dimension for the bit vector
      ::tensorflow::shape_inference::ShapeHandle bit_vector =
          c->Vector(static_cast<int64_t>(
              rdktools::kECFPReasoningFingerprintSize));
      ::tensorflow::shape_inference::ShapeHandle fingerprint_shape;
      TF_RETURN_IF_ERROR(
          c->Concatenate(input_shape, bit_vector, &fingerprint_shape));
      c->set_output(1, fingerprint_shape);
      return ::tensorflow::OkStatus();
    })
    .Doc(R"doc(
Generate ECFP (Morgan fingerprint) reasoning traces for SMILES tensors.

Each input SMILES string is converted into a multi-line explanation describing
which fragment environments contribute to the fingerprint, including a per-atom
chain summary. Invalid SMILES yield the literal string "[invalid]".

input_strings: A tensor of SMILES strings to analyse.
output_strings: A tensor of reasoning traces with the same shape as input.
output_fingerprints: A tensor containing Morgan bit vectors alongside each trace.
)doc");

// Kernel implementation
StringProcessOp::StringProcessOp(OpKernelConstruction* context) : OpKernel(context) {
  // Constructor can be used to read attributes if needed
  // For example: OP_REQUIRES_OK(context, context->GetAttr("some_attr", &some_value_));
}

void StringProcessOp::Compute(OpKernelContext* context) {
  // Get the input tensor
  const Tensor& input_tensor = context->input(0);

  // Validate input
  OP_REQUIRES(context, input_tensor.dtype() == DT_STRING,
              errors::InvalidArgument("Input must be of type string"));

  // Create output tensor with the same shape as input
  Tensor* output_tensor = nullptr;
  OP_REQUIRES_OK(context,
                 context->allocate_output(0, input_tensor.shape(),
                                          &output_tensor));

  Tensor* fingerprint_tensor = nullptr;
  TensorShape fingerprint_shape = input_tensor.shape();
  fingerprint_shape.AddDim(
      static_cast<int64_t>(rdktools::kECFPReasoningFingerprintSize));
  OP_REQUIRES_OK(context,
                 context->allocate_output(1, fingerprint_shape,
                                          &fingerprint_tensor));

  // Get flat views of the tensors
  auto input_flat = input_tensor.flat<tstring>();
  auto output_flat = output_tensor->flat<tstring>();
  auto fingerprint_flat = fingerprint_tensor->flat<uint8>();

  const int64_t num_elements = input_flat.size();
  for (int64_t i = 0; i < num_elements; ++i) {
    const std::string smiles = input_flat(i);
    rdktools::ReasoningTraceResult trace_result;
    try {
      trace_result = rdktools::ecfp_reasoning_trace_from_smiles(
          smiles, 2U, true, false, true);
    } catch (const std::exception& e) {
      trace_result = rdktools::ReasoningTraceResult(
          std::string("[error] ") + e.what(),
          std::vector<std::uint8_t>(
              rdktools::kECFPReasoningFingerprintSize, 0));
    }

    std::string trace = std::move(std::get<0>(trace_result));
    std::vector<std::uint8_t> fingerprint =
        std::move(std::get<1>(trace_result));

    if (fingerprint.size() != rdktools::kECFPReasoningFingerprintSize) {
      fingerprint.resize(rdktools::kECFPReasoningFingerprintSize, 0);
    }

    if (trace.empty()) {
      if (smiles.empty()) {
        output_flat(i) = "";
      } else {
        output_flat(i) = "[invalid]";
      }
    } else {
      output_flat(i) = std::move(trace);
    }

    const int64_t base_index =
        i * static_cast<int64_t>(rdktools::kECFPReasoningFingerprintSize);
    for (std::size_t j = 0; j < fingerprint.size(); ++j) {
      fingerprint_flat(base_index + static_cast<int64_t>(j)) =
          fingerprint[j];
    }
  }
}

// Register the kernel for CPU
REGISTER_KERNEL_BUILDER(Name("StringProcess").Device(DEVICE_CPU), StringProcessOp);

}  // namespace tensorflow
