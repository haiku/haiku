/*
 * Copyright 2021-2022, Oliver Ruiz Dorantes. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <boot/platform.h>
#include <boot/stage2.h>

#include "aarch64.h"


void
arch_cache_disable()
{
	if (arch_mmu_cache_enabled()) {
		uint64 sctlr = _arch_mmu_get_sctlr();
		sctlr &= ~(SCTLR_M | SCTLR_C | SCTLR_I);
		_arch_mmu_set_sctlr(sctlr);

		_arch_cache_clean_poc();
		_arch_mmu_invalidate_tlb_all(arch_exception_level());
	}
}


void
arch_cache_enable()
{
	if (!arch_mmu_cache_enabled()) {
		uint64 sctlr = _arch_mmu_get_sctlr();
		sctlr |= (SCTLR_M | SCTLR_C | SCTLR_I);
		_arch_mmu_set_sctlr(sctlr);
	}
}
