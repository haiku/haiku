/*
 * Copyright 2004-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#include <module.h>
#include <PCI.h>
#include <USB.h>
#include <KernelExport.h>
#include <stdlib.h>

#include "uhci.h"
#include "uhci_hardware.h"
#include "usb_p.h"


#define TRACE_UHCI
#ifdef TRACE_UHCI
#define TRACE(x) dprintf x
#else
#define TRACE(x) /* nothing */
#endif


pci_module_info *UHCI::sPCIModule = NULL;


static int32
uhci_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			TRACE(("usb_uhci_module: init module\n"));
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE(("usb_uhci_module: uninit module\n"));
			break;
		default:
			return EINVAL;
	}

	return B_OK;
}	


host_controller_info uhci_module = {
	{
		"busses/usb/uhci/nielx",
		NULL,
		uhci_std_ops
	},
	NULL,
	UHCI::AddTo
};


module_info *modules[] = {
	(module_info *)&uhci_module,
	NULL
};


//
// #pragma mark -
//


Queue::Queue(Stack *stack)
{
	fStack = stack;

	void *physicalAddress;
	fStatus = fStack->AllocateChunk((void **)&fQueueHead, &physicalAddress, 32);
	if (fStatus < B_OK)
		return;

	fQueueHead->this_phy = (addr_t)physicalAddress;
	fQueueHead->element_phy = QH_TERMINATE;
	fQueueHead->element_log = 0;

	fStrayDescriptor = NULL;
}


Queue::~Queue()
{
	fStack->FreeChunk(fQueueHead, (void *)fQueueHead->this_phy, 32);

	if (fStrayDescriptor)
		fStack->FreeChunk(fStrayDescriptor, (void *)fStrayDescriptor->this_phy, 32);
}


status_t
Queue::InitCheck()
{
	return fStatus;
}


status_t
Queue::LinkTo(Queue *other)
{
	if (!other)
		return B_BAD_VALUE;

	fQueueHead->link_phy = other->fQueueHead->this_phy | QH_NEXT_IS_QH;
	fQueueHead->link_log = other->fQueueHead;
	return B_OK;
}


status_t
Queue::TerminateByStrayDescriptor()
{
	// According to the *BSD USB sources, there needs to be a stray transfer
	// descriptor in order to get some chipset to work nicely (like the PIIX).
	void *physicalAddress;
	status_t result = fStack->AllocateChunk((void **)&fStrayDescriptor,
		&physicalAddress, 32);
	if (result < B_OK) {
		TRACE(("usb_uhci: failed to allocate a stray transfer descriptor\n"));
		return result;
	}

	fStrayDescriptor->status = 0;
	fStrayDescriptor->this_phy = (addr_t)physicalAddress;
	fStrayDescriptor->link_phy = TD_TERMINATE;
	fStrayDescriptor->link_log = 0;
	fStrayDescriptor->buffer_phy = 0;
	fStrayDescriptor->buffer_log = 0;
	fStrayDescriptor->token = TD_TOKEN_NULL | (0x7f << TD_TOKEN_DEVADDR_SHIFT)
		| 0x69;

	fQueueHead->link_phy = fStrayDescriptor->this_phy;
	fQueueHead->link_log = fStrayDescriptor;
	fQueueHead->element_phy = QH_TERMINATE;
	fQueueHead->element_log = 0;

	return B_OK;
}


status_t
Queue::AppendDescriptor(uhci_td *descriptor)
{
	if (fQueueHead->element_phy & QH_TERMINATE) {
		// the queue is empty, make this the first element
		fQueueHead->element_phy = descriptor->this_phy;
		fQueueHead->element_log = descriptor;
		TRACE(("usb_uhci: first transfer in queue\n"));
	} else {
		// there are transfers linked, append to the queue
		uhci_td *element = (uhci_td *)fQueueHead->element_log;
		while ((element->link_phy & TD_TERMINATE) == 0)
			element = (uhci_td *)element->link_log;

		element->link_phy = descriptor->this_phy;
		element->link_log = descriptor;
		TRACE(("usb_uhci: appended transfer to queue\n"));
	}

	return B_OK;
}


