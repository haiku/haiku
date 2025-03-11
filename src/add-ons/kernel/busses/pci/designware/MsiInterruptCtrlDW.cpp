/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "DWPCIController.h"

#include <interrupts.h>


status_t
MsiInterruptCtrlDW::Init(PciDbiRegs volatile* dbiRegs, int32 msiIrq)
{
	dprintf("MsiInterruptCtrlDW::Init()\n");
	dprintf("  msiIrq: %" B_PRId32 "\n", msiIrq);

	fDbiRegs = dbiRegs;

	physical_entry pe;
	status_t result = get_memory_map(&fMsiData, sizeof(fMsiData), &pe, 1);
	if (result != B_OK) {
		dprintf("  unable to get MSI Memory map!\n");
		return result;
	}

	fMsiPhysAddr = pe.address;
	dprintf("  fMsiPhysAddr: %#" B_PRIxADDR "\n", fMsiPhysAddr);
	fDbiRegs->msiAddrLo = (uint32)fMsiPhysAddr;
	fDbiRegs->msiAddrHi = (uint32)(fMsiPhysAddr >> 32);

	fDbiRegs->msiIntr[0].enable = 0xffffffff;
	fDbiRegs->msiIntr[0].mask = 0xffffffff;

	result = install_io_interrupt_handler(msiIrq, InterruptReceived, this, 0);
	if (result != B_OK) {
		dprintf("  unable to attach MSI irq handler!\n");
		return result;
	}
	result = allocate_io_interrupt_vectors(32, &fMsiStartIrq, INTERRUPT_TYPE_IRQ);

	if (result != B_OK) {
		dprintf("  unable to attach MSI irq handler!\n");
		return result;
	}

	msi_set_interface(static_cast<MSIInterface*>(this));

	dprintf("  fMsiStartIrq: %" B_PRId32 "\n", fMsiStartIrq);

	return B_OK;
}


status_t
MsiInterruptCtrlDW::AllocateVectors(uint32 count, uint32& startVector, uint64& address,
	uint32& data)
{
	if (count != 1)
		return B_ERROR;

	for (int i = 0; i < 32; i++) {
		if (((1 << i) & fAllocatedMsiIrqs[0]) == 0) {
			fAllocatedMsiIrqs[0] |= (1 << i);
			fDbiRegs->msiIntr[0].mask &= ~(1 << i);

			startVector = fMsiStartIrq + i;
			address = fMsiPhysAddr;
			data = i;
			return B_OK;
		}
	}
	return B_ERROR;
}


void
MsiInterruptCtrlDW::FreeVectors(uint32 count, uint32 startVector)
{
	int32 irq = (int32)startVector - fMsiStartIrq;
	while (count > 0) {
		if (irq >= 0 && irq < 32 && ((1 << irq) & fAllocatedMsiIrqs[0]) != 0) {
			fDbiRegs->msiIntr[0].mask |= (1 << (uint32)irq);
			fAllocatedMsiIrqs[0] &= ~(1 << (uint32)irq);
		}
		irq++;
		count--;
	}
}


int32
MsiInterruptCtrlDW::InterruptReceived(void* arg)
{
	return static_cast<MsiInterruptCtrlDW*>(arg)->InterruptReceivedInt();
}


int32
MsiInterruptCtrlDW::InterruptReceivedInt()
{
//	dprintf("MsiInterruptCtrlDW::InterruptReceivedInt()\n");
	uint32 status = fDbiRegs->msiIntr[0].status;
	for (int i = 0; i < 32; i++) {
		if (((1 << i) & status) != 0) {
//			dprintf("MSI IRQ: %d (%ld)\n", i, fStartMsiIrq + i);
			io_interrupt_handler(fMsiStartIrq + i, false);
			fDbiRegs->msiIntr[0].status = (1 << i);
		}
	}
	return B_HANDLED_INTERRUPT;
}
