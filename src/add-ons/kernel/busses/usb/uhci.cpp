/*
 * Copyright 2004-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */

#include <module.h>
#include <PCI.h>
#include <USB3.h>
#include <KernelExport.h>

#include "uhci.h"

#define USB_MODULE_NAME "uhci"

pci_module_info *UHCI::sPCIModule = NULL;

static int32 sDebuggerCommandAdded = 0;


#ifdef HAIKU_TARGET_PLATFORM_HAIKU


class DebugTransfer : public Transfer {
public:
	DebugTransfer(Pipe *pipe)
		:
		Transfer(pipe)
	{
	}

	uhci_td	*firstDescriptor;
	uhci_qh	*transferQueue;
};


/*!	The function is an evil hack to allow <tt> <kdebug>usb_keyboard </tt> to
	execute transfers.
	When invoked the first time, a new transfer is started, each time the
	function is called afterwards, it is checked whether the transfer is already
	completed. If called with argv[1] == "cancel" the function cancels a
	possibly pending transfer.
*/
static int
debug_process_transfer(int argc, char **argv)
{
	Pipe *pipe = (Pipe *)get_debug_variable("_usbPipe", 0);
	if (pipe == NULL)
		return 2;

	// check if we have a UHCI bus at all
	if (pipe->GetBusManager()->TypeName()[0] != 'u')
		return 3;

	uint8 *data = (uint8 *)get_debug_variable("_usbTransferData", 0);
	if (data == NULL)
		return 4;

	size_t length = (size_t)get_debug_variable("_usbTransferLength", 0);
	if (length == 0)
		return 5;

	static uint8 transferBuffer[sizeof(DebugTransfer)]
		__attribute__((aligned(16)));
	static DebugTransfer* transfer;

	UHCI *bus = (UHCI *)pipe->GetBusManager();

	if (argc > 1 && strcmp(argv[1], "cancel") == 0) {
		if (transfer != NULL) {
			bus->CancelDebugTransfer(transfer);
			transfer = NULL;
		}
		return 0;
	}

	if (transfer != NULL) {
		bool stillPending;
		status_t error = bus->CheckDebugTransfer(transfer, stillPending);
		if (!stillPending)
			transfer = NULL;

		return error == B_OK ? 0 : 6;
	}

	transfer = new(transferBuffer) DebugTransfer(pipe);
	transfer->SetData(data, length);

	if (bus->StartDebugTransfer(transfer) != B_OK) {
		transfer = NULL;
		return 7;
	}

	return 0;
}
#endif


static int32
uhci_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			TRACE_MODULE("init module\n");
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE_MODULE("uninit module\n");
			break;
		default:
			return EINVAL;
	}

	return B_OK;
}


