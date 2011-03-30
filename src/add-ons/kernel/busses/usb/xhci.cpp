/*
 * Copyright 2006-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Some code borrowed from the Haiku EHCI driver
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 * 		Jian Chiang <j.jian.chiang@gmail.com>
 */


#include <module.h>
#include <PCI.h>
#include <USB3.h>
#include <KernelExport.h>

#define TRACE_USB
#include "xhci.h"

#define USB_MODULE_NAME	"xhci"

pci_module_info *XHCI::sPCIModule = NULL;


static int32
xhci_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			TRACE_MODULE("xhci init module\n");
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE_MODULE("xhci uninit module\n");
			return B_OK;
	}

	return EINVAL;
}


usb_host_controller_info xhci_module = {
	{
		"busses/usb/xhci",
		0,
		xhci_std_ops
	},
	NULL,
	XHCI::AddTo
};


module_info *modules[] = {
	(module_info *)&xhci_module,
	NULL
};


XHCI::XHCI(pci_info *info, Stack *stack)
	:	BusManager(stack),
		fCapabilityRegisters(NULL),
		fOperationalRegisters(NULL),
		fRegisterArea(-1),
		fPCIInfo(info),
		fStack(stack),
		fPortCount(0)
{
	if (BusManager::InitCheck() < B_OK) {
		TRACE_ERROR("bus manager failed to init\n");
		return;
	}

	TRACE("constructing new XHCI host controller driver\n");
	fInitOK = false;

	// enable busmaster and memory mapped access
	uint16 command = sPCIModule->read_pci_config(fPCIInfo->bus,
		fPCIInfo->device, fPCIInfo->function, PCI_command, 2);
	command &= ~PCI_command_io;
	command |= PCI_command_master | PCI_command_memory;

	sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2, command);

	// map the registers
	uint32 offset = fPCIInfo->u.h0.base_registers[0] & (B_PAGE_SIZE - 1);
	addr_t physicalAddress = fPCIInfo->u.h0.base_registers[0] - offset;
	size_t mapSize = (fPCIInfo->u.h0.base_register_sizes[0] + offset
		+ B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	TRACE("map physical memory 0x%08lx (base: 0x%08lx; offset: %lx); size: %ld\n",
		fPCIInfo->u.h0.base_registers[0], physicalAddress, offset,
		fPCIInfo->u.h0.base_register_sizes[0]);

	fRegisterArea = map_physical_memory("EHCI memory mapped registers",
		physicalAddress, mapSize, B_ANY_KERNEL_BLOCK_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_READ_AREA | B_WRITE_AREA,
		(void **)&fCapabilityRegisters);
	if (fRegisterArea < B_OK) {
		TRACE("failed to map register memory\n");
		return;
	}

	fCapabilityRegisters += offset;
	fOperationalRegisters = fCapabilityRegisters + ReadCapReg8(XHCI_CAPLENGTH);
	fRuntimeRegisters = fCapabilityRegisters + ReadCapReg8(XHCI_RTSOFF);
	TRACE("mapped capability registers: 0x%08lx\n", (uint32)fCapabilityRegisters);
	TRACE("mapped operational registers: 0x%08lx\n", (uint32)fOperationalRegisters);
	TRACE("mapped rumtime registers: 0x%08lx\n", (uint32)fRuntimeRegisters);

	TRACE("structural parameters1: 0x%08lx\n", ReadCapReg32(XHCI_HCSPARAMS1));
	TRACE("structural parameters2: 0x%08lx\n", ReadCapReg32(XHCI_HCSPARAMS2));
	TRACE("structural parameters3: 0x%08lx\n", ReadCapReg32(XHCI_HCSPARAMS3));
	TRACE("capability parameters: 0x%08lx\n", ReadCapReg32(XHCI_HCCPARAMS));

	// read port count from capability register
	fPortCount = (ReadCapReg32(XHCI_HCSPARAMS1) >> 24) & 0x7f;

	uint32 extendedCapPointer = ((ReadCapReg32(XHCI_HCCPARAMS) >> 16) & 0xffff)
		<< 2;
	if (extendedCapPointer > 0) {
		TRACE("extended capabilities register at %ld\n", extendedCapPointer);

		uint32 legacySupport = ReadCapReg32(extendedCapPointer);
		if ((legacySupport & XHCI_LEGSUP_CAPID_MASK) == XHCI_LEGSUP_CAPID) {
			if ((legacySupport & XHCI_LEGSUP_BIOSOWNED) != 0) {
				TRACE_ALWAYS("the host controller is bios owned, claiming"
					" ownership\n");
				WriteCapReg32(extendedCapPointer, legacySupport 
					| XHCI_LEGSUP_OSOWNED);
				for (int32 i = 0; i < 20; i++) {
					legacySupport = ReadCapReg32(extendedCapPointer);

					if ((legacySupport & XHCI_LEGSUP_BIOSOWNED) == 0)
						break;

					TRACE_ALWAYS("controller is still bios owned, waiting\n");
					snooze(50000);
				}
			}

			if (legacySupport & XHCI_LEGSUP_BIOSOWNED) {
				TRACE_ERROR("bios won't give up control over the host "
					"controller (ignoring)\n");
			} else if (legacySupport & XHCI_LEGSUP_OSOWNED) {
				TRACE_ALWAYS("successfully took ownership of the host "
					"controller\n");
			}

			// Force off the BIOS owned flag, and clear all SMIs. Some BIOSes
			// do indicate a successful handover but do not remove their SMIs
			// and then freeze the system when interrupts are generated.
			WriteCapReg32(extendedCapPointer, legacySupport & ~XHCI_LEGSUP_BIOSOWNED);
			WriteCapReg32(extendedCapPointer + XHCI_LEGCTLSTS,
				XHCI_LEGCTLSTS_DISABLE_SMI);
		} else {
			TRACE("extended capability is not a legacy support register\n");
		}
	} else {
		TRACE("no extended capabilities register\n");
	}

	// halt the host controller
	if (ControllerHalt() < B_OK) {
		return;
	}

	// reset the host controller
	if (ControllerReset() < B_OK) {
		TRACE_ERROR("host controller failed to reset\n");
		return;
	}

	fInitOK = true;
	TRACE("XHCI host controller driver constructed\n");
}


