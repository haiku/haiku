/*
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_INTERRUPT_CONTROLLER_H
#define KERNEL_BOOT_INTERRUPT_CONTROLLER_H


#include <boot/addr_range.h>
#include <SupportDefs.h>


#define		INTC_KIND_GICV1		"gicv1"
#define		INTC_KIND_GICV2		"gicv2"
#define		INTC_KIND_OMAP3		"omap3"
#define		INTC_KIND_PXA		"pxa"
#define		INTC_KIND_SUN4I		"sun4i"


typedef struct {
	char kind[32];
	addr_range regs1;
	addr_range regs2;
} __attribute__((packed)) intc_info;


#endif /* KERNEL_BOOT_INTERRUPT_CONTROLLER_H */
