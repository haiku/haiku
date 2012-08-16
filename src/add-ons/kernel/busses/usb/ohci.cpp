/*
 * Copyright 2005-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jan-Rixt Van Hoye
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <module.h>
#include <PCI.h>
#include <PCI_x86.h>
#include <USB3.h>
#include <KernelExport.h>
#include <util/AutoLock.h>

#include "ohci.h"

#define USB_MODULE_NAME "ohci"

pci_module_info *OHCI::sPCIModule = NULL;
pci_x86_module_info *OHCI::sPCIx86Module = NULL;


static int32
ohci_std_ops(int32 op, ...)
{
	switch (op)	{
		case B_MODULE_INIT:
			TRACE_MODULE("init module\n");
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE_MODULE("uninit module\n");
			return B_OK;
	}

	return EINVAL;
}


usb_host_controller_info ohci_module = {
	{
		"busses/usb/ohci",
		0,
		ohci_std_ops
	},
	NULL,
	OHCI::AddTo
};


module_info *modules[] = {
	(module_info *)&ohci_module,
	NULL
};


OHCI::OHCI(pci_info *info, Stack *stack)
	:	BusManager(stack),
		fPCIInfo(info),
		fStack(stack),
		fOperationalRegisters(NULL),
		fRegisterArea(-1),
		fHccaArea(-1),
		fHcca(NULL),
		fInterruptEndpoints(NULL),
		fDummyControl(NULL),
		fDummyBulk(NULL),
		fDummyIsochronous(NULL),
		fFirstTransfer(NULL),
		fLastTransfer(NULL),
		fFinishTransfersSem(-1),
		fFinishThread(-1),
		fStopFinishThread(false),
		fProcessingPipe(NULL),
		fRootHub(NULL),
		fRootHubAddress(0),
		fPortCount(0)
{
	if (!fInitOK) {
		TRACE_ERROR("bus manager failed to init\n");
		return;
	}

	TRACE("constructing new OHCI host controller driver\n");
	fInitOK = false;

	mutex_init(&fEndpointLock, "ohci endpoint lock");

	// enable busmaster and memory mapped access
	uint16 command = sPCIModule->read_pci_config(fPCIInfo->bus,
		fPCIInfo->device, fPCIInfo->function, PCI_command, 2);
	command &= ~PCI_command_io;
	command |= PCI_command_master | PCI_command_memory;

	sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2, command);

	// map the registers
	uint32 offset = sPCIModule->read_pci_config(fPCIInfo->bus,
		fPCIInfo->device, fPCIInfo->function, PCI_base_registers, 4);
	offset &= PCI_address_memory_32_mask;
	TRACE_ALWAYS("iospace offset: 0x%" B_PRIx32 "\n", offset);
	fRegisterArea = map_physical_memory("OHCI memory mapped registers",
		offset,	B_PAGE_SIZE, B_ANY_KERNEL_BLOCK_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_READ_AREA | B_WRITE_AREA,
		(void **)&fOperationalRegisters);
	if (fRegisterArea < B_OK) {
		TRACE_ERROR("failed to map register memory\n");
		return;
	}

	TRACE("mapped operational registers: %p\n", fOperationalRegisters);

	// Check the revision of the controller, which should be 10h
	uint32 revision = _ReadReg(OHCI_REVISION) & 0xff;
	TRACE("version %ld.%ld%s\n", OHCI_REVISION_HIGH(revision),
		OHCI_REVISION_LOW(revision), OHCI_REVISION_LEGACY(revision)
		? ", legacy support" : "");
	if (OHCI_REVISION_HIGH(revision) != 1 || OHCI_REVISION_LOW(revision) != 0) {
		TRACE_ERROR("unsupported OHCI revision\n");
		return;
	}

	phys_addr_t hccaPhysicalAddress;
	fHccaArea = fStack->AllocateArea((void **)&fHcca, &hccaPhysicalAddress,
		sizeof(ohci_hcca), "USB OHCI Host Controller Communication Area");

	if (fHccaArea < B_OK) {
		TRACE_ERROR("unable to create the HCCA block area\n");
		return;
	}

	memset(fHcca, 0, sizeof(ohci_hcca));

	// Set Up Host controller
	// Dummy endpoints
	fDummyControl = _AllocateEndpoint();
	if (!fDummyControl)
		return;

	fDummyBulk = _AllocateEndpoint();
	if (!fDummyBulk) {
		_FreeEndpoint(fDummyControl);
		return;
	}

	fDummyIsochronous = _AllocateEndpoint();
	if (!fDummyIsochronous) {
		_FreeEndpoint(fDummyControl);
		_FreeEndpoint(fDummyBulk);
		return;
	}

	// Static endpoints that get linked in the HCCA
	fInterruptEndpoints = new(std::nothrow)
		ohci_endpoint_descriptor *[OHCI_STATIC_ENDPOINT_COUNT];
	if (!fInterruptEndpoints) {
		TRACE_ERROR("failed to allocate memory for interrupt endpoints\n");
		_FreeEndpoint(fDummyControl);
		_FreeEndpoint(fDummyBulk);
		_FreeEndpoint(fDummyIsochronous);
		return;
	}

	for (int32 i = 0; i < OHCI_STATIC_ENDPOINT_COUNT; i++) {
		fInterruptEndpoints[i] = _AllocateEndpoint();
		if (!fInterruptEndpoints[i]) {
			TRACE_ERROR("failed to allocate interrupt endpoint %" B_PRId32 "\n",
				i);
			while (--i >= 0)
				_FreeEndpoint(fInterruptEndpoints[i]);
			_FreeEndpoint(fDummyBulk);
			_FreeEndpoint(fDummyControl);
			_FreeEndpoint(fDummyIsochronous);
			return;
		}
	}

	// build flat tree so that at each of the static interrupt endpoints
	// fInterruptEndpoints[i] == interrupt endpoint for interval 2^i
	uint32 interval = OHCI_BIGGEST_INTERVAL;
	uint32 intervalIndex = OHCI_STATIC_ENDPOINT_COUNT - 1;
	while (interval > 1) {
		uint32 insertIndex = interval / 2;
		while (insertIndex < OHCI_BIGGEST_INTERVAL) {
			fHcca->interrupt_table[insertIndex]
				= fInterruptEndpoints[intervalIndex]->physical_address;
			insertIndex += interval;
		}

		intervalIndex--;
		interval /= 2;
	}

	// setup the empty slot in the list and linking of all -> first
	fHcca->interrupt_table[0] = fInterruptEndpoints[0]->physical_address;
	for (int32 i = 1; i < OHCI_STATIC_ENDPOINT_COUNT; i++) {
		fInterruptEndpoints[i]->next_physical_endpoint
			= fInterruptEndpoints[0]->physical_address;
		fInterruptEndpoints[i]->next_logical_endpoint
			= fInterruptEndpoints[0];
	}

	// Now link the first endpoint to the isochronous endpoint
	fInterruptEndpoints[0]->next_physical_endpoint
		= fDummyIsochronous->physical_address;

	// Disable all interrupts before handoff/reset
	_WriteReg(OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTERRUPTS);

	// Determine in what context we are running (Kindly copied from FreeBSD)
	uint32 control = _ReadReg(OHCI_CONTROL);
	if (control & OHCI_INTERRUPT_ROUTING) {
		TRACE_ALWAYS("smm is in control of the host controller\n");
		uint32 status = _ReadReg(OHCI_COMMAND_STATUS);
		_WriteReg(OHCI_COMMAND_STATUS, status | OHCI_OWNERSHIP_CHANGE_REQUEST);
		for (uint32 i = 0; i < 100 && (control & OHCI_INTERRUPT_ROUTING); i++) {
			snooze(1000);
			control = _ReadReg(OHCI_CONTROL);
		}

		if ((control & OHCI_INTERRUPT_ROUTING) != 0) {
			TRACE_ERROR("smm does not respond. resetting...\n");
			_WriteReg(OHCI_CONTROL, OHCI_HC_FUNCTIONAL_STATE_RESET);
			snooze(USB_DELAY_BUS_RESET);
		} else
			TRACE_ALWAYS("ownership change successful\n");
	} else {
		TRACE("cold started\n");
		snooze(USB_DELAY_BUS_RESET);
	}

	// This reset should not be necessary according to the OHCI spec, but
	// without it some controllers do not start.
	_WriteReg(OHCI_CONTROL, OHCI_HC_FUNCTIONAL_STATE_RESET);
	snooze(USB_DELAY_BUS_RESET);

	// We now own the host controller and the bus has been reset
	uint32 frameInterval = _ReadReg(OHCI_FRAME_INTERVAL);
	uint32 intervalValue = OHCI_GET_INTERVAL_VALUE(frameInterval);

	_WriteReg(OHCI_COMMAND_STATUS, OHCI_HOST_CONTROLLER_RESET);
	// Nominal time for a reset is 10 us
	uint32 reset = 0;
	for (uint32 i = 0; i < 10; i++) {
		spin(10);
		reset = _ReadReg(OHCI_COMMAND_STATUS) & OHCI_HOST_CONTROLLER_RESET;
		if (reset == 0)
			break;
	}

	if (reset) {
		TRACE_ERROR("error resetting the host controller (timeout)\n");
		return;
	}

	// The controller is now in SUSPEND state, we have 2ms to go OPERATIONAL.

	// Set up host controller register
	_WriteReg(OHCI_HCCA, (uint32)hccaPhysicalAddress);
	_WriteReg(OHCI_CONTROL_HEAD_ED, (uint32)fDummyControl->physical_address);
	_WriteReg(OHCI_BULK_HEAD_ED, (uint32)fDummyBulk->physical_address);
	// Switch on desired functional features
	control = _ReadReg(OHCI_CONTROL);
	control &= ~(OHCI_CONTROL_BULK_SERVICE_RATIO_MASK | OHCI_ENABLE_LIST
		| OHCI_HC_FUNCTIONAL_STATE_MASK | OHCI_INTERRUPT_ROUTING);
	control |= OHCI_ENABLE_LIST | OHCI_CONTROL_BULK_RATIO_1_4
		| OHCI_HC_FUNCTIONAL_STATE_OPERATIONAL;
	// And finally start the controller
	_WriteReg(OHCI_CONTROL, control);

	// The controller is now OPERATIONAL.
	frameInterval = (_ReadReg(OHCI_FRAME_INTERVAL) & OHCI_FRAME_INTERVAL_TOGGLE)
		^ OHCI_FRAME_INTERVAL_TOGGLE;
	frameInterval |= OHCI_FSMPS(intervalValue) | intervalValue;
	_WriteReg(OHCI_FRAME_INTERVAL, frameInterval);
	// 90% periodic
	uint32 periodic = OHCI_PERIODIC(intervalValue);
	_WriteReg(OHCI_PERIODIC_START, periodic);

	// Fiddle the No Over Current Protection bit to avoid chip bug
	uint32 desca = _ReadReg(OHCI_RH_DESCRIPTOR_A);
	_WriteReg(OHCI_RH_DESCRIPTOR_A, desca | OHCI_RH_NO_OVER_CURRENT_PROTECTION);
	_WriteReg(OHCI_RH_STATUS, OHCI_RH_LOCAL_POWER_STATUS_CHANGE);
	snooze(OHCI_ENABLE_POWER_DELAY);
	_WriteReg(OHCI_RH_DESCRIPTOR_A, desca);

	// The AMD756 requires a delay before re-reading the register,
	// otherwise it will occasionally report 0 ports.
	uint32 numberOfPorts = 0;
	for (uint32 i = 0; i < 10 && numberOfPorts == 0; i++) {
		snooze(OHCI_READ_DESC_DELAY);
		uint32 descriptor = _ReadReg(OHCI_RH_DESCRIPTOR_A);
		numberOfPorts = OHCI_RH_GET_PORT_COUNT(descriptor);
	}
	if (numberOfPorts > OHCI_MAX_PORT_COUNT)
		numberOfPorts = OHCI_MAX_PORT_COUNT;
	fPortCount = numberOfPorts;
	TRACE("port count is %d\n", fPortCount);

	// Create semaphore the finisher thread will wait for
	fFinishTransfersSem = create_sem(0, "OHCI Finish Transfers");
	if (fFinishTransfersSem < B_OK) {
		TRACE_ERROR("failed to create semaphore\n");
		return;
	}

	// Create the finisher service thread
	fFinishThread = spawn_kernel_thread(_FinishThread, "ohci finish thread",
		B_URGENT_DISPLAY_PRIORITY, (void *)this);
	resume_thread(fFinishThread);

	// Find the right interrupt vector, using MSIs if available.
	uint8 interruptVector = fPCIInfo->u.h0.interrupt_line;
	if (sPCIx86Module != NULL && sPCIx86Module->get_msi_count(fPCIInfo->bus,
			fPCIInfo->device, fPCIInfo->function) >= 1) {
		uint8 msiVector = 0;
		if (sPCIx86Module->configure_msi(fPCIInfo->bus, fPCIInfo->device,
				fPCIInfo->function, 1, &msiVector) == B_OK
			&& sPCIx86Module->enable_msi(fPCIInfo->bus, fPCIInfo->device,
				fPCIInfo->function) == B_OK) {
			TRACE_ALWAYS("using message signaled interrupts\n");
			interruptVector = msiVector;
		}
	}

	// Install the interrupt handler
	TRACE("installing interrupt handler\n");
	install_io_interrupt_handler(interruptVector, _InterruptHandler,
		(void *)this, 0);

	// Enable interesting interrupts now that the handler is in place
	_WriteReg(OHCI_INTERRUPT_ENABLE, OHCI_NORMAL_INTERRUPTS
		| OHCI_MASTER_INTERRUPT_ENABLE);

	TRACE("OHCI host controller driver constructed\n");
	fInitOK = true;
}


OHCI::~OHCI()
{
	int32 result = 0;
	fStopFinishThread = true;
	delete_sem(fFinishTransfersSem);
	wait_for_thread(fFinishThread, &result);

	_LockEndpoints();
	mutex_destroy(&fEndpointLock);

	if (fHccaArea >= B_OK)
		delete_area(fHccaArea);
	if (fRegisterArea >= B_OK)
		delete_area(fRegisterArea);

	_FreeEndpoint(fDummyControl);
	_FreeEndpoint(fDummyBulk);
	_FreeEndpoint(fDummyIsochronous);

	if (fInterruptEndpoints != NULL) {
		for (int i = 0; i < OHCI_STATIC_ENDPOINT_COUNT; i++)
			_FreeEndpoint(fInterruptEndpoints[i]);
	}

	delete [] fInterruptEndpoints;
	delete fRootHub;

	put_module(B_PCI_MODULE_NAME);
}


status_t
OHCI::Start()
{
	TRACE("starting OHCI host controller\n");

	uint32 control = _ReadReg(OHCI_CONTROL);
	if ((control & OHCI_HC_FUNCTIONAL_STATE_MASK)
		!= OHCI_HC_FUNCTIONAL_STATE_OPERATIONAL) {
		TRACE_ERROR("controller not started (0x%08" B_PRIx32 ")!\n", control);
		return B_ERROR;
	} else
		TRACE("controller is operational!\n");

	fRootHubAddress = AllocateAddress();
	fRootHub = new(std::nothrow) OHCIRootHub(RootObject(), fRootHubAddress);
	if (!fRootHub) {
		TRACE_ERROR("no memory to allocate root hub\n");
		return B_NO_MEMORY;
	}

	if (fRootHub->InitCheck() < B_OK) {
		TRACE_ERROR("root hub failed init check\n");
		return B_ERROR;
	}

	SetRootHub(fRootHub);
	TRACE_ALWAYS("successfully started the controller\n");
	return BusManager::Start();
}


status_t
OHCI::SubmitTransfer(Transfer *transfer)
{
	// short circuit the root hub
	if (transfer->TransferPipe()->DeviceAddress() == fRootHubAddress)
		return fRootHub->ProcessTransfer(this, transfer);

	uint32 type = transfer->TransferPipe()->Type();
	if (type & USB_OBJECT_CONTROL_PIPE) {
		TRACE("submitting request\n");
		return _SubmitRequest(transfer);
	}

	if ((type & USB_OBJECT_BULK_PIPE) || (type & USB_OBJECT_INTERRUPT_PIPE)) {
		TRACE("submitting %s transfer\n",
			(type & USB_OBJECT_BULK_PIPE) ? "bulk" : "interrupt");
		return _SubmitTransfer(transfer);
	}

	if (type & USB_OBJECT_ISO_PIPE) {
		TRACE("submitting isochronous transfer\n");
		return _SubmitIsochronousTransfer(transfer);
	}

	TRACE_ERROR("tried to submit transfer for unknown pipe type %" B_PRIu32 "\n",
		type);
	return B_ERROR;
}


status_t
OHCI::CancelQueuedTransfers(Pipe *pipe, bool force)
{
	if (pipe->Type() & USB_OBJECT_ISO_PIPE)
		return _CancelQueuedIsochronousTransfers(pipe, force);

	if (!Lock())
		return B_ERROR;

	struct transfer_entry {
		Transfer *			transfer;
		transfer_entry *	next;
	};

	transfer_entry *list = NULL;
	transfer_data *current = fFirstTransfer;
	while (current) {
		if (current->transfer && current->transfer->TransferPipe() == pipe) {
			// Check if the skip bit is already set
			if (!(current->endpoint->flags & OHCI_ENDPOINT_SKIP)) {
				current->endpoint->flags |= OHCI_ENDPOINT_SKIP;
				// In case the controller is processing
				// this endpoint, wait for it to finish
				snooze(1000);
			}

			// Clear the endpoint
			current->endpoint->head_physical_descriptor
				= current->endpoint->tail_physical_descriptor;

			if (!force) {
				// If the transfer is canceled by force, the one causing the
				// cancel is probably not the one who initiated the transfer
				// and the callback is likely not safe anymore
				transfer_entry *entry
					= (transfer_entry *)malloc(sizeof(transfer_entry));
				if (entry != NULL) {
					entry->transfer = current->transfer;
					current->transfer = NULL;
					entry->next = list;
					list = entry;
				}
			}
			current->canceled = true;
		}
		current = current->link;
	}

	Unlock();

	while (list != NULL) {
		transfer_entry *next = list->next;
		list->transfer->Finished(B_CANCELED, 0);
		delete list->transfer;
		free(list);
		list = next;
	}

	// wait for any transfers that might have made it before canceling
	while (fProcessingPipe == pipe)
		snooze(1000);

	// notify the finisher so it can clean up the canceled transfers
	release_sem_etc(fFinishTransfersSem, 1, B_DO_NOT_RESCHEDULE);
	return B_OK;
}


status_t
OHCI::NotifyPipeChange(Pipe *pipe, usb_change change)
{
	TRACE("pipe change %d for pipe %p\n", change, pipe);
	if (pipe->DeviceAddress() == fRootHubAddress) {
		// no need to insert/remove endpoint descriptors for the root hub
		return B_OK;
	}

	switch (change) {
		case USB_CHANGE_CREATED:
			return _InsertEndpointForPipe(pipe);

		case USB_CHANGE_DESTROYED:
			return _RemoveEndpointForPipe(pipe);

		case USB_CHANGE_PIPE_POLICY_CHANGED:
			TRACE("pipe policy changing unhandled!\n");
			break;

		default:
			TRACE_ERROR("unknown pipe change!\n");
			return B_ERROR;
	}

	return B_OK;
}


status_t
OHCI::AddTo(Stack *stack)
{
#ifdef TRACE_USB
	set_dprintf_enabled(true);
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	load_driver_symbols("ohci");
#endif
#endif

	if (!sPCIModule) {
		status_t status = get_module(B_PCI_MODULE_NAME, (module_info **)&sPCIModule);
		if (status < B_OK) {
			TRACE_MODULE_ERROR("getting pci module failed! 0x%08" B_PRIx32 "\n",
				status);
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

	// Try to get the PCI x86 module as well so we can enable possible MSIs.
	if (sPCIx86Module == NULL && get_module(B_PCI_X86_MODULE_NAME,
			(module_info **)&sPCIx86Module) != B_OK) {
		// If it isn't there, that's not critical though.
		TRACE_MODULE_ERROR("failed to get pci x86 module\n");
		sPCIx86Module = NULL;
	}

	for (uint32 i = 0 ; sPCIModule->get_nth_pci_info(i, item) >= B_OK; i++) {
		if (item->class_base == PCI_serial_bus && item->class_sub == PCI_usb
			&& item->class_api == PCI_usb_ohci) {
			if (item->u.h0.interrupt_line == 0
				|| item->u.h0.interrupt_line == 0xFF) {
				TRACE_MODULE_ERROR("found device with invalid IRQ -"
					" check IRQ assignement\n");
				continue;
			}

			TRACE_MODULE("found device at IRQ %u\n",
				item->u.h0.interrupt_line);
			OHCI *bus = new(std::nothrow) OHCI(item, stack);
			if (!bus) {
				delete item;
				sPCIModule = NULL;
				put_module(B_PCI_MODULE_NAME);

				if (sPCIx86Module != NULL) {
					sPCIx86Module = NULL;
					put_module(B_PCI_X86_MODULE_NAME);
				}

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

		if (sPCIx86Module != NULL) {
			sPCIx86Module = NULL;
			put_module(B_PCI_X86_MODULE_NAME);
		}

		return ENODEV;
	}

	delete item;
	return B_OK;
}


status_t
OHCI::GetPortStatus(uint8 index, usb_port_status *status)
{
	if (index >= fPortCount) {
		TRACE_ERROR("get port status for invalid port %u\n", index);
		return B_BAD_INDEX;
	}

	status->status = status->change = 0;
	uint32 portStatus = _ReadReg(OHCI_RH_PORT_STATUS(index));

	// status
	if (portStatus & OHCI_RH_PORTSTATUS_CCS)
		status->status |= PORT_STATUS_CONNECTION;
	if (portStatus & OHCI_RH_PORTSTATUS_PES)
		status->status |= PORT_STATUS_ENABLE;
	if (portStatus & OHCI_RH_PORTSTATUS_PSS)
		status->status |= PORT_STATUS_SUSPEND;
	if (portStatus & OHCI_RH_PORTSTATUS_POCI)
		status->status |= PORT_STATUS_OVER_CURRENT;
	if (portStatus & OHCI_RH_PORTSTATUS_PRS)
		status->status |= PORT_STATUS_RESET;
	if (portStatus & OHCI_RH_PORTSTATUS_PPS)
		status->status |= PORT_STATUS_POWER;
	if (portStatus & OHCI_RH_PORTSTATUS_LSDA)
		status->status |= PORT_STATUS_LOW_SPEED;

	// change
	if (portStatus & OHCI_RH_PORTSTATUS_CSC)
		status->change |= PORT_STATUS_CONNECTION;
	if (portStatus & OHCI_RH_PORTSTATUS_PESC)
		status->change |= PORT_STATUS_ENABLE;
	if (portStatus & OHCI_RH_PORTSTATUS_PSSC)
		status->change |= PORT_STATUS_SUSPEND;
	if (portStatus & OHCI_RH_PORTSTATUS_OCIC)
		status->change |= PORT_STATUS_OVER_CURRENT;
	if (portStatus & OHCI_RH_PORTSTATUS_PRSC)
		status->change |= PORT_STATUS_RESET;

	TRACE("port %u status 0x%04x change 0x%04x\n", index,
		status->status, status->change);
	return B_OK;
}


status_t
OHCI::SetPortFeature(uint8 index, uint16 feature)
{
	TRACE("set port feature index %u feature %u\n", index, feature);
	if (index > fPortCount)
		return B_BAD_INDEX;

	switch (feature) {
		case PORT_ENABLE:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_PES);
			return B_OK;

		case PORT_SUSPEND:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_PSS);
			return B_OK;

		case PORT_RESET:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_PRS);
			return B_OK;

		case PORT_POWER:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_PPS);
			return B_OK;
	}

	return B_BAD_VALUE;
}


status_t
OHCI::ClearPortFeature(uint8 index, uint16 feature)
{
	TRACE("clear port feature index %u feature %u\n", index, feature);
	if (index > fPortCount)
		return B_BAD_INDEX;

	switch (feature) {
		case PORT_ENABLE:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_CCS);
			return B_OK;

		case PORT_SUSPEND:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_POCI);
			return B_OK;

		case PORT_POWER:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_LSDA);
			return B_OK;

		case C_PORT_CONNECTION:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_CSC);
			return B_OK;

		case C_PORT_ENABLE:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_PESC);
			return B_OK;

		case C_PORT_SUSPEND:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_PSSC);
			return B_OK;

		case C_PORT_OVER_CURRENT:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_OCIC);
			return B_OK;

		case C_PORT_RESET:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_PRSC);
			return B_OK;
	}

	return B_BAD_VALUE;
}


int32
OHCI::_InterruptHandler(void *data)
{
	return ((OHCI *)data)->_Interrupt();
}


int32
OHCI::_Interrupt()
{
	static spinlock lock = B_SPINLOCK_INITIALIZER;
	acquire_spinlock(&lock);

	uint32 status = 0;
	uint32 acknowledge = 0;
	bool finishTransfers = false;
	int32 result = B_HANDLED_INTERRUPT;

	// The LSb of done_head is used to inform the HCD that an interrupt
	// condition exists for both the done list and for another event recorded in
	// the HcInterruptStatus register. If done_head is 0, then the interrupt
	// was caused by other than the HccaDoneHead update and the
	// HcInterruptStatus register needs to be accessed to determine that exact
	// interrupt cause. If HccDoneHead is nonzero, then a done list update
	// interrupt is indicated and if the LSb of the Dword is nonzero, then an
	// additional interrupt event is indicated and HcInterruptStatus should be
	// checked to determine its cause.
	uint32 doneHead = fHcca->done_head;
	if (doneHead != 0) {
		status = OHCI_WRITEBACK_DONE_HEAD;
		if (doneHead & OHCI_DONE_INTERRUPTS)
			status |= _ReadReg(OHCI_INTERRUPT_STATUS)
				& _ReadReg(OHCI_INTERRUPT_ENABLE);
	} else {
		status = _ReadReg(OHCI_INTERRUPT_STATUS) & _ReadReg(OHCI_INTERRUPT_ENABLE)
			& ~OHCI_WRITEBACK_DONE_HEAD;
		if (status == 0) {
			// Nothing to be done (PCI shared interrupt)
			release_spinlock(&lock);
			return B_UNHANDLED_INTERRUPT;
		}
	}

	if (status & OHCI_SCHEDULING_OVERRUN) {
		TRACE_MODULE("scheduling overrun occured\n");
		acknowledge |= OHCI_SCHEDULING_OVERRUN;
	}

	if (status & OHCI_WRITEBACK_DONE_HEAD) {
		TRACE_MODULE("transfer descriptors processed\n");
		fHcca->done_head = 0;
		acknowledge |= OHCI_WRITEBACK_DONE_HEAD;
		result = B_INVOKE_SCHEDULER;
		finishTransfers = true;
	}

	if (status & OHCI_RESUME_DETECTED) {
		TRACE_MODULE("resume detected\n");
		acknowledge |= OHCI_RESUME_DETECTED;
	}

	if (status & OHCI_UNRECOVERABLE_ERROR) {
		TRACE_MODULE_ERROR("unrecoverable error - controller halted\n");
		_WriteReg(OHCI_CONTROL, OHCI_HC_FUNCTIONAL_STATE_RESET);
		// TODO: clear all pending transfers, reset and resetup the controller
	}

	if (status & OHCI_ROOT_HUB_STATUS_CHANGE) {
		TRACE_MODULE("root hub status change\n");
		// Disable the interrupt as it will otherwise be retriggered until the
		// port has been reset and the change is cleared explicitly.
		// TODO: renable it once we use status changes instead of polling
		_WriteReg(OHCI_INTERRUPT_DISABLE, OHCI_ROOT_HUB_STATUS_CHANGE);
		acknowledge |= OHCI_ROOT_HUB_STATUS_CHANGE;
	}

	if (acknowledge != 0)
		_WriteReg(OHCI_INTERRUPT_STATUS, acknowledge);

	release_spinlock(&lock);

	if (finishTransfers)
		release_sem_etc(fFinishTransfersSem, 1, B_DO_NOT_RESCHEDULE);

	return result;
}


status_t
OHCI::_AddPendingTransfer(Transfer *transfer,
	ohci_endpoint_descriptor *endpoint, ohci_general_td *firstDescriptor,
	ohci_general_td *dataDescriptor, ohci_general_td *lastDescriptor,
	bool directionIn)
{
	if (!transfer || !endpoint || !lastDescriptor)
		return B_BAD_VALUE;

	transfer_data *data = new(std::nothrow) transfer_data;
	if (!data)
		return B_NO_MEMORY;

	status_t result = transfer->InitKernelAccess();
	if (result < B_OK) {
		delete data;
		return result;
	}

	data->transfer = transfer;
	data->endpoint = endpoint;
	data->incoming = directionIn;
	data->canceled = false;
	data->link = NULL;

	// the current tail will become the first descriptor
	data->first_descriptor = (ohci_general_td *)endpoint->tail_logical_descriptor;

	// the data and first descriptors might be the same
	if (dataDescriptor == firstDescriptor)
		data->data_descriptor = data->first_descriptor;
	else
		data->data_descriptor = dataDescriptor;

	// even the last and the first descriptor might be the same
	if (lastDescriptor == firstDescriptor)
		data->last_descriptor = data->first_descriptor;
	else
		data->last_descriptor = lastDescriptor;

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
OHCI::_CancelQueuedIsochronousTransfers(Pipe *pipe, bool force)
{
	// TODO
	return B_ERROR;
}


int32
OHCI::_FinishThread(void *data)
{
	((OHCI *)data)->_FinishTransfers();
	return B_OK;
}


void
OHCI::_FinishTransfers()
{
	while (!fStopFinishThread) {
		if (acquire_sem(fFinishTransfersSem) < B_OK)
			continue;

		// eat up sems that have been released by multiple interrupts
		int32 semCount = 0;
		get_sem_count(fFinishTransfersSem, &semCount);
		if (semCount > 0)
			acquire_sem_etc(fFinishTransfersSem, semCount, B_RELATIVE_TIMEOUT, 0);

		if (!Lock())
			continue;

		TRACE("finishing transfers (first transfer: %p; last"
			" transfer: %p)\n", fFirstTransfer, fLastTransfer);
		transfer_data *lastTransfer = NULL;
		transfer_data *transfer = fFirstTransfer;
		Unlock();

		while (transfer) {
			bool transferDone = false;
			ohci_general_td *descriptor = transfer->first_descriptor;
			ohci_endpoint_descriptor *endpoint = transfer->endpoint;
			status_t callbackStatus = B_OK;

			MutexLocker endpointLocker(endpoint->lock);

			if ((endpoint->head_physical_descriptor & OHCI_ENDPOINT_HEAD_MASK)
				!= endpoint->tail_physical_descriptor) {
				// there are still active transfers on this endpoint, we need
				// to wait for all of them to complete, otherwise we'd read
				// a potentially bogus data toggle value below
				TRACE("endpoint %p still has active tds\n", endpoint);
				lastTransfer = transfer;
				transfer = transfer->link;
				continue;
			}

			endpointLocker.Unlock();

			while (descriptor && !transfer->canceled) {
				uint32 status = OHCI_TD_GET_CONDITION_CODE(descriptor->flags);
				if (status == OHCI_TD_CONDITION_NOT_ACCESSED) {
					// td is still active
					TRACE("td %p still active\n", descriptor);
					break;
				}

				if (status != OHCI_TD_CONDITION_NO_ERROR) {
					// an error occured, but we must ensure that the td
					// was actually done
					if (endpoint->head_physical_descriptor & OHCI_ENDPOINT_HALTED) {
						// the endpoint is halted, this guaratees us that this
						// descriptor has passed (we don't know if the endpoint
						// was halted because of this td, but we do not need
						// to know, as when it was halted by another td this
						// still ensures that this td was handled before).
						TRACE_ERROR("td error: 0x%08" B_PRIx32 "\n", status);

						switch (status) {
							case OHCI_TD_CONDITION_CRC_ERROR:
							case OHCI_TD_CONDITION_BIT_STUFFING:
							case OHCI_TD_CONDITION_TOGGLE_MISMATCH:
								callbackStatus = B_DEV_CRC_ERROR;
								break;

							case OHCI_TD_CONDITION_STALL:
								callbackStatus = B_DEV_STALLED;
								break;

							case OHCI_TD_CONDITION_NO_RESPONSE:
								callbackStatus = B_TIMED_OUT;
								break;

							case OHCI_TD_CONDITION_PID_CHECK_FAILURE:
								callbackStatus = B_DEV_BAD_PID;
								break;

							case OHCI_TD_CONDITION_UNEXPECTED_PID:
								callbackStatus = B_DEV_UNEXPECTED_PID;
								break;

							case OHCI_TD_CONDITION_DATA_OVERRUN:
								callbackStatus = B_DEV_DATA_OVERRUN;
								break;

							case OHCI_TD_CONDITION_DATA_UNDERRUN:
								callbackStatus = B_DEV_DATA_UNDERRUN;
								break;

							case OHCI_TD_CONDITION_BUFFER_OVERRUN:
								callbackStatus = B_DEV_FIFO_OVERRUN;
								break;

							case OHCI_TD_CONDITION_BUFFER_UNDERRUN:
								callbackStatus = B_DEV_FIFO_UNDERRUN;
								break;

							default:
								callbackStatus = B_ERROR;
								break;
						}

						transferDone = true;
						break;
					} else {
						// an error occured but the endpoint is not halted so
						// the td is in fact still active
						TRACE("td %p active with error\n", descriptor);
						break;
					}
				}

				// the td has completed without an error
				TRACE("td %p done\n", descriptor);

				if (descriptor == transfer->last_descriptor
					|| descriptor->buffer_physical != 0) {
					// this is the last td of the transfer or a short packet
					callbackStatus = B_OK;
					transferDone = true;
					break;
				}

				descriptor
					= (ohci_general_td *)descriptor->next_logical_descriptor;
			}

			if (transfer->canceled) {
				// when a transfer is canceled, all transfers to that endpoint
				// are canceled by setting the head pointer to the tail pointer
				// which causes all of the tds to become "free" (as they are
				// inaccessible and not accessed anymore (as setting the head
				// pointer required disabling the endpoint))
				callbackStatus = B_OK;
				transferDone = true;
			}

			if (!transferDone) {
				lastTransfer = transfer;
				transfer = transfer->link;
				continue;
			}

			// remove the transfer from the list first so we are sure
			// it doesn't get canceled while we still process it
			transfer_data *next = transfer->link;
			if (Lock()) {
				if (lastTransfer)
					lastTransfer->link = transfer->link;

				if (transfer == fFirstTransfer)
					fFirstTransfer = transfer->link;
				if (transfer == fLastTransfer)
					fLastTransfer = lastTransfer;

				// store the currently processing pipe here so we can wait
				// in cancel if we are processing something on the target pipe
				if (!transfer->canceled)
					fProcessingPipe = transfer->transfer->TransferPipe();

				transfer->link = NULL;
				Unlock();
			}

			// break the descriptor chain on the last descriptor
			transfer->last_descriptor->next_logical_descriptor = NULL;
			TRACE("transfer %p done with status 0x%08" B_PRIx32 "\n",
				transfer, callbackStatus);

			// if canceled the callback has already been called
			if (!transfer->canceled) {
				size_t actualLength = 0;
				if (callbackStatus == B_OK) {
					if (transfer->data_descriptor && transfer->incoming) {
						// data to read out
						iovec *vector = transfer->transfer->Vector();
						size_t vectorCount = transfer->transfer->VectorCount();

						transfer->transfer->PrepareKernelAccess();
						actualLength = _ReadDescriptorChain(
							transfer->data_descriptor,
							vector, vectorCount);
					} else if (transfer->data_descriptor) {
						// read the actual length that was sent
						actualLength = _ReadActualLength(
							transfer->data_descriptor);
					}

					// get the last data toggle and store it for next time
					transfer->transfer->TransferPipe()->SetDataToggle(
						(endpoint->head_physical_descriptor
							& OHCI_ENDPOINT_TOGGLE_CARRY) != 0);

					if (transfer->transfer->IsFragmented()) {
						// this transfer may still have data left
						TRACE("advancing fragmented transfer\n");
						transfer->transfer->AdvanceByFragment(actualLength);
						if (transfer->transfer->VectorLength() > 0) {
							TRACE("still %ld bytes left on transfer\n",
								transfer->transfer->VectorLength());
							// TODO actually resubmit the transfer
						}

						// the transfer is done, but we already set the
						// actualLength with AdvanceByFragment()
						actualLength = 0;
					}
				}

				transfer->transfer->Finished(callbackStatus, actualLength);
				fProcessingPipe = NULL;
			}

			if (callbackStatus != B_OK) {
				// remove the transfer and make the head pointer valid again
				// (including clearing the halt state)
				_RemoveTransferFromEndpoint(transfer);
			}

			// free the descriptors
			_FreeDescriptorChain(transfer->first_descriptor);

			delete transfer->transfer;
			delete transfer;
			transfer = next;
		}
	}
}


status_t
OHCI::_SubmitRequest(Transfer *transfer)
{
	usb_request_data *requestData = transfer->RequestData();
	bool directionIn = (requestData->RequestType & USB_REQTYPE_DEVICE_IN) != 0;

	ohci_general_td *setupDescriptor
		= _CreateGeneralDescriptor(sizeof(usb_request_data));
	if (!setupDescriptor) {
		TRACE_ERROR("failed to allocate setup descriptor\n");
		return B_NO_MEMORY;
	}

	setupDescriptor->flags = OHCI_TD_DIRECTION_PID_SETUP
		| OHCI_TD_SET_CONDITION_CODE(OHCI_TD_CONDITION_NOT_ACCESSED)
		| OHCI_TD_TOGGLE_0
		| OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_NONE);

	ohci_general_td *statusDescriptor = _CreateGeneralDescriptor(0);
	if (!statusDescriptor) {
		TRACE_ERROR("failed to allocate status descriptor\n");
		_FreeGeneralDescriptor(setupDescriptor);
		return B_NO_MEMORY;
	}

	statusDescriptor->flags
		= (directionIn ? OHCI_TD_DIRECTION_PID_OUT : OHCI_TD_DIRECTION_PID_IN)
		| OHCI_TD_SET_CONDITION_CODE(OHCI_TD_CONDITION_NOT_ACCESSED)
		| OHCI_TD_TOGGLE_1
		| OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_IMMEDIATE);

	iovec vector;
	vector.iov_base = requestData;
	vector.iov_len = sizeof(usb_request_data);
	_WriteDescriptorChain(setupDescriptor, &vector, 1);

	status_t result;
	ohci_general_td *dataDescriptor = NULL;
	if (transfer->VectorCount() > 0) {
		ohci_general_td *lastDescriptor = NULL;
		result = _CreateDescriptorChain(&dataDescriptor, &lastDescriptor,
			directionIn ? OHCI_TD_DIRECTION_PID_IN : OHCI_TD_DIRECTION_PID_OUT,
			transfer->VectorLength());
		if (result < B_OK) {
			_FreeGeneralDescriptor(setupDescriptor);
			_FreeGeneralDescriptor(statusDescriptor);
			return result;
		}

		if (!directionIn) {
			_WriteDescriptorChain(dataDescriptor, transfer->Vector(),
				transfer->VectorCount());
		}

		_LinkDescriptors(setupDescriptor, dataDescriptor);
		_LinkDescriptors(lastDescriptor, statusDescriptor);
	} else {
		_LinkDescriptors(setupDescriptor, statusDescriptor);
	}

	// Add to the transfer list
	ohci_endpoint_descriptor *endpoint
		= (ohci_endpoint_descriptor *)transfer->TransferPipe()->ControllerCookie();

	MutexLocker endpointLocker(endpoint->lock);
	result = _AddPendingTransfer(transfer, endpoint, setupDescriptor,
		dataDescriptor, statusDescriptor, directionIn);
	if (result < B_OK) {
		TRACE_ERROR("failed to add pending transfer\n");
		_FreeDescriptorChain(setupDescriptor);
		return result;
	}

	// Add the descriptor chain to the endpoint
	_SwitchEndpointTail(endpoint, setupDescriptor, statusDescriptor);
	endpointLocker.Unlock();

	// Tell the controller to process the control list
	endpoint->flags &= ~OHCI_ENDPOINT_SKIP;
	_WriteReg(OHCI_COMMAND_STATUS, OHCI_CONTROL_LIST_FILLED);
	return B_OK;
}


status_t
OHCI::_SubmitTransfer(Transfer *transfer)
{
	Pipe *pipe = transfer->TransferPipe();
	bool directionIn = (pipe->Direction() == Pipe::In);

	ohci_general_td *firstDescriptor = NULL;
	ohci_general_td *lastDescriptor = NULL;
	status_t result = _CreateDescriptorChain(&firstDescriptor, &lastDescriptor,
		directionIn ? OHCI_TD_DIRECTION_PID_IN : OHCI_TD_DIRECTION_PID_OUT,
		transfer->VectorLength());

	if (result < B_OK)
		return result;

	// Apply data toggle to the first descriptor (the others will use the carry)
	firstDescriptor->flags &= ~OHCI_TD_TOGGLE_CARRY;
	firstDescriptor->flags |= pipe->DataToggle() ? OHCI_TD_TOGGLE_1
		: OHCI_TD_TOGGLE_0;

	// Set the last descriptor to generate an interrupt
	lastDescriptor->flags &= ~OHCI_TD_INTERRUPT_MASK;
	lastDescriptor->flags |=
		OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_IMMEDIATE);

	if (!directionIn) {
		_WriteDescriptorChain(firstDescriptor, transfer->Vector(),
			transfer->VectorCount());
	}

	// Add to the transfer list
	ohci_endpoint_descriptor *endpoint
		= (ohci_endpoint_descriptor *)pipe->ControllerCookie();

	MutexLocker endpointLocker(endpoint->lock);
	result = _AddPendingTransfer(transfer, endpoint, firstDescriptor,
		firstDescriptor, lastDescriptor, directionIn);
	if (result < B_OK) {
		TRACE_ERROR("failed to add pending transfer\n");
		_FreeDescriptorChain(firstDescriptor);
		return result;
	}

	// Add the descriptor chain to the endpoint
	_SwitchEndpointTail(endpoint, firstDescriptor, lastDescriptor);
	endpointLocker.Unlock();

	endpoint->flags &= ~OHCI_ENDPOINT_SKIP;
	if (pipe->Type() & USB_OBJECT_BULK_PIPE) {
		// Tell the controller to process the bulk list
		_WriteReg(OHCI_COMMAND_STATUS, OHCI_BULK_LIST_FILLED);
	}

	return B_OK;
}


status_t
OHCI::_SubmitIsochronousTransfer(Transfer *transfer)
{
	TRACE_ERROR("isochronous transfers not implemented\n");
	return B_ERROR;
}


void
OHCI::_SwitchEndpointTail(ohci_endpoint_descriptor *endpoint,
	ohci_general_td *first, ohci_general_td *last)
{
	// fill in the information of the first descriptor into the current tail
	ohci_general_td *tail = (ohci_general_td *)endpoint->tail_logical_descriptor;
	tail->flags = first->flags;
	tail->buffer_physical = first->buffer_physical;
	tail->next_physical_descriptor = first->next_physical_descriptor;
	tail->last_physical_byte_address = first->last_physical_byte_address;
	tail->buffer_size = first->buffer_size;
	tail->buffer_logical = first->buffer_logical;
	tail->next_logical_descriptor = first->next_logical_descriptor;

	// the first descriptor becomes the new tail
	first->flags = 0;
	first->buffer_physical = 0;
	first->next_physical_descriptor = 0;
	first->last_physical_byte_address = 0;
	first->buffer_size = 0;
	first->buffer_logical = NULL;
	first->next_logical_descriptor = NULL;

	if (first == last)
		_LinkDescriptors(tail, first);
	else
		_LinkDescriptors(last, first);

	// update the endpoint tail pointer to reflect the change
	endpoint->tail_logical_descriptor = first;
	endpoint->tail_physical_descriptor = (uint32)first->physical_address;
	TRACE("switched tail from %p to %p\n", tail, first);

#if 0
	_PrintEndpoint(endpoint);
	_PrintDescriptorChain(tail);
#endif
}


void
OHCI::_RemoveTransferFromEndpoint(transfer_data *transfer)
{
	// The transfer failed and the endpoint was halted. This means that the
	// endpoint head pointer might point somewhere into the descriptor chain
	// of this transfer. As we do not know if this transfer actually caused
	// the halt on the endpoint we have to make sure this is the case. If we
	// find the head to point to somewhere into the descriptor chain then
	// simply advancing the head pointer to the link of the last transfer
	// will bring the endpoint into a valid state again. This operation is
	// safe as the endpoint is currently halted and we therefore can change
	// the head pointer.
	ohci_endpoint_descriptor *endpoint = transfer->endpoint;
	ohci_general_td *descriptor = transfer->first_descriptor;
	while (descriptor) {
		if ((endpoint->head_physical_descriptor & OHCI_ENDPOINT_HEAD_MASK)
			== descriptor->physical_address) {
			// This descriptor caused the halt. Advance the head pointer. This
			// will either move the head to the next valid transfer that can
			// then be restarted, or it will move the head to the tail when
			// there are no more transfer descriptors. Setting the head will
			// also clear the halt state as it is stored in the first bit of
			// the head pointer.
			endpoint->head_physical_descriptor
				= transfer->last_descriptor->next_physical_descriptor;
			return;
		}

		descriptor = (ohci_general_td *)descriptor->next_logical_descriptor;
	}
}


ohci_endpoint_descriptor *
OHCI::_AllocateEndpoint()
{
	ohci_endpoint_descriptor *endpoint;
	phys_addr_t physicalAddress;

	mutex *lock = (mutex *)malloc(sizeof(mutex));
	if (lock == NULL) {
		TRACE_ERROR("no memory to allocate endpoint lock\n");
		return NULL;
	}

	// Allocate memory chunk
	if (fStack->AllocateChunk((void **)&endpoint, &physicalAddress,
		sizeof(ohci_endpoint_descriptor)) < B_OK) {
		TRACE_ERROR("failed to allocate endpoint descriptor\n");
		free(lock);
		return NULL;
	}

	mutex_init(lock, "ohci endpoint lock");

	endpoint->flags = OHCI_ENDPOINT_SKIP;
	endpoint->physical_address = (uint32)physicalAddress;
	endpoint->head_physical_descriptor = 0;
	endpoint->tail_logical_descriptor = NULL;
	endpoint->tail_physical_descriptor = 0;
	endpoint->next_logical_endpoint = NULL;
	endpoint->next_physical_endpoint = 0;
	endpoint->lock = lock;
	return endpoint;
}


void
OHCI::_FreeEndpoint(ohci_endpoint_descriptor *endpoint)
{
	if (!endpoint)
		return;

	mutex_destroy(endpoint->lock);
	free(endpoint->lock);

	fStack->FreeChunk((void *)endpoint, endpoint->physical_address,
		sizeof(ohci_endpoint_descriptor));
}


status_t
OHCI::_InsertEndpointForPipe(Pipe *pipe)
{
	TRACE("inserting endpoint for device %u endpoint %u\n",
		pipe->DeviceAddress(), pipe->EndpointAddress());

	ohci_endpoint_descriptor *endpoint = _AllocateEndpoint();
	if (!endpoint) {
		TRACE_ERROR("cannot allocate memory for endpoint\n");
		return B_NO_MEMORY;
	}

	uint32 flags = OHCI_ENDPOINT_SKIP;

	// Set up device and endpoint address
	flags |= OHCI_ENDPOINT_SET_DEVICE_ADDRESS(pipe->DeviceAddress())
		| OHCI_ENDPOINT_SET_ENDPOINT_NUMBER(pipe->EndpointAddress());

	// Set the direction
	switch (pipe->Direction()) {
		case Pipe::In:
			flags |= OHCI_ENDPOINT_DIRECTION_IN;
			break;

		case Pipe::Out:
			flags |= OHCI_ENDPOINT_DIRECTION_OUT;
			break;

		case Pipe::Default:
			flags |= OHCI_ENDPOINT_DIRECTION_DESCRIPTOR;
			break;

		default:
			TRACE_ERROR("direction unknown\n");
			_FreeEndpoint(endpoint);
			return B_ERROR;
	}

	// Set up the speed
	switch (pipe->Speed()) {
		case USB_SPEED_LOWSPEED:
			flags |= OHCI_ENDPOINT_LOW_SPEED;
			break;

		case USB_SPEED_FULLSPEED:
			flags |= OHCI_ENDPOINT_FULL_SPEED;
			break;

		default:
			TRACE_ERROR("unacceptable speed\n");
			_FreeEndpoint(endpoint);
			return B_ERROR;
	}

	// Set the maximum packet size
	flags |= OHCI_ENDPOINT_SET_MAX_PACKET_SIZE(pipe->MaxPacketSize());
	endpoint->flags = flags;

	// Add the endpoint to the appropriate list
	uint32 type = pipe->Type();
	ohci_endpoint_descriptor *head = NULL;
	if (type & USB_OBJECT_CONTROL_PIPE)
		head = fDummyControl;
	else if (type & USB_OBJECT_BULK_PIPE)
		head = fDummyBulk;
	else if (type & USB_OBJECT_INTERRUPT_PIPE)
		head = _FindInterruptEndpoint(pipe->Interval());
	else if (type & USB_OBJECT_ISO_PIPE)
		head = fDummyIsochronous;
	else
		TRACE_ERROR("unknown pipe type\n");

	if (head == NULL) {
		TRACE_ERROR("no list found for endpoint\n");
		_FreeEndpoint(endpoint);
		return B_ERROR;
	}

	// Create (necessary) tail descriptor
	if (pipe->Type() & USB_OBJECT_ISO_PIPE) {
		// Set the isochronous bit format
		endpoint->flags |= OHCI_ENDPOINT_ISOCHRONOUS_FORMAT;
		// TODO
		_FreeEndpoint(endpoint);
		return B_ERROR;
	} else {
		ohci_general_td *tail = _CreateGeneralDescriptor(0);
		tail->flags = 0;
		endpoint->tail_logical_descriptor = tail;
		endpoint->head_physical_descriptor = tail->physical_address;
		endpoint->tail_physical_descriptor = tail->physical_address;
	}

	if (!_LockEndpoints()) {
		if (endpoint->tail_logical_descriptor) {
			_FreeGeneralDescriptor(
				(ohci_general_td *)endpoint->tail_logical_descriptor);
		}

		_FreeEndpoint(endpoint);
		return B_ERROR;
	}

	pipe->SetControllerCookie((void *)endpoint);
	endpoint->next_logical_endpoint = head->next_logical_endpoint;
	endpoint->next_physical_endpoint = head->next_physical_endpoint;
	head->next_logical_endpoint = (void *)endpoint;
	head->next_physical_endpoint = (uint32)endpoint->physical_address;

	_UnlockEndpoints();
	return B_OK;
}


status_t
OHCI::_RemoveEndpointForPipe(Pipe *pipe)
{
	TRACE("removing endpoint for device %u endpoint %u\n",
		pipe->DeviceAddress(), pipe->EndpointAddress());

	ohci_endpoint_descriptor *endpoint
		= (ohci_endpoint_descriptor *)pipe->ControllerCookie();
	if (endpoint == NULL)
		return B_OK;

	// TODO implement properly, but at least disable it for now
	endpoint->flags |= OHCI_ENDPOINT_SKIP;
	return B_OK;
}


ohci_endpoint_descriptor *
OHCI::_FindInterruptEndpoint(uint8 interval)
{
	uint32 index = 0;
	uint32 power = 1;
	while (power <= OHCI_BIGGEST_INTERVAL / 2) {
		if (power * 2 > interval)
			break;

		power *= 2;
		index++;
	}

	return fInterruptEndpoints[index];
}


ohci_general_td *
OHCI::_CreateGeneralDescriptor(size_t bufferSize)
{
	ohci_general_td *descriptor;
	phys_addr_t physicalAddress;

	if (fStack->AllocateChunk((void **)&descriptor, &physicalAddress,
		sizeof(ohci_general_td)) != B_OK) {
		TRACE_ERROR("failed to allocate general descriptor\n");
		return NULL;
	}

	descriptor->physical_address = (uint32)physicalAddress;
	descriptor->next_physical_descriptor = 0;
	descriptor->next_logical_descriptor = NULL;
	descriptor->buffer_size = bufferSize;
	if (bufferSize == 0) {
		descriptor->buffer_physical = 0;
		descriptor->buffer_logical = NULL;
		descriptor->last_physical_byte_address = 0;
		return descriptor;
	}

	if (fStack->AllocateChunk(&descriptor->buffer_logical,
		&physicalAddress, bufferSize) != B_OK) {
		TRACE_ERROR("failed to allocate space for buffer\n");
		fStack->FreeChunk(descriptor, descriptor->physical_address,
			sizeof(ohci_general_td));
		return NULL;
	}
	descriptor->buffer_physical = physicalAddress;

	descriptor->last_physical_byte_address
		= descriptor->buffer_physical + bufferSize - 1;
	return descriptor;
}


void
OHCI::_FreeGeneralDescriptor(ohci_general_td *descriptor)
{
	if (!descriptor)
		return;

	if (descriptor->buffer_logical) {
		fStack->FreeChunk(descriptor->buffer_logical,
			descriptor->buffer_physical, descriptor->buffer_size);
	}

	fStack->FreeChunk((void *)descriptor, descriptor->physical_address,
		sizeof(ohci_general_td));
}


status_t
OHCI::_CreateDescriptorChain(ohci_general_td **_firstDescriptor,
	ohci_general_td **_lastDescriptor, uint32 direction, size_t bufferSize)
{
	size_t blockSize = 8192;
	int32 descriptorCount = (bufferSize + blockSize - 1) / blockSize;
	if (descriptorCount == 0)
		descriptorCount = 1;

	ohci_general_td *firstDescriptor = NULL;
	ohci_general_td *lastDescriptor = *_firstDescriptor;
	for (int32 i = 0; i < descriptorCount; i++) {
		ohci_general_td *descriptor = _CreateGeneralDescriptor(
			min_c(blockSize, bufferSize));

		if (!descriptor) {
			_FreeDescriptorChain(firstDescriptor);
			return B_NO_MEMORY;
		}

		descriptor->flags = direction
			| OHCI_TD_BUFFER_ROUNDING
			| OHCI_TD_SET_CONDITION_CODE(OHCI_TD_CONDITION_NOT_ACCESSED)
			| OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_NONE)
			| OHCI_TD_TOGGLE_CARRY;

		// link to previous
		if (lastDescriptor)
			_LinkDescriptors(lastDescriptor, descriptor);

		bufferSize -= blockSize;
		lastDescriptor = descriptor;
		if (!firstDescriptor)
			firstDescriptor = descriptor;
	}

	*_firstDescriptor = firstDescriptor;
	*_lastDescriptor = lastDescriptor;
	return B_OK;
}


void
OHCI::_FreeDescriptorChain(ohci_general_td *topDescriptor)
{
	ohci_general_td *current = topDescriptor;
	ohci_general_td *next = NULL;

	while (current) {
		next = (ohci_general_td *)current->next_logical_descriptor;
		_FreeGeneralDescriptor(current);
		current = next;
	}
}


size_t
OHCI::_WriteDescriptorChain(ohci_general_td *topDescriptor, iovec *vector,
	size_t vectorCount)
{
	ohci_general_td *current = topDescriptor;
	size_t actualLength = 0;
	size_t vectorIndex = 0;
	size_t vectorOffset = 0;
	size_t bufferOffset = 0;

	while (current) {
		if (!current->buffer_logical)
			break;

		while (true) {
			size_t length = min_c(current->buffer_size - bufferOffset,
				vector[vectorIndex].iov_len - vectorOffset);

			TRACE("copying %ld bytes to bufferOffset %ld from"
				" vectorOffset %ld at index %ld of %ld\n", length, bufferOffset,
				vectorOffset, vectorIndex, vectorCount);
			memcpy((uint8 *)current->buffer_logical + bufferOffset,
				(uint8 *)vector[vectorIndex].iov_base + vectorOffset, length);

			actualLength += length;
			vectorOffset += length;
			bufferOffset += length;

			if (vectorOffset >= vector[vectorIndex].iov_len) {
				if (++vectorIndex >= vectorCount) {
					TRACE("wrote descriptor chain (%ld bytes, no"
						" more vectors)\n", actualLength);
					return actualLength;
				}

				vectorOffset = 0;
			}

			if (bufferOffset >= current->buffer_size) {
				bufferOffset = 0;
				break;
			}
		}

		if (!current->next_logical_descriptor)
			break;

		current = (ohci_general_td *)current->next_logical_descriptor;
	}

	TRACE("wrote descriptor chain (%ld bytes)\n", actualLength);
	return actualLength;
}


size_t
OHCI::_ReadDescriptorChain(ohci_general_td *topDescriptor, iovec *vector,
	size_t vectorCount)
{
	ohci_general_td *current = topDescriptor;
	size_t actualLength = 0;
	size_t vectorIndex = 0;
	size_t vectorOffset = 0;
	size_t bufferOffset = 0;

	while (current && OHCI_TD_GET_CONDITION_CODE(current->flags)
		!= OHCI_TD_CONDITION_NOT_ACCESSED) {
		if (!current->buffer_logical)
			break;

		size_t bufferSize = current->buffer_size;
		if (current->buffer_physical != 0) {
			bufferSize -= current->last_physical_byte_address
				- current->buffer_physical + 1;
		}

		while (true) {
			size_t length = min_c(bufferSize - bufferOffset,
				vector[vectorIndex].iov_len - vectorOffset);

			TRACE("copying %ld bytes to vectorOffset %ld from"
				" bufferOffset %ld at index %ld of %ld\n", length, vectorOffset,
				bufferOffset, vectorIndex, vectorCount);
			memcpy((uint8 *)vector[vectorIndex].iov_base + vectorOffset,
				(uint8 *)current->buffer_logical + bufferOffset, length);

			actualLength += length;
			vectorOffset += length;
			bufferOffset += length;

			if (vectorOffset >= vector[vectorIndex].iov_len) {
				if (++vectorIndex >= vectorCount) {
					TRACE("read descriptor chain (%ld bytes, no more vectors)\n",
						actualLength);
					return actualLength;
				}

				vectorOffset = 0;
			}

			if (bufferOffset >= bufferSize) {
				bufferOffset = 0;
				break;
			}
		}

		current = (ohci_general_td *)current->next_logical_descriptor;
	}

	TRACE("read descriptor chain (%ld bytes)\n", actualLength);
	return actualLength;
}


size_t
OHCI::_ReadActualLength(ohci_general_td *topDescriptor)
{
	ohci_general_td *current = topDescriptor;
	size_t actualLength = 0;

	while (current && OHCI_TD_GET_CONDITION_CODE(current->flags)
		!= OHCI_TD_CONDITION_NOT_ACCESSED) {
		size_t length = current->buffer_size;
		if (current->buffer_physical != 0) {
			length -= current->last_physical_byte_address
				- current->buffer_physical + 1;
		}

		actualLength += length;
		current = (ohci_general_td *)current->next_logical_descriptor;
	}

	TRACE("read actual length (%ld bytes)\n", actualLength);
	return actualLength;
}


void
OHCI::_LinkDescriptors(ohci_general_td *first, ohci_general_td *second)
{
	first->next_physical_descriptor = second->physical_address;
	first->next_logical_descriptor = second;
}


ohci_isochronous_td *
OHCI::_CreateIsochronousDescriptor()
{
	// TODO
	return NULL;
}


void
OHCI::_FreeIsochronousDescriptor(ohci_isochronous_td *descriptor)
{
	// TODO
}


bool
OHCI::_LockEndpoints()
{
	return (mutex_lock(&fEndpointLock) == B_OK);
}


void
OHCI::_UnlockEndpoints()
{
	mutex_unlock(&fEndpointLock);
}


inline void
OHCI::_WriteReg(uint32 reg, uint32 value)
{
	*(volatile uint32 *)(fOperationalRegisters + reg) = value;
}


inline uint32
OHCI::_ReadReg(uint32 reg)
{
	return *(volatile uint32 *)(fOperationalRegisters + reg);
}


void
OHCI::_PrintEndpoint(ohci_endpoint_descriptor *endpoint)
{
	TRACE_ALWAYS("endpoint %p\n", endpoint);
	dprintf("\tflags........... 0x%08" B_PRIx32 "\n", endpoint->flags);
	dprintf("\ttail_physical... 0x%08" B_PRIx32 "\n", endpoint->tail_physical_descriptor);
	dprintf("\thead_physical... 0x%08" B_PRIx32 "\n", endpoint->head_physical_descriptor);
	dprintf("\tnext_physical... 0x%08" B_PRIx32 "\n", endpoint->next_physical_endpoint);
	dprintf("\tphysical........ 0x%08" B_PRIx32 "\n", endpoint->physical_address);
	dprintf("\ttail_logical.... %p\n", endpoint->tail_logical_descriptor);
	dprintf("\tnext_logical.... %p\n", endpoint->next_logical_endpoint);
}


void
OHCI::_PrintDescriptorChain(ohci_general_td *topDescriptor)
{
	while (topDescriptor) {
		TRACE_ALWAYS("descriptor %p\n", topDescriptor);
		dprintf("\tflags........... 0x%08" B_PRIx32 "\n", topDescriptor->flags);
		dprintf("\tbuffer_physical. 0x%08" B_PRIx32 "\n", topDescriptor->buffer_physical);
		dprintf("\tnext_physical... 0x%08" B_PRIx32 "\n", topDescriptor->next_physical_descriptor);
		dprintf("\tlast_byte....... 0x%08" B_PRIx32 "\n", topDescriptor->last_physical_byte_address);
		dprintf("\tphysical........ 0x%08" B_PRIx32 "\n", topDescriptor->physical_address);
		dprintf("\tbuffer_size..... %lu\n", topDescriptor->buffer_size);
		dprintf("\tbuffer_logical.. %p\n", topDescriptor->buffer_logical);
		dprintf("\tnext_logical.... %p\n", topDescriptor->next_logical_descriptor);

		topDescriptor = (ohci_general_td *)topDescriptor->next_logical_descriptor;
	}
}
