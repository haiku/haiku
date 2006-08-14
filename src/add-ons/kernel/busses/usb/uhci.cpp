/*
 * Copyright 2004-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
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
#define TRACE_ERROR(x) dprintf x
#else
#define TRACE(x) /* nothing */
#define TRACE_ERROR(x) dprintf x
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


#ifdef TRACE_UHCI

void
print_descriptor_chain(uhci_td *descriptor)
{
	while (descriptor) {
		dprintf("ph: 0x%08x; lp: 0x%08x; vf: %s; q: %s; t: %s; st: 0x%08x; to: 0x%08x\n",
			descriptor->this_phy & 0xffffffff, descriptor->link_phy & 0xfffffff0,
			descriptor->link_phy & 0x4 ? "y" : "n",
			descriptor->link_phy & 0x2 ? "qh" : "td",
			descriptor->link_phy & 0x1 ? "y" : "n",
			descriptor->status, descriptor->token);

		if (descriptor->link_phy & TD_TERMINATE)
			break;

		descriptor = (uhci_td *)descriptor->link_log;
	}
}

#endif // TRACE_UHCI


//
// #pragma mark -
//


Queue::Queue(Stack *stack)
{
	fStack = stack;

	if (benaphore_init(&fLock, "uhci queue lock") < B_OK) {
		TRACE_ERROR(("usb_uhci: failed to create queue lock\n"));
		return;
	}

	void *physicalAddress;
	fStatus = fStack->AllocateChunk((void **)&fQueueHead, &physicalAddress, 32);
	if (fStatus < B_OK)
		return;

	fQueueHead->this_phy = (addr_t)physicalAddress;
	fQueueHead->element_phy = QH_TERMINATE;

	fStrayDescriptor = NULL;
	fQueueTop = NULL;
}


Queue::~Queue()
{
	Lock();
	benaphore_destroy(&fLock);

	fStack->FreeChunk(fQueueHead, (void *)fQueueHead->this_phy, 32);

	if (fStrayDescriptor)
		fStack->FreeChunk(fStrayDescriptor, (void *)fStrayDescriptor->this_phy, 32);
}


status_t
Queue::InitCheck()
{
	return fStatus;
}


bool
Queue::Lock()
{
	return (benaphore_lock(&fLock) == B_OK);
}


void
Queue::Unlock()
{
	benaphore_unlock(&fLock);
}


status_t
Queue::LinkTo(Queue *other)
{
	if (!other)
		return B_BAD_VALUE;

	if (!Lock())
		return B_ERROR;

	fQueueHead->link_phy = other->fQueueHead->this_phy | QH_NEXT_IS_QH;
	fQueueHead->link_log = other->fQueueHead;
	Unlock();

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
		TRACE_ERROR(("usb_uhci: failed to allocate a stray transfer descriptor\n"));
		return result;
	}

	fStrayDescriptor->status = 0;
	fStrayDescriptor->this_phy = (addr_t)physicalAddress;
	fStrayDescriptor->link_phy = TD_TERMINATE;
	fStrayDescriptor->link_log = 0;
	fStrayDescriptor->buffer_phy = 0;
	fStrayDescriptor->buffer_log = 0;
	fStrayDescriptor->buffer_size = 0;
	fStrayDescriptor->token = TD_TOKEN_NULL_DATA
		| (0x7f << TD_TOKEN_DEVADDR_SHIFT) | TD_TOKEN_IN;

	if (!Lock()) {
		fStack->FreeChunk(fStrayDescriptor, (void *)fStrayDescriptor->this_phy,
			32);
		return B_ERROR;
	}

	fQueueHead->link_phy = fStrayDescriptor->this_phy;
	fQueueHead->link_log = fStrayDescriptor;
	fQueueHead->element_phy = QH_TERMINATE;
	Unlock();

	return B_OK;
}