usb_host_controller_info uhci_module = {
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
		dprintf("ph: 0x%08" B_PRIx32 "; lp: 0x%08" B_PRIx32 "; vf: %s; q: %s; "
			"t: %s; st: 0x%08" B_PRIx32 "; to: 0x%08" B_PRIx32 "\n",
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

	mutex_init(&fLock, "uhci queue lock");

	phys_addr_t physicalAddress;
	fStatus = fStack->AllocateChunk((void **)&fQueueHead, &physicalAddress,
		sizeof(uhci_qh));
	if (fStatus < B_OK)
		return;

	fQueueHead->this_phy = (uint32)physicalAddress;
	fQueueHead->element_phy = QH_TERMINATE;

	fStrayDescriptor = NULL;
	fQueueTop = NULL;
}


Queue::~Queue()
{
	Lock();
	mutex_destroy(&fLock);

	fStack->FreeChunk(fQueueHead, fQueueHead->this_phy, sizeof(uhci_qh));

	if (fStrayDescriptor)
		fStack->FreeChunk(fStrayDescriptor, fStrayDescriptor->this_phy,
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
	return (mutex_lock(&fLock) == B_OK);
}


void
Queue::Unlock()
{
	mutex_unlock(&fLock);
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
	phys_addr_t physicalAddress;
	status_t result = fStack->AllocateChunk((void **)&fStrayDescriptor,
		&physicalAddress, sizeof(uhci_td));
	if (result < B_OK) {
		TRACE_ERROR("failed to allocate a stray transfer descriptor\n");
		return result;
	}

	fStrayDescriptor->status = 0;
	fStrayDescriptor->this_phy = (uint32)physicalAddress;
	fStrayDescriptor->link_phy = TD_TERMINATE;
	fStrayDescriptor->link_log = NULL;
	fStrayDescriptor->buffer_phy = 0;
	fStrayDescriptor->buffer_log = NULL;
	fStrayDescriptor->buffer_size = 0;
	fStrayDescriptor->token = TD_TOKEN_NULL_DATA
		| (0x7f << TD_TOKEN_DEVADDR_SHIFT) | TD_TOKEN_IN;

	if (!Lock()) {
		fStack->FreeChunk(fStrayDescriptor, fStrayDescriptor->this_phy,
			sizeof(uhci_td));
		return B_ERROR;
	}

	fQueueHead->link_phy = fStrayDescriptor->this_phy;
	fQueueHead->link_log = fStrayDescriptor;
	Unlock();

	return B_OK;
}


status_t
Queue::AppendTransfer(uhci_qh *transfer, bool lock)
{
	if (lock && !Lock())
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

	if (lock)
		Unlock();
	return B_OK;
}


status_t
Queue::RemoveTransfer(uhci_qh *transfer, bool lock)
{
	if (lock && !Lock())
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

		if (lock)
			Unlock();
		return B_OK;
	} else {
		uhci_qh *element = fQueueTop;
		while (element) {
			if (element->link_log == transfer) {
				element->link_log = transfer->link_log;
				element->link_phy = transfer->link_phy;
				if (lock)
					Unlock();
				return B_OK;
			}

			element = (uhci_qh *)element->link_log;
		}
	}

	if (lock)
		Unlock();
	return B_BAD_VALUE;
}


uint32
Queue::PhysicalAddress()
{
	return fQueueHead->this_phy;
}


void
Queue::PrintToStream()
{
#ifdef TRACE_USB
	TRACE("queue:\n");
	dprintf("link phy: 0x%08" B_PRIx32 "; link type: %s; terminate: %s\n",
		fQueueHead->link_phy & 0xfff0,
		fQueueHead->link_phy & 0x0002 ? "QH" : "TD",
		fQueueHead->link_phy & 0x0001 ? "yes" : "no");
	dprintf("elem phy: 0x%08" B_PRIx32 "; elem type: %s; terminate: %s\n",
		fQueueHead->element_phy & 0xfff0,
		fQueueHead->element_phy & 0x0002 ? "QH" : "TD",
		fQueueHead->element_phy & 0x0001 ? "yes" : "no");
#endif
}


//
// #pragma mark -
//


UHCI::UHCI(pci_info *info, Stack *stack)
	:	BusManager(stack),
		fPCIInfo(info),
		fStack(stack),
		fEnabledInterrupts(0),
		fFrameArea(-1),
		fFrameList(NULL),
		fFrameBandwidth(NULL),
		fFirstIsochronousDescriptor(NULL),
		fLastIsochronousDescriptor(NULL),
		fQueueCount(0),
		fQueues(NULL),
		fFirstTransfer(NULL),
		fLastTransfer(NULL),
		fFinishTransfersSem(-1),
		fFinishThread(-1),
		fStopThreads(false),
		fProcessingPipe(NULL),
		fFreeList(NULL),
		fCleanupThread(-1),
		fCleanupSem(-1),
		fCleanupCount(0),
		fFirstIsochronousTransfer(NULL),
		fLastIsochronousTransfer(NULL),
		fFinishIsochronousTransfersSem(-1),
		fFinishIsochronousThread(-1),
		fRootHub(NULL),
		fRootHubAddress(0),
		fPortResetChange(0)
{
	// Create a lock for the isochronous transfer list
	mutex_init(&fIsochronousLock, "UHCI isochronous lock");

	if (!fInitOK) {
		TRACE_ERROR("bus manager failed to init\n");
		return;
	}

	TRACE("constructing new UHCI host controller driver\n");
	fInitOK = false;

	fRegisterBase = sPCIModule->read_pci_config(fPCIInfo->bus,
		fPCIInfo->device, fPCIInfo->function, PCI_memory_base, 4);
	fRegisterBase &= PCI_address_io_mask;
	TRACE("iospace offset: 0x%08" B_PRIx32 "\n", fRegisterBase);

	if (fRegisterBase == 0) {
		fRegisterBase = fPCIInfo->u.h0.base_registers[0];
		TRACE_ALWAYS("register base: 0x%08" B_PRIx32 "\n", fRegisterBase);
	}

	// enable pci address access
	uint16 command = PCI_command_io | PCI_command_master | PCI_command_memory;
	command |= sPCIModule->read_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2);

	sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2, command);

	// disable interrupts
	WriteReg16(UHCI_USBINTR, 0);

	// make sure we gain control of the UHCI controller instead of the BIOS
	sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_LEGSUP, 2, PCI_LEGSUP_USBPIRQDEN
		| PCI_LEGSUP_CLEAR_SMI);

	// do a global and host reset
	GlobalReset();
	if (ControllerReset() < B_OK) {
		TRACE_ERROR("host failed to reset\n");
		return;
	}

	// Setup the frame list
	phys_addr_t physicalAddress;
	fFrameArea = fStack->AllocateArea((void **)&fFrameList, &physicalAddress,
		4096, "USB UHCI framelist");

	if (fFrameArea < B_OK) {
		TRACE_ERROR("unable to create an area for the frame pointer list\n");
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
	// 4: debug queue
	// TODO: 4: bandwidth reclamation queue
	fQueueCount = 5;
	fQueues = new(std::nothrow) Queue *[fQueueCount];
	if (!fQueues) {
		delete_area(fFrameArea);
		return;
	}

	for (int32 i = 0; i < fQueueCount; i++) {
		fQueues[i] = new(std::nothrow) Queue(fStack);
		if (!fQueues[i] || fQueues[i]->InitCheck() < B_OK) {
			TRACE_ERROR("cannot create queues\n");
			delete_area(fFrameArea);
			return;
		}

		if (i > 0)
			fQueues[i - 1]->LinkTo(fQueues[i]);
	}

	// Make sure the last queue terminates
	fQueues[fQueueCount - 1]->TerminateByStrayDescriptor();

	// Create the array that will keep bandwidth information
	fFrameBandwidth = new(std::nothrow) uint16[NUMBER_OF_FRAMES];

	// Create lists for managing isochronous transfer descriptors
	fFirstIsochronousDescriptor = new(std::nothrow) uhci_td *[NUMBER_OF_FRAMES];
	if (!fFirstIsochronousDescriptor) {
		TRACE_ERROR("faild to allocate memory for first isochronous descriptor\n");
		return;
	}

	fLastIsochronousDescriptor = new(std::nothrow) uhci_td *[NUMBER_OF_FRAMES];
	if (!fLastIsochronousDescriptor) {
		TRACE_ERROR("failed to allocate memory for last isochronous descriptor\n");
		delete [] fFirstIsochronousDescriptor;
		return;
	}

	for (int32 i = 0; i < NUMBER_OF_FRAMES; i++) {
		fFrameList[i] =	fQueues[UHCI_INTERRUPT_QUEUE]->PhysicalAddress()
			| FRAMELIST_NEXT_IS_QH;
		fFrameBandwidth[i] = MAX_AVAILABLE_BANDWIDTH;
		fFirstIsochronousDescriptor[i] = NULL;
		fLastIsochronousDescriptor[i] = NULL;
	}

	// Create semaphore the finisher and cleanup threads will wait for
	fFinishTransfersSem = create_sem(0, "UHCI Finish Transfers");
	if (fFinishTransfersSem < B_OK) {
		TRACE_ERROR("failed to create finisher semaphore\n");
		return;
	}

	fCleanupSem = create_sem(0, "UHCI Cleanup");
	if (fCleanupSem < B_OK) {
		TRACE_ERROR("failed to create cleanup semaphore\n");
		return;
	}

	// Create the finisher service and cleanup threads
	fFinishThread = spawn_kernel_thread(FinishThread,
		"uhci finish thread", B_URGENT_DISPLAY_PRIORITY, (void *)this);
	resume_thread(fFinishThread);

	fCleanupThread = spawn_kernel_thread(CleanupThread,
		"uhci cleanup thread", B_NORMAL_PRIORITY, (void *)this);
	resume_thread(fCleanupThread);

	// Create semaphore the isochronous finisher thread will wait for
	fFinishIsochronousTransfersSem = create_sem(0,
		"UHCI Isochronous Finish Transfers");
	if (fFinishIsochronousTransfersSem < B_OK) {
		TRACE_ERROR("failed to create isochronous finisher semaphore\n");
		return;
	}

	// Create the isochronous finisher service thread
	fFinishIsochronousThread = spawn_kernel_thread(FinishIsochronousThread,
		"uhci isochronous finish thread", B_URGENT_DISPLAY_PRIORITY,
		(void *)this);
	resume_thread(fFinishIsochronousThread);

	// Install the interrupt handler
	TRACE("installing interrupt handler\n");
	install_io_interrupt_handler(fPCIInfo->u.h0.interrupt_line,
		InterruptHandler, (void *)this, 0);

	// Enable interrupts
	fEnabledInterrupts = UHCI_USBSTS_USBINT | UHCI_USBSTS_ERRINT
		| UHCI_USBSTS_HOSTERR | UHCI_USBSTS_HCPRERR | UHCI_USBSTS_HCHALT;
	WriteReg16(UHCI_USBINTR, UHCI_USBINTR_CRC | UHCI_USBINTR_IOC
		| UHCI_USBINTR_SHORT);

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	if (atomic_add(&sDebuggerCommandAdded, 1) == 0) {
		add_debugger_command("uhci_process_transfer",
			&debug_process_transfer,
			"Processes a USB transfer with the given variables");
	}
#endif

	TRACE("UHCI host controller driver constructed\n");
	fInitOK = true;
}