XHCI::~XHCI()
{
	TRACE("tear down XHCI host controller driver\n");

	WriteOpReg(XHCI_CMD, 0);

	delete_area(fRegisterArea);
	put_module(B_PCI_MODULE_NAME);
}


status_t
XHCI::Start()
{
	TRACE("starting XHCI host controller\n");
	TRACE("usbcmd: 0x%08lx; usbsts: 0x%08lx\n", ReadOpReg(XHCI_CMD),
		ReadOpReg(XHCI_STS));

	TRACE_ALWAYS("successfully started the controller\n");
	return BusManager::Start();
}


status_t
XHCI::SubmitTransfer(Transfer *transfer)
{
	return B_OK;
}


status_t
XHCI::CancelQueuedTransfers(Pipe *pipe, bool force)
{
	return B_OK;
}


status_t
XHCI::NotifyPipeChange(Pipe *pipe, usb_change change)
{
	TRACE("pipe change %d for pipe %p\n", change, pipe);
	switch (change) {
		case USB_CHANGE_CREATED:
		case USB_CHANGE_DESTROYED: {
			// ToDo: we should create and keep a single queue head
			// for all transfers to/from this pipe
			break;
		}

		case USB_CHANGE_PIPE_POLICY_CHANGED: {
			// ToDo: for isochronous pipes we might need to adapt to new
			// pipe policy settings here
			break;
		}
	}

	return B_OK;
}


