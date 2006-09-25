/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <module.h>
#include <PCI.h>
#include <USB3.h>
#include <KernelExport.h>

#include "ehci.h"

pci_module_info *EHCI::sPCIModule = NULL;


static int32
ehci_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			TRACE(("usb_ehci_module: init module\n"));
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE(("usb_ehci_module: uninit module\n"));
			return B_OK;
	}

	return EINVAL;
}


host_controller_info ehci_module = {
	{
		"busses/usb/ehci",
		0,
		ehci_std_ops
	},
	NULL,
	EHCI::AddTo
};


module_info *modules[] = {
	(module_info *)&ehci_module,
	NULL
};


//
// #pragma mark -
//


#ifdef TRACE_USB

void
print_descriptor_chain(ehci_qtd *descriptor)
{
	while (descriptor) {
		dprintf(" %08lx n%08lx a%08lx t%08lx %08lx %08lx %08lx %08lx %08lx s%ld\n",
			descriptor->this_phy, descriptor->next_phy,
			descriptor->alt_next_phy, descriptor->token,
			descriptor->buffer_phy[0], descriptor->buffer_phy[1],
			descriptor->buffer_phy[2], descriptor->buffer_phy[3],
			descriptor->buffer_phy[4], descriptor->buffer_size);

		if (descriptor->next_phy & EHCI_QTD_TERMINATE)
			break;

		descriptor = (ehci_qtd *)descriptor->next_log;
	}
}

void
print_queue(ehci_qh *queueHead)
{
	dprintf("queue:    t%08lx n%08lx ch%08lx ca%08lx cu%08lx\n",
		queueHead->this_phy, queueHead->next_phy, queueHead->endpoint_chars,
		queueHead->endpoint_caps, queueHead->current_qtd_phy);
	dprintf("overlay:  n%08lx a%08lx t%08lx %08lx %08lx %08lx %08lx %08lx\n",
		queueHead->overlay.next_phy, queueHead->overlay.alt_next_phy,
		queueHead->overlay.token, queueHead->overlay.buffer_phy[0],
		queueHead->overlay.buffer_phy[1], queueHead->overlay.buffer_phy[2],
		queueHead->overlay.buffer_phy[3], queueHead->overlay.buffer_phy[4]);
	print_descriptor_chain((ehci_qtd *)queueHead->element_log);
}

#endif // TRACE_USB


//
// #pragma mark -
//


EHCI::EHCI(pci_info *info, Stack *stack)
	:	BusManager(stack),
		fPCIInfo(info),
		fStack(stack),
		fPeriodicFrameListArea(-1),
		fPeriodicFrameList(NULL),
		fFirstTransfer(NULL),
		fLastTransfer(NULL),
		fFinishThread(-1),
		fCleanupThread(-1),
		fStopThreads(false),
		fFreeListHead(NULL),
		fRootHub(NULL),
		fRootHubAddress(0),
		fPortCount(0),
		fPortResetChange(0),
		fPortSuspendChange(0)
{
	if (BusManager::InitCheck() < B_OK) {
		TRACE_ERROR(("usb_ehci: bus manager failed to init\n"));
		return;
	}

	TRACE(("usb_ehci: constructing new EHCI Host Controller Driver\n"));
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

	TRACE(("usb_ehci: map physical memory 0x%08lx (base: 0x%08lx; offset: %lx); size: %ld\n", fPCIInfo->u.h0.base_registers[0], physicalAddress, offset, fPCIInfo->u.h0.base_register_sizes[0]));
	fRegisterArea = map_physical_memory("EHCI memory mapped registers",
		(void *)physicalAddress, mapSize, B_ANY_KERNEL_BLOCK_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_READ_AREA | B_WRITE_AREA,
		(void **)&fCapabilityRegisters);
	if (fRegisterArea < B_OK) {
		TRACE(("usb_ehci: failed to map register memory\n"));
		return;
	}

	fCapabilityRegisters += offset;
	fOperationalRegisters = fCapabilityRegisters + ReadCapReg8(EHCI_CAPLENGTH);
	TRACE(("usb_ehci: mapped capability registers: 0x%08lx\n", (uint32)fCapabilityRegisters));
	TRACE(("usb_ehci: mapped operational registers: 0x%08lx\n", (uint32)fOperationalRegisters));

	TRACE(("usb_ehci: structural parameters: 0x%08lx\n", ReadCapReg32(EHCI_HCSPARAMS)));
	TRACE(("usb_ehci: capability parameters: 0x%08lx\n", ReadCapReg32(EHCI_HCCPARAMS)));

	// read port count from capability register
	fPortCount = ReadCapReg32(EHCI_HCSPARAMS) & 0x0f;

	uint32 extendedCapPointer = ReadCapReg32(EHCI_HCCPARAMS) >> EHCI_ECP_SHIFT;
	extendedCapPointer &= EHCI_ECP_MASK;
	if (extendedCapPointer > 0) {
		TRACE(("usb_ehci: extended capabilities register at %ld\n", extendedCapPointer));

		uint32 legacySupport = sPCIModule->read_pci_config(fPCIInfo->bus,
			fPCIInfo->device, fPCIInfo->function, extendedCapPointer, 4);
		if ((legacySupport & EHCI_LEGSUP_CAPID_MASK) == EHCI_LEGSUP_CAPID) {
			if (legacySupport & EHCI_LEGSUP_BIOSOWNED) {
				TRACE(("usb_ehci: the host controller is bios owned\n"));
			}

			TRACE(("usb_ehci: claiming ownership of the host controller\n"));
			sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
				fPCIInfo->function, extendedCapPointer, 4, EHCI_LEGSUP_OSOWNED);

			for (int32 i = 0; i < 10; i++) {
				legacySupport = sPCIModule->read_pci_config(fPCIInfo->bus,
					fPCIInfo->device, fPCIInfo->function, extendedCapPointer, 4);

				if (legacySupport & EHCI_LEGSUP_BIOSOWNED) {
					TRACE(("usb_ehci: controller is still bios owned, waiting\n"));
					snooze(50000);
				} else
					break;
			}

			if (legacySupport & EHCI_LEGSUP_BIOSOWNED) {
				TRACE_ERROR(("usb_ehci: bios won't give up control over the host controller\n"));
				return;
			} else if (legacySupport & EHCI_LEGSUP_OSOWNED) {
				TRACE(("usb_ehci: successfully took ownership of the host controller\n"));
			}
		} else {
			TRACE(("usb_ehci: extended capability is not a legacy support register\n"));
		}
	} else {
		TRACE(("usb_ehci: no extended capabilities register\n"));
	}

	// disable interrupts
	WriteOpReg(EHCI_USBINTR, 0);

	// reset the segment register
	WriteOpReg(EHCI_CTRDSSEGMENT, 0);

	// reset the host controller
	if (ControllerReset() < B_OK) {
		TRACE_ERROR(("usb_ehci: host controller failed to reset\n"));
		return;
	}

	// create semaphores the finisher thread will wait for
	fAsyncAdvanceSem = create_sem(0, "EHCI Async Advance");
	fFinishTransfersSem = create_sem(0, "EHCI Finish Transfers");
	fCleanupSem = create_sem(0, "EHCI Cleanup");
	if (fFinishTransfersSem < B_OK || fAsyncAdvanceSem < B_OK
		|| fCleanupSem < B_OK) {
		TRACE_ERROR(("usb_ehci: failed to create semaphores\n"));
		return;
	}

	// create finisher service thread
	fFinishThread = spawn_kernel_thread(FinishThread, "ehci finish thread",
		B_NORMAL_PRIORITY, (void *)this);
	resume_thread(fFinishThread);

	// create cleanup service thread
	fCleanupThread = spawn_kernel_thread(CleanupThread, "ehci cleanup thread",
		B_NORMAL_PRIORITY, (void *)this);
	resume_thread(fCleanupThread);

	// install the interrupt handler and enable interrupts
	install_io_interrupt_handler(fPCIInfo->u.h0.interrupt_line,
		InterruptHandler, (void *)this, 0);
	WriteOpReg(EHCI_USBINTR, EHCI_USBINTR_HOSTSYSERR
		| EHCI_USBINTR_USBERRINT | EHCI_USBINTR_USBINT | EHCI_USBINTR_INTONAA);

	// allocate the periodic frame list
	fPeriodicFrameListArea = fStack->AllocateArea((void **)&fPeriodicFrameList,
		(void **)&physicalAddress, B_PAGE_SIZE, "USB EHCI Periodic Framelist");
	if (fPeriodicFrameListArea < B_OK) {
		TRACE_ERROR(("usb_ehci: unable to allocate periodic framelist\n"));
		return;
	}

	// terminate all elements
	for (int32 i = 0; i < 1024; i++)
		fPeriodicFrameList[i] = EHCI_PFRAMELIST_TERM;

	WriteOpReg(EHCI_PERIODICLISTBASE, (uint32)physicalAddress);

	// allocate a queue head that will always stay in the async frame list
	fAsyncQueueHead = CreateQueueHead();
	if (!fAsyncQueueHead) {
		TRACE_ERROR(("usb_ehci: unable to allocate stray async queue head\n"));
		return;
	}

	fAsyncQueueHead->next_phy = fAsyncQueueHead->this_phy | EHCI_QH_TYPE_QH;
	fAsyncQueueHead->next_log = fAsyncQueueHead;
	fAsyncQueueHead->prev_log = fAsyncQueueHead;
	fAsyncQueueHead->endpoint_chars = EHCI_QH_CHARS_EPS_HIGH | EHCI_QH_CHARS_RECHEAD;
	fAsyncQueueHead->endpoint_caps = 1 << EHCI_QH_CAPS_MULT_SHIFT;
	fAsyncQueueHead->current_qtd_phy = EHCI_QTD_TERMINATE;
	fAsyncQueueHead->overlay.next_phy = EHCI_QTD_TERMINATE;

	WriteOpReg(EHCI_ASYNCLISTADDR, (uint32)fAsyncQueueHead->this_phy
		| EHCI_QH_TYPE_QH);
	TRACE(("usb_ehci: set the async list addr to 0x%08lx\n", ReadOpReg(EHCI_ASYNCLISTADDR)));

	fInitOK = true;
	TRACE(("usb_ehci: EHCI Host Controller Driver constructed\n"));
}


