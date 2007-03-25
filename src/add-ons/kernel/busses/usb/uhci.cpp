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
#include <USB3.h>
#include <KernelExport.h>
#include <stdlib.h>

#include "uhci.h"
#include "uhci_hardware.h"
#include "usb_p.h"


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
		"busses/usb/uhci",
		0,
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


#ifdef TRACE_USB

void
print_descriptor_chain(uhci_td *descriptor)
{
	while (descriptor) {
		dprintf("ph: 0x%08lx; lp: 0x%08lx; vf: %s; q: %s; t: %s; st: 0x%08lx; to: 0x%08lx\n",
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

#endif // TRACE_USB


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
	fStatus = fStack->AllocateChunk((void **)&fQueueHead, &physicalAddress,
		sizeof(uhci_qh));
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

	fStack->FreeChunk(fQueueHead, (void *)fQueueHead->this_phy, sizeof(uhci_qh));

	if (fStrayDescriptor)
		fStack->FreeChunk(fStrayDescriptor, (void *)fStrayDescriptor->this_phy,
			sizeof(uhci_td));
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
		&physicalAddress, sizeof(uhci_td));
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
			sizeof(uhci_td));
		return B_ERROR;
	}

	fQueueHead->link_phy = fStrayDescriptor->this_phy;
	fQueueHead->link_log = fStrayDescriptor;
	Unlock();

	return B_OK;
}


status_t
Queue::AppendTransfer(uhci_qh *transfer)
{
	if (!Lock())
		return B_ERROR;

	transfer->link_log = NULL;
	transfer->link_phy = fQueueHead->link_phy;

	if (!fQueueTop) {
		// the list is empty, make this the first element
		fQueueTop = transfer;
		fQueueHead->element_phy = transfer->this_phy | QH_NEXT_IS_QH;
	} else {
		// append the transfer queue to the list
		uhci_qh *element = fQueueTop;
		while (element && element->link_log)
			element = (uhci_qh *)element->link_log;

		element->link_log = transfer;
		element->link_phy = transfer->this_phy | QH_NEXT_IS_QH;
	}

	Unlock();
	return B_OK;
}


status_t
Queue::RemoveTransfer(uhci_qh *transfer)
{
	if (!Lock())
		return B_ERROR;

	if (fQueueTop == transfer) {
		// this was the top element
		fQueueTop = (uhci_qh *)transfer->link_log;
		if (!fQueueTop) {
			// this was the only element, terminate this queue
			fQueueHead->element_phy = QH_TERMINATE;
		} else {
			// there are elements left, adjust the element pointer
			fQueueHead->element_phy = transfer->link_phy;
		}

		Unlock();
		return B_OK;
	} else {
		uhci_qh *element = fQueueTop;
		while (element) {
			if (element->link_log == transfer) {
				element->link_log = transfer->link_log;
				element->link_phy = transfer->link_phy;
				Unlock();
				return B_OK;
			}

			element = (uhci_qh *)element->link_log;
		}
	}

	Unlock();
	return B_BAD_VALUE;
}


addr_t
Queue::PhysicalAddress()
{
	return fQueueHead->this_phy;
}


void
Queue::PrintToStream()
{
#ifdef TRACE_USB
	dprintf("USB UHCI Queue:\n");
	dprintf("link phy: 0x%08lx; link type: %s; terminate: %s\n", fQueueHead->link_phy & 0xfff0, fQueueHead->link_phy & 0x0002 ? "QH" : "TD", fQueueHead->link_phy & 0x0001 ? "yes" : "no");
	dprintf("elem phy: 0x%08lx; elem type: %s; terminate: %s\n", fQueueHead->element_phy & 0xfff0, fQueueHead->element_phy & 0x0002 ? "QH" : "TD", fQueueHead->element_phy & 0x0001 ? "yes" : "no");
#endif
}


//
// #pragma mark -
//


