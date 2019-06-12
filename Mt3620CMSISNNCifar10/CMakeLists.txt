#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
PROJECT(Cifar10_RTApp_MT3620_BareMetal C)
add_library(cmsisnn STATIC
	#./CMSIS/NN/Source/ActivationFunctions/arm_nn_activations_q15.c
	#./CMSIS/NN/Source/ActivationFunctions/arm_nn_activations_q7.c
	#./CMSIS/NN/Source/ActivationFunctions/arm_relu_q15.c
	./CMSIS/NN/Source/ActivationFunctions/arm_relu_q7.c
	#./CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_1x1_HWC_q7_fast_nonsquare.c
	#./CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q15_basic.c
	#./CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q15_fast.c
	#./CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q15_fast_nonsquare.c
	#./CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q7_basic.c
	#./CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q7_basic_nonsquare.c
	./CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q7_fast.c
	#./CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q7_fast_nonsquare.c
	./CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q7_RGB.c
	#./CMSIS/NN/Source/ConvolutionFunctions/arm_depthwise_separable_conv_HWC_q7.c
	#./CMSIS/NN/Source/ConvolutionFunctions/arm_depthwise_separable_conv_HWC_q7_nonsquare.c
	./CMSIS/NN/Source/ConvolutionFunctions/arm_nn_mat_mult_kernel_q7_q15.c
	./CMSIS/NN/Source/ConvolutionFunctions/arm_nn_mat_mult_kernel_q7_q15_reordered.c
	#./CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_mat_q7_vec_q15.c
	#./CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_mat_q7_vec_q15_opt.c
	#./CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q15.c
	#./CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q15_opt.c
	./CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q7.c
	./CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q7_opt.c
	#./CMSIS/NN/Source/NNSupportFunctions/arm_nn_mult_q15.c
	#./CMSIS/NN/Source/NNSupportFunctions/arm_nn_mult_q7.c
	#./CMSIS/NN/Source/NNSupportFunctions/arm_nntables.c
	./CMSIS/NN/Source/NNSupportFunctions/arm_q7_to_q15_no_shift.c
	./CMSIS/NN/Source/NNSupportFunctions/arm_q7_to_q15_reordered_no_shift.c
	./CMSIS/NN/Source/PoolingFunctions/arm_pool_q7_HWC.c
	#./CMSIS/NN/Source/SoftmaxFunctions/arm_softmax_q15.c
	./CMSIS/NN/Source/SoftmaxFunctions/arm_softmax_q7.c
	)
SET_TARGET_PROPERTIES(cmsisnn PROPERTIES VERSION 5.5.1)

include_directories(
					${CMAKE_SOURCE_DIR}/include/ 
					${CMAKE_SOURCE_DIR}/CMSIS/Include/ 
					${CMAKE_SOURCE_DIR}/CMSIS/NN/Include/)
# Create executable
ADD_EXECUTABLE(${PROJECT_NAME} main.c)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} cmsisnn)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

# Add MakeImage post-build command
INCLUDE("${AZURE_SPHERE_MAKE_IMAGE_FILE}")