EHCI::~EHCI()
{
	TRACE(("usb_ehci: tear down EHCI Host Controller Driver\n"));

	WriteOpReg(EHCI_USBCMD, 0);
	WriteOpReg(EHCI_CONFIGFLAG, 0);
	CancelAllPendingTransfers();

	int32 result = 0;
	fStopThreads = true;
	delete_sem(fAsyncAdvanceSem);
	delete_sem(fFinishTransfersSem);
	wait_for_thread(fFinishThread, &result);
	wait_for_thread(fCleanupThread, &result);

	delete fRootHub;
	delete_area(fPeriodicFrameListArea);
	delete_area(fRegisterArea);
	put_module(B_PCI_MODULE_NAME);
}


status_t
EHCI::Start()
{
	TRACE(("usb_ehci: starting EHCI Host Controller\n"));
	TRACE(("usb_ehci: usbcmd: 0x%08lx; usbsts: 0x%08lx\n", ReadOpReg(EHCI_USBCMD), ReadOpReg(EHCI_USBSTS)));

	uint32 frameListSize = (ReadOpReg(EHCI_USBCMD) >> EHCI_USBCMD_FLS_SHIFT)
		& EHCI_USBCMD_FLS_MASK;
	WriteOpReg(EHCI_USBCMD, ReadOpReg(EHCI_USBCMD) | EHCI_USBCMD_RUNSTOP
		| EHCI_USBCMD_ASENABLE /*| EHCI_USBCMD_PSENABLE*/
		| (frameListSize << EHCI_USBCMD_FLS_SHIFT)
		| (2 << EHCI_USBCMD_ITC_SHIFT));

	// route all ports to us
	WriteOpReg(EHCI_CONFIGFLAG, EHCI_CONFIGFLAG_FLAG);

	bool running = false;
	for (int32 i = 0; i < 10; i++) {
		uint32 status = ReadOpReg(EHCI_USBSTS);
		TRACE(("usb_ehci: try %ld: status 0x%08lx\n", i, status));

		if (status & EHCI_USBSTS_HCHALTED) {
			snooze(10000);
		} else {
			running = true;
			break;
		}
	}

	// set the interrupt threshold
	WriteOpReg(EHCI_USBCMD, ReadOpReg(EHCI_USBCMD)
		| (1 << EHCI_USBCMD_ITC_SHIFT));

	if (!running) {
		TRACE(("usb_ehci: Host Controller didn't start\n"));
		return B_ERROR;
	}

	fRootHubAddress = AllocateAddress();
	fRootHub = new(std::nothrow) EHCIRootHub(RootObject(), fRootHubAddress);
	if (!fRootHub) {
		TRACE_ERROR(("usb_ehci: no memory to allocate root hub\n"));
		return B_NO_MEMORY;
	}

	if (fRootHub->InitCheck() < B_OK) {
		TRACE_ERROR(("usb_ehci: root hub failed init check\n"));
		return fRootHub->InitCheck();
	}

	SetRootHub(fRootHub);
	TRACE(("usb_ehci: Host Controller started\n"));
	return BusManager::Start();
}


status_t
EHCI::SubmitTransfer(Transfer *transfer)
{
	// short circuit the root hub
	if (transfer->TransferPipe()->DeviceAddress() == fRootHubAddress)
		return fRootHub->ProcessTransfer(this, transfer);

	uint32 type = transfer->TransferPipe()->Type();
	if ((type & USB_OBJECT_CONTROL_PIPE) > 0
		|| (type & USB_OBJECT_BULK_PIPE) > 0) {
		TRACE(("usb_ehci: submitting async transfer\n"));
		return SubmitAsyncTransfer(transfer);
	}

	if ((type & USB_OBJECT_INTERRUPT_PIPE) > 0
		|| (type & USB_OBJECT_ISO_PIPE) > 0) {
		TRACE(("usb_ehci: submitting periodic transfer\n"));
		return SubmitPeriodicTransfer(transfer);
	}

	TRACE_ERROR(("usb_ehci: tried to submit transfer for unknown pipe type %lu\n", type));
 	return B_ERROR;
}