status_t
Queue::AppendDescriptorChain(uhci_td *descriptor)
{
	TRACE(("usb_uhci: appending descriptor chain\n"));

	if (!Lock())
		return B_ERROR;

#ifdef TRACE_UHCI
	print_descriptor_chain(descriptor);
#endif

	if (fQueueTop == NULL) {
		// the queue is empty, make this the first element
		fQueueTop = descriptor;
		fQueueHead->element_phy = descriptor->this_phy;
		TRACE(("usb_uhci: first transfer in queue\n"));
	} else {
		// there are transfers linked, append to the queue
		uhci_td *element = fQueueTop;
		while ((element->link_phy & TD_TERMINATE) == 0)
			element = (uhci_td *)element->link_log;

		element->link_log = descriptor;
		element->link_phy = descriptor->this_phy | TD_DEPTH_FIRST;
		TRACE(("usb_uhci: appended transfer to queue\n"));
	}

	Unlock();
	return B_OK;
}


status_t
Queue::RemoveDescriptorChain(uhci_td *firstDescriptor, uhci_td *lastDescriptor)
{
	TRACE(("usb_uhci: removing descriptor chain\n"));

	if (!Lock())
		return B_ERROR;

	if (fQueueTop == firstDescriptor) {
		// it is the first chain in this queue
		if ((lastDescriptor->link_phy & TD_TERMINATE) > 0) {
			// it is the only chain in this queue
			fQueueTop = NULL;
			fQueueHead->element_phy = QH_TERMINATE;
		} else {
			// there are still linked transfers
			fQueueTop = (uhci_td *)lastDescriptor->link_log;
			fQueueHead->element_phy = fQueueTop->this_phy & TD_LINK_MASK;
		}
	} else {
		// unlink the chain
		uhci_td *element = fQueueTop;
		while (element) {
			if (element->link_log == firstDescriptor) {
				element->link_log = lastDescriptor->link_log;
				element->link_phy = lastDescriptor->link_phy;
				break;
			}

			element = (uhci_td *)element->link_log;
		}

		element = firstDescriptor;
		while (element && element != lastDescriptor) {
			if ((fQueueHead->element_phy & TD_LINK_MASK) == element->this_phy) {
				fQueueHead->element_phy = lastDescriptor->link_phy;
				break;
			}

			element = (uhci_td *)element->link_log;
		}
	}

	lastDescriptor->link_log = NULL;
	lastDescriptor->link_phy = TD_TERMINATE;

#ifdef TRACE_UHCI
	print_descriptor_chain(firstDescriptor);
#endif

	Unlock();
	return B_OK;
}


addr_t
Queue::PhysicalAddress()
{
	return fQueueHead->this_phy;
}


void
Queue::PrintToStream()
{
#ifdef TRACE_UHCI
	dprintf("USB UHCI Queue:\n");
	dprintf("link phy: 0x%08x; link type: %s; terminate: %s\n", fQueueHead->link_phy & 0xfff0, fQueueHead->link_phy & 0x0002 ? "QH" : "TD", fQueueHead->link_phy & 0x0001 ? "yes" : "no");
	dprintf("elem phy: 0x%08x; elem type: %s; terminate: %s\n", fQueueHead->element_phy & 0xfff0, fQueueHead->element_phy & 0x0002 ? "QH" : "TD", fQueueHead->element_phy & 0x0001 ? "yes" : "no");
	dprintf("elements:\n");
	print_descriptor_chain(fQueueTop);
#endif
}


//
// #pragma mark -
//