status_t
Queue::AddTransfer(Transfer *transfer, bigtime_t timeout)
{
	// Allocate the transfer descriptor for the transfer
	void *physicalAddress;
	uhci_td *transferDescriptor;
	if (fStack->AllocateChunk((void **)&transferDescriptor, &physicalAddress, 32) < B_OK) {
		TRACE(("usb_uhci: failed to allocate a transfer descriptor\n"));
		return ENOMEM;
	}

	transferDescriptor->this_phy = (addr_t)physicalAddress;
	transferDescriptor->status = TD_STATUS_ACTIVE | TD_STATUS_3_ERRORS;
	if (transfer->GetPipe()->Speed() == Pipe::LowSpeed)
		transferDescriptor->status |= TD_STATUS_LOWSPEED;

	transferDescriptor->token = ((sizeof(usb_request_data) - 1) << 21)
		| (transfer->GetPipe()->EndpointAddress() << 15)
		| (transfer->GetPipe()->DeviceAddress() << 8) | 0x2D;

	// Create a physical space for the request
	if (fStack->AllocateChunk(&transferDescriptor->buffer_log,
		&transferDescriptor->buffer_phy, sizeof(usb_request_data)) < B_OK) {
		TRACE(("usb_uhci: unable to allocate space for the request buffer\n"));
		fStack->FreeChunk(transferDescriptor,
			(void *)transferDescriptor->this_phy, 32);
		return ENOMEM;
	}

	memcpy(transferDescriptor->buffer_log, transfer->GetRequestData(),
		sizeof(usb_request_data));

	// Create a status transfer descriptor
	uhci_td *statusDescriptor;
	if (fStack->AllocateChunk((void **)&statusDescriptor, &physicalAddress, 32) < B_OK) {
		TRACE(("usb_uhci: failed to allocate the status descriptor\n"));
		fStack->FreeChunk(transferDescriptor->buffer_log,
			(void *)transferDescriptor->buffer_phy, sizeof(usb_request_data));
		fStack->FreeChunk(transferDescriptor,
			(void *)transferDescriptor->this_phy, 32);
		return ENOMEM;
	}

	statusDescriptor->this_phy = (addr_t)physicalAddress;
	statusDescriptor->status = TD_STATUS_IOC | TD_STATUS_ACTIVE |
		TD_STATUS_3_ERRORS;
	if (transfer->GetPipe()->Speed() == Pipe::LowSpeed)
		statusDescriptor->status |= TD_STATUS_LOWSPEED;

	statusDescriptor->token = TD_TOKEN_NULL | TD_TOKEN_DATA1
		| (transfer->GetPipe()->EndpointAddress() << 15)
		| (transfer->GetPipe()->DeviceAddress() << 8) | 0x2d;

	statusDescriptor->buffer_phy = statusDescriptor->buffer_log = 0;
	statusDescriptor->link_phy = QH_TERMINATE;
	statusDescriptor->link_log = 0;

	if (transfer->GetBuffer() && transfer->GetBufferLength() > 0) {
		// ToDo: split up data to maxlen and create / link tds
	} else {
		// Link transfer and status descriptors directly
		transferDescriptor->link_phy = statusDescriptor->this_phy | TD_DEPTH_FIRST;
		transferDescriptor->link_log = statusDescriptor;
	}

	status_t result = AppendDescriptor(transferDescriptor);
	if (result < B_OK) {
		TRACE(("usb_uhci: failed to append descriptors\n"));
		fStack->FreeChunk(transferDescriptor->buffer_log,
			(void *)transferDescriptor->buffer_phy, sizeof(usb_request_data));
		fStack->FreeChunk(transferDescriptor,
			(void *)transferDescriptor->this_phy, 32);
		fStack->FreeChunk(statusDescriptor,
			(void *)statusDescriptor->this_phy, 32);
		return result;
	}

	if (timeout > 0) {
		// wait for the transfer to finish
		while (true) {
			if ((statusDescriptor->status & TD_STATUS_ACTIVE) == 0) {
				TRACE(("usb_uhci: transfer completed successfuly\n"));
				return B_OK;
			}
			if (transferDescriptor->status & 0x7e00) {
				TRACE(("usb_uhci: transfer failed: 0x%04x\n", transferDescriptor->status));
				return B_ERROR;
			}

			TRACE(("usb_uhci: in progress\n"));
			snooze(1000);
		}
	}

	return EINPROGRESS;	
}


