/*
 * Copyright 2005-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *             Jan-Rixt Van Hoye
 *             Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */

#include <module.h>
#include <PCI.h>
#include <USB3.h>
#include <KernelExport.h>

#include "ohci.h"

pci_module_info *OHCI::sPCIModule = NULL;


static int32
ohci_std_ops(int32 op, ...)
{
	switch (op)	{
		case B_MODULE_INIT:
			TRACE(("usb_ohci_module: init module\n"));
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE(("usb_ohci_module: uninit module\n"));
			return B_OK;
	}

	return EINVAL;
}


host_controller_info ohci_module = {
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


//------------------------------------------------------
//	OHCI:: 	Reverse the bits in a value between 0 and 31
//			(Section 3.3.2) 
//------------------------------------------------------
static uint8 revbits[OHCI_NUMBER_OF_INTERRUPTS] =
  { 0x00, 0x10, 0x08, 0x18, 0x04, 0x14, 0x0c, 0x1c,
    0x02, 0x12, 0x0a, 0x1a, 0x06, 0x16, 0x0e, 0x1e,
    0x01, 0x11, 0x09, 0x19, 0x05, 0x15, 0x0d, 0x1d,
    0x03, 0x13, 0x0b, 0x1b, 0x07, 0x17, 0x0f, 0x1f };
 

OHCI::OHCI(pci_info *info, Stack *stack)
	:	BusManager(stack),
		fPCIInfo(info),
		fStack(stack),
		fRegisterArea(-1),
		fHccaArea(-1),
		fDummyControl(NULL),
		fDummyBulk(NULL),
		fDummyIsochronous(NULL),
		fFirstTransfer(NULL),
		fFinishTransfer(NULL),
		fFinishThread(-1),
		fStopFinishThread(false),
		fRootHub(NULL),
		fRootHubAddress(0),
		fPortCount(0)
{
	if (!fInitOK) {
		TRACE_ERROR(("usb_ohci: bus manager failed to init\n"));
		return;
	}

	TRACE(("usb_ohci: constructing new OHCI Host Controller Driver\n"));
	fInitOK = false;

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
	TRACE(("usb_ohci: iospace offset: %lx\n", offset));
	fRegisterArea = map_physical_memory("OHCI memory mapped registers",
		(void *)offset,	B_PAGE_SIZE, B_ANY_KERNEL_BLOCK_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_READ_AREA | B_WRITE_AREA,
		(void **)&fOperationalRegisters);
	if (fRegisterArea < B_OK) {
		TRACE_ERROR(("usb_ohci: failed to map register memory\n"));
		return;
	}

	TRACE(("usb_ohci: mapped operational registers: 0x%08lx\n",
		*fOperationalRegisters));

	// Check the revision of the controller, which should be 10h
	uint32 revision = _ReadReg(OHCI_REVISION) & 0xff;
	TRACE(("usb_ohci: version %ld.%ld%s\n", OHCI_REVISION_HIGH(revision),
		OHCI_REVISION_LOW(revision), OHCI_REVISION_LEGACY(revision) 
		? ", legacy support" : ""));
	if (OHCI_REVISION_HIGH(revision) != 1 || OHCI_REVISION_LOW(revision) != 0) {
		TRACE_ERROR(("usb_ohci: unsupported OHCI revision\n"));
		return;
	}

	// Set up the Host Controller Communications Area
	// which is 256 bytes (2048 bits) and must be aligned
	void *hccaPhysicalAddress;
	fHccaArea = fStack->AllocateArea((void **)&fHcca, &hccaPhysicalAddress,
		2048, "USB OHCI Host Controller Communication Area");

	if (fHccaArea < B_OK) {
		TRACE_ERROR(("usb_ohci: unable to create the HCCA block area\n"));
		return;
	}

	memset((void *)fHcca, 0, sizeof(ohci_hcca));

	// Set Up Host controller
	// Dummy endpoints
	fDummyControl = _AllocateEndpoint();
	if (!fDummyControl)
		return;
	fDummyControl->flags |= OHCI_ENDPOINT_SKIP;

	fDummyBulk = _AllocateEndpoint();
	if (!fDummyBulk) {
		_FreeEndpoint(fDummyControl);
		return;
	}
	fDummyBulk->flags |= OHCI_ENDPOINT_SKIP;

	fDummyIsochronous = _AllocateEndpoint();
	if (!fDummyIsochronous) {
		_FreeEndpoint(fDummyControl);
		_FreeEndpoint(fDummyBulk);
		return;
	}
	fDummyIsochronous->flags |= OHCI_ENDPOINT_SKIP;

	// Static endpoints that get linked in the HCCA
	fInterruptEndpoints = new(std::nothrow)
		ohci_endpoint_descriptor *[OHCI_NUMBER_OF_INTERRUPTS];
	if (!fInterruptEndpoints) {
		TRACE_ERROR(("ohci_usb: cannot allocate memory for"
			" fInterruptEndpoints array\n"));
		_FreeEndpoint(fDummyControl);
		_FreeEndpoint(fDummyBulk);
		_FreeEndpoint(fDummyIsochronous);
		return;
	}
	for (uint32 i = 0; i < OHCI_NUMBER_OF_INTERRUPTS; i++) {
		fInterruptEndpoints[i] = _AllocateEndpoint();
		if (!fInterruptEndpoints[i]) {
			TRACE_ERROR(("ohci_usb: cannot allocate memory for"
				" fInterruptEndpoints[%ld] endpoint\n", i));
			while (--i >= 0)
				_FreeEndpoint(fInterruptEndpoints[i]);
			_FreeEndpoint(fDummyBulk);
			_FreeEndpoint(fDummyControl);
			_FreeEndpoint(fDummyIsochronous);
			return;
		}
		// Make them point all to the dummy isochronous endpoint
		fInterruptEndpoints[i]->flags |= OHCI_ENDPOINT_SKIP;
		fInterruptEndpoints[i]->next_physical_endpoint
			= fDummyIsochronous->physical_address;
	}

	// Fill HCCA interrupt table.
	for (uint32 i = 0; i < OHCI_NUMBER_OF_INTERRUPTS; i++)
		fHcca->interrupt_table[i] = fInterruptEndpoints[i]->physical_address;

	// Determine in what context we are running (Kindly copied from FreeBSD)
	uint32 control = _ReadReg(OHCI_CONTROL);
	if (control & OHCI_INTERRUPT_ROUTING) {
		TRACE(("usb_ohci: SMM is in control of the host controller\n"));
		_WriteReg(OHCI_COMMAND_STATUS, OHCI_OWNERSHIP_CHANGE_REQUEST);
		for (uint32 i = 0; i < 100 	&& (control & OHCI_INTERRUPT_ROUTING); i++) {
			snooze(1000);
			control = _ReadReg(OHCI_CONTROL);
		}
		if (!(control & OHCI_INTERRUPT_ROUTING)) {
			TRACE(("usb_ohci: SMM does not respond. Resetting...\n"));
			_WriteReg(OHCI_CONTROL, OHCI_HC_FUNCTIONAL_STATE_RESET);
			snooze(USB_DELAY_BUS_RESET);
		}
	} else if ((control & OHCI_HC_FUNCTIONAL_STATE_MASK)
		!= OHCI_HC_FUNCTIONAL_STATE_RESET) {
		TRACE(("usb_ohci: BIOS is in control of the host controller\n"));
		if ((control & OHCI_HC_FUNCTIONAL_STATE_MASK)
			!= OHCI_HC_FUNCTIONAL_STATE_OPERATIONAL) {
			_WriteReg(OHCI_CONTROL, OHCI_HC_FUNCTIONAL_STATE_OPERATIONAL);
			// TOFIX: shorter delay
			snooze(USB_DELAY_BUS_RESET);
		}
	}

	// This reset should not be necessary according to the OHCI spec, but
	// without it some controllers do not start.
	_WriteReg(OHCI_CONTROL, OHCI_HC_FUNCTIONAL_STATE_RESET);
	snooze(USB_DELAY_BUS_RESET);

	// We now own the host controller and the bus has been reset
	uint32 frameInterval = _ReadReg(OHCI_FRAME_INTERVAL);
	uint32 intervalValue = OHCI_GET_INTERVAL_VALUE(frameInterval);

	// Disable interrupts right before we reset
	cpu_status former = disable_interrupts();
	_WriteReg(OHCI_COMMAND_STATUS, OHCI_HOST_CONTROLLER_RESET);
	for (uint32 i = 0; i < 10; i++) {
		spin(10);
		if (!(_ReadReg(OHCI_COMMAND_STATUS) & OHCI_HOST_CONTROLLER_RESET))
			break;
	}

	if (_ReadReg(OHCI_COMMAND_STATUS) & OHCI_HOST_CONTROLLER_RESET) {
		TRACE_ERROR(("usb_ohci: Error resetting the host controller (timeout)\n"));
		restore_interrupts(former);
		return;
	}

	// The controller is now in SUSPEND state, we have 2ms to go OPERATIONAL.
	// Interrupts are disabled.

	// Set up host controller register
	_WriteReg(OHCI_HCCA, (uint32)hccaPhysicalAddress);
	_WriteReg(OHCI_CONTROL_HEAD_ED, (uint32)fDummyControl->physical_address);
	_WriteReg(OHCI_BULK_HEAD_ED, (uint32)fDummyBulk->physical_address);
	// Disable all interrupts and then switch on all desired interrupts
	_WriteReg(OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTERRUPTS);
	_WriteReg(OHCI_INTERRUPT_ENABLE, OHCI_NORMAL_INTERRUPTS
		| OHCI_MASTER_INTERRUPT_ENABLE);
	// Switch on desired functional features
	control = _ReadReg(OHCI_CONTROL);
	control &= ~(OHCI_CONTROL_BULK_SERVICE_RATIO_MASK | OHCI_ENABLE_LIST
		| OHCI_HC_FUNCTIONAL_STATE_MASK | OHCI_INTERRUPT_ROUTING);
	control |= OHCI_ENABLE_LIST | OHCI_CONTROL_BULK_RATIO_1_4
		| OHCI_HC_FUNCTIONAL_STATE_OPERATIONAL;
	// And finally start the controller
	_WriteReg(OHCI_CONTROL, control);

	restore_interrupts(former);

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
	fPortCount = numberOfPorts;

	// Create semaphore the finisher thread will wait for
	fFinishTransfersSem = create_sem(0, "OHCI Finish Transfers");
	if (fFinishTransfersSem < B_OK) {
		TRACE_ERROR(("usb_ohci: failed to create semaphore\n"));
		return;
	}

	// Create the finisher service thread
	fFinishThread = spawn_kernel_thread(_FinishThread, "ohci finish thread",
		B_URGENT_DISPLAY_PRIORITY, (void *)this);
	resume_thread(fFinishThread);

	// Install the interrupt handler
	TRACE(("usb_ohci: installing interrupt handler\n"));
	install_io_interrupt_handler(fPCIInfo->u.h0.interrupt_line,
		_InterruptHandler, (void *)this, 0);

	TRACE(("usb_ohci: OHCI Host Controller Driver constructed\n")); 
	fInitOK = true;
}


OHCI::~OHCI()
{
	int32 result = 0;
	fStopFinishThread = true;
	delete_sem(fFinishTransfersSem);
	wait_for_thread(fFinishThread, &result);

	if (fHccaArea > 0)
		delete_area(fHccaArea);
	if (fRegisterArea > 0)
		delete_area(fRegisterArea);
	if (fDummyControl)
		_FreeEndpoint(fDummyControl);
	if (fDummyBulk)
		_FreeEndpoint(fDummyBulk);
	if (fDummyIsochronous)
		_FreeEndpoint(fDummyIsochronous);
	if (fRootHub)
		delete fRootHub;
	for (int i = 0; i < OHCI_NUMBER_OF_INTERRUPTS; i++)
		if (fInterruptEndpoints[i])
			_FreeEndpoint(fInterruptEndpoints[i]);
	delete [] fInterruptEndpoints;
	put_module(B_PCI_MODULE_NAME);
}


int32
OHCI::_InterruptHandler(void *data)
{
	return ((OHCI *)data)->_Interrupt();
}


int32
OHCI::_Interrupt()
{
	static spinlock lock = 0;
	acquire_spinlock(&lock);

	int32 result = B_UNHANDLED_INTERRUPT;

	release_spinlock(&lock);

	return result;
}


int32
OHCI::_FinishThread(void *data)
{
	((OHCI *)data)->_FinishTransfer();
	return B_OK;
}


void
OHCI::_FinishTransfer()
{
	while (!fStopFinishThread) {
		if (acquire_sem(fFinishTransfersSem) < B_OK)
			continue;
	}
}


status_t
OHCI::Start()
{
	TRACE(("usb_ohci: starting OHCI Host Controller\n"));

	if (!(_ReadReg(OHCI_CONTROL) & OHCI_HC_FUNCTIONAL_STATE_OPERATIONAL)) {
		TRACE_ERROR(("usb_ohci: Controller not started!\n"));
		return B_ERROR;
	}

	fRootHubAddress = AllocateAddress();
	fRootHub = new(std::nothrow) OHCIRootHub(RootObject(), fRootHubAddress);
	if (!fRootHub) {
		TRACE_ERROR(("usb_ohci: no memory to allocate root hub\n"));
		return B_NO_MEMORY;
	}

	if (fRootHub->InitCheck() < B_OK) {
		TRACE_ERROR(("usb_ohci: root hub failed init check\n"));
		return B_ERROR;
	}

	SetRootHub(fRootHub);
	TRACE(("usb_ohci: Host Controller started\n"));
	return BusManager::Start();
}


status_t
OHCI::SubmitTransfer(Transfer *transfer)
{
	if (transfer->TransferPipe()->DeviceAddress() == fRootHubAddress)
		return fRootHub->ProcessTransfer(this, transfer);

	uint32 type = transfer->TransferPipe()->Type();
	if ((type & USB_OBJECT_CONTROL_PIPE)) {
		TRACE(("usb_ohci: submitting control request\n"));
		return _SubmitControlRequest(transfer);
	}

	if ((type & USB_OBJECT_INTERRUPT_PIPE) || (type & USB_OBJECT_BULK_PIPE)) {
		// TODO
		return B_OK;
	}

	if ((type & USB_OBJECT_ISO_PIPE)) {
		TRACE(("usb_ohci: submitting isochronous transfer\n"));
		return _SubmitIsochronousTransfer(transfer);
	}

	TRACE_ERROR(("usb_ohci: tried to submit transfer for unknown pipe"
		" type %lu\n", type));
	return B_ERROR;
}


status_t
OHCI::_SubmitControlRequest(Transfer *transfer)
{
	usb_request_data *requestData = transfer->RequestData();
	bool directionIn = (requestData->RequestType & USB_REQTYPE_DEVICE_IN) > 0;

	ohci_general_td *setupDescriptor
		= _CreateGeneralDescriptor(sizeof(usb_request_data));
	if (!setupDescriptor) {
		TRACE_ERROR(("usb_ohci: failed to allocate setup descriptor\n"));
		return B_NO_MEMORY;
	}
	// Flags set up could be moved into _CreateGeneralDescriptor
	setupDescriptor->flags |= OHCI_TD_DIRECTION_PID_SETUP
		| OHCI_TD_NO_CONDITION_CODE
		| OHCI_TD_TOGGLE_0
		| OHCI_TD_SET_DELAY_INTERRUPT(6); // Not sure about this.

	ohci_general_td *statusDescriptor
		= _CreateGeneralDescriptor(0);
	if (!statusDescriptor) {
		TRACE_ERROR(("usb_ohci: failed to allocate status descriptor\n"));
		_FreeGeneralDescriptor(setupDescriptor);
		return B_NO_MEMORY;
	}
	statusDescriptor->flags 
		|= (directionIn ? OHCI_TD_DIRECTION_PID_OUT : OHCI_TD_DIRECTION_PID_IN)
		| OHCI_TD_NO_CONDITION_CODE
		| OHCI_TD_TOGGLE_1
		| OHCI_TD_SET_DELAY_INTERRUPT(1);

	iovec vector;
	vector.iov_base = requestData;
	vector.iov_len = sizeof(usb_request_data);
	_WriteDescriptorChain(setupDescriptor, &vector, 1);

	if (transfer->VectorCount() > 0) {
		ohci_general_td *dataDescriptor = NULL;
		ohci_general_td *lastDescriptor = NULL;
		status_t result = _CreateDescriptorChain(&dataDescriptor,
			&lastDescriptor,
			directionIn ? OHCI_TD_DIRECTION_PID_OUT : OHCI_TD_DIRECTION_PID_IN,
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

	// TODO
	// 1. Insert the chain descriptors to the endpoint
	// 2. Clear the Skip bit in the enpoint
	_WriteReg(OHCI_COMMAND_STATUS, OHCI_CONTROL_LIST_FILLED);

	return B_OK;
}


status_t
OHCI::_SubmitIsochronousTransfer(Transfer *transfer)
{
	return B_ERROR;
}


void
OHCI::_LinkDescriptors(ohci_general_td *first, ohci_general_td *second)
{
	first->next_physical_descriptor = second->physical_address;
	first->next_logical_descriptor = second;
}


status_t
OHCI::_CreateDescriptorChain(ohci_general_td **_firstDescriptor,
	ohci_general_td **_lastDescriptor, uint8 direction, size_t bufferSize)
{
	return B_ERROR;
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

			TRACE(("usb_ohci: copying %ld bytes to bufferOffset %ld from"
				" vectorOffset %ld at index %ld of %ld\n", length, bufferOffset,
				vectorOffset, vectorIndex, vectorCount));
			memcpy((uint8 *)current->buffer_logical + bufferOffset,
				(uint8 *)vector[vectorIndex].iov_base + vectorOffset, length);

			actualLength += length;
			vectorOffset += length;
			bufferOffset += length;

			if (vectorOffset >= vector[vectorIndex].iov_len) {
				if (++vectorIndex >= vectorCount) {
					TRACE(("usb_ohci: wrote descriptor chain (%ld bytes, no"
						" more vectors)\n", actualLength));
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

	TRACE(("usb_ohci: wrote descriptor chain (%ld bytes)\n", actualLength));
	return actualLength;
}


status_t
OHCI::NotifyPipeChange(Pipe *pipe, usb_change change)
{
	TRACE(("usb_ohci: pipe change %d for pipe 0x%08lx\n", change, (uint32)pipe));
	switch (change) {
		case USB_CHANGE_CREATED: {
			TRACE(("usb_ohci: inserting endpoint\n"));
			return _InsertEndpointForPipe(pipe);
		}
		case USB_CHANGE_DESTROYED: {
			TRACE(("usb_ohci: removing endpoint\n"));
			return _RemoveEndpointForPipe(pipe);
		}
		case USB_CHANGE_PIPE_POLICY_CHANGED: {
			TRACE(("usb_ohci: pipe policy changing unhandled!\n"));
			break;
		}
		default: {
			TRACE_ERROR(("usb_ohci: unknown pipe change!\n"));
			return B_ERROR;
		}
	}
	return B_OK;
}


status_t
OHCI::AddTo(Stack *stack)
{
#ifdef TRACE_USB
	set_dprintf_enabled(true); 
	load_driver_symbols("ohci");
#endif
	
	if (!sPCIModule) {
		status_t status = get_module(B_PCI_MODULE_NAME, (module_info **)&sPCIModule);
		if (status < B_OK) {
			TRACE_ERROR(("usb_ohci: getting pci module failed! 0x%08lx\n",
				status));
			return status;
		}
	}

	TRACE(("usb_ohci: searching devices\n"));
	bool found = false;
	pci_info *item = new(std::nothrow) pci_info;
	if (!item) {
		sPCIModule = NULL;
		put_module(B_PCI_MODULE_NAME);
		return B_NO_MEMORY;
	}

	for (uint32 i = 0 ; sPCIModule->get_nth_pci_info(i, item) >= B_OK; i++) {

		if (item->class_base == PCI_serial_bus && item->class_sub == PCI_usb
			&& item->class_api == PCI_usb_ohci) {
			if (item->u.h0.interrupt_line == 0
				|| item->u.h0.interrupt_line == 0xFF) {
				TRACE_ERROR(("usb_ohci: found device with invalid IRQ -"
					" check IRQ assignement\n"));
				continue;
			}

			TRACE(("usb_ohci: found device at IRQ %u\n",
				item->u.h0.interrupt_line));
			OHCI *bus = new(std::nothrow) OHCI(item, stack);
			if (!bus) {
				delete item;
				sPCIModule = NULL;
				put_module(B_PCI_MODULE_NAME);
				return B_NO_MEMORY;
			}

			if (bus->InitCheck() < B_OK) {
				TRACE_ERROR(("usb_ohci: bus failed init check\n"));
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
		TRACE_ERROR(("usb_ohci: no devices found\n"));
		delete item;
		sPCIModule = NULL;
		put_module(B_PCI_MODULE_NAME);
		return ENODEV;
	}

	delete item;
	return B_OK;
}


status_t
OHCI::GetPortStatus(uint8 index, usb_port_status *status)
{
	TRACE(("usb_ohci::%s(%ud, )\n", __FUNCTION__, index));
	if (index >= fPortCount)
		return B_BAD_INDEX;
	
	status->status = status->change = 0;
	uint32 portStatus = _ReadReg(OHCI_RH_PORT_STATUS(index));
	
	TRACE(("usb_ohci: RootHub::GetPortStatus: Port %i Value 0x%lx\n", OHCI_RH_PORT_STATUS(index), portStatus));
	
	// status
	if (portStatus & OHCI_RH_PORTSTATUS_CCS)
		status->status |= PORT_STATUS_CONNECTION;
	if (portStatus & OHCI_RH_PORTSTATUS_PES)
		status->status |= PORT_STATUS_ENABLE;
	if (portStatus & OHCI_RH_PORTSTATUS_PRS)
		status->status |= PORT_STATUS_RESET;
	if (portStatus & OHCI_RH_PORTSTATUS_LSDA)
		status->status |= PORT_STATUS_LOW_SPEED;
	if (portStatus & OHCI_RH_PORTSTATUS_PSS)
		status->status |= PORT_STATUS_SUSPEND;
	if (portStatus & OHCI_RH_PORTSTATUS_POCI)
		status->status |= PORT_STATUS_OVER_CURRENT;
	if (portStatus & OHCI_RH_PORTSTATUS_PPS)
		status->status |= PORT_STATUS_POWER;
	
	
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
	
	return B_OK;
}


status_t
OHCI::SetPortFeature(uint8 index, uint16 feature)
{
	TRACE(("OHCI::%s(%ud, %ud)\n", __FUNCTION__, index, feature));
	if (index > fPortCount)
		return B_BAD_INDEX;
	
	switch (feature) {
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
	TRACE(("OHCI::%s(%ud, %ud)\n", __FUNCTION__, index, feature));
	if (index > fPortCount)
		return B_BAD_INDEX;
	
	switch (feature) {
		case C_PORT_RESET:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_CSC);
			return B_OK;
		
		case C_PORT_CONNECTION:
			_WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_RH_PORTSTATUS_CSC);
			return B_OK;
	}
	
	return B_BAD_VALUE;
}


ohci_endpoint_descriptor*
OHCI::_AllocateEndpoint()
{
	ohci_endpoint_descriptor *endpoint;
	void* physicalAddress;

	// Allocate memory chunk
	if (fStack->AllocateChunk((void **)&endpoint, &physicalAddress,
		sizeof(ohci_endpoint_descriptor)) < B_OK) {
		TRACE_ERROR(("usb_ohci: failed to allocate endpoint descriptor\n"));
		return NULL;
	}
	memset((void *)endpoint, 0, sizeof(ohci_endpoint_descriptor));

	endpoint->physical_address = (addr_t)physicalAddress;

	endpoint->head_physical_descriptor = NULL;
	endpoint->tail_physical_descriptor = NULL;

	endpoint->head_logical_descriptor = NULL;
	endpoint->tail_logical_descriptor = NULL;

	return endpoint;
}


void
OHCI::_FreeEndpoint(ohci_endpoint_descriptor *endpoint)
{
	if (!endpoint)
		return;

	fStack->FreeChunk((void *)endpoint, (void *)endpoint->physical_address,
		sizeof(ohci_endpoint_descriptor));
}


ohci_general_td*
OHCI::_CreateGeneralDescriptor(size_t bufferSize)
{
	ohci_general_td *descriptor;
	void *physicalAddress;

	if (fStack->AllocateChunk((void **)&descriptor, &physicalAddress,
		sizeof(ohci_general_td)) != B_OK) {
		TRACE_ERROR(("usb_ohci: failed to allocate general descriptor\n"));
		return NULL;
	}
	memset((void *)descriptor, 0, sizeof(ohci_general_td));
	descriptor->physical_address = (addr_t)physicalAddress;

	if (!bufferSize) {
		descriptor->buffer_physical = 0;
		descriptor->buffer_logical = NULL;
		descriptor->last_physical_byte_address = 0;
		return descriptor;
	}

	if (fStack->AllocateChunk(&descriptor->buffer_logical,
		(void **)&descriptor->buffer_physical, bufferSize) != B_OK) {
		TRACE_ERROR(("usb_ohci: failed to allocate space for buffer\n"));
		fStack->FreeChunk(descriptor, (void *)descriptor->physical_address,
			sizeof(ohci_general_td));
		return NULL;
	}
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
			(void *)descriptor->buffer_physical, descriptor->buffer_size);
	}

	fStack->FreeChunk((void *)descriptor, (void *)descriptor->physical_address,
		sizeof(ohci_general_td));
}


status_t
OHCI::_InsertEndpointForPipe(Pipe *pipe)
{
	TRACE(("OHCI: Inserting Endpoint for device %u function %u\n",
		pipe->DeviceAddress(), pipe->EndpointAddress()));

	ohci_endpoint_descriptor *endpoint = _AllocateEndpoint();
	if (!endpoint) {
		TRACE_ERROR(("usb_ohci: cannot allocate memory for endpoint\n"));
		return B_NO_MEMORY;
	}

	uint32 flags = 0;
	flags |= OHCI_ENDPOINT_SKIP;

	// Set up flag field for the endpoint
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
			TRACE_ERROR(("usb_ohci: direction unknown. Wrong value!\n"));
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
		case USB_SPEED_HIGHSPEED:
		default:
			TRACE_ERROR(("usb_ohci: unaccetable speed. Wrong value!\n"));
			_FreeEndpoint(endpoint);
			return B_ERROR;
	}

	// Set the maximum packet size
	flags |= OHCI_ENDPOINT_SET_MAX_PACKET_SIZE(pipe->MaxPacketSize());

	endpoint->flags = flags;

	// Add the endpoint to the appropriate list
	ohci_endpoint_descriptor *head = NULL;
	switch (pipe->Type()) {
		case USB_OBJECT_CONTROL_PIPE:
			head = fDummyControl;
			break;
		case USB_OBJECT_BULK_PIPE:
			head = fDummyBulk;
			break;
		case USB_OBJECT_ISO_PIPE:
			// Set the isochronous bit format
			endpoint->flags = OHCI_ENDPOINT_ISOCHRONOUS_FORMAT;
			head = fDummyIsochronous;
			break;
		case USB_OBJECT_INTERRUPT_PIPE:
			head = _FindInterruptEndpoint((static_cast<InterruptPipe*>(pipe))->Interval());
			break;
		default: 
			TRACE_ERROR(("usb_ohci: unknown type of pipe. Wrong value!\n"));
			_FreeEndpoint(endpoint);
			return B_ERROR;
	}

	Lock();
	endpoint->next_logical_endpoint = head->next_logical_endpoint;
	endpoint->next_physical_endpoint = head->next_physical_endpoint;
	head->next_logical_endpoint = (void *)endpoint;
	head->next_physical_endpoint = (uint32)endpoint->physical_address;
	Unlock();

	return B_OK;
}


ohci_endpoint_descriptor*
OHCI::_FindInterruptEndpoint(uint8 interval)
{
	return NULL;
}


status_t
OHCI::_RemoveEndpointForPipe(Pipe *pipe)
{
	return B_ERROR;
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


status_t
OHCI::CancelQueuedTransfers(Pipe *pipe, bool force)
{
	return B_ERROR;
}
