/*
 * Copyright 2007-2009, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ahci_controller.h"
#include "util.h"

#include <KernelExport.h>
#include <stdio.h>
#include <string.h>
#include <new>

#define TRACE(a...) dprintf("ahci: " a)
#define FLOW(a...)	dprintf("ahci: " a)


AHCIController::AHCIController(device_node *node,
		pci_device_module_info *pciModule, pci_device *device)
	:
	fNode(node),
	fPCI(pciModule),
	fPCIDevice(device),
	fPCIVendorID(0xffff),
	fPCIDeviceID(0xffff),
	fFlags(0),
	fCommandSlotCount(0),
	fPortCountMax(0),
	fPortCountAvail(0),
	fPortImplementedMask(0),
	fIRQ(0),
	fInstanceCheck(-1)
{
	memset(fPort, 0, sizeof(fPort));

	ASSERT(sizeof(ahci_port) == 128);
	ASSERT(sizeof(ahci_hba) == 4352);
	ASSERT(sizeof(fis) == 256);
	ASSERT(sizeof(command_list_entry) == 32);
	ASSERT(sizeof(command_table) == 128);
	ASSERT(sizeof(prd) == 16);
}


AHCIController::~AHCIController()
{
}


status_t
AHCIController::Init()
{
	pci_info pciInfo;
	fPCI->get_pci_info(fPCIDevice, &pciInfo);

	fPCIVendorID = pciInfo.vendor_id;
	fPCIDeviceID = pciInfo.device_id;

	TRACE("AHCIController::Init %u:%u:%u vendor %04x, device %04x\n",
		pciInfo.bus, pciInfo.device, pciInfo.function, fPCIVendorID, fPCIDeviceID);

// --- Instance check workaround begin
	char sName[32];
	snprintf(sName, sizeof(sName), "ahci-inst-%u-%u-%u", pciInfo.bus, pciInfo.device, pciInfo.function);
	if (find_port(sName) >= 0) {
		dprintf("AHCIController::Init ERROR: an instance for object %u:%u:%u already exists\n",
			pciInfo.bus, pciInfo.device, pciInfo.function);
		return B_ERROR;
	}
	fInstanceCheck = create_port(1, sName);
// --- Instance check workaround end

	get_device_info(fPCIVendorID, fPCIDeviceID, NULL, &fFlags);

	uchar capabilityOffset;
	status_t res = fPCI->find_pci_capability(fPCIDevice, PCI_cap_id_sata, &capabilityOffset);
	if (res == B_OK) {
		uint32 satacr0;
		uint32 satacr1;
		TRACE("PCI SATA capability found at offset 0x%x\n", capabilityOffset);
		satacr0 = fPCI->read_pci_config(fPCIDevice, capabilityOffset, 4);
		satacr1 = fPCI->read_pci_config(fPCIDevice, capabilityOffset + 4, 4);
		TRACE("satacr0 = 0x%08" B_PRIx32 ", satacr1 = 0x%08" B_PRIx32 "\n",
			satacr0, satacr1);
	}

	uint16 pcicmd = fPCI->read_pci_config(fPCIDevice, PCI_command, 2);
	TRACE("pcicmd old 0x%04x\n", pcicmd);
	pcicmd &= ~(PCI_command_io | PCI_command_int_disable);
	pcicmd |= PCI_command_master | PCI_command_memory;
	TRACE("pcicmd new 0x%04x\n", pcicmd);
	fPCI->write_pci_config(fPCIDevice, PCI_command, 2, pcicmd);

	if (fPCIVendorID == PCI_VENDOR_JMICRON) {
		uint32 ctrl = fPCI->read_pci_config(fPCIDevice, PCI_JMICRON_CONTROLLER_CONTROL_1, 4);
		TRACE("Jmicron controller control 1 old 0x%08" B_PRIx32 "\n", ctrl);
		ctrl &= ~((1 << 9) | (1 << 12) | (1 << 14));	// disable SFF 8038i emulation
		ctrl |= (1 << 8) | (1 << 13) | (1 << 15);		// enable AHCI controller
		TRACE("Jmicron controller control 1 new 0x%08" B_PRIx32 "\n", ctrl);
		fPCI->write_pci_config(fPCIDevice, PCI_JMICRON_CONTROLLER_CONTROL_1, 4, ctrl);
	}

	fIRQ = pciInfo.u.h0.interrupt_line;
	if (fIRQ == 0 || fIRQ == 0xff) {
		TRACE("Error: PCI IRQ not assigned\n");
		return B_ERROR;
	}

	phys_addr_t addr = pciInfo.u.h0.base_registers[5];
	size_t size = pciInfo.u.h0.base_register_sizes[5];

	TRACE("registers at %#" B_PRIxPHYSADDR ", size %#" B_PRIxSIZE "\n", addr,
		size);
	if (addr == 0) {
		TRACE("PCI base address register 5 not assigned\n");
		return B_ERROR;
	}

	fRegsArea = map_mem((void **)&fRegs, addr, size, 0, "AHCI HBA regs");
	if (fRegsArea < B_OK) {
		TRACE("mapping registers failed\n");
		return B_ERROR;
	}

	if (ResetController() < B_OK) {
		TRACE("controller reset failed\n");
		goto err;
	}

	fCommandSlotCount = 1 + ((fRegs->cap >> CAP_NCS_SHIFT) & CAP_NCS_MASK);
	fPortCountMax = 1 + ((fRegs->cap >> CAP_NP_SHIFT) & CAP_NP_MASK);

	fPortImplementedMask = fRegs->pi;
	if (fPortImplementedMask == 0) {
		fPortImplementedMask = 0xffffffff >> (32 - fPortCountMax);
		TRACE("ports-implemented mask is zero, using 0x%" B_PRIx32 " instead.\n",
			fPortImplementedMask);
	}

	fPortCountAvail = count_bits_set(fPortImplementedMask);

	TRACE("cap: Interface Speed Support: generation %" B_PRIu32 "\n",	(fRegs->cap >> CAP_ISS_SHIFT) & CAP_ISS_MASK);
	TRACE("cap: Number of Command Slots: %d (raw %#" B_PRIx32 ")\n",	fCommandSlotCount, (fRegs->cap >> CAP_NCS_SHIFT) & CAP_NCS_MASK);
	TRACE("cap: Number of Ports: %d (raw %#" B_PRIx32 ")\n",			fPortCountMax, (fRegs->cap >> CAP_NP_SHIFT) & CAP_NP_MASK);
	TRACE("cap: Supports Port Multiplier: %s\n",		(fRegs->cap & CAP_SPM) ? "yes" : "no");
	TRACE("cap: Supports External SATA: %s\n",			(fRegs->cap & CAP_SXS) ? "yes" : "no");
	TRACE("cap: Enclosure Management Supported: %s\n",	(fRegs->cap & CAP_EMS) ? "yes" : "no");

	TRACE("cap: Supports Command List Override: %s\n",	(fRegs->cap & CAP_SCLO) ? "yes" : "no");
	TRACE("cap: Supports Staggered Spin-up: %s\n",	(fRegs->cap & CAP_SSS) ? "yes" : "no");
	TRACE("cap: Supports Mechanical Presence Switch: %s\n",	(fRegs->cap & CAP_SMPS) ? "yes" : "no");

	TRACE("cap: Supports 64-bit Addressing: %s\n",		(fRegs->cap & CAP_S64A) ? "yes" : "no");
	TRACE("cap: Supports Native Command Queuing: %s\n",	(fRegs->cap & CAP_SNCQ) ? "yes" : "no");
	TRACE("cap: Supports SNotification Register: %s\n",	(fRegs->cap & CAP_SSNTF) ? "yes" : "no");
	TRACE("cap: Supports Command List Override: %s\n",	(fRegs->cap & CAP_SCLO) ? "yes" : "no");


	TRACE("cap: Supports AHCI mode only: %s\n",			(fRegs->cap & CAP_SAM) ? "yes" : "no");
	TRACE("ghc: AHCI Enable: %s\n",						(fRegs->ghc & GHC_AE) ? "yes" : "no");
	TRACE("Ports Implemented Mask: %#08" B_PRIx32 "\n",	fPortImplementedMask);
	TRACE("Number of Available Ports: %d\n",			fPortCountAvail);
	TRACE("AHCI Version %" B_PRIu32 ".%" B_PRIu32 "\n",	fRegs->vs >> 16, fRegs->vs & 0xff);
	TRACE("Interrupt %u\n",								fIRQ);

	// setup interrupt handler
	if (install_io_interrupt_handler(fIRQ, Interrupt, this, 0) < B_OK) {
		TRACE("can't install interrupt handler\n");
		goto err;
	}

	for (int i = 0; i <= fPortCountMax; i++) {
		if (fPortImplementedMask & (1 << i)) {
			fPort[i] = new (std::nothrow)AHCIPort(this, i);
			if (!fPort[i]) {
				TRACE("out of memory creating port %d", i);
				break;
			}
			status_t status = fPort[i]->Init1();
			if (status < B_OK) {
				TRACE("init-1 port %d failed", i);
				delete fPort[i];
				fPort[i] = NULL;
			}
		}
	}

	// enable interrupts
	fRegs->ghc |= GHC_IE;
	FlushPostedWrites();

	for (int i = 0; i <= fPortCountMax; i++) {
		if (fPort[i]) {
			status_t status = fPort[i]->Init2();
			if (status < B_OK) {
				TRACE("init-2 port %d failed", i);
				fPort[i]->Uninit();
				delete fPort[i];
				fPort[i] = NULL;
			}
		}
	}


	return B_OK;

err:
	delete_area(fRegsArea);
	return B_ERROR;
}


void
AHCIController::Uninit()
{
	TRACE("AHCIController::Uninit\n");

	for (int i = 0; i <= fPortCountMax; i++) {
		if (fPort[i]) {
			fPort[i]->Uninit();
			delete fPort[i];
		}
	}

	// disable interrupts
	fRegs->ghc &= ~GHC_IE;
	FlushPostedWrites();

	// clear pending interrupts
	fRegs->is = 0xffffffff;
	FlushPostedWrites();

  	// well...
  	remove_io_interrupt_handler(fIRQ, Interrupt, this);

	delete_area(fRegsArea);

// --- Instance check workaround begin
	delete_port(fInstanceCheck);
// --- Instance check workaround end
}


status_t
AHCIController::ResetController()
{
	uint32 saveCaps = fRegs->cap & (CAP_SMPS | CAP_SSS | CAP_SPM | CAP_EMS | CAP_SXS);
	uint32 savePI = fRegs->pi;

	fRegs->ghc |= GHC_HR;
	FlushPostedWrites();
	if (wait_until_clear(&fRegs->ghc, GHC_HR, 1000000) < B_OK)
		return B_TIMED_OUT;

	fRegs->ghc |= GHC_AE;
	FlushPostedWrites();
	fRegs->cap |= saveCaps;
	fRegs->pi = savePI;
	FlushPostedWrites();

	if (fPCIVendorID == PCI_VENDOR_INTEL) {
		// Intel PCS—Port Control and Status
		// SATA port enable bits must be set
		int portCount = 1 + ((fRegs->cap >> CAP_NP_SHIFT) & CAP_NP_MASK);
		if (portCount > 8)
			panic("Intel AHCI: too many SATA ports! Please report at http://dev.haiku-os.org");
		uint16 pcs = fPCI->read_pci_config(fPCIDevice, 0x92, 2);
		pcs |= (0xff >> (8 - portCount));
		fPCI->write_pci_config(fPCIDevice, 0x92, 2, pcs);
	}
	return B_OK;
}


int32
AHCIController::Interrupt(void *data)
{
	AHCIController *self = (AHCIController *)data;
	uint32 interruptPending = self->fRegs->is & self->fPortImplementedMask;

	if (interruptPending == 0)
		return B_UNHANDLED_INTERRUPT;

	for (int i = 0; i < self->fPortCountMax; i++) {
		if (interruptPending & (1 << i)) {
			if (self->fPort[i]) {
				self->fPort[i]->Interrupt();
			} else {
				FLOW("interrupt on non-existent port %d\n", i);
			}
		}
	}

	// clear pending interrupts
	self->fRegs->is = interruptPending;

	return B_INVOKE_SCHEDULER;
}


void
AHCIController::ExecuteRequest(scsi_ccb *request)
{
	if (request->target_lun || !fPort[request->target_id]) {
		request->subsys_status = SCSI_DEV_NOT_THERE;
		gSCSI->finished(request, 1);
		return;
	}

	fPort[request->target_id]->ScsiExecuteRequest(request);
}


uchar
AHCIController::AbortRequest(scsi_ccb *request)
{
	if (request->target_lun || !fPort[request->target_id])
		return SCSI_DEV_NOT_THERE;

	return fPort[request->target_id]->ScsiAbortRequest(request);
}


uchar
AHCIController::TerminateRequest(scsi_ccb *request)
{
	if (request->target_lun || !fPort[request->target_id])
		return SCSI_DEV_NOT_THERE;

	return fPort[request->target_id]->ScsiTerminateRequest(request);
}


uchar
AHCIController::ResetDevice(uchar targetID, uchar targetLUN)
{
	if (targetLUN || !fPort[targetID])
		return SCSI_DEV_NOT_THERE;

	return fPort[targetID]->ScsiResetDevice();
}


void
AHCIController::GetRestrictions(uchar targetID, bool *isATAPI,
	bool *noAutoSense, uint32 *maxBlocks)
{
	if (!fPort[targetID])
		return;

	fPort[targetID]->ScsiGetRestrictions(isATAPI, noAutoSense, maxBlocks);
}
