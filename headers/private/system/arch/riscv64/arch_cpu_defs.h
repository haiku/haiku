/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_ARCH_RISCV64_DEFS_H
#define _SYSTEM_ARCH_RISCV64_DEFS_H


#include <SupportDefs.h>

#define B_ALWAYS_INLINE __attribute__((always_inline)) inline


#ifdef __cplusplus

enum {
	modeU = 0,
	modeS = 1,
	modeM = 3,
};

// fs, xs
enum {
	extStatusOff     = 0,
	extStatusInitial = 1,
	extStatusClean   = 2,
	extStatusDirty   = 3,
};

union MstatusReg {
	struct {
		uint64 ie:      4; // interrupt enable
		uint64 pie:     4; // previous interrupt enable
		uint64 spp:     1; // previous mode (supervisor)
		uint64 unused1: 2;
		uint64 mpp:     2; // previous mode (machine)
		uint64 fs:      2; // FPU status
		uint64 xs:      2; // extensions status
		uint64 mprv:    1; // modify privilege
		uint64 sum:     1; // permit supervisor user memory access
		uint64 mxr:     1; // make executable readable
		uint64 tvm:     1; // trap virtual memory
		uint64 tw:      1; // timeout wait (trap WFI)
		uint64 tsr:     1; // trap SRET
		uint64 unused2: 9;
		uint64 uxl:     2; // U-mode XLEN
		uint64 sxl:     2; // S-mode XLEN
		uint64 unused3: 27;
		uint64 sd:      1; // status dirty
	};
	uint64 val;
};

union SstatusReg {
	struct {
		uint64 ie:      2; // interrupt enable
		uint64 unused1: 2;
		uint64 pie:     2; // previous interrupt enable
		uint64 unused2: 2;
		uint64 spp:     1; // previous mode (supervisor)
		uint64 unused3: 4;
		uint64 fs:      2; // FPU status
		uint64 xs:      2; // extensions status
		uint64 unused4: 1;
		uint64 sum:     1; // permit supervisor user memory access
		uint64 mxr:     1; // make executable readable
		uint64 unused5: 12;
		uint64 uxl:     2; // U-mode XLEN
		uint64 unused6: 29;
		uint64 sd:      1; // status dirty
	};
	uint64 val;
};

enum {
	softInt    = 0,
	uSoftInt   = softInt + modeU,
	sSoftInt   = softInt + modeS,
	mSoftInt   = softInt + modeM,
	timerInt   = 4,
	uTimerInt  = timerInt + modeU,
	sTimerInt  = timerInt + modeS,
	mTimerInt  = timerInt + modeM,
	externInt  = 8,
	uExternInt = externInt + modeU,
	sExternInt = externInt + modeS,
	mExternInt = externInt + modeM,
};

enum {
	causeInterrupt        = 1ULL << 63, // rest bits are interrupt number
	causeExecMisalign     = 0,
	causeExecAccessFault  = 1,
	causeIllegalInst      = 2,
	causeBreakpoint       = 3,
	causeLoadMisalign     = 4,
	causeLoadAccessFault  = 5,
	causeStoreMisalign    = 6,
	causeStoreAccessFault = 7,
	causeECall            = 8,
	causeUEcall           = causeECall + modeU,
	causeSEcall           = causeECall + modeS,
	causeMEcall           = causeECall + modeM,
	causeExecPageFault    = 12,
	causeLoadPageFault    = 13,
	causeStorePageFault   = 15,
};

// physical memory protection
enum {
	pmpR = 0,
	pmpW = 1,
	pmpX = 2,
};

enum {
	// naturally aligned power of two
	pmpMatchNapot = 3 << 3,
};

enum {
	pageBits = 12,
	pteCount = 512,
	pteIdxBits = 9,
};

enum {
	pteValid    = 0,
	pteRead     = 1,
	pteWrite    = 2,
	pteExec     = 3,
	pteUser     = 4,
	pteGlobal   = 5,
	pteAccessed = 6,
	pteDirty    = 7,
};

union Pte {
	struct {
		uint64 flags:     8;
		uint64 rsw:       2;
		uint64 ppn:      44;
		uint64 reserved: 10;
	};
	uint64 val;
};

enum {
	satpModeBare =  0,
	satpModeSv39 =  8,
	satpModeSv48 =  9,
	satpModeSv57 = 10,
	satpModeSv64 = 11,
};

union SatpReg {
	struct {
		uint64 ppn:  44;
		uint64 asid: 16;
		uint64 mode:  4;
	};
	uint64 val;
};

static B_ALWAYS_INLINE uint64 VirtAdrPte(uint64 physAdr, uint32 level)
{
	return (physAdr >> (pageBits + pteIdxBits*level)) % (1 << pteIdxBits);
}

static B_ALWAYS_INLINE uint64 VirtAdrOfs(uint64 physAdr)
{
	return physAdr % PAGESIZE;
}