UHCI::UHCI(pci_info *info, Stack *stack)
	:	BusManager(),
		fPCIInfo(info),
		fStack(stack),
		fFrameArea(-1),
		fFrameList(NULL),
		fQueueCount(0),
		fQueues(NULL),	
		fFirstTransfer(NULL),
		fLastTransfer(NULL),
		fFinishTransfers(false),
		fFinishThread(-1),
		fStopFinishThread(false),
		fRootHub(NULL),
		fRootHubAddress(0)
{
	TRACE(("usb_uhci: constructing new UHCI Host Controller Driver\n"));
	fInitOK = false;

	if (benaphore_init(&fUHCILock, "usb uhci lock") < B_OK) {
		TRACE_ERROR(("usb_uhci: failed to create busmanager lock\n"));
		return;
	}

	fRegisterBase = sPCIModule->read_pci_config(fPCIInfo->bus,
		fPCIInfo->device, fPCIInfo->function, PCI_memory_base, 4);
	fRegisterBase &= PCI_address_io_mask;
	TRACE(("usb_uhci: iospace offset: 0x%08x\n", fRegisterBase));

	//fRegisterBase = fPCIInfo->u.h0.base_registers[0];
	//TRACE(("usb_uhci: register base: 0x%08x\n", fRegisterBase));

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
	if (ControllerReset() < B_OK) {
		TRACE_ERROR(("usb_uhci: host failed to reset\n"));
		return;
	}

	// Setup the frame list
	void *physicalAddress;
	fFrameArea = fStack->AllocateArea((void **)&fFrameList,
		(void **)&physicalAddress, 4096, "USB UHCI framelist");

	if (fFrameArea < B_OK) {
		TRACE_ERROR(("usb_uhci: unable to create an area for the frame pointer list\n"));
		return;
	}

	// Set base pointer and reset frame number
	WriteReg32(UHCI_FRBASEADD, (uint32)physicalAddress);	
	WriteReg16(UHCI_FRNUM, 0);

	fQueueCount = 4;
	fQueues = new(std::nothrow) Queue *[fQueueCount];
	if (!fQueues) {
		delete_area(fFrameArea);
		return;
	}

	for (int32 i = 0; i < fQueueCount; i++) {
		fQueues[i] = new(std::nothrow) Queue(fStack);
		if (!fQueues[i] || fQueues[i]->InitCheck() < B_OK) {
			TRACE_ERROR(("usb_uhci: cannot create queues\n"));
			delete_area(fFrameArea);
			return;
		}

		if (i > 0)
			fQueues[i - 1]->LinkTo(fQueues[i]);
	}

	// Make sure the last queue terminates
	fQueues[fQueueCount - 1]->TerminateByStrayDescriptor();

	for (int32 i = 0; i < 1024; i++)
		fFrameList[i] = fQueues[0]->PhysicalAddress() | FRAMELIST_NEXT_IS_QH;

	// Create the finisher service thread
	fFinishThread = spawn_kernel_thread(FinishThread,
		"uhci finish thread", B_NORMAL_PRIORITY, (void *)this);
	resume_thread(fFinishThread);

	// Install the interrupt handler
	TRACE(("usb_uhci: installing interrupt handler\n"));
	install_io_interrupt_handler(fPCIInfo->u.h0.interrupt_line,
		InterruptHandler, (void *)this, 0);

	// Enable interrupts
	WriteReg16(UHCI_USBINTR, UHCI_USBINTR_CRC | UHCI_USBINTR_RESUME
		| UHCI_USBINTR_IOC | UHCI_USBINTR_SHORT);

	TRACE(("usb_uhci: UHCI Host Controller Driver constructed\n"));
	fInitOK = true;
}


UHCI::~UHCI()
{
	int32 result = 0;
	fStopFinishThread = true;
	wait_for_thread(fFinishThread, &result);

	Lock();
	transfer_data *transfer = fFirstTransfer;
	while (transfer) {
		transfer_data *next = transfer->link;
		delete transfer;
		transfer = next;
	}

	for (int32 i = 0; i < fQueueCount; i++)
		delete fQueues[i];

	delete [] fQueues;
	delete fRootHub;
	delete_area(fFrameArea);
	benaphore_destroy(&fUHCILock);

	put_module(B_PCI_MODULE_NAME);
}


