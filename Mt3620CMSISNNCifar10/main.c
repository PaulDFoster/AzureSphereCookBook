
/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

   /* ----------------------------------------------------------------------
   * Copyright (C) 2010-2018 Arm Limited. All rights reserved.
   *
   *
   * Project:       CMSIS NN Library
   * Title:         arm_nnexamples_cifar10.cpp
   *
   * Description:   Convolutional Neural Network Example
   *
   * Target Processor: Cortex-M4/Cortex-M7
   *
   * Redistribution and use in source and binary forms, with or without
   * modification, are permitted provided that the following conditions
   * are met:
   *   - Redistributions of source code must retain the above copyright
   *     notice, this list of conditions and the following disclaimer.
   *   - Redistributions in binary form must reproduce the above copyright
   *     notice, this list of conditions and the following disclaimer in
   *     the documentation and/or other materials provided with the
   *     distribution.
   *   - Neither the name of Arm LIMITED nor the names of its contributors
   *     may be used to endorse or promote products derived from this
   *     software without specific prior written permission.
   *
   * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   * POSSIBILITY OF SUCH DAMAGE.
   * -------------------------------------------------------------------- */

   /**
	* @ingroup groupExamples
	*/

	/**
	 * @defgroup CNNExample Convolutional Neural Network Example
	 *
	 * \par Description:
	 * \par
	 * Demonstrates a convolutional neural network (CNN) example with the use of convolution,
	 * ReLU activation, pooling and fully-connected functions.
	 *
	 * \par Model definition:
	 * \par
	 * The CNN used in this example is based on CIFAR-10 example from Caffe [1].
	 * The neural network consists
	 * of 3 convolution layers interspersed by ReLU activation and max pooling layers, followed by a
	 * fully-connected layer at the end. The input to the network is a 32x32 pixel color image, which will
	 * be classified into one of the 10 output classes.
	 * This example model implementation needs 32.3 KB to store weights, 40 KB for activations and
	 * 3.1 KB for storing the \c im2col data.
	 *
	 * \image html CIFAR10_CNN.gif "Neural Network model definition"
	 *
	 * \par Variables Description:
	 * \par
	 * \li \c conv1_wt, \c conv2_wt, \c conv3_wt are convolution layer weight matrices
	 * \li \c conv1_bias, \c conv2_bias, \c conv3_bias are convolution layer bias arrays
	 * \li \c ip1_wt, ip1_bias point to fully-connected layer weights and biases
	 * \li \c image_data points to the input image data
	 * \li \c output_data points to the classification output
	 * \li \c col_buffer is a buffer to store the \c im2col output
	 * \li \c scratch_buffer is used to store the activation data (intermediate layer outputs)
	 *
	 * \par CMSIS DSP Software Library Functions Used:
	 * \par
	 * - arm_convolve_HWC_q7_RGB()
	 * - arm_convolve_HWC_q7_fast()
	 * - arm_relu_q7()
	 * - arm_maxpool_q7_HWC()
	 * - arm_avepool_q7_HWC()
	 * - arm_fully_connected_q7_opt()
	 * - arm_fully_connected_q7()
	 *
	 * <b> Refer  </b>
	 * \link arm_nnexamples_cifar10.cpp \endlink
	 *
	 * \par [1] https://github.com/BVLC/caffe
	 */

#define __ARM_ARCH_7EM__ 1

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "arm_math.h"
#include "arm_nnexamples_cifar10_parameter.h"
#include "arm_nnexamples_cifar10_weights.h"

#include "arm_nnfunctions.h"
#include "arm_nnexamples_cifar10_inputs.h"

// include the input and weights

static q7_t conv1_wt[CONV1_IM_CH * CONV1_KER_DIM * CONV1_KER_DIM * CONV1_OUT_CH] = CONV1_WT;
static q7_t conv1_bias[CONV1_OUT_CH] = CONV1_BIAS;

static q7_t conv2_wt[CONV2_IM_CH * CONV2_KER_DIM * CONV2_KER_DIM * CONV2_OUT_CH] = CONV2_WT;
static q7_t conv2_bias[CONV2_OUT_CH] = CONV2_BIAS;

static q7_t conv3_wt[CONV3_IM_CH * CONV3_KER_DIM * CONV3_KER_DIM * CONV3_OUT_CH] = CONV3_WT;
static q7_t conv3_bias[CONV3_OUT_CH] = CONV3_BIAS;