UHCI::UHCI(pci_info *info, Stack *stack)
	:	BusManager(stack),
		fPCIInfo(info),
		fStack(stack),
		fFrameArea(-1),
		fFrameList(NULL),
		fQueueCount(0),
		fQueues(NULL),	
		fFirstTransfer(NULL),
		fLastTransfer(NULL),
		fFinishThread(-1),
		fStopFinishThread(false),
		fRootHub(NULL),
		fRootHubAddress(0),
		fPortResetChange(0)
{
	if (!fInitOK) {
		TRACE_ERROR(("usb_uhci: bus manager failed to init\n"));
		return;
	}

	TRACE(("usb_uhci: constructing new UHCI Host Controller Driver\n"));
	fInitOK = false;

	fRegisterBase = sPCIModule->read_pci_config(fPCIInfo->bus,
		fPCIInfo->device, fPCIInfo->function, PCI_memory_base, 4);
	fRegisterBase &= PCI_address_io_mask;
	TRACE_ERROR(("usb_uhci: iospace offset: 0x%08lx\n", fRegisterBase));

	if (fRegisterBase == 0) {
		fRegisterBase = fPCIInfo->u.h0.base_registers[0];
		TRACE_ERROR(("usb_uhci: register base: 0x%08lx\n", fRegisterBase));
	}

	// enable pci address access
	uint16 command = PCI_command_io | PCI_command_master | PCI_command_memory;
	command |= sPCIModule->read_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2);

	sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2, command);

	// make sure we gain control of the UHCI controller instead of the BIOS
	sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_LEGSUP, 2, PCI_LEGSUP_USBPIRQDEN);

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

	// Set the max packet size for bandwidth reclamation to 64 bytes
	WriteReg16(UHCI_USBCMD, ReadReg16(UHCI_USBCMD) | UHCI_USBCMD_MAXP);

	// we will create four queues:
	// 0: interrupt transfers
	// 1: low speed control transfers
	// 2: full speed control transfers
	// 3: bulk transfers
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

	// create semaphore the finisher thread will wait for
	fFinishTransfersSem = create_sem(0, "UHCI Finish Transfers");
	if (fFinishTransfersSem < B_OK) {
		TRACE_ERROR(("usb_uhci: failed to create semaphore\n"));
		return;
	}

	// Create the finisher service thread
	fFinishThread = spawn_kernel_thread(FinishThread,
		"uhci finish thread", B_URGENT_DISPLAY_PRIORITY, (void *)this);
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
	delete_sem(fFinishTransfersSem);
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

	put_module(B_PCI_MODULE_NAME);
	Unlock();
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
		TRACE(("usb_uhci: current loop %ld, status 0x%04x\n", i, status));

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

	fRootHubAddress = AllocateAddress();
	fRootHub = new(std::nothrow) UHCIRootHub(RootObject(), fRootHubAddress);
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

	TRACE(("usb_uhci: controller is started. status: %u curframe: %u\n",
		ReadReg16(UHCI_USBSTS), ReadReg16(UHCI_FRNUM)));
	return BusManager::Start();
}


