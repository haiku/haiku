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
ohci_std_ops( int32 op , ... )
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

	memset((void*)fHcca, 0, sizeof(ohci_hcca));

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

	// Create the interrupt tree
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

	// Static endpoints that get linked in the HCCA
	for (uint32 i = 0; i < OHCI_NUMBER_OF_INTERRUPTS; i++) {
		fInterruptEndpoints[i] = _AllocateEndpoint();
		if (!fInterruptEndpoints[i]) {
			while (--i >= 0)
				_FreeEndpoint(fInterruptEndpoints[i]);
			_FreeEndpoint(fDummyBulk);
			_FreeEndpoint(fDummyControl);
			_FreeEndpoint(fDummyIsochronous);
			return;
		}
		fInterruptEndpoints[i]->flags |= OHCI_ENDPOINT_SKIP;
		fInterruptEndpoints[i]->next_physical_endpoint
			= fDummyIsochronous->physical_address;
	}

#if 0
	ohci_endpoint_descriptor *current;
	for( uint32 i = 0; i < OHCI_NUMBER_OF_ENDPOINTS; i++) {
		current = _AllocateEndpoint();
		if (!current) {
			TRACE_ERROR(("usb_ohci: failed to create interrupts tree\n"));
			while (--i >= 0)
				_FreeEndpoint(fInterruptEndpoints[i]);
			_FreeEndpoint(fDummyBulk);
			_FreeEndpoint(fDummyControl);
			_FreeEndpoint(fDummyIsochronous);
			return;
		}
		fInterruptEndpoints[i] = current;
		current->flags |= OHCI_ENDPOINT_SKIP;
		if (i != 0)
			previous = fInterruptEndpoints[(i - 1) / 2];
		else
			previous = fDummyIsochronous;
		current->next_logical_endpoint = previous;
		current->next_physical_endpoint = previous->physical_address;
	}
#endif

	// Fill HCCA interrupt table. The bit reversal is to get
	// the tree set up properly to spread the interrupts.
	for (uint32 i = 0; i < OHCI_NUMBER_OF_INTERRUPTS; i++)
		fHcca->interrupt_table[revbits[i]] =
			fInterruptEndpoints[OHCI_NUMBER_OF_ENDPOINTS - OHCI_NUMBER_OF_INTERRUPTS + i]->physical_address;


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
	_WriteReg(OHCI_COMMAND_STATUS, OHCI_HOST_CONTROLLER_RESET);

	for (uint32 i = 0; i < 10; i++) {
		snooze(10);
		if (!(_ReadReg(OHCI_COMMAND_STATUS) & OHCI_HOST_CONTROLLER_RESET))
			break;
	}

	if (_ReadReg(OHCI_COMMAND_STATUS) & OHCI_HOST_CONTROLLER_RESET) {
		TRACE_ERROR(("usb_ohci: Error resetting the host controller (timeout)\n"));
		return;
	}

	// The controller is now in SUSPEND state, we have 2ms to go OPERATIONAL.
	// In order to do so we need to disable interrupts.

	cpu_status former = disable_interrupts();

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

	// TODO: Enable interrupts. Maybe disable first somewhere before :)

	TRACE(("usb_ohci: OHCI Host Controller Driver constructed\n")); 
	fInitOK = true;
}


OHCI::~OHCI()
{
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
	// TODO: block on the semaphore
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
	if ((type & USB_OBJECT_CONTROL_PIPE) || (type & USB_OBJECT_BULK_PIPE)) {
		TRACE(("usb_ohci: submitting async transfer\n"));
		return _SubmitAsyncTransfer(transfer);
	}

	if ((type & USB_OBJECT_INTERRUPT_PIPE) || (type & USB_OBJECT_ISO_PIPE)) {
		TRACE(("usb_ohci: submitting periodic transfer\n"));
		return _SubmitPeriodicTransfer(transfer);
	}

	TRACE_ERROR(("usb_ohci: tried to submit transfer for unknow pipe"
		" type %lu\n", type));
	return B_ERROR;
}


status_t
OHCI::_SubmitAsyncTransfer(Transfer *transfer)
{
	return B_ERROR;
}


status_t
OHCI::_SubmitPeriodicTransfer(Transfer *transfer)
{
	return B_ERROR;
}


