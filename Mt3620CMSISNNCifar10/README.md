# AzureSphereCookBook
## ARM CMSIS NN Cifar10 example on MT3620 Cortex-M4 core
12th June 2019

This project ports the ARM CMSIS NN Cifar10 example to run on an Azure Sphere Cortex-M4 core. See the excellent ARM tutorial for details: https://www.dlology.com/blog/how-to-run-deep-learning-model-on-microcontroller-with-cmsis-nn/

See original Github repo: https://github.com/Tony607/arm_nn_examples/tree/master/cifar10 

This project was created by dupicating the Hello World sample and modifying it as required. See https://docs.microsoft.com/en-us/azure-sphere/app-development/develop-debug-rt-app

Project built with Azure Sphere OS 19.05 for the MT3620 RDB. 

Key parts to this project:

1. linker.ld: Increased TCM memory allocation to 192kb

```XML
MEMORY
{
    TCM0 (rwx) : ORIGIN = 0x00100000, LENGTH = 192K
    FLASH (rx) : ORIGIN = 0x10000000, LENGTH = 1M
}
```
2. CMakeLists.txt: Defined the CMSIS NN library functions required. Only required functionality is enabled. Uncomment files if functionality is required in future developments.
```c
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
```
3. Change evaluation image data in include\arm_nnexamples_cifar10_inputs.h to test
```c
/* A CIFAR10 test set image - a dog, label 5 */
//#define IMG_DATA {91,64,30,....

/* Custom image - a cat, label 3 */
#define IMG_DATA {231,227,224,....

```
4. Observe application output using real-time core UART output. See https://docs.microsoft.com/en-us/azure-sphere/quickstarts/qs-real-time-application#set-up-hardware-to-display-output

```
start execution
input pre-processing
conv1 img_buffer2 -> img_buffer1
pool1 img_buffer1 -> img_buffer2
conv2 img_buffer2 -> img_buffer1
pool2 img_buffer1 -> img_buffer2
conv3 img_buffer2 -> img_buffer1
pool3 img_buffer-> img_buffer2
0: 0
1: 0
2: 0
3: 102
4: 0
5: 0
6: 25
7: 0
8: 0
9: 0
Complete.

```