status_t
Queue::RemoveInactiveDescriptors()
{
	if (fQueueHead->element_phy & QH_TERMINATE)
		return B_OK;

	uhci_td *element = (uhci_td *)fQueueHead->element_log;
	while (element) {
		if (element->status & TD_STATUS_ACTIVE)
			break;

		fStack->FreeChunk(element, (void *)element->this_phy, 32);

		if (element->link_phy & TD_TERMINATE) {
			fQueueHead->element_phy = QH_TERMINATE;
			fQueueHead->element_log = NULL;
			break;
		}

		element = (uhci_td *)element->link_log;
	}

	return B_OK;
}


addr_t
Queue::PhysicalAddress()
{
	return fQueueHead->this_phy;
}


//
// #pragma mark -
//


UHCI::UHCI(pci_info *info, Stack *stack)
	:	BusManager(),
		fPCIInfo(info),
		fStack(stack)
{
	TRACE(("usb_uhci: constructing new UHCI BusManager\n"));

	fRegisterBase = sPCIModule->read_pci_config(fPCIInfo->bus,
		fPCIInfo->device, fPCIInfo->function, PCI_memory_base, 4);
	fRegisterBase &= PCI_address_io_mask;
	TRACE(("usb_uhci: iospace offset: 0x%08x\n", fRegisterBase));

	// enable pci address access
	uint16 command = PCI_command_io | PCI_command_master | PCI_command_memory;
	command |= sPCIModule->read_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2);

	sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2, command);

	// make sure we gain controll of the UHCI controller instead of the BIOS
	sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device, 2,
		PCI_LEGSUP, 2, PCI_LEGSUP_USBPIRQDEN);

	// disable interrupts
	WriteReg16(UHCI_USBINTR, 0);

	// do a global and host reset
	GlobalReset();
	if (ResetController() < B_OK) {
		TRACE(("usb_uhci: host failed to reset\n"));
		fInitOK = false;
		return;
	}

	// Setup the frame list
	void *physicalAddress;
	fFrameArea = fStack->AllocateArea((void **)&fFrameList,
		(void **)&physicalAddress, 4096, "USB UHCI framelist");

	if (fFrameArea < B_OK) {
		TRACE(("usb_uhci: unable to create an area for the frame pointer list\n"));
		fInitOK = false;
		return;
	}

	// Set base pointer and reset frame number
	WriteReg32(UHCI_FRBASEADD, (uint32)physicalAddress);	
	WriteReg16(UHCI_FRNUM, 0);

	for (int32 i = 0; i < 4; i++) {
		fQueues[i] = new Queue(fStack);
		if (fQueues[i]->InitCheck() < B_OK) {
			TRACE(("usb_uhci: cannot create queues\n"));
			delete_area(fFrameArea);
			fInitOK = false;
			return;
		}

		if (i > 0)
			fQueues[i - 1]->LinkTo(fQueues[i]);
	}

	// Make sure the last queue terminates
	fQueues[3]->TerminateByStrayDescriptor();

	for (int32 i = 0; i < 1024; i++)
		fFrameList[i] = fQueues[0]->PhysicalAddress() | FRAMELIST_NEXT_IS_QH;

	TRACE(("usb_uhci: installing interrupt handler\n"));
	// Install the interrupt handler
	install_io_interrupt_handler(fPCIInfo->u.h0.interrupt_line,
		InterruptHandler, (void *)this, 0);

	// Enable interrupts
	WriteReg16(UHCI_USBINTR, UHCI_USBINTR_CRC | UHCI_USBINTR_RESUME
		| UHCI_USBINTR_IOC | UHCI_USBINTR_SHORT);

	TRACE(("usb_uhci: UHCI BusManager constructed\n"));
}


status_t
UHCI::Start()
{
	// Start the host controller, then start the Busmanager
	TRACE(("usb_uhci: starting UHCI BusManager\n"));
	TRACE(("usb_uhci: usbcmd reg 0x%04x, usbsts reg 0x%04x\n",
		ReadReg16(UHCI_USBCMD), ReadReg16(UHCI_USBSTS)));

	// Set the run bit in the command register
	WriteReg16(UHCI_USBCMD, ReadReg16(UHCI_USBCMD) | UHCI_USBCMD_RS);

	bool running = false;
	for (int32 i = 0; i < 10; i++) {
		uint16 status = ReadReg16(UHCI_USBSTS);
		TRACE(("usb_uhci: current loop %u, status 0x%04x\n", i, status));

		if (status & UHCI_USBSTS_HCHALT)
			snooze(10000);
		else {
			running = true;
			break;
		}
	}

	if (!running) {
		TRACE(("usb_uhci: controller won't start running\n"));
		return B_ERROR;
	}

	fRootHubAddress = AllocateAddress();
	fRootHub = new UHCIRootHub(this, fRootHubAddress);
	SetRootHub(fRootHub);

	TRACE(("usb_uhci: controller is started. status: %u curframe: %u\n",
		ReadReg16(UHCI_USBSTS), ReadReg16(UHCI_FRNUM)));
	return BusManager::Start();
}