bool
UHCI::Lock()
{
	return (benaphore_lock(&fUHCILock) == B_OK);
}


void
UHCI::Unlock()
{
	benaphore_unlock(&fUHCILock);
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
		TRACE_ERROR(("usb_uhci: controller won't start running\n"));
		return B_ERROR;
	}

	fPortResetChange[0] = fPortResetChange[1] = false;
	fRootHubAddress = AllocateAddress();
	fRootHub = new(std::nothrow) UHCIRootHub(this, fRootHubAddress);
	if (!fRootHub) {
		TRACE_ERROR(("usb_uhci: no memory to allocate root hub\n"));
		return B_NO_MEMORY;
	}

	if (fRootHub->InitCheck() < B_OK) {
		TRACE_ERROR(("usb_uhci: root hub failed init check\n"));
		delete fRootHub;
		return B_ERROR;
	}

	SetRootHub(fRootHub);
	SetStack(fStack);

	TRACE(("usb_uhci: controller is started. status: %u curframe: %u\n",
		ReadReg16(UHCI_USBSTS), ReadReg16(UHCI_FRNUM)));
	return BusManager::Start();
}


status_t
UHCI::SubmitTransfer(Transfer *transfer)
{
	//TRACE(("usb_uhci: submit transfer called for device %d\n", transfer->TransferPipe()->DeviceAddress()));

	// Short circuit the root hub
	if (transfer->TransferPipe()->DeviceAddress() == fRootHubAddress)
		return fRootHub->SubmitTransfer(transfer);

	if (transfer->TransferPipe()->Type() == Pipe::Control)
		return SubmitRequest(transfer);

	if (!transfer->Data() || transfer->DataLength() == 0)
		return B_BAD_VALUE;

	Pipe *pipe = transfer->TransferPipe();
	bool directionIn = (pipe->Direction() == Pipe::In);

	uhci_td *firstDescriptor = NULL;
	uhci_td *lastDescriptor = NULL;
	status_t result = CreateDescriptorChain(pipe, &firstDescriptor,
		&lastDescriptor, directionIn ? TD_TOKEN_IN : TD_TOKEN_OUT,
		transfer->DataLength());

	if (result < B_OK)
		return result;
	if (!firstDescriptor || !lastDescriptor)
		return B_NO_MEMORY;

	lastDescriptor->status |= TD_CONTROL_IOC;
	lastDescriptor->link_phy = TD_TERMINATE;
	lastDescriptor->link_log = 0;

	if (!directionIn) {
		WriteDescriptorChain(firstDescriptor, transfer->Data(),
			transfer->DataLength());
	}

	Queue *queue = fQueues[3];
	if (pipe->Type() == Pipe::Interrupt)
		queue = fQueues[2];

	result = AddPendingTransfer(transfer, queue, firstDescriptor,
		firstDescriptor, lastDescriptor, directionIn);
	if (result < B_OK) {
		TRACE_ERROR(("usb_uhci: failed to add pending transfer\n"));
		FreeDescriptorChain(firstDescriptor);
		return result;
	}

	result = queue->AppendDescriptorChain(firstDescriptor);
	if (result < B_OK) {
		TRACE_ERROR(("usb_uhci: failed to append descriptor chain\n"));
		FreeDescriptorChain(firstDescriptor);
		return result;
	}

	return EINPROGRESS;
}


