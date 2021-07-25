#include "arch_traps.h"

#include <KernelExport.h>
#include <arch_cpu_defs.h>


void
WriteMode(int mode)
{
	switch (mode) {
		case modeU: dprintf("u"); break;
		case modeS: dprintf("s"); break;
		case modeM: dprintf("m"); break;
		default: dprintf("%d", mode);
	}
}


void
WriteModeSet(uint32_t val)
{
	bool first = true;
	dprintf("{");
	for (int i = 0; i < 32; i++) {
		if (((1LL << i) & val) != 0) {
			if (first) first = false; else dprintf(", ");
			WriteMode(i);
		}
	}
	dprintf("}");
}


void
WriteExt(uint64_t val)
{
	switch (val) {
		case 0: dprintf("off"); break;
		case 1: dprintf("initial"); break;
		case 2: dprintf("clean"); break;
		case 3: dprintf("dirty"); break;
		default: dprintf("%" B_PRId64, val);
	}
}


void
WriteSstatus(uint64_t val)
{
	SstatusReg status(val);
	dprintf("%#" B_PRIx64, val);
	dprintf(" (");
	dprintf("ie: "); WriteModeSet(status.ie);
	dprintf(", pie: "); WriteModeSet(status.pie);
	dprintf(", spp: "); WriteMode(status.spp);
	dprintf(", fs: "); WriteExt(status.fs);
	dprintf(", xs: "); WriteExt(status.xs);
	dprintf(", sum: %d", (int)status.sum);
	dprintf(", mxr: %d", (int)status.mxr);
	dprintf(", uxl: %d", (int)status.uxl);
	dprintf(", sd: %d", (int)status.sd);
	dprintf(")");
}


void
WriteInterrupt(uint64_t val)
{
	switch (val) {
		case 0 + modeU: dprintf("uSoft"); break;
		case 0 + modeS: dprintf("sSoft"); break;
		case 0 + modeM: dprintf("mSoft"); break;
		case 4 + modeU: dprintf("uTimer"); break;
		case 4 + modeS: dprintf("sTimer"); break;
		case 4 + modeM: dprintf("mTimer"); break;
		case 8 + modeU: dprintf("uExtern"); break;
		case 8 + modeS: dprintf("sExtern"); break;
		case 8 + modeM: dprintf("mExtern"); break;
		default: dprintf("%" B_PRId64, val);
	}
}


void
WriteInterruptSet(uint64_t val)
{
	bool first = true;
	dprintf("{");
	for (int i = 0; i < 64; i++) {
		if (((1LL << i) & val) != 0) {
			if (first) first = false; else dprintf(", ");
			WriteInterrupt(i);
		}
	}
	dprintf("}");
}


void
WriteCause(uint64_t cause)
{
	if ((cause & causeInterrupt) == 0) {
		dprintf("exception ");
		switch (cause) {
			case causeExecMisalign: dprintf("execMisalign"); break;
			case causeExecAccessFault: dprintf("execAccessFault"); break;
			case causeIllegalInst: dprintf("illegalInst"); break;
			case causeBreakpoint: dprintf("breakpoint"); break;
			case causeLoadMisalign: dprintf("loadMisalign"); break;
			case causeLoadAccessFault: dprintf("loadAccessFault"); break;
			case causeStoreMisalign: dprintf("storeMisalign"); break;
			case causeStoreAccessFault: dprintf("storeAccessFault"); break;
			case causeUEcall: dprintf("uEcall"); break;
			case causeSEcall: dprintf("sEcall"); break;
			case causeMEcall: dprintf("mEcall"); break;
			case causeExecPageFault: dprintf("execPageFault"); break;
			case causeLoadPageFault: dprintf("loadPageFault"); break;
			case causeStorePageFault: dprintf("storePageFault"); break;
			default: dprintf("%" B_PRId64, cause);
			}
	} else {
		dprintf("interrupt "); WriteInterrupt(cause & ~causeInterrupt);
	}
}


void
WritePC(addr_t pc)
{
	dprintf("0x%" B_PRIxADDR, pc);
}


void
DoStackTrace(addr_t fp, addr_t pc)
{
	dprintf("Stack:\n");
	dprintf("FP: 0x%" B_PRIxADDR, fp);
	if (pc != 0) {
		dprintf(", PC: "); WritePC(pc);
	}
	dprintf("\n");
	while (fp != 0) {
		pc = *((uint64*)fp - 1);
		fp = *((uint64*)fp - 2);
		dprintf("FP: 0x%" B_PRIxADDR, fp);
		dprintf(", PC: "); WritePC(pc);
		dprintf("\n");
		if (pc == 0) break;
	}
}


void
STrap(iframe* frame)
{
	uint64 cause = Scause();

	dprintf("STrap("); WriteCause(Scause()); dprintf(")\n");
	dprintf("  sstatus: "); WriteSstatus(Sstatus()); dprintf("\n");
	dprintf("  sie: "); WriteInterruptSet(Sie()); dprintf("\n");
	dprintf("  sip: "); WriteInterruptSet(Sip()); dprintf("\n");
	dprintf("  sepc: 0x%" B_PRIxADDR "\n", Sepc());
	dprintf("  stval: 0x%" B_PRIxADDR "\n", Stval());
	dprintf("  sscratch: 0x%" B_PRIxADDR "\n", Sscratch());
	DoStackTrace(Fp(), Sepc());
	for (;;) Wfi();
}


void
arch_traps_init()
{
	dprintf("init_arch_traps()\n");
	SetStvec((uint64)SVec);
}