UHCI::~UHCI()
{
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	if (atomic_add(&sDebuggerCommandAdded, -1) == 1) {
		remove_debugger_command("uhci_process_transfer",
			&debug_process_transfer);
	}
#endif

	int32 result = 0;
	fStopThreads = true;
	delete_sem(fFinishTransfersSem);
	delete_sem(fCleanupSem);
	delete_sem(fFinishIsochronousTransfersSem);
	wait_for_thread(fFinishThread, &result);
	wait_for_thread(fCleanupThread, &result);
	wait_for_thread(fFinishIsochronousThread, &result);

	LockIsochronous();
	isochronous_transfer_data *isoTransfer = fFirstIsochronousTransfer;
	while (isoTransfer) {
		isochronous_transfer_data *next = isoTransfer->link;
		delete isoTransfer;
		isoTransfer = next;
	}
	mutex_destroy(&fIsochronousLock);

	Lock();
	transfer_data *transfer = fFirstTransfer;
	while (transfer) {
		transfer->transfer->Finished(B_CANCELED, 0);
		delete transfer->transfer;

		transfer_data *next = transfer->link;
		delete transfer;
		transfer = next;
	}

	for (int32 i = 0; i < fQueueCount; i++)
		delete fQueues[i];

	delete [] fQueues;
	delete [] fFrameBandwidth;
	delete [] fFirstIsochronousDescriptor;
	delete [] fLastIsochronousDescriptor;
	delete fRootHub;
	delete_area(fFrameArea);

	put_module(B_PCI_MODULE_NAME);
	Unlock();
}


status_t
UHCI::Start()
{
	// Start the host controller, then start the Busmanager
	TRACE("starting UHCI BusManager\n");
	TRACE("usbcmd reg 0x%04x, usbsts reg 0x%04x\n",
		ReadReg16(UHCI_USBCMD), ReadReg16(UHCI_USBSTS));

	// Set the run bit in the command register
	WriteReg16(UHCI_USBCMD, ReadReg16(UHCI_USBCMD) | UHCI_USBCMD_RS);

	bool running = false;
	for (int32 i = 0; i < 10; i++) {
		uint16 status = ReadReg16(UHCI_USBSTS);
		TRACE("current loop %" B_PRId32 ", status 0x%04x\n", i, status);

		if (status & UHCI_USBSTS_HCHALT)
			snooze(10000);
		else {
			running = true;
			break;
		}
	}

	if (!running) {
		TRACE_ERROR("controller won't start running\n");
		return B_ERROR;
	}

	fRootHubAddress = AllocateAddress();
	fRootHub = new(std::nothrow) UHCIRootHub(RootObject(), fRootHubAddress);
	if (!fRootHub) {
		TRACE_ERROR("no memory to allocate root hub\n");
		return B_NO_MEMORY;
	}

	if (fRootHub->InitCheck() < B_OK) {
		TRACE_ERROR("root hub failed init check\n");
		delete fRootHub;
		return B_ERROR;
	}

	SetRootHub(fRootHub);

	TRACE("controller is started. status: %u curframe: %u\n",
		ReadReg16(UHCI_USBSTS), ReadReg16(UHCI_FRNUM));
	TRACE_ALWAYS("successfully started the controller\n");
	return BusManager::Start();
}


status_t
UHCI::SubmitTransfer(Transfer *transfer)
{
	// Short circuit the root hub
	Pipe *pipe = transfer->TransferPipe();
	if (pipe->DeviceAddress() == fRootHubAddress)
		return fRootHub->ProcessTransfer(this, transfer);

	TRACE("submit transfer called for device %d\n", pipe->DeviceAddress());
	if (pipe->Type() & USB_OBJECT_CONTROL_PIPE)
		return SubmitRequest(transfer);

	// Process isochronous transfers
	if (pipe->Type() & USB_OBJECT_ISO_PIPE)
		return SubmitIsochronous(transfer);

	uhci_td *firstDescriptor = NULL;
	uhci_qh *transferQueue = NULL;
	status_t result = CreateFilledTransfer(transfer, &firstDescriptor,
		&transferQueue);
	if (result < B_OK)
		return result;

	Queue *queue = NULL;
	if (pipe->Type() & USB_OBJECT_INTERRUPT_PIPE)
		queue = fQueues[UHCI_INTERRUPT_QUEUE];
	else
		queue = fQueues[UHCI_BULK_QUEUE];

	bool directionIn = (pipe->Direction() == Pipe::In);
	result = AddPendingTransfer(transfer, queue, transferQueue,
		firstDescriptor, firstDescriptor, directionIn);
	if (result < B_OK) {
		TRACE_ERROR("failed to add pending transfer\n");
		FreeDescriptorChain(firstDescriptor);
		FreeTransferQueue(transferQueue);
		return result;
	}

	queue->AppendTransfer(transferQueue);
	return B_OK;
}


status_t
UHCI::StartDebugTransfer(DebugTransfer *transfer)
{
	transfer->firstDescriptor = NULL;
	transfer->transferQueue = NULL;
	status_t result = CreateFilledTransfer(transfer, &transfer->firstDescriptor,
		&transfer->transferQueue);
	if (result < B_OK)
		return result;

	fQueues[UHCI_DEBUG_QUEUE]->AppendTransfer(transfer->transferQueue, false);

	return B_OK;
}


