/*
 * Copyright 2021-2022, Oliver Ruiz Dorantes. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <efi/types.h>

#include <kernel/arch/arm64/arm_registers.h>
#include <kernel/arch/arm64/arch_pte.h>

#include <arch_kernel.h>


extern "C" void _arch_exception_loop(void);
extern "C" void _arch_exception_panic(const char* someString, uint64 someValue);

extern "C" uint64 _arch_transition_EL2_EL1(void);

extern "C" void arch_cache_disable(void);
extern "C" void arch_cache_enable(void);
extern "C" void _arch_cache_flush_invalidate_all(void);
extern "C" void _arch_mmu_invalidate_tlb_all(uint8 el);

extern "C" void _arch_cache_clean_poc(void);


static const uint8 kInvalidExceptionLevel = 0xFFu;


#define	AARCH64_CHECK_ACCESS(operand, address)						\
	__asm __volatile("at	" #operand ", %0" : : "r"((uint64_t)address))

#define	AARCH64_BREAK(id)						\
	__asm __volatile("brk	" #id)


static inline uint64 arch_exception_level()
{
	return (READ_SPECIALREG(CurrentEL) >> 2);
}


// Check arch_cpu.h macro ADDRESS_TRANSLATE_FUNC(stage) for alternative implementation
static inline bool arch_mmu_read_access(addr_t address) {

	switch (arch_exception_level())
	{
		case 0:
			AARCH64_CHECK_ACCESS(S1E0R, address);
			break;
		case 1:
			AARCH64_CHECK_ACCESS(S1E1R, address);
			break;
		case 2:
			AARCH64_CHECK_ACCESS(S1E2R, address);
			break;
		case 3:
			AARCH64_CHECK_ACCESS(S1E3R, address);
			break;
		default:
			return false;
	}
	return !(READ_SPECIALREG(PAR_EL1) & PAR_F);
}


static inline bool arch_mmu_write_access(addr_t address) {

	switch (arch_exception_level())
	{
		case 0:
			AARCH64_CHECK_ACCESS(S1E0W, address);
			break;
		case 1:
			AARCH64_CHECK_ACCESS(S1E1W, address);
			break;
		case 2:
			AARCH64_CHECK_ACCESS(S1E2W, address);
			break;
		case 3:
			AARCH64_CHECK_ACCESS(S1E3W, address);
			break;
		default:
			return false;
	}
	return !(READ_SPECIALREG(PAR_EL1) & PAR_F);
}


static inline uint64 arch_mmu_base_register(bool kernel = false)
{
	switch (arch_exception_level())
	{
		case 1:
			if (kernel) {
				return READ_SPECIALREG(TTBR1_EL1);
			} else {
				return READ_SPECIALREG(TTBR0_EL1);
			}
		case 2:
			if (kernel) {
				/* This register is present only when
				 * FEAT_VHE is implemented. Otherwise,
				 * direct accesses to TTBR1_EL2 are UNDEFINED.
				 */
				return READ_SPECIALREG(TTBR0_EL2); // TTBR1_EL2
			} else {
				return READ_SPECIALREG(TTBR0_EL2);
			}
		case 3:
			return READ_SPECIALREG(TTBR0_EL3);
		default:
			return false;
	}
}


static inline uint64 _arch_mmu_get_sctlr()
{
	switch (arch_exception_level())
	{
		case 1:
			return READ_SPECIALREG(SCTLR_EL1);
		case 2:
			return READ_SPECIALREG(SCTLR_EL2);
		case 3:
			return READ_SPECIALREG(SCTLR_EL3);
		default:
			return false;
	}
}


static inline void _arch_mmu_set_sctlr(uint64 sctlr)
{
	switch (arch_exception_level())
	{
		case 1:
			WRITE_SPECIALREG(SCTLR_EL1, sctlr);
			break;
		case 2:
			WRITE_SPECIALREG(SCTLR_EL2, sctlr);
			break;
		case 3:
			WRITE_SPECIALREG(SCTLR_EL3, sctlr);
			break;
	}
}


static inline bool arch_mmu_enabled()
{
	return _arch_mmu_get_sctlr() & SCTLR_M;
}


static inline bool arch_mmu_cache_enabled()
{
	return _arch_mmu_get_sctlr() & SCTLR_C;
}


static inline uint64 _arch_mmu_get_tcr(int el = kInvalidExceptionLevel) {

	if (el == kInvalidExceptionLevel)
		el = arch_exception_level();

	switch (el)
	{
		case 1:
			return READ_SPECIALREG(TCR_EL1);
		case 2:
			return READ_SPECIALREG(TCR_EL2);
		case 3:
			return READ_SPECIALREG(TCR_EL3);
		default:
			return 0;
	}
}

// TODO: move to arm_registers.h
static constexpr uint64 TG_MASK = 0x3u;
static constexpr uint64 TG_4KB = 0x0u;
static constexpr uint64 TG_16KB = 0x2u;
static constexpr uint64 TG_64KB = 0x1u;

static constexpr uint64 TxSZ_MASK = (1 << 6) - 1;

static constexpr uint64 T0SZ_MASK = TxSZ_MASK;
static constexpr uint64 T1SZ_MASK = TxSZ_MASK << TCR_T1SZ_SHIFT;

static constexpr uint64 IPS_MASK = 0x7UL << TCR_IPS_SHIFT;

static constexpr uint64 TCR_EPD1_DISABLE = (1 << 23);

static inline uint32 arch_mmu_user_address_bits()
{

	uint64 reg = _arch_mmu_get_tcr();

	return 64 - (reg & T0SZ_MASK);
}


static inline uint32 arch_mmu_user_granule()
{
	static constexpr uint64 TCR_TG0_SHIFT = 14u;

	uint64 reg = _arch_mmu_get_tcr();
	return ((reg >> TCR_TG0_SHIFT) & TG_MASK);
}


/*
 * Given that "EL2 and EL3 have a TTBR0, but no TTBR1. This means
 * that is either EL2 or EL3 is using AArch64, they can only use
 * virtual addresses in the range 0x0 to 0x0000FFFF_FFFFFFFF."
 *
 * Following calls might only have sense under EL1
 */
static inline uint32 arch_mmu_kernel_address_bits()
{
	uint64 reg = _arch_mmu_get_tcr();
	return 64 - ((reg & T1SZ_MASK) >> TCR_T1SZ_SHIFT);
}


static inline uint32 arch_mmu_kernel_granule()
{
	uint64 reg = _arch_mmu_get_tcr();
	return ((reg >> TCR_TG1_SHIFT) & TG_MASK);
}


/*
 * Distinguish between kernel(TTBR1) and user(TTBR0) addressing
 */
static inline bool arch_mmu_is_kernel_address(uint64 address)
{
	return address > KERNEL_BASE;
}


static inline constexpr uint32 arch_mmu_entries_per_granularity(uint32 granularity)
{
	return (granularity / 8);
}