status_t
XHCI::AddTo(Stack *stack)
{
#ifdef TRACE_USB
	set_dprintf_enabled(true);
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	load_driver_symbols("xhci");
#endif
#endif

	if (!sPCIModule) {
		status_t status = get_module(B_PCI_MODULE_NAME,
			(module_info **)&sPCIModule);
		if (status < B_OK) {
			TRACE_MODULE_ERROR("getting pci module failed! 0x%08lx\n", status);
			return status;
		}
	}

	TRACE_MODULE("searching devices\n");
	bool found = false;
	pci_info *item = new(std::nothrow) pci_info;
	if (!item) {
		sPCIModule = NULL;
		put_module(B_PCI_MODULE_NAME);
		return B_NO_MEMORY;
	}

	for (int32 i = 0; sPCIModule->get_nth_pci_info(i, item) >= B_OK; i++) {
		if (item->class_base == PCI_serial_bus && item->class_sub == PCI_usb
			&& item->class_api == PCI_usb_xhci) {
			if (item->u.h0.interrupt_line == 0
				|| item->u.h0.interrupt_line == 0xFF) {
				TRACE_MODULE_ERROR("found device with invalid IRQ - check IRQ "
					"assignment\n");
				continue;
			}

			TRACE_MODULE("found device at IRQ %u\n",
				item->u.h0.interrupt_line);
			XHCI *bus = new(std::nothrow) XHCI(item, stack);
			if (!bus) {
				delete item;
				sPCIModule = NULL;
				put_module(B_PCI_MODULE_NAME);
				return B_NO_MEMORY;
			}

			if (bus->InitCheck() < B_OK) {
				TRACE_MODULE_ERROR("bus failed init check\n");
				delete bus;
				continue;
			}

			// the bus took it away
			item = new(std::nothrow) pci_info;

			bus->Start();
			stack->AddBusManager(bus);
			found = true;
		}
	}

	if (!found) {
		TRACE_MODULE_ERROR("no devices found\n");
		delete item;
		sPCIModule = NULL;
		put_module(B_PCI_MODULE_NAME);
		return ENODEV;
	}

	delete item;
	return B_OK;
}


status_t
XHCI::GetPortStatus(uint8 index, usb_port_status *status)
{
	return B_OK;
}


status_t
XHCI::SetPortFeature(uint8 index, uint16 feature)
{
	return B_BAD_VALUE;
}


status_t
XHCI::ClearPortFeature(uint8 index, uint16 feature)
{
	return B_BAD_VALUE;
}


status_t
XHCI::ResetPort(uint8 index)
{
	TRACE("reset port %d\n", index);

	return B_OK;
}


status_t
XHCI::SuspendPort(uint8 index)
{
	return B_OK;
}


status_t
XHCI::ControllerHalt()
{
	WriteOpReg(XHCI_CMD, 0);

	int32 tries = 100;
	while ((ReadOpReg(XHCI_STS) & STS_HCH) == 0) {
		snooze(1000);
		if (tries-- < 0)
			return B_ERROR;
	}

	return B_OK;
}


status_t
XHCI::ControllerReset()
{
	WriteOpReg(XHCI_CMD, CMD_HCRST);

	int32 tries = 100;
	while (ReadOpReg(XHCI_CMD) & CMD_HCRST) {
		snooze(1000);
		if (tries-- < 0)
			return B_ERROR;
	}

	tries = 100;
	while (ReadOpReg(XHCI_STS) & STS_CNR) {
		snooze(1000);
		if (tries-- < 0)
			return B_ERROR;
	}

	return B_OK;
}


status_t
XHCI::LightReset()
{
	return B_ERROR;
}


int32
XHCI::InterruptHandler(void *data)
{
	return ((XHCI *)data)->Interrupt();
}


int32
XHCI::Interrupt()
{
	int32 result = B_HANDLED_INTERRUPT;

	return result;
}

inline void
XHCI::WriteOpReg(uint32 reg, uint32 value)
{
	*(volatile uint32 *)(fOperationalRegisters + reg) = value;
}


inline uint32
XHCI::ReadOpReg(uint32 reg)
{
	return *(volatile uint32 *)(fOperationalRegisters + reg);
}


inline uint8
XHCI::ReadCapReg8(uint32 reg)
{
	return *(volatile uint8 *)(fCapabilityRegisters + reg);
}


inline uint16
XHCI::ReadCapReg16(uint32 reg)
{
	return *(volatile uint16 *)(fCapabilityRegisters + reg);
}


inline uint32
XHCI::ReadCapReg32(uint32 reg)
{
	return *(volatile uint32 *)(fCapabilityRegisters + reg);
}


inline void
XHCI::WriteCapReg32(uint32 reg, uint32 value)
{
	*(volatile uint32 *)(fCapabilityRegisters + reg) = value;
}