status_t
UHCI::SubmitTransfer(Transfer *transfer)
{
	// Short circuit the root hub
	if (transfer->TransferPipe()->DeviceAddress() == fRootHubAddress)
		return fRootHub->ProcessTransfer(this, transfer);

	TRACE(("usb_uhci: submit transfer called for device %d\n", transfer->TransferPipe()->DeviceAddress()));
	if (transfer->TransferPipe()->Type() & USB_OBJECT_CONTROL_PIPE)
		return SubmitRequest(transfer);

	uhci_td *firstDescriptor = NULL;
	uhci_qh *transferQueue = NULL;
	status_t result = CreateFilledTransfer(transfer, &firstDescriptor,
		&transferQueue);
	if (result < B_OK)
		return result;

	Queue *queue = NULL;
	Pipe *pipe = transfer->TransferPipe();
	if (pipe->Type() & USB_OBJECT_INTERRUPT_PIPE) {
		// use interrupt queue
		queue = fQueues[0];
	} else {
		// use bulk queue
		queue = fQueues[3];
	}

	bool directionIn = (pipe->Direction() == Pipe::In);
	result = AddPendingTransfer(transfer, queue, transferQueue,
		firstDescriptor, firstDescriptor, directionIn);
	if (result < B_OK) {
		TRACE_ERROR(("usb_uhci: failed to add pending transfer\n"));
		FreeDescriptorChain(firstDescriptor);
		FreeTransferQueue(transferQueue);
		return result;
	}

	queue->AppendTransfer(transferQueue);
	return B_OK;
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

	iovec vector;
	vector.iov_base = requestData;
	vector.iov_len = sizeof(usb_request_data);
	WriteDescriptorChain(setupDescriptor, &vector, 1);

	statusDescriptor->status |= TD_CONTROL_IOC;
	statusDescriptor->token |= TD_TOKEN_DATA1;
	statusDescriptor->link_phy = TD_TERMINATE;
	statusDescriptor->link_log = NULL;

	uhci_td *dataDescriptor = NULL;
	if (transfer->VectorCount() > 0) {
		uhci_td *lastDescriptor = NULL;
		status_t result = CreateDescriptorChain(pipe, &dataDescriptor,
			&lastDescriptor, directionIn ? TD_TOKEN_IN : TD_TOKEN_OUT,
			transfer->VectorLength());

		if (result < B_OK) {
			FreeDescriptor(setupDescriptor);
			FreeDescriptor(statusDescriptor);
			return result;
		}

		if (!directionIn) {
			WriteDescriptorChain(dataDescriptor, transfer->Vector(),
				transfer->VectorCount());
		}

		LinkDescriptors(setupDescriptor, dataDescriptor);
		LinkDescriptors(lastDescriptor, statusDescriptor);
	} else {
		// Link transfer and status descriptors directly
		LinkDescriptors(setupDescriptor, statusDescriptor);
	}

	Queue *queue = NULL;
	if (pipe->Speed() == USB_SPEED_LOWSPEED) {
		// use the low speed control queue
		queue = fQueues[1];
	} else {
		// use the full speed control queue
		queue = fQueues[2];
	}

	uhci_qh *transferQueue = CreateTransferQueue(setupDescriptor);
	status_t result = AddPendingTransfer(transfer, queue, transferQueue,
		setupDescriptor, dataDescriptor, directionIn);
	if (result < B_OK) {
		TRACE_ERROR(("usb_uhci: failed to add pending transfer\n"));
		FreeDescriptorChain(setupDescriptor);
		FreeTransferQueue(transferQueue);
		return result;
	}

	queue->AppendTransfer(transferQueue);
	return B_OK;
}


