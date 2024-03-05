/*
 * Copyright 2022, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */

#ifndef ARCH_ARM_SOC_SUN4I_H
#define ARCH_ARM_SOC_SUN4I_H


#include "soc.h"


class Sun4iInterruptController : public InterruptController {
public:
	Sun4iInterruptController(uint32_t reg_base);
	void EnableInterrupt(int32 irq);
	void DisableInterrupt(int32 irq);
	void HandleInterrupt();

protected:
	area_id fRegArea;
	uint32 *fRegBase;
};


#endif /* !SOC_SUN4I_H */