status_t
UHCI::CheckDebugTransfer(DebugTransfer *transfer, bool &_stillPending)
{
	bool transferOK = false;
	bool transferError = false;
	uhci_td *descriptor = transfer->firstDescriptor;

	while (descriptor) {
		uint32 status = descriptor->status;
		if (status & TD_STATUS_ACTIVE)
			break;

		if (status & TD_ERROR_MASK) {
			transferError = true;
			break;
		}

		if ((descriptor->link_phy & TD_TERMINATE)
			|| uhci_td_actual_length(descriptor)
				< uhci_td_maximum_length(descriptor)) {
			transferOK = true;
			break;
		}

		descriptor = (uhci_td *)descriptor->link_log;
	}

	if (!transferOK && !transferError) {
		spin(200);
		_stillPending = true;
		return B_OK;
	}

	if (transferOK) {
		uint8 lastDataToggle = 0;
		if (transfer->TransferPipe()->Direction() == Pipe::In) {
			// data to read out
			iovec *vector = transfer->Vector();
			size_t vectorCount = transfer->VectorCount();

			ReadDescriptorChain(transfer->firstDescriptor,
				vector, vectorCount, &lastDataToggle);
		} else {
			// read the actual length that was sent
			ReadActualLength(transfer->firstDescriptor, &lastDataToggle);
		}

		transfer->TransferPipe()->SetDataToggle(lastDataToggle == 0);
	}

	fQueues[UHCI_DEBUG_QUEUE]->RemoveTransfer(transfer->transferQueue, false);
	FreeDescriptorChain(transfer->firstDescriptor);
	FreeTransferQueue(transfer->transferQueue);
	_stillPending = false;
	return transferOK ? B_OK : B_IO_ERROR;
}


void
UHCI::CancelDebugTransfer(DebugTransfer *transfer)
{
	// clear the active bit so the descriptors are canceled
	uhci_td *descriptor = transfer->firstDescriptor;
	while (descriptor) {
		descriptor->status &= ~TD_STATUS_ACTIVE;
		descriptor = (uhci_td *)descriptor->link_log;
	}

	transfer->Finished(B_CANCELED, 0);

	// dequeue and free resources
	fQueues[UHCI_DEBUG_QUEUE]->RemoveTransfer(transfer->transferQueue, false);
	FreeDescriptorChain(transfer->firstDescriptor);
	FreeTransferQueue(transfer->transferQueue);
	// TODO: [bonefish] The Free*() calls cause "PMA: provided address resulted
	// in invalid index" to be printed, so apparently something is not right.
	// Though I have not clue what. This is the same cleanup code as in
	// CheckDebugTransfer() that should undo the CreateFilledTransfer() from
	// StartDebugTransfer().
}