status_t
UHCI::AddPendingTransfer(Transfer *transfer, Queue *queue,
	uhci_qh *transferQueue, uhci_td *firstDescriptor, uhci_td *dataDescriptor,
	bool directionIn)
{
	if (!transfer || !queue || !transferQueue || !firstDescriptor)
		return B_BAD_VALUE;

	transfer_data *data = new(std::nothrow) transfer_data();
	if (!data)
		return B_NO_MEMORY;

	data->transfer = transfer;
	data->queue = queue;
	data->transfer_queue = transferQueue;
	data->first_descriptor = firstDescriptor;
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
					TRACE_ERROR(("usb_uhci: failed to get area of userspace buffer\n"));
					delete data;
					return B_BAD_ADDRESS;
				}

				break;
			}
		}

		if (data->user_area >= B_OK) {
			area_info areaInfo;
			if (get_area_info(data->user_area, &areaInfo) < B_OK) {
				TRACE_ERROR(("usb_uhci: failed to get info about user area\n"));
				delete data;
				return B_BAD_ADDRESS;
			}

			for (size_t i = 0; i < vectorCount; i++) {
				(uint8 *)vector[i].iov_base -= (uint8 *)areaInfo.address;

				if ((size_t)vector[i].iov_base > areaInfo.size
					|| (size_t)vector[i].iov_base + vector[i].iov_len > areaInfo.size) {
					TRACE_ERROR(("usb_uhci: output data buffer spans across multiple areas!\n"));
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
	if (!fFirstTransfer)
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
		if (acquire_sem(fFinishTransfersSem) < B_OK)
			continue;

		// eat up sems that have been released by multiple interrupts
		int32 semCount = 0;
		get_sem_count(fFinishTransfersSem, &semCount);
		if (semCount > 0)
			acquire_sem_etc(fFinishTransfersSem, semCount, B_RELATIVE_TIMEOUT, 0);

		if (!Lock())
			continue;

		TRACE(("usb_uhci: finishing transfers (first transfer: 0x%08lx; last transfer: 0x%08lx)\n", (uint32)fFirstTransfer, (uint32)fLastTransfer));
		transfer_data *lastTransfer = NULL;
		transfer_data *transfer = fFirstTransfer;
		Unlock();

		while (transfer) {
			bool transferDone = false;
			uhci_td *descriptor = transfer->first_descriptor;

			while (descriptor) {
				uint32 status = descriptor->status;
				if (status & TD_STATUS_ACTIVE) {
					TRACE(("usb_uhci: td (0x%08lx) still active\n", descriptor->this_phy));
					// still in progress
					break;
				}

				if (status & TD_ERROR_MASK) {
					TRACE_ERROR(("usb_uhci: td (0x%08lx) error: status: 0x%08lx; token: 0x%08lx;\n", descriptor->this_phy, status, descriptor->token));
					// an error occured. we have to remove the
					// transfer from the queue and clean up

					status_t callbackStatus = B_ERROR;
					uint8 errorCount = status >> TD_ERROR_COUNT_SHIFT;
					errorCount &= TD_ERROR_COUNT_MASK;
					if (errorCount == 0) {
						// the error counter counted down to zero, report why
						int32 reasons = 0;
						if (status & TD_STATUS_ERROR_BUFFER) {
							callbackStatus = transfer->incoming ? B_DEV_DATA_OVERRUN : B_DEV_DATA_UNDERRUN;
							reasons++;
						}
						if (status & TD_STATUS_ERROR_TIMEOUT) {
							callbackStatus = transfer->incoming ? B_DEV_CRC_ERROR : B_TIMED_OUT;
							reasons++;
						}
						if (status & TD_STATUS_ERROR_NAK) {
							callbackStatus = B_DEV_UNEXPECTED_PID;
							reasons++;
						}
						if (status & TD_STATUS_ERROR_BITSTUFF) {
							callbackStatus = B_DEV_CRC_ERROR;
							reasons++;
						}

						if (reasons > 1)
							callbackStatus = B_DEV_MULTIPLE_ERRORS;
					} else if (status & TD_STATUS_ERROR_BABBLE) {
						// there is a babble condition
						callbackStatus = transfer->incoming ? B_DEV_FIFO_OVERRUN : B_DEV_FIFO_UNDERRUN;
					} else {
						// if the error counter didn't count down to zero
						// and there was no babble, then this halt was caused
						// by a stall handshake
						callbackStatus = B_DEV_STALLED;
					}

					transfer->queue->RemoveTransfer(transfer->transfer_queue);
					FreeDescriptorChain(transfer->first_descriptor);
					FreeTransferQueue(transfer->transfer_queue);
					transfer->transfer->Finished(callbackStatus, 0);
					transferDone = true;
					break;
				}

				// either all descriptors are done, or we have a short packet
				if ((descriptor->link_phy & TD_TERMINATE)
					|| (descriptor->status & TD_STATUS_ACTLEN_MASK)
					< (descriptor->token >> TD_TOKEN_MAXLEN_SHIFT)) {
					TRACE(("usb_uhci: td (0x%08lx) ok\n", descriptor->this_phy));
					// we got through without errors so we are finished
					transfer->queue->RemoveTransfer(transfer->transfer_queue);

					size_t actualLength = 0;
					uint8 lastDataToggle = 0;
					if (transfer->data_descriptor && transfer->incoming) {
						// data to read out
						iovec *vector = transfer->transfer->Vector();
						size_t vectorCount = transfer->transfer->VectorCount();

#ifndef HAIKU_TARGET_PLATFORM_HAIKU
						area_id clonedArea = -1;
						void *clonedMemory = NULL;
						if (transfer->user_area >= B_OK) {
							// we got a userspace output buffer, need to clone
							// the area for that space first and map the iovecs
							// to this cloned area.
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
							&lastDataToggle);

#ifndef HAIKU_TARGET_PLATFORM_HAIKU
						if (clonedArea >= B_OK) {
							for (size_t i = 0; i < vectorCount; i++)
								(uint8 *)vector[i].iov_base -= (addr_t)clonedMemory;
							delete_area(clonedArea);
						}
#endif // !HAIKU_TARGET_PLATFORM_HAIKU
					} else {
						// read the actual length that was sent
						actualLength = ReadActualLength(
							transfer->first_descriptor, &lastDataToggle);
					}

					transfer->transfer->TransferPipe()->SetDataToggle(lastDataToggle == 0);
					FreeDescriptorChain(transfer->first_descriptor);
					FreeTransferQueue(transfer->transfer_queue);
					if (transfer->transfer->IsFragmented()) {
						// this transfer may still have data left
						TRACE(("usb_uhci: advancing fragmented transfer\n"));
						transfer->transfer->AdvanceByFragment(actualLength);
						if (transfer->transfer->VectorLength() > 0) {
							TRACE(("usb_uhci: still %ld bytes left on transfer\n", transfer->transfer->VectorLength()));
							// resubmit the advanced transfer so the rest
							// of the buffers are transmitted over the bus
							status_t result = CreateFilledTransfer(transfer->transfer,
								&transfer->first_descriptor,
								&transfer->transfer_queue);
							if (result < B_OK) {
								transfer->transfer->Finished(result, 0);
								transferDone = true;
							};

							transfer->data_descriptor = transfer->first_descriptor;
							transfer->queue->AppendTransfer(transfer->transfer_queue);
							break;
						}

						// the transfer is done, but we already set the
						// actualLength with AdvanceByFragment()
						actualLength = 0;
					}

					transfer->transfer->Finished(B_OK, actualLength);
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
					delete transfer->transfer;
					delete transfer;
					transfer = next;
					Unlock();
				}
			} else {
				lastTransfer = transfer;
				transfer = transfer->link;
			}
		}
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


status_t
UHCI::GetPortStatus(uint8 index, usb_port_status *status)
{
	if (index > 1)
		return B_BAD_INDEX;

	status->status = status->change = 0;
	uint16 portStatus = ReadReg16(UHCI_PORTSC1 + index * 2);

	// build the status
	if (portStatus & UHCI_PORTSC_CURSTAT)
		status->status |= PORT_STATUS_CONNECTION;
	if (portStatus & UHCI_PORTSC_ENABLED)
		status->status |= PORT_STATUS_ENABLE;
	if (portStatus & UHCI_PORTSC_RESET)
		status->status |= PORT_STATUS_RESET;
	if (portStatus & UHCI_PORTSC_LOWSPEED)
		status->status |= PORT_STATUS_LOW_SPEED;

	// build the change
	if (portStatus & UHCI_PORTSC_STATCHA)
		status->change |= PORT_STATUS_CONNECTION;
	if (portStatus & UHCI_PORTSC_ENABCHA)
		status->change |= PORT_STATUS_ENABLE;

	// ToDo: work out suspended/resume

	// there are no bits to indicate reset change
	if (fPortResetChange & (1 << index))
		status->change |= PORT_STATUS_RESET;

	// the port is automagically powered on
	status->status |= PORT_STATUS_POWER;
	return B_OK;
}


status_t
UHCI::SetPortFeature(uint8 index, uint16 feature)
{
	if (index > 1)
		return B_BAD_INDEX;

	switch (feature) {
		case PORT_RESET:
			return ResetPort(index);

		case PORT_POWER:
			// the ports are automatically powered
			return B_OK;
	}

	return B_BAD_VALUE;
}


status_t
UHCI::ClearPortFeature(uint8 index, uint16 feature)
{
	if (index > 1)
		return B_BAD_INDEX;

	uint32 portRegister = UHCI_PORTSC1 + index * 2;
	uint16 portStatus = ReadReg16(portRegister) & UHCI_PORTSC_DATAMASK;

	switch (feature) {
		case C_PORT_RESET:
			fPortResetChange &= ~(1 << index);
			return B_OK;

		case C_PORT_CONNECTION:
			WriteReg16(portRegister, portStatus | UHCI_PORTSC_STATCHA);
			return B_OK;

		case C_PORT_ENABLE:
			WriteReg16(portRegister, portStatus | UHCI_PORTSC_ENABCHA);
			return B_OK;
	}

	return B_BAD_VALUE;
}


status_t
UHCI::ResetPort(uint8 index)
{
	if (index > 1)
		return B_BAD_INDEX;

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

	fPortResetChange |= (1 << index);
	TRACE(("usb_uhci: port was reset: 0x%04x\n", ReadReg16(port)));
	return B_OK;
}


int32
UHCI::InterruptHandler(void *data)
{
	return ((UHCI *)data)->Interrupt();
}


int32
UHCI::Interrupt()
{
	static spinlock lock = 0;
	acquire_spinlock(&lock);

	// Check if we really had an interrupt
	uint16 status = ReadReg16(UHCI_USBSTS);
	if ((status & UHCI_INTERRUPT_MASK) == 0) {
		release_spinlock(&lock);
		return B_UNHANDLED_INTERRUPT;
	}

	uint16 acknowledge = 0;
	bool finishTransfers = false;
	int32 result = B_HANDLED_INTERRUPT;

	if (status & UHCI_USBSTS_USBINT) {
		TRACE(("usb_uhci: transfer finished\n"));
		acknowledge |= UHCI_USBSTS_USBINT;
		result = B_INVOKE_SCHEDULER;
		finishTransfers = true;
	}

	if (status & UHCI_USBSTS_ERRINT) {
		TRACE(("usb_uhci: transfer error\n"));
		acknowledge |= UHCI_USBSTS_ERRINT;
		result = B_INVOKE_SCHEDULER;
		finishTransfers = true;
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
		// ToDo: cancel all transfers and reset the host controller
		// acknowledge not needed
	}

	if (acknowledge)
		WriteReg16(UHCI_USBSTS, acknowledge);

	release_spinlock(&lock);

	if (finishTransfers)
		release_sem_etc(fFinishTransfersSem, 1, B_DO_NOT_RESCHEDULE);

	return result;
}


status_t
UHCI::AddTo(Stack *stack)
{
#ifdef TRACE_USB
	set_dprintf_enabled(true); 
	load_driver_symbols("uhci");
#endif

	if (!sPCIModule) {
		status_t status = get_module(B_PCI_MODULE_NAME, (module_info **)&sPCIModule);
		if (status < B_OK) {
			TRACE_ERROR(("usb_uhci: AddTo(): getting pci module failed! 0x%08lx\n",
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

		if (item->class_base == PCI_serial_bus && item->class_sub == PCI_usb
			&& item->class_api == PCI_usb_uhci) {
			if (item->u.h0.interrupt_line == 0
				|| item->u.h0.interrupt_line == 0xFF) {
				TRACE_ERROR(("usb_uhci: AddTo(): found with invalid IRQ - check IRQ assignement\n"));
				continue;
			}

			TRACE(("usb_uhci: AddTo(): found at IRQ %u\n", item->u.h0.interrupt_line));
			UHCI *bus = new(std::nothrow) UHCI(item, stack);
			if (!bus) {
				delete item;
				sPCIModule = NULL;
				put_module(B_PCI_MODULE_NAME);
				return B_NO_MEMORY;
			}

			if (bus->InitCheck() < B_OK) {
				TRACE_ERROR(("usb_uhci: AddTo(): InitCheck() failed 0x%08lx\n", bus->InitCheck()));
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
		TRACE_ERROR(("usb_uhci: no devices found\n"));
		delete item;
		sPCIModule = NULL;
		put_module(B_PCI_MODULE_NAME);
		return ENODEV;
	}

	delete item;
	return B_OK;
}


status_t
UHCI::CreateFilledTransfer(Transfer *transfer, uhci_td **_firstDescriptor,
	uhci_qh **_transferQueue)
{
	Pipe *pipe = transfer->TransferPipe();
	bool directionIn = (pipe->Direction() == Pipe::In);

	uhci_td *firstDescriptor = NULL;
	uhci_td *lastDescriptor = NULL;
	status_t result = CreateDescriptorChain(pipe, &firstDescriptor,
		&lastDescriptor, directionIn ? TD_TOKEN_IN : TD_TOKEN_OUT,
		transfer->VectorLength());

	if (result < B_OK)
		return result;
	if (!firstDescriptor || !lastDescriptor)
		return B_NO_MEMORY;

	lastDescriptor->status |= TD_CONTROL_IOC;
	lastDescriptor->link_phy = TD_TERMINATE;
	lastDescriptor->link_log = 0;

	if (!directionIn) {
		WriteDescriptorChain(firstDescriptor, transfer->Vector(),
			transfer->VectorCount());
	}

	uhci_qh *transferQueue = CreateTransferQueue(firstDescriptor);
	if (!transferQueue) {
		FreeDescriptorChain(firstDescriptor);
		return B_NO_MEMORY;
	}

	*_firstDescriptor = firstDescriptor;
	*_transferQueue = transferQueue;
	return B_OK;
}


uhci_qh *
UHCI::CreateTransferQueue(uhci_td *descriptor)
{
	uhci_qh *queueHead;
	void *physicalAddress;
	if (fStack->AllocateChunk((void **)&queueHead,
		&physicalAddress, sizeof(uhci_qh)) < B_OK)
		return NULL;

	queueHead->this_phy = (addr_t)physicalAddress;
	queueHead->element_phy = descriptor->this_phy;
	return queueHead;
}


void
UHCI::FreeTransferQueue(uhci_qh *queueHead)
{
	if (!queueHead)
		return;

	fStack->FreeChunk(queueHead, (void *)queueHead->this_phy, sizeof(uhci_qh));
}


uhci_td *
UHCI::CreateDescriptor(Pipe *pipe, uint8 direction, size_t bufferSize)
{
	uhci_td *result;
	void *physicalAddress;

	if (fStack->AllocateChunk((void **)&result, &physicalAddress,
		sizeof(uhci_td)) < B_OK) {
		TRACE_ERROR(("usb_uhci: failed to allocate a transfer descriptor\n"));
		return NULL;
	}

	result->this_phy = (addr_t)physicalAddress;
	result->status = TD_STATUS_ACTIVE | TD_CONTROL_3_ERRORS;
	if (direction == TD_TOKEN_IN)
		result->status |= TD_CONTROL_SPD;
	if (pipe->Speed() == USB_SPEED_LOWSPEED)
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
		fStack->FreeChunk(result, (void *)result->this_phy, sizeof(uhci_td));
		return NULL;
	}

	return result;
}


status_t
UHCI::CreateDescriptorChain(Pipe *pipe, uhci_td **_firstDescriptor,
	uhci_td **_lastDescriptor, uint8 direction, size_t bufferSize)
{
	size_t packetSize = pipe->MaxPacketSize();
	int32 descriptorCount = (bufferSize + packetSize - 1) / packetSize;
	if (descriptorCount == 0)
		descriptorCount = 1;

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

	fStack->FreeChunk(descriptor, (void *)descriptor->this_phy, sizeof(uhci_td));
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
UHCI::WriteDescriptorChain(uhci_td *topDescriptor, iovec *vector,
	size_t vectorCount)
{
	uhci_td *current = topDescriptor;
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

			TRACE(("usb_uhci: copying %ld bytes to bufferOffset %ld from vectorOffset %ld at index %ld of %ld\n", length, bufferOffset, vectorOffset, vectorIndex, vectorCount));
			memcpy((uint8 *)current->buffer_log + bufferOffset,
				(uint8 *)vector[vectorIndex].iov_base + vectorOffset, length);

			actualLength += length;
			vectorOffset += length;
			bufferOffset += length;

			if (vectorOffset >= vector[vectorIndex].iov_len) {
				if (++vectorIndex >= vectorCount) {
					TRACE(("usb_uhci: wrote descriptor chain (%ld bytes, no more vectors)\n", actualLength));
					return actualLength;
				}

				vectorOffset = 0;
			}

			if (bufferOffset >= current->buffer_size) {
				bufferOffset = 0;
				break;
			}
		}

		if (current->link_phy & TD_TERMINATE)
			break;

		current = (uhci_td *)current->link_log;
	}

	TRACE(("usb_uhci: wrote descriptor chain (%ld bytes)\n", actualLength));
	return actualLength;
}


size_t
UHCI::ReadDescriptorChain(uhci_td *topDescriptor, iovec *vector,
	size_t vectorCount, uint8 *lastDataToggle)
{
	uint8 dataToggle = 0;
	uhci_td *current = topDescriptor;
	size_t actualLength = 0;
	size_t vectorIndex = 0;
	size_t vectorOffset = 0;
	size_t bufferOffset = 0;

	while (current && (current->status & TD_STATUS_ACTIVE) == 0) {
		if (!current->buffer_log)
			break;

		dataToggle = (current->token >> TD_TOKEN_DATA_TOGGLE_SHIFT) & 0x01;
		size_t bufferSize = (current->status & TD_STATUS_ACTLEN_MASK) + 1;
		if (bufferSize == TD_STATUS_ACTLEN_NULL + 1)
			bufferSize = 0;

		while (true) {
			size_t length = min_c(bufferSize - bufferOffset,
				vector[vectorIndex].iov_len - vectorOffset);

			TRACE(("usb_uhci: copying %ld bytes to vectorOffset %ld from bufferOffset %ld at index %ld of %ld\n", length, vectorOffset, bufferOffset, vectorIndex, vectorCount));
			memcpy((uint8 *)vector[vectorIndex].iov_base + vectorOffset,
				(uint8 *)current->buffer_log + bufferOffset, length);

			actualLength += length;
			vectorOffset += length;
			bufferOffset += length;

			if (vectorOffset >= vector[vectorIndex].iov_len) {
				if (++vectorIndex >= vectorCount) {
					TRACE(("usb_uhci: read descriptor chain (%ld bytes, no more vectors)\n", actualLength));
					if (lastDataToggle)
						*lastDataToggle = dataToggle;
					return actualLength;
				}

				vectorOffset = 0;
			}

			if (bufferOffset >= bufferSize) {
				bufferOffset = 0;
				break;
			}
		}

		if (current->link_phy & TD_TERMINATE)
			break;

		current = (uhci_td *)current->link_log;
	}

	if (lastDataToggle)
		*lastDataToggle = dataToggle;

	TRACE(("usb_uhci: read descriptor chain (%ld bytes)\n", actualLength));
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

	TRACE(("usb_uhci: read actual length (%ld bytes)\n", actualLength));
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
