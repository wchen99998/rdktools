#include "tf_string_op.hpp"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/shape_inference.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_shape.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/logging.h"

namespace tensorflow {

// Register the custom op
REGISTER_OP("StringProcess")
    .Input("input_strings: string")
    .Output("output_strings: string")
    .SetShapeFn([](::tensorflow::shape_inference::InferenceContext* c) {
      // Output shape is the same as input shape
      c->set_output(0, c->input(0));
      return ::tensorflow::OkStatus();
    })
    .Doc(R"doc(
Process string tensors through a custom C++ operation for TensorFlow data pipelines.

This operation takes string tensors as input and returns string tensors as output.
Currently implemented as a pass-through operation that returns the input unchanged,
but can be extended to perform custom string processing operations.

input_strings: A tensor of strings to process.
output_strings: A tensor of processed strings with the same shape as input.
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
  OP_REQUIRES_OK(context, context->allocate_output(0, input_tensor.shape(), &output_tensor));

  // Get flat views of the tensors
  auto input_flat = input_tensor.flat<tstring>();
  auto output_flat = output_tensor->flat<tstring>();

  // Process strings (currently just pass-through)
  // This is where you would implement your custom string processing logic
  const int64_t num_elements = input_flat.size();
  for (int64_t i = 0; i < num_elements; ++i) {
    // For now, just copy the input to output
    // TODO: Replace with actual string processing logic
    output_flat(i) = input_flat(i);
  }
}

// Register the kernel for CPU
REGISTER_KERNEL_BUILDER(Name("StringProcess").Device(DEVICE_CPU), StringProcessOp);

}  // namespace tensorflow