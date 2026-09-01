/*
 * Copyright 2026 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCH_ARM_GICV3_H
#define ARCH_ARM_GICV3_H

#include <SupportDefs.h>

#include "soc.h"


class GICv3InterruptController : public InterruptController {
public:
	GICv3InterruptController(
		phys_addr_t gicd_phys_addr,
		phys_addr_t gicr_phys_addr,
		uint32_t num_cpus);

	void EnableInterrupt(int32 irq) override;
	void DisableInterrupt(int32 irq) override;
	void HandleInterrupt() override;
	void SendMulticastIci(CPUSet& cpuSet) override;
	void SendUnicastIci(int32 target_cpu) override;

	void SendBroadcastIci() override;
	status_t PerCpuInit() override;


private:
	volatile uint8_t *fGicrBase;
	volatile uint8_t *fGicdBase;

	int32_t fMaxInt;
	uint64_t fGicrStride;
	uint32_t fNumCpus;

	// Register accessors.
	// All register offsets are given in the spec (and in gicv3_regs.h) at byte offsets.
	// Most registers are 32 bit, a smaller number is 64 bit.
	volatile uint32_t& _GicdReg32(size_t offset) const
		{ return *reinterpret_cast<volatile uint32_t*>(fGicdBase + offset); }
	volatile uint64_t& _GicdReg64(size_t offset) const
		{ return *reinterpret_cast<volatile uint64_t*>(fGicdBase + offset); }
	volatile uint32_t& _GicrReg32(size_t offset) const
		{ return *reinterpret_cast<volatile uint32_t*>(fGicrBase + offset); }
	volatile uint64_t& _GicrReg64(size_t offset) const
		{ return *reinterpret_cast<volatile uint64_t*>(fGicrBase + offset); }

	static bool _WaitForMask(volatile uint32_t *reg, uint32_t mask, uint32_t expect);

	void _SetEnable(uint vector, bool enable);
	void _RedistributorSleep(bool sleep);
};

#endif /* ARCH_ARM_GICV3_H */
