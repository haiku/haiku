/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_ARCH_RISCV64_DEFS_H
#define _SYSTEM_ARCH_RISCV64_DEFS_H


#include <SupportDefs.h>


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

struct MstatusReg {
	union {
		struct {
			uint64 ie:      4; // interrupt enable
			uint64 pie:     4; // previous interrupt enable
			uint64 spp:     1; // previous mode (supervisor)
			uint64 unused1: 2;
			uint64 mpp:     2; // previous mode (machine)
			uint64 fs:      2; // FPU status
			uint64 xs:      2; // extensions status
			uint64 mprv:    1; // modify privelege
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

	MstatusReg() {}
	MstatusReg(uint64 val): val(val) {}
};

struct SstatusReg {
	union {
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

	SstatusReg() {}
	SstatusReg(uint64 val): val(val) {}
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
	pageSize = 4096,
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

struct Pte {
	union {
		struct {
			uint64 flags:     8;
			uint64 rsw:       2;
			uint64 ppn:      44;
			uint64 reserved: 10;
		};
		uint64 val;
	};

	Pte() {}
	Pte(uint64 val): val(val) {}
};

enum {
	satpModeBare =  0,
	satpModeSv39 =  8,
	satpModeSv48 =  9,
	satpModeSv57 = 10,
	satpModeSv64 = 11,
};

struct SatpReg {
	union {
		struct {
			uint64 ppn:  44;
			uint64 asid: 16;
			uint64 mode:  4;
		};
		uint64 val;
	};