status_t
UHCI::SubmitTransfer(Transfer *transfer, bigtime_t timeout)
{
	TRACE(("usb_uhci: submit packet called\n"));

	// Short circuit the root hub
	if (transfer->GetPipe()->DeviceAddress() == fRootHubAddress)
		return fRootHub->SubmitTransfer(transfer);

	if (transfer->GetPipe()->Type() == Pipe::Control)
		return fQueues[1]->AddTransfer(transfer, timeout);

	return B_ERROR;
}


void
UHCI::GlobalReset()
{
	WriteReg16(UHCI_USBCMD, UHCI_USBCMD_GRESET);
	snooze(100000);
	WriteReg16(UHCI_USBCMD, 0);
	snooze(10000);
}


status_t
UHCI::ResetController()
{
	WriteReg16(UHCI_USBCMD, UHCI_USBCMD_HCRESET);

	int32 tries = 5;
	while (ReadReg16(UHCI_USBCMD) & UHCI_USBCMD_HCRESET) {
		snooze(10000);
		if (tries-- < 0)
			return B_ERROR;
	}

	return B_OK;
}


uint16
UHCI::PortStatus(int32 index)
{
	if (index > 1)
		return B_BAD_VALUE;

	TRACE(("usb_uhci: read port status of port: %d\n", index));
	return ReadReg16(UHCI_PORTSC1 + index * 2);
}


status_t
UHCI::SetPortStatus(int32 index, uint16 status)
{
	if (index > 1)
		return B_BAD_VALUE;

	TRACE(("usb_uhci: set port status of port %d: 0x%04x\n", index, status));
	WriteReg16(UHCI_PORTSC1 + index * 2, status);
	snooze(1000);
	return B_OK;
}


status_t
UHCI::ResetPort(int32 index)
{
	if (index > 1)
		return B_BAD_VALUE;

	TRACE(("usb_uhci: reset port %d\n", index));

	uint32 port = UHCI_PORTSC1 + index * 2;
	uint16 status = ReadReg16(port);
	status &= UHCI_PORTSC_DATAMASK;
	WriteReg16(port, status | UHCI_PORTSC_RESET);
	snooze(250000);

	status = ReadReg16(port);
	status &= UHCI_PORTSC_DATAMASK;
	WriteReg16(port, status & ~UHCI_PORTSC_RESET);
	snooze(1000);

	for (int32 i = 10; i > 0; i--) {
		// try to enable the port
		status = ReadReg16(port);
		status &= UHCI_PORTSC_DATAMASK;
		WriteReg16(port, status | UHCI_PORTSC_ENABLED);
		snooze(50000);

		status = ReadReg16(port);

		if ((status & UHCI_PORTSC_CURSTAT) == 0) {
			// no device connected. since we waited long enough we can assume
			// that the port was reset and no device is connected.
			break;
		}

		if (status & (UHCI_PORTSC_STATCHA | UHCI_PORTSC_ENABCHA)) {
			// port enabled changed or connection status were set.
			// acknowledge either / both and wait again.
			status &= UHCI_PORTSC_DATAMASK;
			WriteReg16(port, status | UHCI_PORTSC_STATCHA | UHCI_PORTSC_ENABCHA);
			continue;
		}

		if (status & UHCI_PORTSC_ENABLED) {
			// the port is enabled
			break;
		}
	}

	TRACE(("usb_uhci: port was reset: 0x%04x\n", ReadReg16(port)));
	return B_OK;
}


int32
UHCI::InterruptHandler(void *data)
{
	cpu_status status = disable_interrupts();
	spinlock lock = 0;
	acquire_spinlock(&lock);

	int32 result = ((UHCI *)data)->Interrupt();

	release_spinlock(&lock);
	restore_interrupts(status);
	return result;
}


