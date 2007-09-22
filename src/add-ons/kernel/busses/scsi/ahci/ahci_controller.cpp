/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ahci_controller.h"
#include "util.h"

#include <KernelExport.h>
#include <stdio.h>
#include <string.h>
#include <new>

#define TRACE(a...) dprintf("\33[34mahci:\33[0m " a)
#define FLOW(a...)	dprintf("ahci: " a)


AHCIController::AHCIController(device_node_handle node, pci_device_info *device)
	: fNode(node)
	, fPCIDevice(device)
	, fPCIVendorID(0xffff)
	, fPCIDeviceID(0xffff)
	, fCommandSlotCount(0)
	, fPortCountMax(0)
	, fPortCountAvail(0)
	, fIRQ(0)
 	, fInstanceCheck(-1)
{
	memset(fPort, 0, sizeof(fPort));

	ASSERT(sizeof(ahci_port) == 120);
	ASSERT(sizeof(ahci_hba) == 4096);
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

	if (gPCI->get_pci_info(fPCIDevice, &pciInfo) < B_OK) {
		dprintf("AHCIController::Init ERROR: getting PCI info failed!\n");
		return B_ERROR;
	}

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

	uchar capabilityOffset;
	status_t res = gPCI->find_pci_capability(fPCIDevice, PCI_cap_id_sata, &capabilityOffset);
	if (res == B_OK) {
		uint32 satacr0;
		uint32 satacr1;
		TRACE("PCI SATA capability found at offset 0x%x\n", capabilityOffset);
		satacr0 = gPCI->read_pci_config(fPCIDevice, capabilityOffset, 4);
		satacr1 = gPCI->read_pci_config(fPCIDevice, capabilityOffset + 4, 4);
		TRACE("satacr0 = 0x%08lx, satacr1 = 0x%08lx\n", satacr0, satacr1);
	}

	void *addr = (void *)pciInfo.u.h0.base_registers[5];
	size_t size = pciInfo.u.h0.base_register_sizes[5];

	TRACE("registers at %p, size %#lx\n", addr, size);
	
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
	fPortCountAvail = count_bits_set(fRegs->pi);

	if (fRegs->pi == 0) {
		TRACE("controller doesn't implement any ports\n");
		goto err;
	}

	fIRQ = gPCI->read_pci_config(fPCIDevice, PCI_interrupt_line, 1);
	if (fIRQ == 0 || fIRQ == 0xff) {
		TRACE("no IRQ assigned\n");
		goto err;
	}

	TRACE("cap: Interface Speed Support: generation %lu\n",	(fRegs->cap >> CAP_ISS_SHIFT) & CAP_ISS_MASK);
	TRACE("cap: Number of Command Slots: %d (raw %#lx)\n",	fCommandSlotCount, (fRegs->cap >> CAP_NCS_SHIFT) & CAP_NCS_MASK);
	TRACE("cap: Number of Ports: %d (raw %#lx)\n",			fPortCountMax, (fRegs->cap >> CAP_NP_SHIFT) & CAP_NP_MASK);
	TRACE("cap: Supports Port Multiplier: %s\n",		(fRegs->cap & CAP_SPM) ? "yes" : "no");
	TRACE("cap: Supports External SATA: %s\n",			(fRegs->cap & CAP_SXS) ? "yes" : "no");
	TRACE("cap: Enclosure Management Supported: %s\n",	(fRegs->cap & CAP_EMS) ? "yes" : "no");

	TRACE("cap: Supports 64-bit Addressing: %s\n",		(fRegs->cap & CAP_S64A) ? "yes" : "no");
	TRACE("cap: Supports Native Command Queuing: %s\n",	(fRegs->cap & CAP_SNCQ) ? "yes" : "no");
	TRACE("cap: Supports SNotification Register: %s\n",	(fRegs->cap & CAP_SSNTF) ? "yes" : "no");
	TRACE("cap: Supports Command List Override: %s\n",	(fRegs->cap & CAP_SCLO) ? "yes" : "no");


	TRACE("cap: Supports AHCI mode only: %s\n",			(fRegs->cap & CAP_SAM) ? "yes" : "no");
	TRACE("ghc: AHCI Enable: %s\n",						(fRegs->ghc & GHC_AE) ? "yes" : "no");
	TRACE("Ports Implemented Mask: %#08lx\n",			fRegs->pi);
	TRACE("Number of Available Ports: %d\n",			fPortCountAvail);
	TRACE("AHCI Version %lu.%lu\n",						fRegs->vs >> 16, fRegs->vs & 0xff);
	TRACE("Interrupt %u\n",								fIRQ);

	// setup interrupt handler
	if (install_io_interrupt_handler(fIRQ, Interrupt, this, 0) < B_OK) {
		TRACE("can't install interrupt handler\n");
		goto err;
	}

	for (int i = 0; i <= fPortCountMax; i++) {
		if (fRegs->pi & (1 << i)) {
			fPort[i] = new (std::nothrow)AHCIPort(this, i);
			if (!fPort[i]) {
				TRACE("out of memory creating port %d", i);
				break;
			}
			status_t status = fPort[i]->Init();
			if (status < B_OK) {
				TRACE("init port %d failed", i);
				delete fPort[i];
				fPort[i] = NULL;
				break;
			}
		}
	}

	// enable interrupts
	fRegs->ghc |= GHC_IE;
	RegsFlush();

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
	RegsFlush();

	// clear pending interrupts
	fRegs->is = 0xffffffff;
	RegsFlush();

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
	RegsFlush();
	for (int i = 0; i < 20; i++) {
		snooze(50000);
		if ((fRegs->ghc & GHC_HR) == 0)
			break;
	}
	if (fRegs->ghc & GHC_HR)
		return B_TIMED_OUT;

	fRegs->ghc |= GHC_AE;
	RegsFlush();
	fRegs->cap |= saveCaps;
	fRegs->pi = savePI;
	RegsFlush();

	if (fPCIVendorID == 0x8086) {
		// Intel PCS—Port Control and Status
		// In AHCI enabled systems, bits[3:0] must always be set
		gPCI->write_pci_config(fPCIDevice, 0x92, 2, 
			0xf | gPCI->read_pci_config(fPCIDevice, 0x92, 2));
	}
	return B_OK;
}


int32
AHCIController::Interrupt(void *data)
{
	AHCIController *self = (AHCIController *)data;
	uint32 int_stat = self->fRegs->is & self->fRegs->pi;
	if (int_stat == 0)
		return B_UNHANDLED_INTERRUPT;

	for (int i = 0; i < self->fPortCountMax; i++) {
		if (int_stat & (1 << i)) {
			if (self->fPort[i]) {
				self->fPort[i]->Interrupt();
			} else {
				FLOW("interrupt on non-existent port %d\n", i);
			}
		}
	}

	// clear interrupts
	self->fRegs->is = int_stat;

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

	fPort[request->target_id]->ExecuteRequest(request);
}


uchar
AHCIController::AbortRequest(scsi_ccb *request)
{
	if (request->target_lun || !fPort[request->target_id])
		return SCSI_DEV_NOT_THERE;

	return fPort[request->target_id]->AbortRequest(request);
}


uchar
AHCIController::TerminateRequest(scsi_ccb *request)
{
	if (request->target_lun || !fPort[request->target_id])
		return SCSI_DEV_NOT_THERE;

	return fPort[request->target_id]->TerminateRequest(request);
}


uchar
AHCIController::ResetDevice(uchar targetID, uchar targetLUN)
{
	if (targetLUN || !fPort[targetID])
		return SCSI_DEV_NOT_THERE;

	return fPort[targetID]->ResetDevice();
}