status_t
OHCI::NotifyPipeChange(Pipe *pipe, usb_change change)
{
	TRACE(("usb_ohci::%s(%p, %d)\n", __FUNCTION__, pipe, (int)change));
	if (InitCheck())
		return B_ERROR;

	switch (change) {
	case USB_CHANGE_CREATED:
		return _InsertEndpointForPipe(pipe);
	case USB_CHANGE_DESTROYED:
		// Do something
		return B_ERROR;
	case USB_CHANGE_PIPE_POLICY_CHANGED:
	default:
		break;
	}
	return B_ERROR; //We should never go here
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
			TRACE_ERROR(("usb_ohci: AddTo(): getting pci module failed! 0x%08lx\n",
				status));
			return status;
		}
	}

	TRACE(("usb_ohci: AddTo(): setting up hardware\n"));

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
				TRACE_ERROR(("usb_ohci: AddTo(): found with invalid IRQ -"
					" check IRQ assignement\n"));
				continue;
			}

			TRACE(("usb_ohci: AddTo(): found at IRQ %u\n",
				item->u.h0.interrupt_line));
			OHCI *bus = new(std::nothrow) OHCI(item, stack);
			if (!bus) {
				delete item;
				sPCIModule = NULL;
				put_module(B_PCI_MODULE_NAME);
				return B_NO_MEMORY;
			}

			if (bus->InitCheck() < B_OK) {
				TRACE_ERROR(("usb_ohci: AddTo(): InitCheck() failed 0x%08lx\n",
					bus->InitCheck()));
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
	fStack->FreeChunk((void *)endpoint, (void *)endpoint->physical_address,
		sizeof(ohci_endpoint_descriptor));
}


ohci_general_descriptor*
OHCI::_CreateGeneralDescriptor()
{
	ohci_general_descriptor *descriptor;
	void *physicalAddress;

	if (fStack->AllocateChunk((void **)&descriptor, &physicalAddress,
		sizeof(ohci_general_descriptor)) != B_OK) {
		TRACE_ERROR(("usb_ohci: failed to allocate general descriptor\n"));
		return NULL;
	}

	// TODO: Finish methods
	memset((void *)descriptor, 0, sizeof(ohci_general_descriptor));
	descriptor->physical_address = (addr_t)physicalAddress;

	return descriptor;
}	


void
OHCI::_FreeGeneralDescriptor(ohci_general_descriptor *descriptor)
{
	if (!descriptor)
		return;

	fStack->FreeChunk((void *)descriptor, (void *)descriptor->physical_address,
		sizeof(ohci_general_descriptor));
}


status_t
OHCI::_InsertEndpointForPipe(Pipe *p)
{
#if 0
	TRACE(("OHCI: Inserting Endpoint for device %u function %u\n", p->DeviceAddress(), p->EndpointAddress()));

	if (InitCheck())
		return B_ERROR;
	
	//Endpoint *endpoint = _AllocateEndpoint();
	ohci_endpoint_descriptor *endpoint = _AllocateEndpoint();
	if (!endpoint)
		return B_NO_MEMORY;
	
	//Set up properties of the endpoint
	//TODO: does this need its own utility function?
	{
		uint32 properties = 0;
		
		//Set the device address
		properties |= OHCI_ENDPOINT_SET_DEVICE_ADDRESS(p->DeviceAddress());
		
		//Set the endpoint number
		properties |= OHCI_ENDPOINT_SET_ENDPOINT_NUMBER(p->EndpointAddress());
		
		//Set the direction
		switch (p->Direction()) {
		case Pipe::In:
			properties |= OHCI_ENDPOINT_DIRECTION_IN;
			break;
		case Pipe::Out:
			properties |= OHCI_ENDPOINT_DIRECTION_OUT;
			break;
		case Pipe::Default:
			properties |= OHCI_ENDPOINT_DIRECTION_DESCRIPTOR;
			break;
		default:
			//TODO: error
			break;
		}
		
		//Set the speed
		switch (p->Speed()) {
		case USB_SPEED_LOWSPEED:
			//the bit is 0
			break;
		case USB_SPEED_FULLSPEED:
			properties |= OHCI_ENDPOINT_SPEED;
			break;
		case USB_SPEED_HIGHSPEED:
		default:
			//TODO: error
			break;
		}
		
		//Assign the format. Isochronous endpoints require this switch
		if (p->Type() & USB_OBJECT_ISO_PIPE)
			properties |= OHCI_ENDPOINT_ISOCHRONOUS_FORMAT;
		
		//Set the maximum packet size
		properties |= OHCI_ENDPOINT_SET_MAX_PACKET_SIZE(p->MaxPacketSize());
		
		endpoint->ed->flags = properties;
	}
	
	//Check which list we need to add the endpoint in
	Endpoint *listhead;
	if (p->Type() & USB_OBJECT_CONTROL_PIPE)
		listhead = fDummyControl;
	else if (p->Type() & USB_OBJECT_BULK_PIPE)
		listhead = fDummyBulk;
	else if (p->Type() & USB_OBJECT_ISO_PIPE)
		listhead = fDummyIsochronous;
	else {
		_FreeEndpoint(endpoint);
		return B_ERROR;
	}

	//Add the endpoint to the queues
	if ((p->Type() & USB_OBJECT_ISO_PIPE) == 0) {
		//Link the endpoint into the head of the list
		endpoint->SetNext(listhead->next);
		listhead->SetNext(endpoint);
	} else { 
		//Link the endpoint into the tail of the list
		Endpoint *tail = listhead;
		while (tail->next != 0)
			tail = tail->next;
		tail->SetNext(endpoint);
	}
#endif
	return B_OK;
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