#define CSR_REG_MACRO(Name, value) \
	static B_ALWAYS_INLINE uint64 Name() { \
		uint64 x; asm volatile("csrr %0, " #value : "=r" (x)); return x;} \
	static B_ALWAYS_INLINE void Set##Name(uint64 x) { \
		asm volatile("csrw " #value ", %0" : : "r" (x));} \
	static B_ALWAYS_INLINE void SetBits##Name(uint64 x) { \
		asm volatile("csrs " #value ", %0" : : "r" (x));} \
	static B_ALWAYS_INLINE void ClearBits##Name(uint64 x) { \
		asm volatile("csrc " #value ", %0" : : "r" (x));} \
	static B_ALWAYS_INLINE uint64 GetAndSetBits##Name(uint64 x) { \
		uint64 res; \
		asm volatile("csrrs %0, " #value ", %1" : "=r" (res) : "r" (x)); \
		return res; \
	} \
	static B_ALWAYS_INLINE uint64 GetAndClearBits##Name(uint64 x) { \
		uint64 res; \
		asm volatile("csrrc %0, " #value ", %1" : "=r" (res) : "r" (x)); \
		return res; \
	} \

// CPU core ID
CSR_REG_MACRO(Mhartid, mhartid)

// status register
CSR_REG_MACRO(Mstatus, mstatus)
CSR_REG_MACRO(Sstatus, sstatus)

// exception program counter
CSR_REG_MACRO(Mepc, mepc)
CSR_REG_MACRO(Sepc, sepc)

// interrupt pending
CSR_REG_MACRO(Mip, mip)
CSR_REG_MACRO(Sip, sip)

// interrupt enable
CSR_REG_MACRO(Mie, mie)
CSR_REG_MACRO(Sie, sie)

// exception delegation
CSR_REG_MACRO(Medeleg, medeleg)
// interrupt delegation
CSR_REG_MACRO(Mideleg, mideleg)

// trap vector, 2 low bits: mode
CSR_REG_MACRO(Mtvec, mtvec)
CSR_REG_MACRO(Stvec, stvec)

// address translation and protection (pointer to page table and flags)
CSR_REG_MACRO(Satp, satp)

// scratch register
CSR_REG_MACRO(Mscratch, mscratch)
CSR_REG_MACRO(Sscratch, sscratch)

// trap cause
CSR_REG_MACRO(Mcause, mcause)
CSR_REG_MACRO(Scause, scause)

// trap value
CSR_REG_MACRO(Mtval, mtval)
CSR_REG_MACRO(Stval, stval)

// machine-mode counter enable
CSR_REG_MACRO(Mcounteren, mcounteren)

// cycle counter
CSR_REG_MACRO(CpuMcycle, mcycle)
CSR_REG_MACRO(CpuCycle, cycle)
// monotonic timer
CSR_REG_MACRO(CpuTime, time)

// physical memory protection
CSR_REG_MACRO(Pmpaddr0, pmpaddr0)
CSR_REG_MACRO(Pmpcfg0, pmpcfg0)

// flush the TLB
static B_ALWAYS_INLINE void FlushTlbAll() {
	asm volatile("sfence.vma" : : : "memory");}
static B_ALWAYS_INLINE void FlushTlbPage(uint64 x) {
	asm volatile("sfence.vma %0" : : "r" (x) : "memory");}
static B_ALWAYS_INLINE void FlushTlbAllAsid(uint64 asid) {
	asm volatile("sfence.vma x0, %0" : : "r" (asid) : "memory");}
static B_ALWAYS_INLINE void FlushTlbPageAsid(uint64 page, uint64 asid) {
	asm volatile("sfence.vma %0, %0" : : "r" (page), "r" (asid) : "memory");}

// flush instruction cache
static B_ALWAYS_INLINE void FenceI() {
	asm volatile("fence.i" : : : "memory");}

static B_ALWAYS_INLINE uint64 Sp() {
	uint64 x; asm volatile("mv %0, sp" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetSp(uint64 x) {
	asm volatile("mv sp, %0" : : "r" (x));}
static B_ALWAYS_INLINE uint64 Fp() {
	uint64 x; asm volatile("mv %0, fp" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetFp(uint64 x) {
	asm volatile("mv fp, %0" : : "r" (x));}
static B_ALWAYS_INLINE uint64 Tp() {
	uint64 x; asm volatile("mv %0, tp" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetTp(uint64 x) {
	asm volatile("mv tp, %0" : : "r" (x));}
static B_ALWAYS_INLINE uint64 Ra() {
	uint64 x; asm volatile("mv %0, ra" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetRa(uint64 x) {
	asm volatile("mv ra, %0" : : "r" (x));}

static B_ALWAYS_INLINE void Ecall() {asm volatile("ecall");}

// Wait for interrupts, reduce CPU load when inactive.
static B_ALWAYS_INLINE void Wfi() {asm volatile("wfi");}

static B_ALWAYS_INLINE void Mret() {asm volatile("mret");}
static B_ALWAYS_INLINE void Sret() {asm volatile("sret");}

#endif // __cplusplus


#define SPINLOCK_PAUSE()	do {} while (false)


#endif	/* _SYSTEM_ARCH_RISCV64_DEFS_H */

