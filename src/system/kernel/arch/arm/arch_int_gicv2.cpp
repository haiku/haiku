/*
 * Copyright 2021-2026 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <interrupts.h>
#include <interrupt_controller.h>
#include <kernel.h>
#include <vm/vm.h>
#include <smp.h>
#include <KernelExport.h>

#include "arch_int_gicv2.h"
#include "gicv2_regs.h"


#define ICI_IRQ 0


GICv2InterruptController::GICv2InterruptController(phys_addr_t gicd_addr, phys_addr_t gicc_addr)
: InterruptController()
{
	// map memory area for distributor (GICD) and CPU interface (GICC)

	area_id gicd_area = vm_map_physical_memory(B_SYSTEM_TEAM, "intc-gicv2-gicd",
		(void**)&fGicdRegs, B_ANY_KERNEL_ADDRESS, GICD_REG_SIZE,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		gicd_addr ? gicd_addr : GICD_REG_START, false);
	if (gicd_area < 0) {
		panic("not able to map the memory area for gicd\n");
	}

	area_id gicc_area = vm_map_physical_memory(B_SYSTEM_TEAM, "intc-gicv2-gicc",
		(void**)&fGiccRegs, B_ANY_KERNEL_ADDRESS, GICC_REG_SIZE,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		gicc_addr ? gicc_addr : GICC_REG_START, false);
	if (gicc_area < 0) {
		panic("not able to map the memory area for gicc\n");
	}

	// Configure the GIC. Refer to chapter 4, Programmers' Model,
	// in ARM GIC Architecture Spec v2.0
	// https://support.arm.com/documentation/ihi0048/latest/

	// disable GICC and GICD before configuring
	fGiccRegs[GICC_REG_CTLR] = 0x00;
	fGicdRegs[GICD_REG_CTLR] = 0x00;

	// how many registers cover all IRQs?
	uint32 typer = fGicdRegs[GICD_REG_TYPER];
	uint32 lineRegs = (typer & 0x1f) + 1;
	reserve_io_interrupt_vectors(32 * lineRegs, 0, INTERRUPT_TYPE_IRQ);

	for (uint32 i = 1; i < lineRegs; i++) { // 1 bit per IRQ
		// mask and clear state before enabling
		fGicdRegs[GICD_REG_ICENABLER + i] = 0xffffffff;
		fGicdRegs[GICD_REG_ICPENDR + i] = 0xffffffff;
	}

	// route SPIs to the boot CPU
	// TODO: don't assume boot CPU is CPU 0
	for (uint32 i = 8; i < lineRegs * 8; i++) // 8 bits per IRQ
		fGicdRegs[GICD_REG_ITARGETSR + i] = 0x01010101;

	// all SPIs level-sensitive
	for (uint32 i = 2; i < lineRegs * 2; i++) // 2 bits per IRQ
		fGicdRegs[GICD_REG_ICFGR + i] = 0x0;

	// GICD configured, enable (GICC follows per-CPU)
	fGicdRegs[GICD_REG_CTLR] = 0x03;

	// PerCpuInit is not called separately for the boot CPU
	PerCpuInit();
}


status_t
GICv2InterruptController::PerCpuInit()
{
	// configure IRQs 0-31 (SGIs/PPIs) which are banked per CPU interface
	fGicdRegs[GICD_REG_ICENABLER] = 0xffffffff;
	fGicdRegs[GICD_REG_ICPENDR] = 0xffffffff;

	// disable priority masking
	fGiccRegs[GICC_REG_PMR] = 0xff;
	// disable interrupt preemption
	fGiccRegs[GICC_REG_BPR] = 0x07;

	// enable GICC
	fGiccRegs[GICC_REG_CTLR] = 0x01;

	EnableInterrupt(ICI_IRQ);

	return B_OK;
}


void
GICv2InterruptController::EnableInterrupt(int32 irq)
{
	uint32_t ena_reg = GICD_REG_ISENABLER + irq / 32;
	uint32_t ena_val = 1 << (irq % 32);
	fGicdRegs[ena_reg] = ena_val;

	uint32_t prio_reg = GICD_REG_IPRIORITYR + irq / 4;
	uint32_t prio_val = fGicdRegs[prio_reg];
	prio_val |= 0x80 << (irq % 4 * 8);
	fGicdRegs[prio_reg] = prio_val;
}


void
GICv2InterruptController::DisableInterrupt(int32 irq)
{
	fGicdRegs[GICD_REG_ICENABLER + irq / 32] = 1 << (irq % 32);
}


void GICv2InterruptController::HandleInterrupt()
{
	uint32_t iar = fGiccRegs[GICC_REG_IAR];
	uint32_t irqnr = iar & 0x3FF;

	if ((irqnr == 1022) || (irqnr == 1023)) {
		dprintf("spurious interrupt (%d)\n", irqnr);
	} else if (irqnr == ICI_IRQ) {
		smp_intercpu_interrupt_handler(smp_get_current_cpu());
	} else {
		io_interrupt_handler(irqnr, true /* level-sensitive interrupt */);
	}

	fGiccRegs[GICC_REG_EOIR] = iar;
}


void GICv2InterruptController::SendMulticastIci(CPUSet& cpuSet)
{
	fGicdRegs[GICD_REG_SGIR] = ((cpuSet.Bits(0) & 0xff) << 16);
}


void GICv2InterruptController::SendBroadcastIci()
{
	fGicdRegs[GICD_REG_SGIR] = (0b01 << 24);
}
