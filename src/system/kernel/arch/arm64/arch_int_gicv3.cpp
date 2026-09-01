/*
 * Copyright 2017 The Fuchsia Authors
 * Copyright 2026 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "arch_int_gicv3.h"

#include "debug.h"
#include "gicv3_regs.h"
#include "smp.h"
#include "vm/vm.h"

// #define TRACE_ARCH_INT_GICV3
#ifdef TRACE_ARCH_INT_GICV3
#define TRACE(x...) dprintf(x)
#else
#define TRACE(x...) do {} while (0)
#endif

#define ICI_IRQ 0


GICv3InterruptController::GICv3InterruptController(phys_addr_t gicd_phys_addr,
	phys_addr_t gicr_phys_addr, uint32_t num_cpus)
	: fNumCpus(num_cpus)
{
	area_id gicd_area = vm_map_physical_memory(B_SYSTEM_TEAM, "intc-gicv3-gicd", (void**)&fGicdBase,
		B_ANY_KERNEL_ADDRESS, GICD_REG_SIZE, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		gicd_phys_addr, false);
	if (gicd_area < 0) {
		panic("not able to map the memory area for gicd\n");
		return;
	}

	uint32_t pidr2 = GICD_PIDR2;
	uint32_t rev = (pidr2 >> GICD_PIDR2_ARCHREV_SHIFT) & GICD_PIDR2_ARCHREV_MASK;
	if (rev != GICV3 && rev != GICV4) {
		panic("Unknown GIC revision %d", rev);
		return;
	}

	// set the redistributor stride based on the spec and version
	if (rev == GICV3)
		fGicrStride = 0x20000;
	else // rev == GICv4
		fGicrStride = 0x40000;

	size_t redistributor_size = num_cpus * fGicrStride;
	area_id gicr_area = vm_map_physical_memory(B_SYSTEM_TEAM, "intc-gicv3-gicr", (void**)&fGicrBase,
		B_ANY_KERNEL_ADDRESS, redistributor_size, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		gicr_phys_addr, false);
	if (gicr_area < 0) {
		panic("not able to map the memory area for gicr\n");
		return;
	}

	uint32_t typer = GICD_TYPER;
	fMaxInt = static_cast<int32_t>((typer & 0x1f) + 1) * 32;
	reserve_io_interrupt_vectors(fMaxInt, 0, INTERRUPT_TYPE_IRQ);

	dprintf("GICv3 detected: rev %u, max interrupts %u, TYPER %#x\n", rev, fMaxInt, typer);
	// disable the distributor
	GICD_CTLR = 0;
	_WaitForMask(&GICD_CTLR, GICD_CTLR_RWP, 0);
	arm64_isb();

	// distributor config: mask and clear all spis, set group 1, default priority, level-sensitive.
	for (int32 i = 32; i < fMaxInt; i += 32) {
		GICD_ICENABLER(i / 32) = ~0;
		GICD_ICPENDR(i / 32) = ~0;
		GICD_IGROUPR(i / 32) = ~0;
		GICD_IGRPMODR(i / 32) = 0;
		for (uint32 j = 0; j < 8; j++)
			GICD_IPRIORITYR(i / 4 + j) = 0x80808080;
		GICD_ICFGR(i / 16) = 0;
		GICD_ICFGR(i / 16 + 1) = 0;
	}
	_WaitForMask(&GICD_CTLR, GICD_CTLR_RWP, 0);

	// enable distributor with ARE, group 1 enable
	GICD_CTLR = CTLR_ENABLE_G0 | CTLR_ENABLE_G1NS | CTLR_ARE_S;
	_WaitForMask(&GICD_CTLR, GICD_CTLR_RWP, 0);

	// ensure we're running on the cpu gCPU[0] points to
	ASSERT((READ_SPECIALREG(mpidr_el1) & CPU_AFF_MASK) == (gCPU[0].arch.mpidr & CPU_AFF_MASK));

	// set spi to target cpu 0 and its associated affinity. must do this after ARE enable
	for (int32_t i = 32; i < fMaxInt; i++)
		GICD_IROUTER(i) = gCPU[0].arch.mpidr & CPU_AFF_MASK;

	PerCpuInit();

	memory_full_barrier();
	arm64_isb();

	dprintf("GICv3: GICD phys %#lx, "
			"GICR offset/stride %#lx/%#lx\n",
		gicd_phys_addr, gicr_phys_addr, fGicrStride);
}


bool
GICv3InterruptController::_WaitForMask(volatile uint32_t* reg, uint32_t mask, uint32_t expect)
{
	int count = 1000000;
	while ((*reg & mask) != expect) {
		count -= 1;
		if (!count) {
			dprintf("arm_gicv3: wait timeout reg:0x%p, val:0x%x, mask:0x%x\n", reg, *reg, mask);
			return false;
		}
	}
	return true;
}


void
GICv3InterruptController::_SetEnable(uint vector, bool enable)
{
	auto mask = static_cast<uint32_t>(1ULL << (vector % 32));
	if (vector < 32) {
		const int32 cpu_id = smp_get_current_cpu();
		if (enable)
			GICR_ISENABLER0(cpu_id) = mask;
		else
			GICR_ICENABLER0(cpu_id) = mask;
		// Fuchsia was wrongly masking GICD_CTLR_RWP instead of GICR_CTLR_RWP here
		_WaitForMask(&GICR_CTLR(cpu_id), GICR_CTLR_RWP, 0);
	} else {
		if (enable)
			GICD_ISENABLER(vector / 32) = mask;
		else
			GICD_ICENABLER(vector / 32) = mask;
		_WaitForMask(&GICD_CTLR, GICD_CTLR_RWP, 0);
	}
}


// Redistributors for each PE need to be woken up before they will
// distribute interrupts.
// https://developer.arm.com/documentation/198123/0302/Configuring-the-Arm-GIC
void
GICv3InterruptController::_RedistributorSleep(bool sleep)
{
	int32 cpu_id = smp_get_current_cpu();
	ASSERT(!are_interrupts_enabled());

	// GICR_WAKER could be RW or RAZ/WI.  When GICD_CTLR.DS is 1, GICR_WAKER is
	// RW.  However, when GICD_CTLR.DS is 0, GICR_WAKER could be RW or RAZ/WI
	// depending on whether the access is Secure/Non-secure and FEAT_RME.
	//
	// Instead of checking those things we're going to take a shortcut.  In the
	// case we're writing a 1 to WAKER_PROCESSOR_SLEEP we'll read back GICR_WAKER
	// to determine if it's RW or RAZ/WI.  If the former, we'll
	// gic_wait_for_mask.  If the latter, we'll bail out.
	uint waker = GICR_WAKER(cpu_id);
	if (sleep)
		waker |= WAKER_PROCESSOR_SLEEP;
	else
		waker &= ~WAKER_PROCESSOR_SLEEP;
	GICR_WAKER(cpu_id) = waker;
	if (sleep) {
		const uint read_back = GICR_WAKER(cpu_id);
		if ((read_back & WAKER_PROCESSOR_SLEEP) == 0) {
			// Our write didn't take.  Must be RAZ/WI.  Don't bother waiting.
			return;
		}
	}

	uint64_t val = sleep ? WAKER_CHILDREN_ASLEEP : 0;
	_WaitForMask(&GICR_WAKER(cpu_id), WAKER_CHILDREN_ASLEEP, val);
}


status_t
GICv3InterruptController::PerCpuInit()
{
	int32 cpu_id = smp_get_current_cpu();

	// wake up the redistributor
	_RedistributorSleep(false);

	// redistributor config: configure sgi/ppi as non-secure group 1.
	GICR_IGROUPR0(cpu_id) = ~0;
	GICR_IGRPMOD0(cpu_id) = 0;
	_WaitForMask(&GICR_CTLR(cpu_id), GICR_CTLR_RWP, 0);

	// redistributor config: clear and mask sgi/ppi.
	GICR_ICENABLER0(cpu_id) = 0xffffffff;
	GICR_ICPENDR0(cpu_id) = ~0;
	_WaitForMask(&GICR_CTLR(cpu_id), GICR_CTLR_RWP, 0);

	// TODO lpi init (needed for MSI(-X))

	// enable system register interface
	uint32_t sre = gic_read_sre();
	if (!(sre & 0x1)) {
		gic_write_sre(sre | 0x1);
		sre = gic_read_sre();
		ASSERT(sre & 0x1);
	}

	// set priority threshold to max.
	gic_write_pmr(0xff);

	// enable group 1 interrupts.
	gic_write_igrpen(1);

	// known priority for the 32 banked SGIs/PPIs
	for (uint32 n = 0; n < 8; n++)
		GICR_IPRIORITYR(cpu_id, n) = 0x80808080;

	GICR_ICFGR1(cpu_id) = 0; // PPIs level-sensitive

	EnableInterrupt(ICI_IRQ);

	return B_OK;
}


// Extract AFF3, AFF2, and AFF1 field out of a mpidr and format according to the ICC_SGI1R
// register.
constexpr uint64_t
mpidr_aff_mask_to_sgir_mask(uint64_t mpidr)
{
	return static_cast<uint64>(CPU_AFF3(mpidr)) << 48 | static_cast<uint64>(CPU_AFF2(mpidr)) << 32
		| static_cast<uint64>(CPU_AFF1(mpidr)) << 16;
}


// Send a pending IPI for the AFF3-1 cluster we've been accumulating a mask for.
static void
send_sgi_for_cluster(unsigned int irq, uint64_t aff321, uint64_t aff0_mask)
{
	if (aff0_mask) {
		ASSERT((aff0_mask & 0xffff) == aff0_mask);
		const uint64_t sgi1r
			= ((irq & 0xf) << 24) | mpidr_aff_mask_to_sgir_mask(aff321) | aff0_mask;
		gic_write_sgi1r(sgi1r);
	}
}


void
GICv3InterruptController::SendMulticastIci(CPUSet& cpu_mask)
{
	memory_write_barrier();

	uint64_t curr_aff321 = 0; // Current AFF3-1 we're dealing with.
	uint64_t aff0_mask = 0; // 16 bit mask of the AFF0 we're accumulating.

	for (uint32_t cpu = 0; cpu < fNumCpus; cpu++) {
		if (!cpu_mask.GetBit(static_cast<int32_t>(cpu)))
			continue;

		const uint64 mpidr = gCPU[cpu].arch.mpidr;
		const uint64 aff321 = mpidr & (CPU_AFF3_MASK | CPU_AFF2_MASK | CPU_AFF1_MASK);
		const uint64 aff0 = CPU_AFF0(mpidr);

		// Without the RS field set, we can only deal with the first
		// 16 cpus within a single cluster.
		ASSERT(aff0 < 16);

		if (aff321 != curr_aff321) {
			// AFF3-1 has changed, see if we need to fire a pending IPI
			send_sgi_for_cluster(ICI_IRQ, curr_aff321, aff0_mask);
			curr_aff321 = aff321;
			aff0_mask = 0;
		}

		// Accumulate.
		aff0_mask |= 1u << aff0;
	}

	// Fire any leftover accumulated mask.
	send_sgi_for_cluster(ICI_IRQ, curr_aff321, aff0_mask);
}


void
GICv3InterruptController::SendUnicastIci(int32 target_cpu)
{
	memory_write_barrier();

	const uint64 mpidr = gCPU[target_cpu].arch.mpidr;
	const uint64 aff321 = mpidr & (CPU_AFF3_MASK | CPU_AFF2_MASK | CPU_AFF1_MASK);
	const uint64 aff0 = CPU_AFF0(mpidr);
	ASSERT(aff0 < 16);

	send_sgi_for_cluster(ICI_IRQ, aff321, 1u << aff0);
}


void
GICv3InterruptController::SendBroadcastIci()
{
	memory_write_barrier();
	gic_write_sgi1r(ICC_SGI1R_IRM | (static_cast<uint64_t>(ICI_IRQ & 0xf) << 24));
}


void
GICv3InterruptController::EnableInterrupt(int32_t vector)
{
	TRACE("enable vector %u\n", vector);

	if (vector >= fMaxInt)
		return;

	_SetEnable(vector, true);
}


void
GICv3InterruptController::DisableInterrupt(int32_t vector)
{
	TRACE("disable vector %u\n", vector);

	if (vector >= fMaxInt)
		return;

	_SetEnable(vector, false);
}


void
GICv3InterruptController::HandleInterrupt()
{
	// get the current vector
	uint32_t iar = gic_read_iar();
	int32_t irq = static_cast<int32_t>(iar) & 0x3ff;

	TRACE("iar %#x, irq %u\n", iar, irq);

	if (irq >= 1020) {
		if (irq == 1023)
			TRACE("gicv3: no pending interrupt\n");
		else
			dprintf("gicv3: spurious interrupt (%d)\n", irq);

		return;
	}

	TRACE("gicv3: iar %#" B_PRIx32 " cpu %" B_PRId32 " thread %p irq %" B_PRIu32 "\n", iar,
		smp_get_current_cpu(), thread_get_current_thread(), irq);

	// deliver the interrupt
	if (irq == ICI_IRQ)
		smp_intercpu_interrupt_handler(smp_get_current_cpu());
	else
		io_interrupt_handler(irq, true /* level-sensitive interrupt */);

	gic_write_eoir(irq);

	TRACE("cpu %u exit\n", smp_get_current_cpu());
}
