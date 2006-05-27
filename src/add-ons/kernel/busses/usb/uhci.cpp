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
	if (Reset() < B_OK) {
		TRACE(("usb_uhci: host failed to reset\n"));
		m_initok = false;
		return;
	}

	// Setup the frame list
	void *physicalAddress;
	fFrameArea = fStack->AllocateArea((void **)&fFrameList,
		(void **)&physicalAddress, 4096, "USB UHCI framelist");

	if (fFrameArea < B_OK) {
		TRACE(("usb_uhci: unable to create an area for the frame pointer list\n"));
		m_initok = false;
		return;
	}

	// Set base pointer and reset frame number
	WriteReg32(UHCI_FRBASEADD, (uint32)physicalAddress);	
	WriteReg16(UHCI_FRNUM, 0);

	/*
		According to the *BSD USB sources, there needs to be a stray transfer
		descriptor in order to get some chipset to work nicely (like the PIIX).
	*/
	uhci_td *strayDescriptor;
	if (fStack->AllocateChunk((void **)&strayDescriptor, &physicalAddress, 32) != B_OK) {
		TRACE(("usb_uhci: failed to allocate a stray transfer descriptor\n"));
		delete_area(fFrameArea);
		m_initok = false;
		return;
	}

	strayDescriptor->status = 0;
	strayDescriptor->this_phy = (addr_t)physicalAddress;
	strayDescriptor->link_phy = TD_TERMINATE;
	strayDescriptor->link_log = 0;
	strayDescriptor->buffer_phy = 0;
	strayDescriptor->buffer_log = 0;
	strayDescriptor->token = TD_TOKEN_NULL | (0x7f << TD_TOKEN_DEVADDR_SHIFT)
		| 0x69;

	/*
		Setup the virtual structure. I stole this idea from the linux usb stack,
		the idea is that for every interrupt interval there is a queue head.
		These things all link together and eventually point to the control and
		bulk virtual queue heads.
	*/
	for (int32 i = 0; i < 12; i++) {
		// must be aligned on 16-byte boundaries
		if (fStack->AllocateChunk((void **)&fVirtualQueueHead[i],
			&physicalAddress, 32) != B_OK) {
			TRACE(("usb_uhci: failed allocation of skeleton queue head %i, aborting\n",  i));
			delete_area(fFrameArea);
			m_initok = false;
			return;
		}

		fVirtualQueueHead[i]->this_phy = (addr_t)physicalAddress;
		fVirtualQueueHead[i]->element_phy = QH_TERMINATE;
		fVirtualQueueHead[i]->element_log = 0;

		if (i == 0)
			continue;

		// link this queue head to the previous queue head
		fVirtualQueueHead[i - 1]->link_phy = fVirtualQueueHead[i]->this_phy | QH_NEXT_IS_QH;
		fVirtualQueueHead[i - 1]->link_log = fVirtualQueueHead[i];
	}

	// Make sure the fQueueHeadTerminate terminates
	fVirtualQueueHead[11]->link_phy = strayDescriptor->this_phy;
	fVirtualQueueHead[11]->link_log = strayDescriptor;

	// Insert the queues in the frame list. The linux developers mentioned
	// in a comment that they used some magic to distribute the elements all
	// over the place, but I don't really think that it is useful right now
	// (nor do I know how I should do that), instead, I just take the frame
	// number and determine where it should begin

	// NOTE, in c++ this is butt-ugly. We have a addr_t *array (because with
	// an addr_t *array we can apply pointer arithmetic), uhci_qh *pointers
	// that need to be put through the logical | to make sure the pointer is
	// invalid for the hc. The result of that needs to be converted into a
	// addr_t. Get it?
	for (int32 i = 0; i < 1024; i++) {
		int32 frame = i + 1;
		if (frame % 256 == 0)
			fFrameList[i] = fQueueHeadInterrupt256->this_phy | FRAMELIST_NEXT_IS_QH;
		else if (frame % 128 == 0)
			fFrameList[i] = fQueueHeadInterrupt128->this_phy | FRAMELIST_NEXT_IS_QH;
		else if (frame % 64 == 0)
			fFrameList[i] = fQueueHeadInterrupt64->this_phy | FRAMELIST_NEXT_IS_QH;
		else if (frame % 32 == 0)
			fFrameList[i] = fQueueHeadInterrupt32->this_phy | FRAMELIST_NEXT_IS_QH;
		else if (frame % 16 == 0)
			fFrameList[i] = fQueueHeadInterrupt16->this_phy | FRAMELIST_NEXT_IS_QH;
		else if (frame % 8 == 0)
			fFrameList[i] = fQueueHeadInterrupt8->this_phy | FRAMELIST_NEXT_IS_QH;
		else if (frame % 4 == 0)
			fFrameList[i] = fQueueHeadInterrupt4->this_phy | FRAMELIST_NEXT_IS_QH;
		else if (frame % 2 == 0)
			fFrameList[i] = fQueueHeadInterrupt2->this_phy | FRAMELIST_NEXT_IS_QH;
		else
			fFrameList[i] = fQueueHeadInterrupt1->this_phy | FRAMELIST_NEXT_IS_QH;
	}

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

	TRACE(("usb_uhci: controller is started. status: %u curframe: %u\n",
		ReadReg16(UHCI_USBSTS), ReadReg16(UHCI_FRNUM)));
	return BusManager::Start();
}