status_t
UHCI::SubmitRequest(Transfer *transfer)
{
	Pipe *pipe = transfer->TransferPipe();
	usb_request_data *requestData = transfer->RequestData();
	bool directionIn = (requestData->RequestType & USB_REQTYPE_DEVICE_IN) > 0;

	uhci_td *setupDescriptor = CreateDescriptor(pipe, TD_TOKEN_SETUP,
		sizeof(usb_request_data));

	uhci_td *statusDescriptor = CreateDescriptor(pipe,
		directionIn ? TD_TOKEN_OUT : TD_TOKEN_IN, 0);

	if (!setupDescriptor || !statusDescriptor) {
		TRACE_ERROR(("usb_uhci: failed to allocate descriptors\n"));
		FreeDescriptor(setupDescriptor);
		FreeDescriptor(statusDescriptor);
		return B_NO_MEMORY;
	}

	WriteDescriptorChain(setupDescriptor, (const uint8 *)requestData,
		sizeof(usb_request_data));

	statusDescriptor->status |= TD_CONTROL_IOC;
	statusDescriptor->token |= TD_TOKEN_DATA1;
	statusDescriptor->link_phy = TD_TERMINATE;
	statusDescriptor->link_log = NULL;

	uhci_td *dataDescriptor = NULL;
	if (transfer->Data() && transfer->DataLength() > 0) {
		uhci_td *lastDescriptor = NULL;
		status_t result = CreateDescriptorChain(pipe, &dataDescriptor,
			&lastDescriptor, directionIn ? TD_TOKEN_IN : TD_TOKEN_OUT,
			transfer->DataLength());

		if (result < B_OK) {
			FreeDescriptor(setupDescriptor);
			FreeDescriptor(statusDescriptor);
			return result;
		}

		if (!directionIn) {
			WriteDescriptorChain(dataDescriptor, transfer->Data(),
				transfer->DataLength());
		}

		LinkDescriptors(setupDescriptor, dataDescriptor);
		LinkDescriptors(lastDescriptor, statusDescriptor);
	} else {
		// Link transfer and status descriptors directly
		LinkDescriptors(setupDescriptor, statusDescriptor);
	}

	status_t result = AddPendingTransfer(transfer, fQueues[1], setupDescriptor,
		dataDescriptor, statusDescriptor, directionIn);
	if (result < B_OK) {
		TRACE_ERROR(("usb_uhci: failed to add pending transfer\n"));
		FreeDescriptorChain(setupDescriptor);
		return result;
	}

	result = fQueues[1]->AppendDescriptorChain(setupDescriptor);
	if (result < B_OK) {
		TRACE_ERROR(("usb_uhci: failed to append descriptor chain\n"));
		FreeDescriptorChain(setupDescriptor);
		return result;
	}

	return EINPROGRESS;
}


