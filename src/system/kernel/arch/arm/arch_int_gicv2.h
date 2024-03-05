/*
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCH_ARM_GICV2_H
#define ARCH_ARM_GICV2_H

#include <SupportDefs.h>

#include "soc.h"

class GICv2InterruptController : public InterruptController {
public:
	GICv2InterruptController(uint32_t gicd_regs = 0, uint32_t gicc_regs = 0);
	void EnableInterrupt(int32 irq);
	void DisableInterrupt(int32 irq);
	void HandleInterrupt();
private:
	volatile uint32_t *fGicdRegs;
	volatile uint32_t *fGiccRegs;
};

#endif /* ARCH_ARM_GICV2_H */