status_t
UHCI::SetupRootHub()
{
	TRACE(("usb_uhci: setting up root hub\n"));

	fRootHubAddress = AllocateAddress();
	fRootHub = new UHCIRootHub(this, fRootHubAddress);
	SetRootHub(fRootHub);

	return B_OK;
}


status_t
UHCI::SubmitTransfer(Transfer *transfer)
{
	TRACE(("usb_uhci: submit packet called\n"));

	// Short circuit the root hub
	if (transfer->GetPipe()->GetDeviceAddress() == fRootHubAddress)
		return fRootHub->SubmitTransfer(transfer);

	if (transfer->GetPipe()->GetType() == Pipe::Control)
		return InsertControl(transfer);

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
UHCI::Reset()
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


status_t
UHCI::InsertControl(Transfer *transfer)
{
	TRACE(("usb_uhci: InsertControl() frnum %u, usbsts reg %u\n",
		ReadReg16(UHCI_FRNUM), ReadReg16(UHCI_USBSTS)));

	// HACK: this one is to prevent rogue transfers from happening
	if (!transfer->GetBuffer())
		return B_ERROR;

	// Please note that any data structures must be aligned on a 16 byte boundary
	// Also, due to the strange ways of C++' void* handling, this code is much messier
	// than it actually should be. Forgive me. Or blame the compiler.
	// First, set up a Queue Head for the transfer
	uhci_qh *topQueueHead;
	void *physicalAddress;
	if (fStack->AllocateChunk((void **)&topQueueHead, &physicalAddress, 32) < B_OK) {
		TRACE(("usb_uhci: failed to allocate a queue head\n"));
		return ENOMEM;
	}

	topQueueHead->link_phy = QH_TERMINATE;
	topQueueHead->link_log = 0;
	topQueueHead->this_phy = (addr_t)physicalAddress;

	// Allocate the transfer descriptor for the transfer
	uhci_td *transferDescriptor;
	if (fStack->AllocateChunk((void **)&transferDescriptor, &physicalAddress, 32) < B_OK) {
		TRACE(("usb_uhci: failed to allocate the transfer descriptor\n"));
		fStack->FreeChunk(topQueueHead, (void *)topQueueHead->this_phy, 32);
		return ENOMEM;
	}

	transferDescriptor->this_phy = (addr_t)physicalAddress;
	transferDescriptor->status = TD_STATUS_ACTIVE;
	if (transfer->GetPipe()->GetSpeed() == Pipe::LowSpeed)
		transferDescriptor->status |= TD_STATUS_LOWSPEED;

	transferDescriptor->token = ((sizeof(usb_request_data) - 1) << 21)
		| (transfer->GetPipe()->GetEndpointAddress() << 15)
		| (transfer->GetPipe()->GetDeviceAddress() << 8) | 0x2D;

	// Create a physical space for the setup request
	if (fStack->AllocateChunk(&transferDescriptor->buffer_log,
		&transferDescriptor->buffer_phy, sizeof(usb_request_data)) < B_OK) {
		TRACE(("usb_uhci: unable to allocate space for the setup buffer\n"));
		fStack->FreeChunk(transferDescriptor,
			(void *)transferDescriptor->this_phy, 32);
		fStack->FreeChunk(topQueueHead, (void *)topQueueHead->this_phy, 32);
		return ENOMEM;
	}

	memcpy(transferDescriptor->buffer_log, transfer->GetRequestData(), sizeof(usb_request_data));

	// Link this to the queue head
	topQueueHead->element_phy = transferDescriptor->this_phy;
	topQueueHead->element_log = transferDescriptor;

	// TODO: split the buffer into max transfer sizes

	// Finally, create a status transfer descriptor
	uhci_td *statusDescriptor;
	if (fStack->AllocateChunk((void **)&statusDescriptor, &physicalAddress, 32) < B_OK) {
		TRACE(("usb_uhci: failed to allocate the status descriptor\n"));
		fStack->FreeChunk(transferDescriptor->buffer_log,
			(void *)transferDescriptor->buffer_phy, sizeof(usb_request_data));
		fStack->FreeChunk(transferDescriptor,
			(void *)transferDescriptor->this_phy, 32);
		fStack->FreeChunk(topQueueHead, (void *)topQueueHead->this_phy, 32);
		return ENOMEM;
	}

	statusDescriptor->this_phy = (addr_t)physicalAddress;
	statusDescriptor->status = TD_STATUS_IOC;
	if (transfer->GetPipe()->GetSpeed() == Pipe::LowSpeed)
		statusDescriptor->status |= TD_STATUS_LOWSPEED;

	statusDescriptor->token = TD_TOKEN_NULL | TD_TOKEN_DATA1
		| (transfer->GetPipe()->GetEndpointAddress() << 15)
		| (transfer->GetPipe()->GetDeviceAddress() << 8) | 0x69;

	// Invalidate the buffer field
	statusDescriptor->buffer_phy = statusDescriptor->buffer_log = 0;

	// Link to the previous transfer descriptor
	transferDescriptor->link_phy = statusDescriptor->this_phy | TD_DEPTH_FIRST;
	transferDescriptor->link_log = statusDescriptor;

	// This is the end of this chain, so don't link to any next QH/TD
	statusDescriptor->link_phy = QH_TERMINATE;
	statusDescriptor->link_log = 0;

	// First, add the transfer to the list of transfers
	transfer->SetHostPrivate(new hostcontroller_priv);
	transfer->GetHostPrivate()->topqh = topQueueHead;
	transfer->GetHostPrivate()->firsttd = transferDescriptor;
	transfer->GetHostPrivate()->lasttd = statusDescriptor;
	fTransfers.PushBack(transfer);

	// Secondly, append the qh to the control list
	if (fQueueHeadControl->element_phy & QH_TERMINATE) {
		// the control queue is empty, make this the first element
		fQueueHeadControl->element_phy = topQueueHead->this_phy;
		fQueueHeadControl->link_log = (void *)topQueueHead;
		TRACE(("usb_uhci: first transfer in queue\n"));
	} else {
		// there are control transfers linked, append to the queue
		uhci_qh *queueHead = (uhci_qh *)fQueueHeadControl->link_log;
		while ((queueHead->link_phy & QH_TERMINATE) == 0)
			queueHead = (uhci_qh *)queueHead->link_log;

		queueHead->link_phy = topQueueHead->this_phy;
		queueHead->link_log = (void *)topQueueHead;
		TRACE(("usb_uhci: appended transfer to queue\n"));
	}

	return EINPROGRESS;	
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

	// TODO: in the future we might want to support multiple host controllers.
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
			bus->SetupRootHub();
			stack.AddBusManager(bus);
			found = true;
			break;
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