status_t
UHCI::AddPendingTransfer(Transfer *transfer, Queue *queue,
	uhci_td *firstDescriptor, uhci_td *dataDescriptor, uhci_td *lastDescriptor,
	bool directionIn)
{
	transfer_data *data = new(std::nothrow) transfer_data();
	if (!data)
		return B_NO_MEMORY;

	data->transfer = transfer;
	data->queue = queue;
	data->first_descriptor = firstDescriptor;
	data->data_descriptor = dataDescriptor;
	data->last_descriptor = lastDescriptor;
	data->incoming = directionIn;
	data->link = NULL;

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


int32
UHCI::FinishThread(void *data)
{
	((UHCI *)data)->FinishTransfers();
	return B_OK;
}


void
UHCI::FinishTransfers()
{
	while (!fStopFinishThread) {
		while (!fFinishTransfers) {
			if (fStopFinishThread)
				return;

			snooze(1000);
		}

		fFinishTransfers = false;
		if (!Lock())
			continue;

		TRACE(("usb_uhci: finishing transfers (first transfer: 0x%08x; last transfer: 0x%08x)\n", fFirstTransfer, fLastTransfer));
		transfer_data *lastTransfer = NULL;
		transfer_data *transfer = fFirstTransfer;
		Unlock();

		while (transfer) {
			bool transferDone = false;
			uhci_td *descriptor = transfer->first_descriptor;

			while (descriptor) {
				uint32 status = descriptor->status;
				if (status & TD_STATUS_ACTIVE) {
					TRACE(("usb_uhci: td (0x%08x) still active\n", descriptor->this_phy));
					// still in progress
					break;
				}

				if (status & TD_ERROR_MASK) {
					TRACE_ERROR(("usb_uhci: td (0x%08x) error: 0x%08x\n", descriptor->this_phy, status));
					// an error occured. we have to remove the
					// transfer from the queue and clean up

					uint32 callbackStatus = 0;
					if (status & TD_STATUS_ERROR_STALLED)
						callbackStatus |= B_USB_STATUS_DEVICE_STALLED;
					if (status & TD_STATUS_ERROR_TIMEOUT) {
						if (transfer->incoming)
							callbackStatus |= B_USB_STATUS_DEVICE_CRC_ERROR;
						else
							callbackStatus |= B_USB_STATUS_DEVICE_TIMEOUT;
					}

					transfer->queue->RemoveDescriptorChain(
						transfer->first_descriptor,
						transfer->last_descriptor);

					FreeDescriptorChain(transfer->first_descriptor);
					transfer->transfer->Finished(callbackStatus, 0);
					transferDone = true;
					break;
				}

				// either all descriptors are done, or we have a short packet
				if (descriptor == transfer->last_descriptor
					|| (descriptor->status & TD_STATUS_ACTLEN_MASK)
					< (descriptor->token >> TD_TOKEN_MAXLEN_SHIFT)) {
					TRACE(("usb_uhci: td (0x%08x) ok\n", descriptor->this_phy));
					// we got through without errors so we are finished
					transfer->queue->RemoveDescriptorChain(
						transfer->first_descriptor,
						transfer->last_descriptor);

					size_t actualLength = 0;
					uint8 lastDataToggle = 0;
					if (transfer->data_descriptor && transfer->incoming) {
						// data to read out
						actualLength = ReadDescriptorChain(
							transfer->data_descriptor,
							transfer->transfer->Data(),
							transfer->transfer->DataLength(),
							&lastDataToggle);
					} else {
						// read the actual length that was sent
						actualLength = ReadActualLength(
							transfer->first_descriptor, &lastDataToggle);
					}

					FreeDescriptorChain(transfer->first_descriptor);
					transfer->transfer->TransferPipe()->SetDataToggle(lastDataToggle == 0);
					transfer->transfer->Finished(B_USB_STATUS_SUCCESS, actualLength);
					transferDone = true;
					break;
				}

				descriptor = (uhci_td *)descriptor->link_log;
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
					delete transfer;
					transfer = next;

					Unlock();
				}
			} else {
				lastTransfer = transfer;
				transfer = transfer->link;
			}
		}

		Unlock();
	}
}


void
UHCI::GlobalReset()
{
	uint8 sofValue = ReadReg8(UHCI_SOFMOD);

	WriteReg16(UHCI_USBCMD, UHCI_USBCMD_GRESET);
	snooze(100000);
	WriteReg16(UHCI_USBCMD, 0);
	snooze(10000);

	WriteReg8(UHCI_SOFMOD, sofValue);
}


status_t
UHCI::ControllerReset()
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

	SetPortResetChange(index, true);
	TRACE(("usb_uhci: port was reset: 0x%04x\n", ReadReg16(port)));
	return B_OK;
}


bool
UHCI::PortResetChange(int32 index)
{
	if (index > 1)
		return false;

	return fPortResetChange[index];
}


