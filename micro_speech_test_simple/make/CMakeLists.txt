#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
PROJECT(tflitemcu_speech_test CXX)

add_library(tflitemcu_speech_test STATIC
tensorflow/lite/experimental/micro/examples/micro_speech/micro_speech_test.cc 
tensorflow/lite/experimental/micro/examples/micro_speech/micro_features/no_micro_features_data.cc 
tensorflow/lite/experimental/micro/examples/micro_speech/micro_features/tiny_conv_micro_features_model_data.cc 
tensorflow/lite/experimental/micro/examples/micro_speech/micro_features/yes_micro_features_data.cc 
tensorflow/lite/experimental/micro/micro_error_reporter.cc 
tensorflow/lite/experimental/micro/micro_mutable_op_resolver.cc 
tensorflow/lite/experimental/micro/simple_tensor_allocator.cc 
tensorflow/lite/experimental/micro/azuresphere/debug_log.cc 
tensorflow/lite/experimental/micro/debug_log_numbers.cc 
tensorflow/lite/experimental/micro/micro_interpreter.cc 
tensorflow/lite/experimental/micro/micro_allocator.cc

tensorflow/lite/experimental/micro/kernels/all_ops_resolver.cc 
tensorflow/lite/experimental/micro/kernels/arg_min_max.cc
tensorflow/lite/experimental/micro/kernels/ceil.cc
tensorflow/lite/experimental/micro/kernels/comparisons.cc
tensorflow/lite/experimental/micro/kernels/conv.cc
tensorflow/lite/experimental/micro/kernels/depthwise_conv.cc 
tensorflow/lite/experimental/micro/kernels/elementwise.cc
tensorflow/lite/experimental/micro/kernels/floor.cc
tensorflow/lite/experimental/micro/kernels/fully_connected.cc 
tensorflow/lite/experimental/micro/kernels/logical.cc
tensorflow/lite/experimental/micro/kernels/maximum_minimum.cc
tensorflow/lite/experimental/micro/kernels/pack.cc
tensorflow/lite/experimental/micro/kernels/pooling.cc
tensorflow/lite/experimental/micro/kernels/prelu.cc
tensorflow/lite/experimental/micro/kernels/reshape.cc
tensorflow/lite/experimental/micro/kernels/round.cc
tensorflow/lite/experimental/micro/kernels/softmax.cc 
tensorflow/lite/experimental/micro/kernels/split.cc
tensorflow/lite/experimental/micro/kernels/strided_slice.cc
tensorflow/lite/experimental/micro/kernels/unpack.cc

tensorflow/lite/c/c_api_internal.cc 
tensorflow/lite/core/api/error_reporter.cc 
tensorflow/lite/core/api/flatbuffer_conversions.cc 
tensorflow/lite/core/api/op_resolver.cc 
tensorflow/lite/kernels/kernel_util.cc 
tensorflow/lite/kernels/internal/quantization_util.cc
)

#target_link_libraries(tflitemcu_speech_test g)
include_directories(${CMAKE_SOURCE_DIR}/ 
					${CMAKE_SOURCE_DIR}/third_party/flatbuffers/include/ 
					${CMAKE_SOURCE_DIR}/third_party/gemmlowp/ 
)