static q7_t ip1_wt[IP1_DIM * IP1_OUT] = IP1_WT;
static q7_t ip1_bias[IP1_OUT] = IP1_BIAS;

/* Here the image_data should be the raw uint8 type RGB image in [RGB, RGB, RGB ... RGB] format */
uint8_t   image_data[CONV1_IM_CH * CONV1_IM_DIM * CONV1_IM_DIM] = IMG_DATA;
q7_t      output_data[IP1_OUT];

//vector buffer: max(im2col buffer,average pool buffer, fully connected buffer)
q7_t      col_buffer[2 * 5 * 5 * 32 * 2];

q7_t      scratch_buffer[32 * 32 * 10 * 4];


extern uint32_t StackTop; // &StackTop == end of TCM0

static const uintptr_t GPT_BASE = 0x21030000;
static const uintptr_t UART_BASE = 0x21040000;
static const uintptr_t SCB_BASE = 0xE000ED00;

static _Noreturn void DefaultExceptionHandler(void);

static void WriteReg32(uintptr_t baseAddr, size_t offset, uint32_t value);
static uint32_t ReadReg32(uintptr_t baseAddr, size_t offset);

static void Uart_Init(void);
static void Uart_WritePoll(const char *msg);

static void Gpt3_WaitUs(int microseconds);

static _Noreturn void RTCoreMain(void);

// ARM DDI0403E.d SB1.5.2-3
// From SB1.5.3, "The Vector table must be naturally aligned to a power of two whose alignment
// value is greater than or equal to (Number of Exceptions supported x 4), with a minimum alignment
// of 128 bytes.". The array is aligned in linker.ld, using the dedicated section ".vector_table".

// The exception vector table contains a stack pointer, 15 exception handlers, and an entry for
// each interrupt.
#define INTERRUPT_COUNT 100 // from datasheet
#define EXCEPTION_COUNT (16 + INTERRUPT_COUNT)
#define INT_TO_EXC(i_) (16 + (i_))
static const uintptr_t ExceptionVectorTable[EXCEPTION_COUNT]
    __attribute__((section(".vector_table"))) __attribute__((used)) = {
        [0] = (uintptr_t)&StackTop, // Main Stack Pointer (MSP)

        [1] = (uintptr_t)RTCoreMain,               // Reset
        [2] = (uintptr_t)DefaultExceptionHandler,  // NMI
        [3] = (uintptr_t)DefaultExceptionHandler,  // HardFault
        [4] = (uintptr_t)DefaultExceptionHandler,  // MPU Fault
        [5] = (uintptr_t)DefaultExceptionHandler,  // Bus Fault
        [6] = (uintptr_t)DefaultExceptionHandler,  // Usage Fault
        [11] = (uintptr_t)DefaultExceptionHandler, // SVCall
        [12] = (uintptr_t)DefaultExceptionHandler, // Debug monitor
        [14] = (uintptr_t)DefaultExceptionHandler, // PendSV
        [15] = (uintptr_t)DefaultExceptionHandler, // SysTick

        [INT_TO_EXC(0)... INT_TO_EXC(INTERRUPT_COUNT - 1)] = (uintptr_t)DefaultExceptionHandler};

static _Noreturn void DefaultExceptionHandler(void)
{
    for (;;) {
        // empty.
    }
}

static void WriteReg32(uintptr_t baseAddr, size_t offset, uint32_t value)
{
    *(volatile uint32_t *)(baseAddr + offset) = value;
}

static uint32_t ReadReg32(uintptr_t baseAddr, size_t offset)
{
    return *(volatile uint32_t *)(baseAddr + offset);
}

static void Uart_Init(void)
{
    // Configure UART to use 115200-8-N-1.
    WriteReg32(UART_BASE, 0x0C, 0x80); // LCR (enable DLL, DLM)
    WriteReg32(UART_BASE, 0x24, 0x3);  // HIGHSPEED
    WriteReg32(UART_BASE, 0x04, 0);    // Divisor Latch (MS)
    WriteReg32(UART_BASE, 0x00, 1);    // Divisor Latch (LS)
    WriteReg32(UART_BASE, 0x28, 224);  // SAMPLE_COUNT
    WriteReg32(UART_BASE, 0x2C, 110);  // SAMPLE_POINT
    WriteReg32(UART_BASE, 0x58, 0);    // FRACDIV_M
    WriteReg32(UART_BASE, 0x54, 223);  // FRACDIV_L
    WriteReg32(UART_BASE, 0x0C, 0x03); // LCR (8-bit word length)
}