void
UHCI::SetPortResetChange(int32 index, bool value)
{
	if (index > 1)
		return;

	fPortResetChange[index] = value;
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

	fFinishTransfers = true;
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
			TRACE_ERROR(("usb_uhci: AddTo(): getting pci module failed! 0x%08x\n",
				status));
			return status;
		}
	}

	TRACE(("usb_uhci: AddTo(): setting up hardware\n"));

	bool found = false;
	pci_info *item = new(std::nothrow) pci_info;
	if (!item) {
		sPCIModule = NULL;
		put_module(B_PCI_MODULE_NAME);
		return B_NO_MEMORY;
	}

	for (int32 i = 0; sPCIModule->get_nth_pci_info(i, item) >= B_OK; i++) {
		//class_base = 0C (serial bus) class_sub = 03 (usb) prog_int: 00 (UHCI)
		if (item->class_base == 0x0C && item->class_sub == 0x03
			&& item->class_api == 0x00) {
			if (item->u.h0.interrupt_line == 0
				|| item->u.h0.interrupt_line == 0xFF) {
				TRACE_ERROR(("usb_uhci: AddTo(): found with invalid IRQ - check IRQ assignement\n"));
				continue;
			}

			TRACE(("usb_uhci: AddTo(): found at IRQ %u\n", item->u.h0.interrupt_line));
			UHCI *bus = new(std::nothrow) UHCI(item, &stack);
			if (!bus) {
				delete item;
				sPCIModule = NULL;
				put_module(B_PCI_MODULE_NAME);
				return B_NO_MEMORY;
			}

			if (bus->InitCheck() < B_OK) {
				TRACE_ERROR(("usb_uhci: AddTo(): InitCheck() failed 0x%08x\n", bus->InitCheck()));
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


uhci_td *
UHCI::CreateDescriptor(Pipe *pipe, uint8 direction, int32 bufferSize)
{
	uhci_td *result;
	void *physicalAddress;

	if (fStack->AllocateChunk((void **)&result, &physicalAddress, 32) < B_OK) {
		TRACE_ERROR(("usb_uhci: failed to allocate a transfer descriptor\n"));
		return NULL;
	}

	result->this_phy = (addr_t)physicalAddress;
	result->status = TD_STATUS_ACTIVE | TD_CONTROL_3_ERRORS | TD_CONTROL_SPD;
	if (pipe->Speed() == Pipe::LowSpeed)
		result->status |= TD_CONTROL_LOWSPEED;

	result->buffer_size = bufferSize;
	if (bufferSize == 0)
		result->token = TD_TOKEN_NULL_DATA;
	else
		result->token = (bufferSize - 1) << TD_TOKEN_MAXLEN_SHIFT;

	result->token |= (pipe->EndpointAddress() << TD_TOKEN_ENDPTADDR_SHIFT)
		| (pipe->DeviceAddress() << 8) | direction;

	result->link_phy = 0;
	result->link_log = NULL;
	if (bufferSize <= 0) {
		result->buffer_log = NULL;
		result->buffer_phy = NULL;
		return result;
	}

	if (fStack->AllocateChunk(&result->buffer_log, &result->buffer_phy,
		bufferSize) < B_OK) {
		TRACE_ERROR(("usb_uhci: unable to allocate space for the buffer\n"));
		fStack->FreeChunk(result, (void *)result->this_phy, 32);
		return NULL;
	}

	return result;
}


status_t
UHCI::CreateDescriptorChain(Pipe *pipe, uhci_td **_firstDescriptor,
	uhci_td **_lastDescriptor, uint8 direction, int32 bufferSize)
{
	int32 packetSize = pipe->MaxPacketSize();
	int32 descriptorCount = (bufferSize + packetSize - 1) / packetSize;

	bool dataToggle = pipe->DataToggle();
	uhci_td *firstDescriptor = NULL;
	uhci_td *lastDescriptor = *_firstDescriptor;
	for (int32 i = 0; i < descriptorCount; i++) {
		uhci_td *descriptor = CreateDescriptor(pipe, direction,
			min_c(packetSize, bufferSize));

		if (!descriptor) {
			FreeDescriptorChain(firstDescriptor);
			return B_NO_MEMORY;
		}

		if (dataToggle)
			descriptor->token |= TD_TOKEN_DATA1;

		// link to previous
		if (lastDescriptor)
			LinkDescriptors(lastDescriptor, descriptor);

		dataToggle = !dataToggle;
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
UHCI::FreeDescriptor(uhci_td *descriptor)
{
	if (!descriptor)
		return;

	if (descriptor->buffer_log) {
		fStack->FreeChunk(descriptor->buffer_log,
			(void *)descriptor->buffer_phy, descriptor->buffer_size);
	}

	fStack->FreeChunk(descriptor, (void *)descriptor->this_phy, 32);
}


void
UHCI::FreeDescriptorChain(uhci_td *topDescriptor)
{
	uhci_td *current = topDescriptor;
	uhci_td *next = NULL;

	while (current) {
		next = (uhci_td *)current->link_log;
		FreeDescriptor(current);
		current = next;
	}
}


void
UHCI::LinkDescriptors(uhci_td *first, uhci_td *second)
{
	first->link_phy = second->this_phy | TD_DEPTH_FIRST;
	first->link_log = second;
}


size_t
UHCI::WriteDescriptorChain(uhci_td *topDescriptor, const uint8 *buffer,
	size_t bufferLength)
{
	size_t actualLength = 0;
	uhci_td *current = topDescriptor;

	while (current) {
		if (!current->buffer_log)
			break;

		size_t length = min_c(current->buffer_size, bufferLength);
		memcpy(current->buffer_log, buffer, length);

		bufferLength -= length;
		actualLength += length;
		buffer += length;

		if (current->link_phy & TD_TERMINATE)
			break;

		current = (uhci_td *)current->link_log;
	}

	TRACE(("usb_uhci: wrote descriptor chain (%d bytes)\n", actualLength));
	return actualLength;
}


size_t
UHCI::ReadDescriptorChain(uhci_td *topDescriptor, uint8 *buffer,
	size_t bufferLength, uint8 *lastDataToggle)
{
	size_t actualLength = 0;
	uhci_td *current = topDescriptor;
	uint8 dataToggle = 0;

	while (current && (current->status & TD_STATUS_ACTIVE) == 0) {
		if (!current->buffer_log)
			break;

		size_t length = (current->status & TD_STATUS_ACTLEN_MASK) + 1;
		if (length == TD_STATUS_ACTLEN_NULL + 1)
			length = 0;

		length = min_c(length, bufferLength);
		memcpy(buffer, current->buffer_log, length);

		buffer += length;
		bufferLength -= length;
		actualLength += length;
		dataToggle = (current->token >> TD_TOKEN_DATA_TOGGLE_SHIFT) & 0x01;

		if (current->link_phy & TD_TERMINATE)
			break;

		current = (uhci_td *)current->link_log;
	}

	if (lastDataToggle)
		*lastDataToggle = dataToggle;

	TRACE(("usb_uhci: read descriptor chain (%d bytes)\n", actualLength));
	return actualLength;
}


size_t
UHCI::ReadActualLength(uhci_td *topDescriptor, uint8 *lastDataToggle)
{
	size_t actualLength = 0;
	uhci_td *current = topDescriptor;
	uint8 dataToggle = 0;

	while (current && (current->status & TD_STATUS_ACTIVE) == 0) {
		size_t length = (current->status & TD_STATUS_ACTLEN_MASK) + 1;
		if (length == TD_STATUS_ACTLEN_NULL + 1)
			length = 0;

		actualLength += length;
		dataToggle = (current->token >> TD_TOKEN_DATA_TOGGLE_SHIFT) & 0x01;

		if (current->link_phy & TD_TERMINATE)
			break;

		current = (uhci_td *)current->link_log;
	}

	if (lastDataToggle)
		*lastDataToggle = dataToggle;

	TRACE(("usb_uhci: read actual length (%d bytes)\n", actualLength));
	return actualLength;
}


inline void
UHCI::WriteReg8(uint32 reg, uint8 value)
{
	sPCIModule->write_io_8(fRegisterBase + reg, value);
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


inline uint8
UHCI::ReadReg8(uint32 reg)
{
	return sPCIModule->read_io_8(fRegisterBase + reg);
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