status_t
UHCI::CancelQueuedTransfers(Pipe *pipe, bool force)
{
	if (pipe->Type() & USB_OBJECT_ISO_PIPE)
		return CancelQueuedIsochronousTransfers(pipe, force);

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
			// clear the active bit so the descriptors are canceled
			uhci_td *descriptor = current->first_descriptor;
			while (descriptor) {
				descriptor->status &= ~TD_STATUS_ACTIVE;
				descriptor = (uhci_td *)descriptor->link_log;
			}

			if (!force) {
				// if the transfer is canceled by force, the one causing the
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
UHCI::CancelQueuedIsochronousTransfers(Pipe *pipe, bool force)
{
	isochronous_transfer_data *current = fFirstIsochronousTransfer;

	while (current) {
		if (current->transfer->TransferPipe() == pipe) {
			int32 packetCount
				= current->transfer->IsochronousData()->packet_count;
			// Set the active bit off on every descriptor in order to prevent
			// the controller from processing them. Then set off the is_active
			// field of the transfer in order to make the finisher thread skip
			// the transfer. The FinishIsochronousThread will do the rest.
			for (int32 i = 0; i < packetCount; i++)
				current->descriptors[i]->status &= ~TD_STATUS_ACTIVE;

			// TODO: Use the force paramater in order to avoid calling
			// invalid callbacks
			current->is_active = false;
		}

		current = current->link;
	}

	TRACE_ERROR("no isochronous transfer found!\n");
	return B_ERROR;
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
		TRACE_ERROR("failed to allocate descriptors\n");
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
	if (pipe->Speed() == USB_SPEED_LOWSPEED)
		queue = fQueues[UHCI_LOW_SPEED_CONTROL_QUEUE];
	else
		queue = fQueues[UHCI_FULL_SPEED_CONTROL_QUEUE];

	uhci_qh *transferQueue = CreateTransferQueue(setupDescriptor);
	status_t result = AddPendingTransfer(transfer, queue, transferQueue,
		setupDescriptor, dataDescriptor, directionIn);
	if (result < B_OK) {
		TRACE_ERROR("failed to add pending transfer\n");
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

	transfer_data *data = new(std::nothrow) transfer_data;
	if (!data)
		return B_NO_MEMORY;

	status_t result = transfer->InitKernelAccess();
	if (result < B_OK) {
		delete data;
		return result;
	}

	data->transfer = transfer;
	data->queue = queue;
	data->transfer_queue = transferQueue;
	data->first_descriptor = firstDescriptor;
	data->data_descriptor = dataDescriptor;
	data->incoming = directionIn;
	data->canceled = false;
	data->link = NULL;

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


status_t
UHCI::AddPendingIsochronousTransfer(Transfer *transfer, uhci_td **isoRequest,
	bool directionIn)
{
	if (!transfer || !isoRequest)
		return B_BAD_VALUE;

	isochronous_transfer_data *data
		= new(std::nothrow) isochronous_transfer_data;
	if (!data)
		return B_NO_MEMORY;

	status_t result = transfer->InitKernelAccess();
	if (result < B_OK) {
		delete data;
		return result;
	}

	data->transfer = transfer;
	data->descriptors = isoRequest;
	data->last_to_process = transfer->IsochronousData()->packet_count - 1;
	data->incoming = directionIn;
	data->is_active = true;
	data->link = NULL;

	// Put in the isochronous transfer list
	if (!LockIsochronous()) {
		delete data;
		return B_ERROR;
	}

	if (fLastIsochronousTransfer)
		fLastIsochronousTransfer->link = data;
	if (!fFirstIsochronousTransfer)
		fFirstIsochronousTransfer = data;

	fLastIsochronousTransfer = data;
	UnlockIsochronous();
	return B_OK;
}


status_t
UHCI::SubmitIsochronous(Transfer *transfer)
{
	Pipe *pipe = transfer->TransferPipe();
	bool directionIn = (pipe->Direction() == Pipe::In);
	usb_isochronous_data *isochronousData = transfer->IsochronousData();
	size_t packetSize = transfer->DataLength();
	size_t restSize = packetSize % isochronousData->packet_count;
	packetSize /= isochronousData->packet_count;
	uint16 currentFrame;

	if (packetSize > pipe->MaxPacketSize()) {
		TRACE_ERROR("isochronous packetSize is bigger than pipe MaxPacketSize\n");
		return B_BAD_VALUE;
	}

	// Ignore the fact that the last descriptor might need less bandwidth.
	// The overhead is not worthy.
	uint16 bandwidth = transfer->Bandwidth() / isochronousData->packet_count;

	TRACE("isochronous transfer descriptor bandwidth %d\n", bandwidth);

	// The following holds the list of transfer descriptor of the
	// isochronous request. It is used to quickly remove all the isochronous
	// descriptors from the frame list, as descriptors are not link to each
	// other in a queue like for every other transfer.
	uhci_td **isoRequest
		= new(std::nothrow) uhci_td *[isochronousData->packet_count];
	if (isoRequest == NULL) {
		TRACE("failed to create isoRequest array!\n");
		return B_NO_MEMORY;
	}

	// Create the list of transfer descriptors
	for (uint32 i = 0; i < (isochronousData->packet_count - 1); i++) {
		isoRequest[i] = CreateDescriptor(pipe,
			directionIn ? TD_TOKEN_IN : TD_TOKEN_OUT, packetSize);
		// If we ran out of memory, clean up and return
		if (isoRequest[i] == NULL) {
			for (uint32 j = 0; j < i; j++)
				FreeDescriptor(isoRequest[j]);
			delete [] isoRequest;
			return B_NO_MEMORY;
		}
		// Make sure data toggle is set to zero
		isoRequest[i]->token &= ~TD_TOKEN_DATA1;
	}

	// Create the last transfer descriptor which should be of smaller size
	// and set the IOC bit
	isoRequest[isochronousData->packet_count - 1] = CreateDescriptor(pipe,
		directionIn ? TD_TOKEN_IN : TD_TOKEN_OUT,
		(restSize) ? restSize : packetSize);
	// If we are that unlucky...
	if (!isoRequest[isochronousData->packet_count - 1]) {
		for (uint32 i = 0; i < (isochronousData->packet_count - 2); i++)
			FreeDescriptor(isoRequest[i]);
		delete [] isoRequest;
		return B_NO_MEMORY;
	}
	isoRequest[isochronousData->packet_count - 1]->token &= ~TD_TOKEN_DATA1;

	// If direction is out set every descriptor data
	if (!directionIn) {
		iovec *vector = transfer->Vector();
		WriteIsochronousDescriptorChain(isoRequest,
			isochronousData->packet_count, vector);
	} else {
		// Initialize the packet descriptors
		for (uint32 i = 0; i < isochronousData->packet_count; i++) {
			isochronousData->packet_descriptors[i].actual_length = 0;
			isochronousData->packet_descriptors[i].status = B_NO_INIT;
		}
	}

	TRACE("isochronous submitted size=%ld bytes, TDs=%" B_PRId32 ", "
		"packetSize=%ld, restSize=%ld\n", transfer->DataLength(),
		isochronousData->packet_count, packetSize, restSize);

	// Find the entry where to start inserting the first Isochronous descriptor
	if (isochronousData->flags & USB_ISO_ASAP ||
		isochronousData->starting_frame_number == NULL) {
		// find the first available frame with enough bandwidth.
		// This should always be the case, as defining the starting frame
		// number in the driver makes no sense for many reason, one of which
		// is that frame numbers value are host controller specific, and the
		// driver does not know which host controller is running.
		currentFrame = ReadReg16(UHCI_FRNUM);

		// Make sure that:
		// 1. We are at least 5ms ahead the controller
		// 2. We stay in the range 0-1023
		// 3. There is enough bandwidth in the first entry
		currentFrame = (currentFrame + 5) % NUMBER_OF_FRAMES;
	} else {
		// Find out if the frame number specified has enough bandwidth,
		// otherwise find the first next available frame with enough bandwidth
		currentFrame = *isochronousData->starting_frame_number;
	}

	// Find the first entry with enough bandwidth
	// TODO: should we also check the bandwidth of the following packet_count frames?
	uint16 startSeekingFromFrame = currentFrame;
	while (fFrameBandwidth[currentFrame] < bandwidth) {
		currentFrame = (currentFrame + 1) % NUMBER_OF_FRAMES;
		if (currentFrame == startSeekingFromFrame) {
			TRACE_ERROR("not enough bandwidth to queue the isochronous request");
			for (uint32 i = 0; i < isochronousData->packet_count; i++)
				FreeDescriptor(isoRequest[i]);
			delete [] isoRequest;
		return B_ERROR;
		}
	}

	if (isochronousData->starting_frame_number)
		*isochronousData->starting_frame_number = currentFrame;

	// Add transfer to the list
	status_t result = AddPendingIsochronousTransfer(transfer, isoRequest,
		directionIn);
	if (result < B_OK) {
		TRACE_ERROR("failed to add pending isochronous transfer\n");
		for (uint32 i = 0; i < isochronousData->packet_count; i++)
			FreeDescriptor(isoRequest[i]);
		delete [] isoRequest;
		return result;
	}

	TRACE("appended isochronous transfer by starting at frame number %d\n",
		currentFrame);

	// Insert the Transfer Descriptor by starting at
	// the starting_frame_number entry
	// TODO: We don't consider bInterval, and assume it's 1!
	for (uint32 i = 0; i < isochronousData->packet_count; i++) {
		result = LinkIsochronousDescriptor(isoRequest[i], currentFrame);
		if (result < B_OK) {
			TRACE_ERROR("failed to add pending isochronous transfer\n");
			for (uint32 i = 0; i < isochronousData->packet_count; i++)
				FreeDescriptor(isoRequest[i]);
			delete [] isoRequest;
			return result;
		}

		fFrameBandwidth[currentFrame] -= bandwidth;
		currentFrame = (currentFrame + 1) % NUMBER_OF_FRAMES;
	}

	// Wake up the isochronous finisher thread
	release_sem_etc(fFinishIsochronousTransfersSem, 1, B_DO_NOT_RESCHEDULE);

	return B_OK;
}


isochronous_transfer_data *
UHCI::FindIsochronousTransfer(uhci_td *descriptor)
{
	// Simply check every last descriptor of the isochronous transfer list
	if (LockIsochronous()) {
		isochronous_transfer_data *transfer = fFirstIsochronousTransfer;
		if (transfer) {
			while (transfer->descriptors[transfer->last_to_process]
				!= descriptor) {
				transfer = transfer->link;
				if (!transfer)
					break;
			}
		}
		UnlockIsochronous();
		return transfer;
	}
	return NULL;
}


status_t
UHCI::LinkIsochronousDescriptor(uhci_td *descriptor, uint16 frame)
{
	// The transfer descriptor is appended to the last
	// existing isochronous transfer descriptor (if any)
	// in that frame.
	if (LockIsochronous()) {
		if (!fFirstIsochronousDescriptor[frame]) {
			// Insert the transfer descriptor in the first position
			fFrameList[frame] = descriptor->this_phy & ~FRAMELIST_NEXT_IS_QH;
			fFirstIsochronousDescriptor[frame] = descriptor;
			fLastIsochronousDescriptor[frame] = descriptor;
		} else {
			// Append to the last transfer descriptor
			fLastIsochronousDescriptor[frame]->link_log = descriptor;
			fLastIsochronousDescriptor[frame]->link_phy
				= descriptor->this_phy & ~TD_NEXT_IS_QH;
			fLastIsochronousDescriptor[frame] = descriptor;
		}

		descriptor->link_phy
			= fQueues[UHCI_INTERRUPT_QUEUE]->PhysicalAddress() | TD_NEXT_IS_QH;
		UnlockIsochronous();
		return B_OK;
	}
	return B_ERROR;
}


uhci_td *
UHCI::UnlinkIsochronousDescriptor(uint16 frame)
{
	// We always unlink from the top
	if (LockIsochronous()) {
		uhci_td *descriptor = fFirstIsochronousDescriptor[frame];
		if (descriptor) {
			// The descriptor will be freed later.
			fFrameList[frame] = descriptor->link_phy;
			if (descriptor->link_log) {
				fFirstIsochronousDescriptor[frame]
					= (uhci_td *)descriptor->link_log;
			} else {
				fFirstIsochronousDescriptor[frame] = NULL;
				fLastIsochronousDescriptor[frame] = NULL;
			}
		}
		UnlockIsochronous();
		return descriptor;
	}
	return NULL;
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

		TRACE("finishing transfers (first transfer: 0x%08lx; last"
			" transfer: 0x%08lx)\n", (addr_t)fFirstTransfer,
			(addr_t)fLastTransfer);
		transfer_data *lastTransfer = NULL;
		transfer_data *transfer = fFirstTransfer;
		Unlock();

		while (transfer) {
			bool transferDone = false;
			uhci_td *descriptor = transfer->first_descriptor;
			status_t callbackStatus = B_OK;

			while (descriptor) {
				uint32 status = descriptor->status;
				if (status & TD_STATUS_ACTIVE) {
					// still in progress
					TRACE("td (0x%08" B_PRIx32 ") still active\n",
						descriptor->this_phy);
					break;
				}

				if (status & TD_ERROR_MASK) {
					// an error occured
					TRACE_ERROR("td (0x%08" B_PRIx32 ") error: status: 0x%08"
						B_PRIx32 "; token: 0x%08" B_PRIx32 ";\n",
						descriptor->this_phy, status, descriptor->token);

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

					transferDone = true;
					break;
				}

				if ((descriptor->link_phy & TD_TERMINATE)
					|| ((descriptor->status & TD_CONTROL_SPD) != 0
						&& uhci_td_actual_length(descriptor)
							< uhci_td_maximum_length(descriptor))) {
					// all descriptors are done, or we have a short packet
					TRACE("td (0x%08" B_PRIx32 ") ok\n", descriptor->this_phy);
					callbackStatus = B_OK;
					transferDone = true;
					break;
				}

				descriptor = (uhci_td *)descriptor->link_log;
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

			// if canceled the callback has already been called
			if (!transfer->canceled) {
				size_t actualLength = 0;
				if (callbackStatus == B_OK) {
					uint8 lastDataToggle = 0;
					if (transfer->data_descriptor && transfer->incoming) {
						// data to read out
						iovec *vector = transfer->transfer->Vector();
						size_t vectorCount = transfer->transfer->VectorCount();

						transfer->transfer->PrepareKernelAccess();
						actualLength = ReadDescriptorChain(
							transfer->data_descriptor,
							vector, vectorCount,
							&lastDataToggle);
					} else if (transfer->data_descriptor) {
						// read the actual length that was sent
						actualLength = ReadActualLength(
							transfer->data_descriptor, &lastDataToggle);
					}

					transfer->transfer->TransferPipe()->SetDataToggle(lastDataToggle == 0);

					if (transfer->transfer->IsFragmented()) {
						// this transfer may still have data left
						TRACE("advancing fragmented transfer\n");
						transfer->transfer->AdvanceByFragment(actualLength);
						if (transfer->transfer->VectorLength() > 0) {
							TRACE("still %ld bytes left on transfer\n",
								transfer->transfer->VectorLength());

							Transfer *resubmit = transfer->transfer;

							// free the used descriptors
							transfer->queue->RemoveTransfer(
								transfer->transfer_queue);
							AddToFreeList(transfer);

							// resubmit the advanced transfer so the rest
							// of the buffers are transmitted over the bus
							resubmit->PrepareKernelAccess();
							if (SubmitTransfer(resubmit) != B_OK)
								resubmit->Finished(B_ERROR, 0);

							transfer = next;
							continue;
						}

						// the transfer is done, but we already set the
						// actualLength with AdvanceByFragment()
						actualLength = 0;
					}
				}

				transfer->transfer->Finished(callbackStatus, actualLength);
				fProcessingPipe = NULL;
			}

			// remove and free the hardware queue and its descriptors
			transfer->queue->RemoveTransfer(transfer->transfer_queue);
			delete transfer->transfer;
			AddToFreeList(transfer);
			transfer = next;
		}
	}
}


void
UHCI::AddToFreeList(transfer_data *transfer)
{
	transfer->free_after_frame = ReadReg16(UHCI_FRNUM);
	if (!Lock())
		return;

	transfer->link = fFreeList;
	fFreeList = transfer;
	Unlock();

	if (atomic_add(&fCleanupCount, 1) == 0)
		release_sem(fCleanupSem);
}


int32
UHCI::CleanupThread(void *data)
{
	((UHCI *)data)->Cleanup();
	return B_OK;
}


void
UHCI::Cleanup()
{
	while (!fStopThreads) {
		if (acquire_sem(fCleanupSem) != B_OK)
			continue;

		bigtime_t nextTime = system_time() + 1000;
		while (atomic_get(&fCleanupCount) != 0) {
			// wait for the frame to pass
			snooze_until(nextTime, B_SYSTEM_TIMEBASE);
			nextTime += 1000;

			if (!Lock())
				continue;

			// find the first entry we may free
			transfer_data **link = &fFreeList;
			transfer_data *transfer = fFreeList;
			uint16 frameNumber = ReadReg16(UHCI_FRNUM);
			while (transfer) {
				if (transfer->free_after_frame != frameNumber) {
					*link = NULL;
					break;
				}

				link = &transfer->link;
				transfer = transfer->link;
			}

			Unlock();

			// the transfers below this one are all freeable
			while (transfer) {
				transfer_data *next = transfer->link;
				FreeDescriptorChain(transfer->first_descriptor);
				FreeTransferQueue(transfer->transfer_queue);
				delete transfer;
				atomic_add(&fCleanupCount, -1);
				transfer = next;
			}
		}
	}
}


int32
UHCI::FinishIsochronousThread(void *data)
{
       ((UHCI *)data)->FinishIsochronousTransfers();
       return B_OK;
}


void
UHCI::FinishIsochronousTransfers()
{
	/* This thread stays one position behind the controller and processes every
	 * isochronous descriptor. Once it finds the last isochronous descriptor
	 * of a transfer, it processes the entire transfer.
	 */

	while (!fStopThreads) {
		// Go to sleep if there are not isochronous transfer to process
		if (acquire_sem(fFinishIsochronousTransfersSem) < B_OK)
			return;

		bool transferDone = false;
		uint16 currentFrame = ReadReg16(UHCI_FRNUM);

		// Process the frame list until one transfer is processed
		while (!transferDone) {
			// wait 1ms in order to be sure to be one position behind
			// the controller
			if (currentFrame == ReadReg16(UHCI_FRNUM))
				snooze(1000);

			// Process the frame till it has isochronous descriptors in it.
			while (!(fFrameList[currentFrame] & FRAMELIST_NEXT_IS_QH)) {
				uhci_td *current = UnlinkIsochronousDescriptor(currentFrame);

				// Process the transfer if we found the last descriptor
				isochronous_transfer_data *transfer
					= FindIsochronousTransfer(current);
					// Process the descriptors only if it is still active and
					// belongs to an inbound transfer. If the transfer is not
					// active, it means the request has been removed, so simply
					// remove the descriptors.
				if (transfer && transfer->is_active) {
					if (current->token & TD_TOKEN_IN) {
						iovec *vector = transfer->transfer->Vector();
						transfer->transfer->PrepareKernelAccess();
						ReadIsochronousDescriptorChain(transfer, vector);
					}

					// Remove the transfer
					if (LockIsochronous()) {
						if (transfer == fFirstIsochronousTransfer) {
							fFirstIsochronousTransfer = transfer->link;
							if (transfer == fLastIsochronousTransfer)
								fLastIsochronousTransfer = NULL;
						} else {
							isochronous_transfer_data *temp
								= fFirstIsochronousTransfer;
							while (transfer != temp->link)
								temp = temp->link;

							if (transfer == fLastIsochronousTransfer)
								fLastIsochronousTransfer = temp;
							temp->link = temp->link->link;
						}
						UnlockIsochronous();
					}

					transfer->transfer->Finished(B_OK, 0);

					uint32 packetCount =
						transfer->transfer->IsochronousData()->packet_count;
					for (uint32 i = 0; i < packetCount; i++)
						FreeDescriptor(transfer->descriptors[i]);

					delete [] transfer->descriptors;
					delete transfer->transfer;
					delete transfer;
					transferDone = true;
				}
			}

			// Make sure to reset the frame bandwidth
			fFrameBandwidth[currentFrame] = MAX_AVAILABLE_BANDWIDTH;
			currentFrame = (currentFrame + 1) % NUMBER_OF_FRAMES;
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

	TRACE("reset port %d\n", index);

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
	TRACE("port was reset: 0x%04x\n", ReadReg16(port));
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
	static spinlock lock = B_SPINLOCK_INITIALIZER;
	acquire_spinlock(&lock);

	// Check if we really had an interrupt
	uint16 status = ReadReg16(UHCI_USBSTS);
	if ((status & fEnabledInterrupts) == 0) {
		if (status != 0) {
			TRACE("discarding not enabled interrupts 0x%08x\n", status);
			WriteReg16(UHCI_USBSTS, status);
		}

		release_spinlock(&lock);
		return B_UNHANDLED_INTERRUPT;
	}

	uint16 acknowledge = 0;
	bool finishTransfers = false;
	int32 result = B_HANDLED_INTERRUPT;

	if (status & UHCI_USBSTS_USBINT) {
		TRACE_MODULE("transfer finished\n");
		acknowledge |= UHCI_USBSTS_USBINT;
		result = B_INVOKE_SCHEDULER;
		finishTransfers = true;
	}

	if (status & UHCI_USBSTS_ERRINT) {
		TRACE_MODULE("transfer error\n");
		acknowledge |= UHCI_USBSTS_ERRINT;
		result = B_INVOKE_SCHEDULER;
		finishTransfers = true;
	}

	if (status & UHCI_USBSTS_RESDET) {
		TRACE_MODULE("resume detected\n");
		acknowledge |= UHCI_USBSTS_RESDET;
	}

	if (status & UHCI_USBSTS_HOSTERR) {
		TRACE_MODULE_ERROR("host system error\n");
		acknowledge |= UHCI_USBSTS_HOSTERR;
	}

	if (status & UHCI_USBSTS_HCPRERR) {
		TRACE_MODULE_ERROR("process error\n");
		acknowledge |= UHCI_USBSTS_HCPRERR;
	}

	if (status & UHCI_USBSTS_HCHALT) {
		TRACE_MODULE_ERROR("host controller halted\n");
		// at least disable interrupts so we do not flood the system
		WriteReg16(UHCI_USBINTR, 0);
		fEnabledInterrupts = 0;
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
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	load_driver_symbols("uhci");
#endif
#endif

	if (!sPCIModule) {
		status_t status = get_module(B_PCI_MODULE_NAME, (module_info **)&sPCIModule);
		if (status < B_OK) {
			TRACE_MODULE_ERROR("AddTo(): getting pci module failed! 0x%08"
				B_PRIx32 "\n", status);
			return status;
		}
	}

	TRACE_MODULE("AddTo(): setting up hardware\n");

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
				TRACE_MODULE_ERROR("AddTo(): found with invalid IRQ - check IRQ assignement\n");
				continue;
			}

			TRACE_MODULE("AddTo(): found at IRQ %u\n",
				item->u.h0.interrupt_line);
			UHCI *bus = new(std::nothrow) UHCI(item, stack);
			if (!bus) {
				delete item;
				sPCIModule = NULL;
				put_module(B_PCI_MODULE_NAME);
				return B_NO_MEMORY;
			}

			if (bus->InitCheck() < B_OK) {
				TRACE_MODULE_ERROR("AddTo(): InitCheck() failed 0x%08" B_PRIx32
					"\n", bus->InitCheck());
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
	lastDescriptor->link_log = NULL;

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
	phys_addr_t physicalAddress;
	if (fStack->AllocateChunk((void **)&queueHead, &physicalAddress,
		sizeof(uhci_qh)) < B_OK)
		return NULL;

	queueHead->this_phy = (uint32)physicalAddress;
	queueHead->element_phy = descriptor->this_phy;
	return queueHead;
}


void
UHCI::FreeTransferQueue(uhci_qh *queueHead)
{
	if (!queueHead)
		return;

	fStack->FreeChunk(queueHead, queueHead->this_phy, sizeof(uhci_qh));
}


uhci_td *
UHCI::CreateDescriptor(Pipe *pipe, uint8 direction, size_t bufferSize)
{
	uhci_td *result;
	phys_addr_t physicalAddress;

	if (fStack->AllocateChunk((void **)&result, &physicalAddress,
		sizeof(uhci_td)) < B_OK) {
		TRACE_ERROR("failed to allocate a transfer descriptor\n");
		return NULL;
	}

	result->this_phy = (uint32)physicalAddress;
	result->status = TD_STATUS_ACTIVE;
	if (pipe->Type() & USB_OBJECT_ISO_PIPE)
		result->status |= TD_CONTROL_ISOCHRONOUS;
	else {
		result->status |= TD_CONTROL_3_ERRORS;
		if (direction == TD_TOKEN_IN
			&& (pipe->Type() & USB_OBJECT_CONTROL_PIPE) == 0)
			result->status |= TD_CONTROL_SPD;
	}
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
		result->buffer_phy = 0;
		return result;
	}

	if (fStack->AllocateChunk(&result->buffer_log, &physicalAddress,
		bufferSize) < B_OK) {
		TRACE_ERROR("unable to allocate space for the buffer\n");
		fStack->FreeChunk(result, result->this_phy, sizeof(uhci_td));
		return NULL;
	}
	result->buffer_phy = physicalAddress;

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
			descriptor->buffer_phy, descriptor->buffer_size);
	}

	fStack->FreeChunk(descriptor, descriptor->this_phy, sizeof(uhci_td));
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

			TRACE("copying %ld bytes to bufferOffset %ld from"
				" vectorOffset %ld at index %ld of %ld\n", length, bufferOffset,
				vectorOffset, vectorIndex, vectorCount);
			memcpy((uint8 *)current->buffer_log + bufferOffset,
				(uint8 *)vector[vectorIndex].iov_base + vectorOffset, length);

			actualLength += length;
			vectorOffset += length;
			bufferOffset += length;

			if (vectorOffset >= vector[vectorIndex].iov_len) {
				if (++vectorIndex >= vectorCount) {
					TRACE("wrote descriptor chain (%ld bytes, no more vectors)\n",
						actualLength);
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

	TRACE("wrote descriptor chain (%ld bytes)\n", actualLength);
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
		size_t bufferSize = uhci_td_actual_length(current);

		while (true) {
			size_t length = min_c(bufferSize - bufferOffset,
				vector[vectorIndex].iov_len - vectorOffset);

			TRACE("copying %ld bytes to vectorOffset %ld from"
				" bufferOffset %ld at index %ld of %ld\n", length, vectorOffset,
				bufferOffset, vectorIndex, vectorCount);
			memcpy((uint8 *)vector[vectorIndex].iov_base + vectorOffset,
				(uint8 *)current->buffer_log + bufferOffset, length);

			actualLength += length;
			vectorOffset += length;
			bufferOffset += length;

			if (vectorOffset >= vector[vectorIndex].iov_len) {
				if (++vectorIndex >= vectorCount) {
					TRACE("read descriptor chain (%ld bytes, no more vectors)\n",
						actualLength);
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

	TRACE("read descriptor chain (%ld bytes)\n", actualLength);
	return actualLength;
}


size_t
UHCI::ReadActualLength(uhci_td *topDescriptor, uint8 *lastDataToggle)
{
	size_t actualLength = 0;
	uhci_td *current = topDescriptor;
	uint8 dataToggle = 0;

	while (current && (current->status & TD_STATUS_ACTIVE) == 0) {
		actualLength += uhci_td_actual_length(current);
		dataToggle = (current->token >> TD_TOKEN_DATA_TOGGLE_SHIFT) & 0x01;

		if (current->link_phy & TD_TERMINATE)
			break;

		current = (uhci_td *)current->link_log;
	}

	if (lastDataToggle)
		*lastDataToggle = dataToggle;

	TRACE("read actual length (%ld bytes)\n", actualLength);
	return actualLength;
}


void
UHCI::WriteIsochronousDescriptorChain(uhci_td **isoRequest, uint32 packetCount,
	iovec *vector)
{
	size_t vectorOffset = 0;
	for (uint32 i = 0; i < packetCount; i++) {
		size_t bufferSize = isoRequest[i]->buffer_size;
		memcpy((uint8 *)isoRequest[i]->buffer_log,
			(uint8 *)vector->iov_base + vectorOffset, bufferSize);
		vectorOffset += bufferSize;
	}
}


void
UHCI::ReadIsochronousDescriptorChain(isochronous_transfer_data *transfer,
	iovec *vector)
{
	size_t vectorOffset = 0;
	usb_isochronous_data *isochronousData
		= transfer->transfer->IsochronousData();

	for (uint32 i = 0; i < isochronousData->packet_count; i++) {
		uhci_td *current = transfer->descriptors[i];

		size_t bufferSize = current->buffer_size;
		size_t actualLength = uhci_td_actual_length(current);

		isochronousData->packet_descriptors[i].actual_length = actualLength;

		if (actualLength > 0)
			isochronousData->packet_descriptors[i].status = B_OK;
		else {
			isochronousData->packet_descriptors[i].status = B_ERROR;
			vectorOffset += bufferSize;
			continue;
		}
		memcpy((uint8 *)vector->iov_base + vectorOffset,
			(uint8 *)current->buffer_log, bufferSize);

		vectorOffset += bufferSize;
	}
}


bool
UHCI::LockIsochronous()
{
	return (mutex_lock(&fIsochronousLock) == B_OK);
}


void
UHCI::UnlockIsochronous()
{
	mutex_unlock(&fIsochronousLock);
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
