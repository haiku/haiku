/*
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_UART_H
#define KERNEL_BOOT_UART_H


#include <boot/addr_range.h>
#include <SupportDefs.h>


typedef struct {
	char kind[32];
	addr_range regs;
	uint32 irq;
	int64 clock;
} __attribute__((packed)) uart_info;


#endif /* KERNEL_BOOT_UART_H */
