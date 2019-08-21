/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// Reference implementation of the DebugLog() function that's required for a
// platform to support the TensorFlow Lite for Microcontrollers library. This is
// the only function that's absolutely required to be available on a target
// device, since it's used for communicating test results back to the host so
// that we can verify the implementation is working correctly.
// It's designed to be as easy as possible to supply an implementation though.
// On platforms that have a POSIX stack or C library, it can be written as a
// single call to `fprintf(stderr, "%s", s)` to output a string to the error
// stream of the console, but if there's no OS or C library available, there's
// almost always an equivalent way to write out a string to some serial
// interface that can be used instead. For example on Arm M-series MCUs, calling
// the `bkpt #0xAB` assembler instruction will output the string in r1 to
// whatever debug serial connection is available. If you're running mbed, you
// can do the same by creating `Serial pc(USBTX, USBRX)` and then calling
// `pc.printf("%s", s)`.
// To add an equivalent function for your own platform, create your own
// implementation file, and place it in a subfolder with named after the OS
// you're targeting. For example, see the Cortex M bare metal version in
// tensorflow/lite/experimental/micro/bluepill/debug_log.cc or the mbed one on
// tensorflow/lite/experimental/micro/mbed/debug_log.cc.

#include "tensorflow/lite/experimental/micro/debug_log.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
// Microsoft Azure Sphere RTCore debug_log implementation
// Target MT3620 Cortex-M4F core

static const uintptr_t UART_BASE = 0x21040000;
static int UART_INITIALISED = 0;

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

static void Uart_WritePoll(const char *msg)
{
	if (UART_INITIALISED == 0) {
		Uart_Init();
		UART_INITIALISED = 1;
	}

	while (*msg) {
		// When LSR[5] is set, can write another character.
		while (!(ReadReg32(UART_BASE, 0x14) & (UINT32_C(1) << 5))) {
			// empty.
		}

		WriteReg32(UART_BASE, 0x0, *msg++);
	}
}

// For Arm Cortex-M devices, calling SYS_WRITE0 will output the zero-terminated
// string pointed to by R1 to any debug console that's attached to the system.
extern "C" void DebugLog(const char* s) {
	Uart_WritePoll(s);
}