static void WriteIntegerAsStringWithBaseWidth(int value, int base, int width)
{
	// Maximum decimal length is minus sign, ten digits, and null terminator.
	char txt[1 + 10 + 1];
	char *p = txt;

	bool isNegative = value < 0;
	if (isNegative) {
		*p++ = '-';
	}

	static const char digits[] = "0123456789abcdef";
	do {
		*p++ = digits[__builtin_abs(value % base)];
		value /= base;
	} while (value && ((width == -1) || (p - txt < width)));

	// Append '0' if required to reach width.
	if (width != -1 && p - txt < width) {
		int requiredZeroes = width - (p - txt);
		__builtin_memset(p, '0', requiredZeroes);
		p += requiredZeroes;
	}

	*p = '\0';

	// Reverse the digits, not including any negative sign.
	char *low = isNegative ? &txt[1] : &txt[0];
	char *high = p - 1;
	while (low < high) {
		char tmp = *low;
		*low = *high;
		*high = tmp;
		++low;
		--high;
	}

	return Uart_WritePoll(txt);
}

void Uart_WriteIntegerPoll(int value)
{
	WriteIntegerAsStringWithBaseWidth(value, 10, -1);
}

static void Uart_WritePoll(const char *msg)
{
    while (*msg) {
        // When LSR[5] is set, can write another character.
        while (!(ReadReg32(UART_BASE, 0x14) & (UINT32_C(1) << 5))) {
            // empty.
        }

        WriteReg32(UART_BASE, 0x0, *msg++);
    }
}

static void Gpt3_WaitUs(int microseconds)
{
    // GPT3_INIT = initial counter value
    WriteReg32(GPT_BASE, 0x54, 0x0);

    // GPT3_CTRL
    uint32_t ctrlOn = 0x0;
    ctrlOn |= (0x19) << 16; // OSC_CNT_1US (default value)
    ctrlOn |= 0x1;          // GPT3_EN = 1 -> GPT3 enabled
    WriteReg32(GPT_BASE, 0x50, ctrlOn);

    // GPT3_CNT
    while (ReadReg32(GPT_BASE, 0x58) < microseconds) {
        // empty.
    }

    // GPT_CTRL -> disable timer
    WriteReg32(GPT_BASE, 0x50, 0x0);
}

