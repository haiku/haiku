/*
** Copyright 2021 Haiku, Inc. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef KERNEL_ARCH_ARM64_KERNEL_ARGS_H
#define KERNEL_ARCH_ARM64_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif


#include <util/FixedWidthPointer.h>
#include <boot/uart.h>


#define _PACKED __attribute__((packed))


typedef struct {
	// needed for UEFI, otherwise kernel acpi support can't find ACPI root
	FixedWidthPointer<void> acpi_root;
//	TODO:  Deal with this later in the port
	FixedWidthPointer<void> fdt;
//	uart_info		uart;
} _PACKED arch_kernel_args;

#endif	/* KERNEL_ARCH_ARM64_KERNEL_ARGS_H */
