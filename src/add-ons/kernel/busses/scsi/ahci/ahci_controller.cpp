/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ahci_controller.h"
#include "util.h"

#include <KernelExport.h>
#include <stdio.h>

#define TRACE(a...) dprintf("\33[34mahci:\33[0m " a)
#define FLOW(a...)	dprintf("ahci: " a)


AHCIController::AHCIController(device_node_handle node, pci_device_info *device)
	: fNode(node)
	, fPCIDevice(device)
	, fDevicePresentMask(0)
	, fCommandSlotCount(0)
	, fPortCount(0)
 	, fInstanceCheck(-1)
{
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

	TRACE("AHCIController::Init %u:%u:%u vendor %04x, device %04x\n", 
		pciInfo.bus, pciInfo.device, pciInfo.function, pciInfo.vendor_id, pciInfo.device_id);

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

	fCommandSlotCount = 1 + ((fRegs->cap >> CAP_NCS_SHIFT) & CAP_NCS_MASK);
	fPortCount = 1 + ((fRegs->cap >> CAP_NP_SHIFT) & CAP_NP_MASK);

	TRACE("cap: Interface Speed Support: generation %lu\n",	(fRegs->cap >> CAP_ISS_SHIFT) & CAP_ISS_MASK);
	TRACE("cap: Number of Command Slots: %d (raw %#lx)\n",	fCommandSlotCount, (fRegs->cap >> CAP_NCS_SHIFT) & CAP_NCS_MASK);
	TRACE("cap: Number of Ports: %d (raw %#lx)\n",			fPortCount, (fRegs->cap >> CAP_NCS_SHIFT) & CAP_NCS_MASK);
	TRACE("cap: Supports Port Multiplier: %s\n",		(fRegs->cap & CAP_SPM) ? "yes" : "no");
	TRACE("cap: Supports External SATA: %s\n",			(fRegs->cap & CAP_SXS) ? "yes" : "no");
	TRACE("cap: Enclosure Management Supported: %s\n",	(fRegs->cap & CAP_EMS) ? "yes" : "no");
	TRACE("cap: Supports AHCI mode only: %s\n",			(fRegs->cap & CAP_SAM) ? "yes" : "no");
	TRACE("ghc: AHCI Enable: %s\n",						(fRegs->ghc & GHC_AE) ? "yes" : "no");
	TRACE("Ports Implemented: %08lx\n",					fRegs->pi);
	TRACE("AHCI Version %lu.%lu\n",						fRegs->vs >> 16, fRegs->vs & 0xff);

	// disable interrupts
	fRegs->ghc &= ~GHC_IE;
	// clear pending interrupts
	fRegs->is = 0xffffffff;


	return B_OK;
}


void
AHCIController::Uninit()
{
	TRACE("AHCIController::Uninit\n");

	// disable interrupts
	fRegs->ghc &= ~GHC_IE;
	// clear pending interrupts
	fRegs->is = 0xffffffff;

	delete_area(fRegsArea);

// --- Instance check workaround begin
	delete_port(fInstanceCheck);
// --- Instance check workaround end
}


void
AHCIController::ExecuteRequest(scsi_ccb *request)
{
	if (request->target_lun || !IsDevicePresent(request->target_id)) {
		request->subsys_status = SCSI_DEV_NOT_THERE;
		gSCSI->finished(request, 1);
		return;
	}

	TRACE("AHCIController::ExecuteRequest opcode %u, length %u\n", request->cdb[0], request->cdb_length);

	request->subsys_status = SCSI_DEV_NOT_THERE;
	gSCSI->finished(request, 1);
	return;

	request->subsys_status = SCSI_REQ_CMP;
	gSCSI->finished(request, 1);
}


uchar
AHCIController::AbortRequest(scsi_ccb *request)
{
	if (request->target_lun || !IsDevicePresent(request->target_id))
		return SCSI_DEV_NOT_THERE;

	return SCSI_REQ_CMP;
}


uchar
AHCIController::TerminateRequest(scsi_ccb *request)
{
	if (request->target_lun || !IsDevicePresent(request->target_id))
		return SCSI_DEV_NOT_THERE;

	return SCSI_REQ_CMP;
}


uchar
AHCIController::ResetDevice(uchar targetID, uchar targetLUN)
{
	if (targetLUN || !IsDevicePresent(targetID))
		return SCSI_DEV_NOT_THERE;

	return SCSI_REQ_CMP;
}

