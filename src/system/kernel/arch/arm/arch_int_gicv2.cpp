/*
 * Copyright 2021-2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <interrupts.h>
#include <interrupt_controller.h>
#include <kernel.h>
#include <vm/vm.h>
#include <smp.h>

#include "arch_int_gicv2.h"
#include "gicv2_regs.h"


#define ICI_IRQ 0
#define SHARED_IRQ_BASE 32


GICv2InterruptController::GICv2InterruptController(uint32_t gicd_addr, uint32_t gicc_addr)
: InterruptController()
{
	reserve_io_interrupt_vectors(1020, 0, INTERRUPT_TYPE_IRQ);

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

	// disable GICD
	fGicdRegs[GICD_REG_CTLR] = 0;

	// disable GICC
	fGiccRegs[GICC_REG_CTLR] = 0;

	// TODO: disable all interrupts
	fGicdRegs[GICD_REG_ICENABLER] = 0xffffffff;
	fGicdRegs[GICD_REG_ICENABLER+1] = 0xffffffff;

	// enable GICD
	fGicdRegs[GICD_REG_CTLR] = 0x03;

	_PerCpuInit();
	call_all_cpus_sync([](void *arg, int cpu) {
		GICv2InterruptController* self = (GICv2InterruptController *)arg;
		self->_PerCpuInit();
	}, this);

	EnableInterrupt(ICI_IRQ);
}


void GICv2InterruptController::_PerCpuInit()
{
	// set PMR and BPR
	fGiccRegs[GICC_REG_PMR] = 0xff;
	fGiccRegs[GICC_REG_BPR] = 0x07;

	// enable GICC
	fGiccRegs[GICC_REG_CTLR] = 0x01;
}


void GICv2InterruptController::EnableInterrupt(int32 irq)
{
	_EnableInterrupt(irq);
	if (irq < SHARED_IRQ_BASE) {
		call_all_cpus_sync([](void *arg, int cpu) {
			int32 irq = (int32)(addr_t)arg;
			GICv2InterruptController *self =
				(GICv2InterruptController *)InterruptController::Get();

			self->_EnableInterrupt(irq);
		}, (void*)(addr_t)irq);
	}
}


void GICv2InterruptController::_EnableInterrupt(int32 irq)
{
	uint32_t ena_reg = GICD_REG_ISENABLER + irq / 32;
	uint32_t ena_val = 1 << (irq % 32);
	fGicdRegs[ena_reg] = ena_val;

	uint32_t prio_reg = GICD_REG_IPRIORITYR + irq / 4;
	uint32_t prio_val = fGicdRegs[prio_reg];
	prio_val |= 0x80 << (irq % 4 * 8);
	fGicdRegs[prio_reg] = prio_val;
}


void GICv2InterruptController::DisableInterrupt(int32 irq)
{
	_DisableInterrupt(irq);
	if (irq < SHARED_IRQ_BASE) {
		call_all_cpus_sync([](void *arg, int cpu) {
			int32 irq = (int32)(addr_t)arg;
			GICv2InterruptController *self =
				(GICv2InterruptController *)InterruptController::Get();

			self->_DisableInterrupt(irq);
		}, (void*)(addr_t)irq);
	}
}


void GICv2InterruptController::_DisableInterrupt(int32 irq)
{
	fGicdRegs[GICD_REG_ICENABLER + irq / 32] = 1 << (irq % 32);
}


void GICv2InterruptController::HandleInterrupt()
{
	uint32_t iar = fGiccRegs[GICC_REG_IAR];
	uint32_t irqnr = iar & 0x3FF;
	if ((irqnr == 1022) || (irqnr == 1023)) {
		dprintf("spurious interrupt\n");
	} else if (irqnr == ICI_IRQ) {
		smp_intercpu_interrupt_handler(smp_get_current_cpu());
	} else {
		io_interrupt_handler(irqnr, true);
	}

	fGiccRegs[GICC_REG_EOIR] = iar;
}


void GICv2InterruptController::SendMulticastIci(CPUSet& cpuSet)
{
	fGicdRegs[GICD_REG_SGIR] = (cpuSet.Bits(0) << 16);
}


void GICv2InterruptController::SendBroadcastIci()
{
	fGicdRegs[GICD_REG_SGIR] = (0b01 << 24);
}