int32
UHCI::Interrupt()
{
	// Check if we really had an interrupt
	uint16 status = ReadReg16(UHCI_USBSTS);
	if ((status & UHCI_INTERRUPT_MASK) == 0)
		return B_UNHANDLED_INTERRUPT;

	TRACE(("usb_uhci: Interrupt()\n"));
	TRACE(("usb_uhci: status: 0x%04x\n", status));

	uint16 acknowledge = 0;
	if (status & UHCI_USBSTS_USBINT) {
		TRACE(("usb_uhci: transfer finished\n"));
		acknowledge |= UHCI_USBSTS_USBINT;
	}

	if (status & UHCI_USBSTS_ERRINT) {
		TRACE(("usb_uhci: transfer error\n"));
		acknowledge |= UHCI_USBSTS_ERRINT;
	}

	if (status & UHCI_USBSTS_RESDET) {
		TRACE(("usb_uhci: resume detected\n"));
		acknowledge |= UHCI_USBSTS_RESDET;
	}

	if (status & UHCI_USBSTS_HOSTERR) {
		TRACE(("usb_uhci: host system error\n"));
		acknowledge |= UHCI_USBSTS_HOSTERR;
	}

	if (status & UHCI_USBSTS_HCPRERR) {
		TRACE(("usb_uhci: process error\n"));
		acknowledge |= UHCI_USBSTS_HCPRERR;
	}

	if (status & UHCI_USBSTS_HCHALT) {
		TRACE(("usb_uhci: host controller halted\n"));
		// acknowledge not needed
	}

	if (acknowledge)
		WriteReg16(UHCI_USBSTS, acknowledge);

	return B_HANDLED_INTERRUPT;
}


bool
UHCI::AddTo(Stack &stack)
{
#ifdef TRACE_UHCI
	set_dprintf_enabled(true); 
	load_driver_symbols("uhci");
#endif

	if (!sPCIModule) {
		status_t status = get_module(B_PCI_MODULE_NAME, (module_info **)&sPCIModule);
		if (status < B_OK) {
			TRACE(("usb_uhci: AddTo(): getting pci module failed! 0x%08x\n",
				status));
			return status;
		}
	}

	TRACE(("usb_uhci: AddTo(): setting up hardware\n"));

	bool found = false;
	pci_info *item = new pci_info;
	for (int32 i = 0; sPCIModule->get_nth_pci_info(i, item) >= B_OK; i++) {
		//class_base = 0C (serial bus) class_sub = 03 (usb) prog_int: 00 (UHCI)
		if (item->class_base == 0x0C && item->class_sub == 0x03
			&& item->class_api == 0x00) {
			if (item->u.h0.interrupt_line == 0
				|| item->u.h0.interrupt_line == 0xFF) {
				TRACE(("usb_uhci: AddTo(): found with invalid IRQ - check IRQ assignement\n"));
				continue;
			}

			TRACE(("usb_uhci: AddTo(): found at IRQ %u\n", item->u.h0.interrupt_line));
			UHCI *bus = new UHCI(item, &stack);
			if (bus->InitCheck() < B_OK) {
				TRACE(("usb_uhci: AddTo(): InitCheck() failed 0x%08x\n", bus->InitCheck()));
				delete bus;
				continue;
			}

			bus->Start();
			stack.AddBusManager(bus);
			found = true;
		}
	}

	if (!found) {
		TRACE(("usb_uhci: AddTo(): no devices found\n"));
		delete item;
		sPCIModule = NULL;
		put_module(B_PCI_MODULE_NAME);
		return ENODEV;
	}

	return B_OK;
}


inline void
UHCI::WriteReg16(uint32 reg, uint16 value)
{
	sPCIModule->write_io_16(fRegisterBase + reg, value);
}


inline void
UHCI::WriteReg32(uint32 reg, uint32 value)
{
	sPCIModule->write_io_32(fRegisterBase + reg, value);
}


inline uint16
UHCI::ReadReg16(uint32 reg)
{
	return sPCIModule->read_io_16(fRegisterBase + reg);
}


inline uint32
UHCI::ReadReg32(uint32 reg)
{
	return sPCIModule->read_io_32(fRegisterBase + reg);
}
