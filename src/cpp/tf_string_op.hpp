#pragma once

#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/shape_inference.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_shape.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/logging.h"

namespace tensorflow {

// Forward declaration of the StringProcessOp kernel
class StringProcessOp : public OpKernel {
 public:
  explicit StringProcessOp(OpKernelConstruction* context);
  void Compute(OpKernelContext* context) override;

 private:
  StringProcessOp(const StringProcessOp&) = delete;
  void operator=(const StringProcessOp&) = delete;
};

}  // namespace tensorflow