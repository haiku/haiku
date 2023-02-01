/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_TIMER_H
#define KERNEL_BOOT_TIMER_H


#include <boot/addr_range.h>
#include <SupportDefs.h>


#define		TIMER_KIND_ARMV7	"armv7"
#define		TIMER_KIND_OMAP3	"omap3"
#define		TIMER_KIND_PXA		"pxa"


typedef struct {
	char		kind[32];
	addr_range	regs;
	uint32_t	interrupt;
} __attribute__((packed)) boot_timer_info;


#endif /* KERNEL_BOOT_TIMER_H */