status_t
EHCI::SubmitAsyncTransfer(Transfer *transfer)
{
	ehci_qh *queueHead = CreateQueueHead();
	if (!queueHead) {
		TRACE_ERROR(("usb_ehci: failed to allocate async queue head\n"));
		return B_NO_MEMORY;
	}

	Pipe *pipe = transfer->TransferPipe();
	switch (pipe->Speed()) {
		case USB_SPEED_LOWSPEED:
			queueHead->endpoint_chars = EHCI_QH_CHARS_EPS_LOW;
			break;
		case USB_SPEED_FULLSPEED:
			queueHead->endpoint_chars = EHCI_QH_CHARS_EPS_FULL;
			break;
		case USB_SPEED_HIGHSPEED:
			queueHead->endpoint_chars = EHCI_QH_CHARS_EPS_HIGH;
			break;
		default:
			TRACE_ERROR(("usb_ehci: unknown pipe speed\n"));
			FreeQueueHead(queueHead);
			return B_ERROR;
	}

	if (pipe->Type() & USB_OBJECT_CONTROL_PIPE) {
		queueHead->endpoint_chars |=
			(pipe->Speed() != USB_SPEED_HIGHSPEED ? EHCI_QH_CHARS_CONTROL : 0);
	}

	queueHead->endpoint_chars |= (3 << EHCI_QH_CHARS_RL_SHIFT)
		| (pipe->MaxPacketSize() << EHCI_QH_CHARS_MPL_SHIFT)
		| (pipe->EndpointAddress() << EHCI_QH_CHARS_EPT_SHIFT)
		| (pipe->DeviceAddress() << EHCI_QH_CHARS_DEV_SHIFT)
		| EHCI_QH_CHARS_TOGGLE;
	queueHead->endpoint_caps = (1 << EHCI_QH_CAPS_MULT_SHIFT)
		| (0x1c << EHCI_QH_CAPS_SCM_SHIFT);

	status_t result;
	bool directionIn;
	ehci_qtd *dataDescriptor;
	if (pipe->Type() & USB_OBJECT_CONTROL_PIPE)
		result = FillQueueWithRequest(transfer, queueHead, &dataDescriptor,
			&directionIn);
	else
		result = FillQueueWithData(transfer, queueHead, &dataDescriptor,
			&directionIn);

	if (result < B_OK) {
		TRACE_ERROR(("usb_ehci: failed to fill transfer queue with data\n"));
		FreeQueueHead(queueHead);
		return result;
	}

	result = AddPendingTransfer(transfer, queueHead, dataDescriptor, directionIn);
	if (result < B_OK) {
		TRACE_ERROR(("usb_ehci: failed to add pending transfer\n"));
		FreeQueueHead(queueHead);
		return result;
	}

#ifdef TRACE_USB
	TRACE(("usb_ehci: linking queue\n"));
	print_queue(queueHead);
#endif

	result = LinkQueueHead(queueHead);
	if (result < B_OK) {
		TRACE_ERROR(("usb_ehci: failed to link queue head to the async list\n"));
		FreeQueueHead(queueHead);
		return result;
	}

	return B_OK;
}


status_t
EHCI::SubmitPeriodicTransfer(Transfer *transfer)
{
	return B_ERROR;
}