static int ExecuteModel()
{

	Uart_WritePoll("start execution\r\n");
	/* start the execution */

	q7_t     *img_buffer1 = scratch_buffer;
	q7_t     *img_buffer2 = img_buffer1 + 32 * 32 * 32;

	/* input pre-processing */
	Uart_WritePoll("input pre-processing\r\n");
	int mean_data[3] = INPUT_MEAN_SHIFT;
	unsigned int scale_data[3] = INPUT_RIGHT_SHIFT;
	for (int i = 0; i < 32 * 32 * 3; i += 3) {
		img_buffer2[i] = (q7_t)__SSAT(((((int)image_data[i] - mean_data[0]) << 7) + (0x1 << (scale_data[0] - 1)))
			>> scale_data[0], 8);
		img_buffer2[i + 1] = (q7_t)__SSAT(((((int)image_data[i + 1] - mean_data[1]) << 7) + (0x1 << (scale_data[1] - 1)))
			>> scale_data[1], 8);
		img_buffer2[i + 2] = (q7_t)__SSAT(((((int)image_data[i + 2] - mean_data[2]) << 7) + (0x1 << (scale_data[2] - 1)))
			>> scale_data[2], 8);
	}

	// conv1 img_buffer2 -> img_buffer1
	Uart_WritePoll("conv1 img_buffer2 -> img_buffer1\r\n");
	arm_convolve_HWC_q7_RGB(img_buffer2, CONV1_IM_DIM, CONV1_IM_CH, conv1_wt, CONV1_OUT_CH, CONV1_KER_DIM, CONV1_PADDING,
		CONV1_STRIDE, conv1_bias, CONV1_BIAS_LSHIFT, CONV1_OUT_RSHIFT, img_buffer1, CONV1_OUT_DIM,
		(q15_t *)col_buffer, NULL);

	arm_relu_q7(img_buffer1, CONV1_OUT_DIM * CONV1_OUT_DIM * CONV1_OUT_CH);

	// pool1 img_buffer1 -> img_buffer2
	Uart_WritePoll("pool1 img_buffer1 -> img_buffer2\r\n");
	arm_maxpool_q7_HWC(img_buffer1, CONV1_OUT_DIM, CONV1_OUT_CH, POOL1_KER_DIM,
		POOL1_PADDING, POOL1_STRIDE, POOL1_OUT_DIM, NULL, img_buffer2);

	// conv2 img_buffer2 -> img_buffer1
	Uart_WritePoll("conv2 img_buffer2 -> img_buffer1\r\n");
	arm_convolve_HWC_q7_fast(img_buffer2, CONV2_IM_DIM, CONV2_IM_CH, conv2_wt, CONV2_OUT_CH, CONV2_KER_DIM,
		CONV2_PADDING, CONV2_STRIDE, conv2_bias, CONV2_BIAS_LSHIFT, CONV2_OUT_RSHIFT, img_buffer1,
		CONV2_OUT_DIM, (q15_t *)col_buffer, NULL);

	arm_relu_q7(img_buffer1, CONV2_OUT_DIM * CONV2_OUT_DIM * CONV2_OUT_CH);

	// pool2 img_buffer1 -> img_buffer2
	Uart_WritePoll("pool2 img_buffer1 -> img_buffer2\r\n");
	arm_maxpool_q7_HWC(img_buffer1, CONV2_OUT_DIM, CONV2_OUT_CH, POOL2_KER_DIM,
		POOL2_PADDING, POOL2_STRIDE, POOL2_OUT_DIM, col_buffer, img_buffer2);

	// conv3 img_buffer2 -> img_buffer1
	Uart_WritePoll("conv3 img_buffer2 -> img_buffer1\r\n");
	arm_convolve_HWC_q7_fast(img_buffer2, CONV3_IM_DIM, CONV3_IM_CH, conv3_wt, CONV3_OUT_CH, CONV3_KER_DIM,
		CONV3_PADDING, CONV3_STRIDE, conv3_bias, CONV3_BIAS_LSHIFT, CONV3_OUT_RSHIFT, img_buffer1,
		CONV3_OUT_DIM, (q15_t *)col_buffer, NULL);

	arm_relu_q7(img_buffer1, CONV3_OUT_DIM * CONV3_OUT_DIM * CONV3_OUT_CH);

	// pool3 img_buffer-> img_buffer2
	Uart_WritePoll("pool3 img_buffer-> img_buffer2\r\n");
	arm_maxpool_q7_HWC(img_buffer1, CONV3_OUT_DIM, CONV3_OUT_CH, POOL3_KER_DIM,
		POOL3_PADDING, POOL3_STRIDE, POOL3_OUT_DIM, col_buffer, img_buffer2);

	arm_fully_connected_q7_opt(img_buffer2, ip1_wt, IP1_DIM, IP1_OUT, IP1_BIAS_LSHIFT, IP1_OUT_RSHIFT, ip1_bias,
		output_data, (q15_t *)img_buffer1);

	arm_softmax_q7(output_data, 10, output_data);

	for (int i = 0; i < 10; i++)
	{
		Uart_WriteIntegerPoll(i);
		Uart_WritePoll(": ");
		Uart_WriteIntegerPoll(output_data[i]);
		Uart_WritePoll("\r\n");
		 // "%d: %d\n", i, output_data[i]);
	}

	return 0;
}

static _Noreturn void RTCoreMain(void)
{
    // SCB->VTOR = ExceptionVectorTable
    WriteReg32(SCB_BASE, 0x08, (uint32_t)ExceptionVectorTable);

    Uart_Init();

    // This minimal Azure Sphere app repeatedly prints "Tick" then "Tock" to the
    // debug UART, at one second intervals. Use this app to test the device and SDK
    // installation succeeded, and that you can deploy and debug applications on the
    // real-time core.

    static const int tickPeriodUs = 1 * 1000 * 1000;
	int res = ExecuteModel();
	Uart_WritePoll("Complete.\r\n");
    while (true) {
        Gpt3_WaitUs(tickPeriodUs);

    }
}