	SatpReg() {}
	SatpReg(uint64 val): val(val) {}
};

static B_ALWAYS_INLINE uint64 VirtAdrPte(uint64 physAdr, uint32 level)
{
	return (physAdr >> (pageBits + pteIdxBits*level)) % (1 << pteIdxBits);
}

static B_ALWAYS_INLINE uint64 VirtAdrOfs(uint64 physAdr)
{
	return physAdr % pageSize;
}

// CPU core ID
static B_ALWAYS_INLINE uint64 Mhartid() {
	uint64 x; asm volatile("csrr %0, mhartid" : "=r" (x)); return x;}

// status register
static B_ALWAYS_INLINE uint64 Mstatus() {
	uint64 x; asm volatile("csrr %0, mstatus" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetMstatus(uint64 x) {
	asm volatile("csrw mstatus, %0" : : "r" (x));}
static B_ALWAYS_INLINE uint64 Sstatus() {
	uint64 x; asm volatile("csrr %0, sstatus" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetSstatus(uint64 x) {
	asm volatile("csrw sstatus, %0" : : "r" (x));}

// exception program counter
static B_ALWAYS_INLINE uint64 Mepc() {
	uint64 x; asm volatile("csrr %0, mepc" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetMepc(uint64 x) {
	asm volatile("csrw mepc, %0" : : "r" (x));}
static B_ALWAYS_INLINE uint64 Sepc() {
	uint64 x; asm volatile("csrr %0, sepc" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetSepc(uint64 x) {
	asm volatile("csrw sepc, %0" : : "r" (x));}

// interrupt pending
static B_ALWAYS_INLINE uint64 Mip() {
	uint64 x; asm volatile("csrr %0, mip" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetMip(uint64 x) {
	asm volatile("csrw mip, %0" : : "r" (x));}
static B_ALWAYS_INLINE uint64 Sip() {
	uint64 x; asm volatile("csrr %0, sip" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetSip(uint64 x) {
	asm volatile("csrw sip, %0" : : "r" (x));}

// interrupt enable
static B_ALWAYS_INLINE uint64 Sie() {
	uint64 x; asm volatile("csrr %0, sie" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetSie(uint64 x) {
	asm volatile("csrw sie, %0" : : "r" (x));}
static B_ALWAYS_INLINE uint64 Mie() {
	uint64 x; asm volatile("csrr %0, mie" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetMie(uint64 x) {
	asm volatile("csrw mie, %0" : : "r" (x));}

// exception delegation
static B_ALWAYS_INLINE uint64 Medeleg() {
	uint64 x; asm volatile("csrr %0, medeleg" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetMedeleg(uint64 x) {
	asm volatile("csrw medeleg, %0" : : "r" (x));}
// interrupt delegation
static B_ALWAYS_INLINE uint64 Mideleg() {
	uint64 x; asm volatile("csrr %0, mideleg" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetMideleg(uint64 x) {
	asm volatile("csrw mideleg, %0" : : "r" (x));}

// trap vector, 2 low bits: mode
static B_ALWAYS_INLINE uint64 Mtvec() {
	uint64 x; asm volatile("csrr %0, mtvec" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetMtvec(uint64 x) {
	asm volatile("csrw mtvec, %0" : : "r" (x));}
static B_ALWAYS_INLINE uint64 Stvec() {
	uint64 x; asm volatile("csrr %0, stvec" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetStvec(uint64 x) {
	asm volatile("csrw stvec, %0" : : "r" (x));}

// address translation and protection (pointer to page table and flags)
static B_ALWAYS_INLINE uint64 Satp() {
	uint64 x; asm volatile("csrr %0, satp" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetSatp(uint64 x) {
	asm volatile("csrw satp, %0" : : "r" (x) : "memory");}

// scratch register
static B_ALWAYS_INLINE uint64 Mscratch() {
	uint64 x; asm volatile("csrr %0, mscratch" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetMscratch(uint64 x) {
	asm volatile("csrw mscratch, %0" : : "r" (x));}
static B_ALWAYS_INLINE uint64 Sscratch() {
	uint64 x; asm volatile("csrr %0, sscratch" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetSscratch(uint64 x) {
	asm volatile("csrw sscratch, %0" : : "r" (x));}

// trap cause
static B_ALWAYS_INLINE uint64 Mcause() {
	uint64 x; asm volatile("csrr %0, mcause" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetMcause(uint64 x) {
	asm volatile("csrw mcause, %0" : : "r" (x));}
static B_ALWAYS_INLINE uint64 Scause() {
	uint64 x; asm volatile("csrr %0, scause" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetScause(uint64 x) {
	asm volatile("csrw scause, %0" : : "r" (x));}

// trap value
static B_ALWAYS_INLINE uint64 Mtval() {
	uint64 x; asm volatile("csrr %0, mtval" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetMtval(uint64 x) {
	asm volatile("csrw mtval, %0" : : "r" (x));}
static B_ALWAYS_INLINE uint64 Stval() {
	uint64 x; asm volatile("csrr %0, stval" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetStval(uint64 x) {
	asm volatile("csrw stval, %0" : : "r" (x));}

// machine-mode counter enable
static B_ALWAYS_INLINE uint64 Mcounteren() {
	uint64 x; asm volatile("csrr %0, mcounteren" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetMcounteren(uint64 x) {
	asm volatile("csrw mcounteren, %0" : : "r" (x));}

// cycle counter
static B_ALWAYS_INLINE uint64 CpuMcycle() {
	uint64 x; asm volatile("csrr %0, mcycle" : "=r" (x)); return x;}
static B_ALWAYS_INLINE uint64 CpuCycle() {
	uint64 x; asm volatile("csrr %0, cycle" : "=r" (x)); return x;}
// monotonic timer
static B_ALWAYS_INLINE uint64 CpuTime() {
	uint64 x; asm volatile("csrr %0, time" : "=r" (x)); return x;}

// physical memory protection
static B_ALWAYS_INLINE uint64 Pmpaddr0() {
	uint64 x; asm volatile("csrr %0, pmpaddr0" : "=r" (x)); return x;}
static B_ALWAYS_INLINE uint64 Pmpcfg0() {
	uint64 x; asm volatile("csrr %0, pmpcfg0" : "=r" (x)); return x;}
static B_ALWAYS_INLINE void SetPmpaddr0(uint64 x) {
	asm volatile("csrw pmpaddr0, %0" : : "r" (x));}
static B_ALWAYS_INLINE void SetPmpcfg0(uint64 x) {
	asm volatile("csrw pmpcfg0, %0" : : "r" (x));}

// flush the TLB
static B_ALWAYS_INLINE void FlushTlbAll() {
	asm volatile("sfence.vma" : : : "memory");}
static B_ALWAYS_INLINE void FlushTlbPage(uint64 x) {
	asm volatile("sfence.vma %0" : : "r" (x) : "memory");}
static B_ALWAYS_INLINE void FlushTlbAllAsid(uint64 asid) {
	asm volatile("sfence.vma x0, %0" : : "r" (asid) : "memory");}
static B_ALWAYS_INLINE void FlushTlbPageAsid(uint64 page, uint64 asid) {
	asm volatile("sfence.vma %0, %0" : : "r" (page), "r" (asid) : "memory");}

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


#define SPINLOCK_PAUSE()	do {} while (false)


#endif	/* _SYSTEM_ARCH_RISCV64_DEFS_H */