status_t
EHCI::AddTo(Stack *stack)
{
#ifdef TRACE_USB
	set_dprintf_enabled(true); 
	load_driver_symbols("ehci");
#endif

	if (!sPCIModule) {
		status_t status = get_module(B_PCI_MODULE_NAME, (module_info **)&sPCIModule);
		if (status < B_OK) {
			TRACE_ERROR(("usb_ehci: getting pci module failed! 0x%08lx\n", status));
			return status;
		}
	}

	TRACE(("usb_ehci: searching devices\n"));
	bool found = false;
	pci_info *item = new(std::nothrow) pci_info;
	if (!item) {
		sPCIModule = NULL;
		put_module(B_PCI_MODULE_NAME);
		return B_NO_MEMORY;
	}

	for (int32 i = 0; sPCIModule->get_nth_pci_info(i, item) >= B_OK; i++) {

		if (item->class_base == PCI_serial_bus && item->class_sub == PCI_usb
			&& item->class_api == PCI_usb_ehci) {
			if (item->u.h0.interrupt_line == 0
				|| item->u.h0.interrupt_line == 0xFF) {
				TRACE_ERROR(("usb_ehci: found device with invalid IRQ - check IRQ assignement\n"));
				continue;
			}

			TRACE(("usb_ehci: found device at IRQ %u\n", item->u.h0.interrupt_line));
			EHCI *bus = new(std::nothrow) EHCI(item, stack);
			if (!bus) {
				delete item;
				sPCIModule = NULL;
				put_module(B_PCI_MODULE_NAME);
				return B_NO_MEMORY;
			}

			if (bus->InitCheck() < B_OK) {
				TRACE_ERROR(("usb_ehci: bus failed init check\n"));
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
		TRACE_ERROR(("usb_ehci: no devices found\n"));
		delete item;
		sPCIModule = NULL;
		put_module(B_PCI_MODULE_NAME);
		return ENODEV;
	}

	delete item;
	return B_OK;
}


status_t
EHCI::GetPortStatus(uint8 index, usb_port_status *status)
{
	if (index >= fPortCount)
		return B_BAD_INDEX;

	status->status = status->change = 0;
	uint32 portStatus = ReadOpReg(EHCI_PORTSC + index * sizeof(uint32));

	// build the status
	if (portStatus & EHCI_PORTSC_CONNSTATUS)
		status->status |= PORT_STATUS_CONNECTION;
	if (portStatus & EHCI_PORTSC_ENABLE)
		status->status |= PORT_STATUS_ENABLE;
	if (portStatus & EHCI_PORTSC_ENABLE)
		status->status |= PORT_STATUS_HIGH_SPEED;
	if (portStatus & EHCI_PORTSC_OCACTIVE)
		status->status |= PORT_STATUS_OVER_CURRENT;
	if (portStatus & EHCI_PORTSC_PORTRESET)
		status->status |= PORT_STATUS_RESET;
	if (portStatus & EHCI_PORTSC_PORTPOWER)
		status->status |= PORT_STATUS_POWER;
	if (portStatus & EHCI_PORTSC_SUSPEND)
		status->status |= PORT_STATUS_SUSPEND;
	if (portStatus & EHCI_PORTSC_DMINUS)
		status->status |= PORT_STATUS_LOW_SPEED;

	// build the change
	if (portStatus & EHCI_PORTSC_CONNCHANGE)
		status->change |= PORT_STATUS_CONNECTION;
	if (portStatus & EHCI_PORTSC_ENABLECHANGE)
		status->change |= PORT_STATUS_ENABLE;
	if (portStatus & EHCI_PORTSC_OCCHANGE)
		status->change |= PORT_STATUS_OVER_CURRENT;

	// there are no bits to indicate suspend and reset change
	if (fPortResetChange & (1 << index))
		status->change |= PORT_STATUS_RESET;
	if (fPortSuspendChange & (1 << index))
		status->change |= PORT_STATUS_SUSPEND;

	return B_OK;
}


status_t
EHCI::SetPortFeature(uint8 index, uint16 feature)
{
	if (index >= fPortCount)
		return B_BAD_INDEX;

	uint32 portRegister = EHCI_PORTSC + index * sizeof(uint32);
	uint32 portStatus = ReadOpReg(portRegister) & EHCI_PORTSC_DATAMASK;

	switch (feature) {
		case PORT_SUSPEND:
			return SuspendPort(index);

		case PORT_RESET:
			return ResetPort(index);

		case PORT_POWER:
			WriteOpReg(portRegister, portStatus | EHCI_PORTSC_PORTPOWER);
			return B_OK;
	}

	return B_BAD_VALUE;
}


status_t
EHCI::ClearPortFeature(uint8 index, uint16 feature)
{
	if (index >= fPortCount)
		return B_BAD_INDEX;

	uint32 portRegister = EHCI_PORTSC + index * sizeof(uint32);
	uint32 portStatus = ReadOpReg(portRegister) & EHCI_PORTSC_DATAMASK;

	switch (feature) {
		case PORT_ENABLE:
			WriteOpReg(portRegister, portStatus & ~EHCI_PORTSC_ENABLE);
			return B_OK;

		case PORT_POWER:
			WriteOpReg(portRegister, portStatus & ~EHCI_PORTSC_PORTPOWER);
			return B_OK;

		case C_PORT_CONNECTION:
			WriteOpReg(portRegister, portStatus | EHCI_PORTSC_CONNCHANGE);
			return B_OK;

		case C_PORT_ENABLE:
			WriteOpReg(portRegister, portStatus | EHCI_PORTSC_ENABLECHANGE);
			return B_OK;

		case C_PORT_OVER_CURRENT:
			WriteOpReg(portRegister, portStatus | EHCI_PORTSC_OCCHANGE);
			return B_OK;

		case C_PORT_RESET:
			fPortResetChange &= ~(1 << index);
			return B_OK;

		case C_PORT_SUSPEND:
			fPortSuspendChange &= ~(1 << index);
			return B_OK;
	}

	return B_BAD_VALUE;
}


status_t
EHCI::ResetPort(uint8 index)
{
	TRACE(("usb_ehci: reset port %d\n", index));
	uint32 portRegister = EHCI_PORTSC + index * sizeof(uint32);
	uint32 portStatus = ReadOpReg(portRegister) & EHCI_PORTSC_DATAMASK;

	if (portStatus & EHCI_PORTSC_DMINUS) {
		TRACE(("usb_ehci: lowspeed device connected, giving up port ownership\n"));
		// there is a lowspeed device connected.
		// we give the ownership to a companion controller.
		WriteOpReg(portRegister, portStatus | EHCI_PORTSC_PORTOWNER);
		fPortResetChange |= (1 << index);
		return B_OK;
	}

	// enable reset signaling
	WriteOpReg(portRegister, portStatus | EHCI_PORTSC_PORTRESET);
	snooze(250000);

	// disable reset signaling
	portStatus = ReadOpReg(portRegister) & EHCI_PORTSC_DATAMASK;
	WriteOpReg(portRegister, portStatus & ~EHCI_PORTSC_PORTRESET);
	snooze(2000);

	portStatus = ReadOpReg(portRegister) & EHCI_PORTSC_DATAMASK;
	if (portStatus & EHCI_PORTSC_PORTRESET) {
		TRACE(("usb_ehci: port reset won't complete\n"));
		return B_ERROR;
	}

	if ((portStatus & EHCI_PORTSC_ENABLE) == 0) {
		TRACE(("usb_ehci: fullspeed device connected, giving up port ownership\n"));
		// the port was not enabled, this means that no high speed device is
		// attached to this port. we give up ownership to a companion controler
		WriteOpReg(portRegister, portStatus | EHCI_PORTSC_PORTOWNER);
	}

	fPortResetChange |= (1 << index);
	return B_OK;
}


status_t
EHCI::SuspendPort(uint8 index)
{
	uint32 portRegister = EHCI_PORTSC + index * sizeof(uint32);
	uint32 portStatus = ReadOpReg(portRegister) & EHCI_PORTSC_DATAMASK;
	WriteOpReg(portRegister, portStatus | EHCI_PORTSC_SUSPEND);
	fPortSuspendChange |= (1 << index);
	return B_OK;
}


status_t
EHCI::ControllerReset()
{
	// halt the controller first
	WriteOpReg(EHCI_USBCMD, 0);
	snooze(10000);

	// then reset it
	WriteOpReg(EHCI_USBCMD, EHCI_USBCMD_HCRESET);

	int32 tries = 5;
	while (ReadOpReg(EHCI_USBCMD) & EHCI_USBCMD_HCRESET) {
		snooze(10000);
		if (tries-- < 0)
			return B_ERROR;
	}

	return B_OK;
}


status_t
EHCI::LightReset()
{
	return B_ERROR;
}


int32
EHCI::InterruptHandler(void *data)
{
	return ((EHCI *)data)->Interrupt();
}


int32
EHCI::Interrupt()
{
	spinlock lock = 0;
	acquire_spinlock(&lock);

	// check if any interrupt was generated
	uint32 status = ReadOpReg(EHCI_USBSTS);
	if ((status & EHCI_USBSTS_INTMASK) == 0) {
		release_spinlock(&lock);
		return B_UNHANDLED_INTERRUPT;
	}

	uint32 acknowledge = 0;
	bool asyncAdvance = false;
	bool finishTransfers = false;
	int32 result = B_HANDLED_INTERRUPT;

	if (status & EHCI_USBSTS_USBINT) {
		TRACE(("usb_ehci: transfer finished\n"));
		acknowledge |= EHCI_USBSTS_USBINT;
		result = B_INVOKE_SCHEDULER;
		finishTransfers = true;
	}

	if (status & EHCI_USBSTS_USBERRINT) {
		TRACE(("usb_ehci: transfer error\n"));
		acknowledge |= EHCI_USBSTS_USBERRINT;
		result = B_INVOKE_SCHEDULER;
		finishTransfers = true;
	}

	if (status & EHCI_USBSTS_PORTCHANGE) {
		TRACE(("usb_ehci: port change detected\n"));
		acknowledge |= EHCI_USBSTS_PORTCHANGE;
	}

	if (status & EHCI_USBSTS_FLROLLOVER) {
		TRACE(("usb_ehci: frame list rolled over\n"));
		acknowledge |= EHCI_USBSTS_FLROLLOVER;
	}

	if (status & EHCI_USBSTS_INTONAA) {
		TRACE(("usb_ehci: interrupt on async advance\n"));
		acknowledge |= EHCI_USBSTS_INTONAA;
		asyncAdvance = true;
		result = B_INVOKE_SCHEDULER;
	}

	if (status & EHCI_USBSTS_HOSTSYSERR) {
		TRACE_ERROR(("usb_ehci: host system error!\n"));
		acknowledge |= EHCI_USBSTS_HOSTSYSERR;
	}

	if (acknowledge)
		WriteOpReg(EHCI_USBSTS, acknowledge);

	release_spinlock(&lock);

	if (asyncAdvance)
		release_sem_etc(fAsyncAdvanceSem, 1, B_DO_NOT_RESCHEDULE);
	if (finishTransfers)
		release_sem_etc(fFinishTransfersSem, 1, B_DO_NOT_RESCHEDULE);

	return result;
}


status_t
EHCI::AddPendingTransfer(Transfer *transfer, ehci_qh *queueHead,
	ehci_qtd *dataDescriptor, bool directionIn)
{
	transfer_data *data = new(std::nothrow) transfer_data();
	if (!data)
		return B_NO_MEMORY;

	data->transfer = transfer;
	data->queue_head = queueHead;
	data->data_descriptor = dataDescriptor;
	data->user_area = -1;
	data->incoming = directionIn;
	data->link = NULL;

#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	if (directionIn) {
		// we might need to access a buffer in userspace. this will not
		// be possible in the kernel space finisher thread unless we
		// get the proper area id for the space we need and then clone it
		// before writing to it. this is of course terribly inefficient...
		iovec *vector = transfer->Vector();
		size_t vectorCount = transfer->VectorCount();
		for (size_t i = 0; i < vectorCount; i++) {
			if (IS_USER_ADDRESS(vector[i].iov_base)) {
				data->user_area = area_for(vector[i].iov_base);
				if (data->user_area < B_OK) {
					TRACE_ERROR(("usb_ehci: failed to get area of userspace buffer\n"));
					delete data;
					return B_BAD_ADDRESS;
				}

				break;
			}
		}

		if (data->user_area >= B_OK) {
			area_info areaInfo;
			if (get_area_info(data->user_area, &areaInfo) < B_OK) {
				TRACE_ERROR(("usb_ehci: failed to get info about user area\n"));
				delete data;
				return B_BAD_ADDRESS;
			}

			for (size_t i = 0; i < vectorCount; i++) {
				(uint8 *)vector[i].iov_base -= (uint8 *)areaInfo.address;

				if ((size_t)vector[i].iov_base > areaInfo.size
					|| (size_t)vector[i].iov_base + vector[i].iov_len > areaInfo.size) {
					TRACE_ERROR(("usb_ehci: output data buffer spans across multiple areas!\n"));
					delete data;
					return B_BAD_ADDRESS;
				}
			}
		}
	}
#endif // !HAIKU_TARGET_PLATFORM_HAIKU

	if (!Lock()) {
		delete data;
		return B_ERROR;
	}

	if (fLastTransfer)
		fLastTransfer->link = data;
	else
		fFirstTransfer = data;

	fLastTransfer = data;
	Unlock();

	return B_OK;
}


status_t
EHCI::CancelPendingTransfer(Transfer *transfer)
{
	if (!Lock())
		return B_ERROR;

	transfer_data *last = NULL;
	transfer_data *current = fFirstTransfer;
	while (current) {
		if (current->transfer == transfer) {
			current->transfer->Finished(B_CANCELED, 0);
			delete current->transfer;

			if (last)
				last->link = current->link;
			else
				fFirstTransfer = current->link;

			if (fLastTransfer == current)
				fLastTransfer = last;

			delete current;
			Unlock();
			return B_OK;
		}

		last = current;
		current = current->link;
	}

	Unlock();
	return B_BAD_VALUE;
}


status_t
EHCI::CancelAllPendingTransfers()
{
	if (!Lock())
		return B_ERROR;

	transfer_data *transfer = fFirstTransfer;
	while (transfer) {
		transfer->transfer->Finished(B_CANCELED, 0);
		delete transfer->transfer;

		transfer_data *next = transfer->link;
		delete transfer;
		transfer = next;
	}

	fFirstTransfer = NULL;
	fLastTransfer = NULL;
	Unlock();
	return B_OK;
}


int32
EHCI::FinishThread(void *data)
{
	((EHCI *)data)->FinishTransfers();
	return B_OK;
}


void
EHCI::FinishTransfers()
{
	while (!fStopThreads) {
		if (acquire_sem(fFinishTransfersSem) < B_OK)
			continue;

		// eat up sems that have been released by multiple interrupts
		int32 semCount = 0;
		get_sem_count(fFinishTransfersSem, &semCount);
		if (semCount > 0)
			acquire_sem_etc(fFinishTransfersSem, semCount, B_RELATIVE_TIMEOUT, 0);

		if (!Lock())
			continue;

		TRACE(("usb_ehci: finishing transfers\n"));
		transfer_data *lastTransfer = NULL;
		transfer_data *transfer = fFirstTransfer;
		Unlock();

		while (transfer) {
			bool transferDone = false;
			ehci_qtd *descriptor = (ehci_qtd *)transfer->queue_head->element_log;

#ifdef TRACE_USB
			print_queue(transfer->queue_head);
#endif

			while (descriptor) {
				uint32 status = descriptor->token;
				if (status & EHCI_QTD_STATUS_ACTIVE) {
					// still in progress
					TRACE(("usb_ehci: qtd (0x%08lx) still active\n", descriptor->this_phy));
					break;
				}

				if (status & EHCI_QTD_STATUS_ERRMASK) {
					// a transfer error occured
					TRACE_ERROR(("usb_ehci: qtd (0x%08lx) error: 0x%08lx\n", descriptor->this_phy, status));

					status_t callbackStatus = B_ERROR;
					uint8 errorCount = status >> EHCI_QTD_ERRCOUNT_SHIFT;
					errorCount &= EHCI_QTD_ERRCOUNT_MASK;
					if (errorCount == 0) {
						// the error counter counted down to zero, report why
						int32 reasons = 0;
						if (status & EHCI_QTD_STATUS_BUFFER) {
							callbackStatus = transfer->incoming ? B_DEV_DATA_OVERRUN : B_DEV_DATA_UNDERRUN;
							reasons++;
						}
						if (status & EHCI_QTD_STATUS_TERROR) {
							callbackStatus = B_DEV_CRC_ERROR;
							reasons++;
						}

						if (reasons > 1)
							callbackStatus = B_DEV_MULTIPLE_ERRORS;
					} else if (status & EHCI_QTD_STATUS_BABBLE) {
						// there is a babble condition
						callbackStatus = transfer->incoming ? B_DEV_FIFO_OVERRUN : B_DEV_FIFO_UNDERRUN;
					} else {
						// if the error counter didn't count down to zero
						// and there was no babble, then this halt was caused
						// by a stall handshake
						callbackStatus = B_DEV_STALLED;
					}

					UnlinkQueueHead(transfer->queue_head, &fFreeListHead);
					transfer->transfer->Finished(callbackStatus, 0);
					transferDone = true;
					break;
				}

				if (descriptor->next_phy & EHCI_QTD_TERMINATE) {
					// we arrived at the last (stray) descriptor, we're done
					TRACE(("usb_ehci: qtd (0x%08lx) done\n", descriptor->this_phy));

					size_t actualLength = 0;
					bool nextDataToggle = false;
					if (transfer->data_descriptor && transfer->incoming) {
						// data to read out
						iovec *vector = transfer->transfer->Vector();
						size_t vectorCount = transfer->transfer->VectorCount();

#ifndef HAIKU_TARGET_PLATFORM_HAIKU
						area_id clonedArea = -1;
						if (transfer->user_area >= B_OK) {
							// we got a userspace output buffer, need to clone
							// the area for that space first and map the iovecs
							// to this cloned area.
							void *clonedMemory = NULL;
							clonedArea = clone_area("userspace accessor",
								&clonedMemory, B_ANY_ADDRESS,
								B_WRITE_AREA | B_KERNEL_WRITE_AREA,
								transfer->user_area);

							for (size_t i = 0; i < vectorCount; i++)
								(uint8 *)vector[i].iov_base += (addr_t)clonedMemory;
						}
#endif // !HAIKU_TARGET_PLATFORM_HAIKU

						actualLength = ReadDescriptorChain(
							transfer->data_descriptor,
							vector, vectorCount,
							&nextDataToggle);

#ifndef HAIKU_TARGET_PLATFORM_HAIKU
						if (clonedArea >= B_OK)
							delete_area(clonedArea);
#endif // !HAIKU_TARGET_PLATFORM_HAIKU
					} else {
						// calculate transfered length
						actualLength = ReadActualLength(
							(ehci_qtd *)transfer->queue_head->element_log,
							&nextDataToggle);
					}

					UnlinkQueueHead(transfer->queue_head, &fFreeListHead);
					transfer->transfer->TransferPipe()->SetDataToggle(nextDataToggle);
					transfer->transfer->Finished(B_OK, actualLength);
					transferDone = true;
					break;
				}

				descriptor = (ehci_qtd *)descriptor->next_log;
			}

			if (transferDone) {
				if (Lock()) {
					if (lastTransfer)
						lastTransfer->link = transfer->link;

					if (transfer == fFirstTransfer)
						fFirstTransfer = transfer->link;
					if (transfer == fLastTransfer)
						fLastTransfer = lastTransfer;

					transfer_data *next = transfer->link;
					delete transfer->transfer;
					delete transfer;
					transfer = next;
					Unlock();
				}
			} else {
				if (Lock()) {
					lastTransfer = transfer;
					transfer = transfer->link;
					Unlock();
				}
			}

			release_sem(fCleanupSem);
		}
	}
}


int32
EHCI::CleanupThread(void *data)
{
	((EHCI *)data)->Cleanup();
	return B_OK;
}


void
EHCI::Cleanup()
{
	ehci_qh *lastFreeListHead = NULL;

	while (!fStopThreads) {
		if (acquire_sem(fCleanupSem) < B_OK)
			continue;

		ehci_qh *freeListHead = fFreeListHead;
		if (freeListHead == lastFreeListHead)
			continue;

		// set the doorbell and wait for the host controller to notify us
		WriteOpReg(EHCI_USBCMD, ReadOpReg(EHCI_USBCMD) | EHCI_USBCMD_INTONAAD);
		if (acquire_sem(fAsyncAdvanceSem) < B_OK)
			continue;

		ehci_qh *current = freeListHead;
		while (current != lastFreeListHead) {
			ehci_qh *next = (ehci_qh *)current->next_log;
			FreeQueueHead(current);
			current = next;
		}

		lastFreeListHead = freeListHead;
	}
}


ehci_qh *
EHCI::CreateQueueHead()
{
	ehci_qh *result;
	void *physicalAddress;
	if (fStack->AllocateChunk((void **)&result, &physicalAddress,
		sizeof(ehci_qh)) < B_OK) {
		TRACE_ERROR(("usb_ehci: failed to allocate queue head\n"));
		return NULL;
	}

	result->this_phy = (addr_t)physicalAddress;
	result->next_phy = EHCI_QH_TERMINATE;
	result->next_log = NULL;
	result->prev_log = NULL;

	ehci_qtd *descriptor = CreateDescriptor(0, 0);
	if (!descriptor) {
		TRACE_ERROR(("usb_ehci: failed to allocate initial qtd for queue head\n"));
		fStack->FreeChunk(result, (void *)result->this_phy, sizeof(ehci_qh));
		return NULL;
	}

	descriptor->token &= ~EHCI_QTD_STATUS_ACTIVE;
	result->stray_log = descriptor;
	result->element_log = descriptor;
	result->current_qtd_phy = EHCI_QTD_TERMINATE;
	result->overlay.next_phy = descriptor->this_phy;
	result->overlay.alt_next_phy = EHCI_QTD_TERMINATE;
	result->overlay.token = 0;
	for (int32 i = 0; i < 5; i++) {
		result->overlay.buffer_phy[i] = 0;
		result->overlay.ext_buffer_phy[i] = 0;
	}

	return result;
}


void
EHCI::FreeQueueHead(ehci_qh *queueHead)
{
	if (!queueHead)
		return;

	FreeDescriptorChain((ehci_qtd *)queueHead->element_log);
	FreeDescriptor((ehci_qtd *)queueHead->stray_log);
	fStack->FreeChunk(queueHead, (void *)queueHead->this_phy, sizeof(ehci_qh));
}


status_t
EHCI::LinkQueueHead(ehci_qh *queueHead)
{
	if (!Lock())
		return B_ERROR;

	ehci_qh *prevHead = (ehci_qh *)fAsyncQueueHead->prev_log;
	queueHead->next_phy = fAsyncQueueHead->this_phy | EHCI_QH_TYPE_QH;
	queueHead->next_log = fAsyncQueueHead;
	queueHead->prev_log = prevHead;
	fAsyncQueueHead->prev_log = queueHead;
	prevHead->next_log = queueHead;
	prevHead->next_phy = queueHead->this_phy | EHCI_QH_TYPE_QH;

	Unlock();
	return B_OK;
}


status_t
EHCI::UnlinkQueueHead(ehci_qh *queueHead, ehci_qh **freeListHead)
{
	if (!Lock())
		return B_ERROR;

	ehci_qh *prevHead = (ehci_qh *)queueHead->prev_log;
	ehci_qh *nextHead = (ehci_qh *)queueHead->next_log;
	prevHead->next_phy = queueHead->next_phy | EHCI_QH_TYPE_QH;
	prevHead->next_log = queueHead->next_log;
	nextHead->prev_log = queueHead->prev_log;
	queueHead->next_phy = fAsyncQueueHead->this_phy | EHCI_QH_TYPE_QH;
	queueHead->next_log = NULL;
	queueHead->prev_log = NULL;

	queueHead->next_log = *freeListHead;
	*freeListHead = queueHead;

	Unlock();
	return B_OK;
}


status_t
EHCI::FillQueueWithRequest(Transfer *transfer, ehci_qh *queueHead,
	ehci_qtd **_dataDescriptor, bool *_directionIn)
{
	Pipe *pipe = transfer->TransferPipe();
	usb_request_data *requestData = transfer->RequestData();
	bool directionIn = (requestData->RequestType & USB_REQTYPE_DEVICE_IN) > 0;

	ehci_qtd *setupDescriptor = CreateDescriptor(sizeof(usb_request_data),
		EHCI_QTD_PID_SETUP);
	ehci_qtd *statusDescriptor = CreateDescriptor(0,
		directionIn ? EHCI_QTD_PID_OUT : EHCI_QTD_PID_IN);

	if (!setupDescriptor || !statusDescriptor) {
		TRACE_ERROR(("usb_ehci: failed to allocate descriptors\n"));
		FreeDescriptor(setupDescriptor);
		FreeDescriptor(statusDescriptor);
		return B_NO_MEMORY;
	}

	iovec vector;
	vector.iov_base = requestData;
	vector.iov_len = sizeof(usb_request_data);
	WriteDescriptorChain(setupDescriptor, &vector, 1);

	ehci_qtd *strayDescriptor = (ehci_qtd *)queueHead->stray_log;
	statusDescriptor->token |= EHCI_QTD_IOC | EHCI_QTD_DATA_TOGGLE;

	ehci_qtd *dataDescriptor = NULL;
	if (transfer->VectorCount() > 0) {
		ehci_qtd *lastDescriptor = NULL;
		status_t result = CreateDescriptorChain(pipe, &dataDescriptor,
			&lastDescriptor, strayDescriptor, transfer->VectorLength(),
			directionIn ? EHCI_QTD_PID_IN : EHCI_QTD_PID_OUT);

		if (result < B_OK) {
			FreeDescriptor(setupDescriptor);
			FreeDescriptor(statusDescriptor);
			return result;
		}

		if (!directionIn) {
			WriteDescriptorChain(dataDescriptor, transfer->Vector(),
				transfer->VectorCount());
		}

		LinkDescriptors(setupDescriptor, dataDescriptor, strayDescriptor);
		LinkDescriptors(lastDescriptor, statusDescriptor, strayDescriptor);
	} else {
		// no data: link setup and status descriptors directly
		LinkDescriptors(setupDescriptor, statusDescriptor, strayDescriptor);
	}

	queueHead->element_log = setupDescriptor;
	queueHead->overlay.next_phy = setupDescriptor->this_phy;
	queueHead->overlay.alt_next_phy = EHCI_QTD_TERMINATE;

	*_dataDescriptor = dataDescriptor;
	*_directionIn = directionIn;
	return B_OK;
}


status_t
EHCI::FillQueueWithData(Transfer *transfer, ehci_qh *queueHead,
	ehci_qtd **_dataDescriptor, bool *_directionIn)
{
	Pipe *pipe = transfer->TransferPipe();
	bool directionIn = (pipe->Direction() == Pipe::In);

	ehci_qtd *firstDescriptor = NULL;
	ehci_qtd *lastDescriptor = NULL;
	ehci_qtd *strayDescriptor = (ehci_qtd *)queueHead->stray_log;
	status_t result = CreateDescriptorChain(pipe, &firstDescriptor,
		&lastDescriptor, strayDescriptor, transfer->VectorLength(),
		directionIn ? EHCI_QTD_PID_IN : EHCI_QTD_PID_OUT);

	if (result < B_OK)
		return result;

	lastDescriptor->token |= EHCI_QTD_IOC;
	if (!directionIn) {
		WriteDescriptorChain(firstDescriptor, transfer->Vector(),
			transfer->VectorCount());
	}

	queueHead->element_log = firstDescriptor;
	queueHead->overlay.next_phy = firstDescriptor->this_phy;
	queueHead->overlay.alt_next_phy = EHCI_QTD_TERMINATE;

	*_dataDescriptor = firstDescriptor;
	*_directionIn = directionIn;
	return B_OK;
}


ehci_qtd *
EHCI::CreateDescriptor(size_t bufferSize, uint8 pid)
{
	ehci_qtd *result;
	void *physicalAddress;
	if (fStack->AllocateChunk((void **)&result, &physicalAddress,
		sizeof(ehci_qtd)) < B_OK) {
		TRACE_ERROR(("usb_ehci: failed to allocate a qtd\n"));
		return NULL;
	}

	result->this_phy = (addr_t)physicalAddress;
	result->next_phy = EHCI_QTD_TERMINATE;
	result->next_log = NULL;
	result->alt_next_phy = EHCI_QTD_TERMINATE;
	result->alt_next_log = NULL;
	result->buffer_size = bufferSize;
	result->token = bufferSize << EHCI_QTD_BYTES_SHIFT;
	result->token |= 3 << EHCI_QTD_ERRCOUNT_SHIFT;
	result->token |= pid << EHCI_QTD_PID_SHIFT;
	result->token |= EHCI_QTD_STATUS_ACTIVE;
	if (bufferSize == 0) {
		result->buffer_log = NULL;
		for (int32 i = 0; i < 5; i++) {
			result->buffer_phy[i] = 0;
			result->ext_buffer_phy[i] = 0;
		}

		return result;
	}

	if (fStack->AllocateChunk(&result->buffer_log, &physicalAddress,
		bufferSize) < B_OK) {
		TRACE_ERROR(("usb_ehci: unable to allocate qtd buffer\n"));
		fStack->FreeChunk(result, (void *)result->this_phy, sizeof(ehci_qtd));
		return NULL;
	}

	addr_t physicalBase = (addr_t)physicalAddress;
	result->buffer_phy[0] = physicalBase;
	result->ext_buffer_phy[0] = 0;
	for (int32 i = 1; i < 5; i++) {
		physicalBase += B_PAGE_SIZE;
		result->buffer_phy[i] = physicalBase & EHCI_QTD_PAGE_MASK;
		result->ext_buffer_phy[i] = 0;
	}

	return result;
}


status_t
EHCI::CreateDescriptorChain(Pipe *pipe, ehci_qtd **_firstDescriptor,
	ehci_qtd **_lastDescriptor, ehci_qtd *strayDescriptor, size_t bufferSize,
	uint8 pid)
{
	size_t packetSize = B_PAGE_SIZE * 4;
	int32 descriptorCount = (bufferSize + packetSize - 1) / packetSize;

	bool dataToggle = pipe->DataToggle();
	ehci_qtd *firstDescriptor = NULL;
	ehci_qtd *lastDescriptor = *_firstDescriptor;
	for (int32 i = 0; i < descriptorCount; i++) {
		ehci_qtd *descriptor = CreateDescriptor(min_c(packetSize, bufferSize),
			pid);

		if (!descriptor) {
			FreeDescriptorChain(firstDescriptor);
			return B_NO_MEMORY;
		}

		if (dataToggle)
			descriptor->token |= EHCI_QTD_DATA_TOGGLE;

		if (lastDescriptor)
			LinkDescriptors(lastDescriptor, descriptor, strayDescriptor);

		bufferSize -= packetSize;
		lastDescriptor = descriptor;
		if (!firstDescriptor)
			firstDescriptor = descriptor;
	}

	*_firstDescriptor = firstDescriptor;
	*_lastDescriptor = lastDescriptor;
	return B_OK;
}


void
EHCI::FreeDescriptor(ehci_qtd *descriptor)
{
	if (!descriptor)
		return;

	if (descriptor->buffer_log) {
		fStack->FreeChunk(descriptor->buffer_log,
			(void *)descriptor->buffer_phy[0], descriptor->buffer_size);
	}

	fStack->FreeChunk(descriptor, (void *)descriptor->this_phy, sizeof(ehci_qtd));
}


void
EHCI::FreeDescriptorChain(ehci_qtd *topDescriptor)
{
	ehci_qtd *current = topDescriptor;
	ehci_qtd *next = NULL;

	while (current) {
		next = (ehci_qtd *)current->next_log;
		FreeDescriptor(current);
		current = next;
	}
}


void
EHCI::LinkDescriptors(ehci_qtd *first, ehci_qtd *last, ehci_qtd *alt)
{
	first->next_phy = last->this_phy;
	first->next_log = last;

	if (alt) {
		first->alt_next_phy = alt->this_phy;
		first->alt_next_log = alt;
	} else {
		first->alt_next_phy = EHCI_QTD_TERMINATE;
		first->alt_next_log = NULL;
	}
}


size_t
EHCI::WriteDescriptorChain(ehci_qtd *topDescriptor, iovec *vector,
	size_t vectorCount)
{
	ehci_qtd *current = topDescriptor;
	size_t actualLength = 0;
	size_t vectorIndex = 0;
	size_t vectorOffset = 0;
	size_t bufferOffset = 0;

	while (current) {
		if (!current->buffer_log)
			break;

		while (true) {
			size_t length = min_c(current->buffer_size - bufferOffset,
				vector[vectorIndex].iov_len - vectorOffset);

			memcpy((uint8 *)current->buffer_log + bufferOffset,
				(uint8 *)vector[vectorIndex].iov_base + vectorOffset, length);

			actualLength += length;
			vectorOffset += length;
			bufferOffset += length;

			if (vectorOffset >= vector[vectorIndex].iov_len) {
				if (++vectorIndex >= vectorCount) {
					TRACE(("usb_ehci: wrote descriptor chain (%ld bytes, no more vectors)\n", actualLength));
					return actualLength;
				}

				vectorOffset = 0;
			}

			if (bufferOffset >= current->buffer_size) {
				bufferOffset = 0;
				break;
			}
		}

		if (current->next_phy & EHCI_QTD_TERMINATE)
			break;

		current = (ehci_qtd *)current->next_log;
	}

	TRACE(("usb_ehci: wrote descriptor chain (%ld bytes)\n", actualLength));
	return actualLength;
}


size_t
EHCI::ReadDescriptorChain(ehci_qtd *topDescriptor, iovec *vector,
	size_t vectorCount, bool *nextDataToggle)
{
	uint32 dataToggle = 0;
	ehci_qtd *current = topDescriptor;
	size_t actualLength = 0;
	size_t vectorIndex = 0;
	size_t vectorOffset = 0;
	size_t bufferOffset = 0;

	while (current && (current->token & EHCI_QTD_STATUS_ACTIVE) == 0) {
		if (!current->buffer_log)
			break;

		dataToggle = current->token & EHCI_QTD_DATA_TOGGLE;
		size_t bufferSize = current->buffer_size;
		bufferSize -= (current->token >> EHCI_QTD_BYTES_SHIFT) & EHCI_QTD_BYTES_MASK;

		while (true) {
			size_t length = min_c(bufferSize - bufferOffset,
				vector[vectorIndex].iov_len - vectorOffset);

			memcpy((uint8 *)vector[vectorIndex].iov_base + vectorOffset,
				(uint8 *)current->buffer_log + bufferOffset, length);

			actualLength += length;
			vectorOffset += length;
			bufferOffset += length;

			if (vectorOffset >= vector[vectorIndex].iov_len) {
				if (++vectorIndex >= vectorCount) {
					TRACE(("usb_ehci: read descriptor chain (%ld bytes, no more vectors)\n", actualLength));
					*nextDataToggle = dataToggle > 0 ? true : false;
					return actualLength;
				}

				vectorOffset = 0;
			}

			if (bufferOffset >= bufferSize) {
				bufferOffset = 0;
				break;
			}
		}

		if (current->next_phy & EHCI_QTD_TERMINATE)
			break;

		current = (ehci_qtd *)current->next_log;
	}

	TRACE(("usb_ehci: read descriptor chain (%ld bytes)\n", actualLength));
	*nextDataToggle = dataToggle > 0 ? true : false;
	return actualLength;
}


size_t
EHCI::ReadActualLength(ehci_qtd *topDescriptor, bool *nextDataToggle)
{
	size_t actualLength = 0;
	ehci_qtd *current = topDescriptor;
	uint32 dataToggle = 0;

	while (current && (current->token & EHCI_QTD_STATUS_ACTIVE) == 0) {
		dataToggle = current->token & EHCI_QTD_DATA_TOGGLE;
		size_t length = current->buffer_size;
		length -= (current->token >> EHCI_QTD_BYTES_SHIFT) & EHCI_QTD_BYTES_MASK;
		actualLength += length;

		if (current->next_phy & EHCI_QTD_TERMINATE)
			break;

		current = (ehci_qtd *)current->next_log;
	}

	TRACE(("usb_ehci: read actual length (%ld bytes)\n", actualLength));
	*nextDataToggle = dataToggle > 0 ? true : false;
	return actualLength;
}


inline void
EHCI::WriteOpReg(uint32 reg, uint32 value)
{
	*(volatile uint32 *)(fOperationalRegisters + reg) = value;
}


inline uint32
EHCI::ReadOpReg(uint32 reg)
{
	return *(volatile uint32 *)(fOperationalRegisters + reg);
}


inline uint8
EHCI::ReadCapReg8(uint32 reg)
{
	return *(volatile uint8 *)(fCapabilityRegisters + reg);
}


inline uint16
EHCI::ReadCapReg16(uint32 reg)
{
	return *(volatile uint16 *)(fCapabilityRegisters + reg);
}


inline uint32
EHCI::ReadCapReg32(uint32 reg)
{
	return *(volatile uint32 *)(fCapabilityRegisters + reg);
}
