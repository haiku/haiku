/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#include "traps.h"
#include <KernelExport.h>
#include <arch_cpu_defs.h>
#include <arch_int.h>
#include <Htif.h>
#include <Clint.h>


// TODO: Put machine mode code in separate section and keep it loaded when
// kernel is running.


struct iframe {
	uint64 ra;
	uint64 t6;
	uint64 sp;
	uint64 gp;
	uint64 tp;
	uint64 t0;
	uint64 t1;
	uint64 t2;
	uint64 t5;
	uint64 s1;
	uint64 a0;
	uint64 a1;
	uint64 a2;
	uint64 a3;
	uint64 a4;
	uint64 a5;
	uint64 a6;
	uint64 a7;
	uint64 s2;
	uint64 s3;
	uint64 s4;
	uint64 s5;
	uint64 s6;
	uint64 s7;
	uint64 s8;
	uint64 s9;
	uint64 s10;
	uint64 s11;
	uint64 t3;
	uint64 t4;
	uint64 fp;
	uint64 epc;
};


__attribute__ ((aligned (16))) char sMStack[64*1024];


extern "C" void MVec();
extern "C" void MVecS();

static void
InitPmp()
{
	// Setup physical memory protecton. By default physical memory can be only
	// accessed from machine mode.

	// We allow access to whole physical memory from non-machine mode.

	SetPmpaddr0((~0L) >> 10);
	SetPmpcfg0((1 << pmpR) | (1 << pmpW) | (1 << pmpX) | (pmpMatchNapot));
}


extern "C" status_t __attribute__((naked))
MSyscall(uint64 op, ...)
{
	asm volatile("ecall");
	asm volatile("ret");
}


extern "C" void
MTrap(iframe* frame)
{
	uint64 cause = Mcause();
/*
	HtifOutString("MTrap("); WriteCause(Mcause()); HtifOutString(")\n");
	dprintf("  mstatus: "); WriteMstatus(Mstatus()); dprintf("\n");
	dprintf("  mie: "); WriteInterruptSet(Mie()); dprintf("\n");
	dprintf("  mip: "); WriteInterruptSet(Mip()); dprintf("\n");
	dprintf("  sie: "); WriteInterruptSet(Sie()); dprintf("\n");
	dprintf("  sip: "); WriteInterruptSet(Sip()); dprintf("\n");
	dprintf("  mscratch: 0x%" B_PRIxADDR "\n", Mscratch());
	DoStackTrace(Fp(), 0);
*/
	switch (cause) {
		case causeMEcall:
		case causeSEcall: {
			frame->epc += 4;
			uint64 op = frame->a0;
			switch (op) {
				case kMSyscallSwitchToSmode: {
					HtifOutString("switchToSmodeMmodeSyscall()\n");
					if (cause != causeMEcall) {
						frame->a0 = B_NOT_ALLOWED;
						return;
					}
					MstatusReg status(Mstatus());
					status.mpp = modeS;
					SetMedeleg(
						0xffff & ~((1 << causeMEcall) | (1 << causeSEcall)));
					SetMideleg(0xffff & ~(1 << mTimerInt));
					SetMstatus(status.val);
					dprintf("modeM stack: 0x%" B_PRIxADDR ", 0x%" B_PRIxADDR
						"\n", (addr_t)sMStack,
						(addr_t)(sMStack + sizeof(sMStack)));
					SetMscratch((addr_t)(sMStack + sizeof(sMStack)));
					SetMtvec((uint64)MVecS);
					frame->a0 = B_OK;
					return;
				}
				case kMSyscallSetTimer: {
					bool enable = frame->a1 != 0;
					/*
					dprintf("setTimerMmodeSyscall(%d, %" B_PRIu64 ")\n",
						enable, frame->a2);
					*/
					// dprintf("  mtime: %" B_PRIu64 "\n", gClintRegs->mTime);
					SetMip(Mip() & ~(1 << sTimerInt));
					if (!enable) {
						SetMie(Mie() & ~(1 << mTimerInt));
					} else {
						gClintRegs->mtimecmp[0] = frame->a2;
						SetMie(Mie() | (1 << mTimerInt));
					}
					frame->a0 = B_OK;
					return;
				}
				default:
					frame->a0 = B_NOT_SUPPORTED;
					return;
			}
			break;
		}
		case causeInterrupt + mTimerInt: {
			SetMie(Mie() & ~(1 << mTimerInt));
			SetMip(Mip() | (1 << sTimerInt));
			return;
		}
	}
	HtifOutString("unhandled MTrap\n");
	HtifShutdown();
}


void
traps_init()
{
	SetMtvec((uint64)MVec);
	MstatusReg mstatus(Mstatus());
	mstatus.ie = 1 << modeM;
	SetMstatus(mstatus.val);
	InitPmp();
	MSyscall(kMSyscallSwitchToSmode);
}
