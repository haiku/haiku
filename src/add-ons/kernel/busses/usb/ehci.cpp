/*
 * Copyright 2006-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Jérôme Duval <korli@users.berlios.de>
 */


#include <driver_settings.h>
#include <module.h>
#include <PCI.h>
#include <USB3.h>
#include <KernelExport.h>

#include "ehci.h"

#define USB_MODULE_NAME	"ehci"

pci_module_info *EHCI::sPCIModule = NULL;


static int32
ehci_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			TRACE_MODULE("ehci init module\n");
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE_MODULE("ehci uninit module\n");
			return B_OK;
	}

	return EINVAL;
}


usb_host_controller_info ehci_module = {
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

		if (descriptor->next_phy & EHCI_ITEM_TERMINATE)
			break;

		descriptor = descriptor->next_log;
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
	print_descriptor_chain(queueHead->element_log);
}

#endif // TRACE_USB


//
// #pragma mark -
//


EHCI::EHCI(pci_info *info, Stack *stack)
	:	BusManager(stack),
		fCapabilityRegisters(NULL),
		fOperationalRegisters(NULL),
		fRegisterArea(-1),
		fPCIInfo(info),
		fStack(stack),
		fEnabledInterrupts(0),
		fPeriodicFrameListArea(-1),
		fPeriodicFrameList(NULL),
		fInterruptEntries(NULL),
		fAsyncQueueHead(NULL),
		fAsyncAdvanceSem(-1),
		fFirstTransfer(NULL),
		fLastTransfer(NULL),
		fFinishTransfersSem(-1),
		fFinishThread(-1),
		fProcessingPipe(NULL),
		fFreeListHead(NULL),
		fCleanupSem(-1),
		fCleanupThread(-1),
		fStopThreads(false),
		fNextStartingFrame(-1),
		fFrameBandwidth(NULL),
		fFirstIsochronousTransfer(NULL),
		fLastIsochronousTransfer(NULL),
		fFinishIsochronousTransfersSem(-1),
		fFinishIsochronousThread(-1),
		fRootHub(NULL),
		fRootHubAddress(0),
		fPortCount(0),
		fPortResetChange(0),
		fPortSuspendChange(0),
		fInterruptPollThread(-1)
{
	if (BusManager::InitCheck() < B_OK) {
		TRACE_ERROR("bus manager failed to init\n");
		return;
	}

	TRACE("constructing new EHCI host controller driver\n");
	fInitOK = false;

	// ATI/AMD SB600/SB700 periodic list cache workaround
	// Logic kindly borrowed from NetBSD PR 40056
	if (fPCIInfo->vendor_id == AMD_SBX00_VENDOR) {
		bool applyWorkaround = false;

		if (fPCIInfo->device_id == AMD_SB600_EHCI_CONTROLLER) {
			// always apply on SB600
			applyWorkaround = true;
		} else if (fPCIInfo->device_id == AMD_SB700_SB800_EHCI_CONTROLLER) {
			// only apply on certain chipsets, determined by SMBus revision
			pci_info smbus;
			int32 index = 0;
			while (sPCIModule->get_nth_pci_info(index++, &smbus) >= B_OK) {
				if (smbus.vendor_id == AMD_SBX00_VENDOR
					&& smbus.device_id == AMD_SBX00_SMBUS_CONTROLLER) {

					// Only applies to chipsets < SB710 (rev A14)
					if (smbus.revision == 0x3a || smbus.revision == 0x3b)
						applyWorkaround = true;

					break;
				}
			}
		}

		if (applyWorkaround) {
			// According to AMD errata of SB700 and SB600 register documentation
			// this disables the Periodic List Cache on SB600 and the Advanced
			// Periodic List Cache on early SB700. Both the BSDs and Linux use
			// this workaround.

			TRACE_ALWAYS("disabling SB600/SB700 periodic list cache\n");
			uint32 workaround = sPCIModule->read_pci_config(fPCIInfo->bus,
				fPCIInfo->device, fPCIInfo->function,
				AMD_SBX00_EHCI_MISC_REGISTER, 4);

			sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
				fPCIInfo->function, AMD_SBX00_EHCI_MISC_REGISTER, 4,
				workaround | AMD_SBX00_EHCI_MISC_DISABLE_PERIODIC_LIST_CACHE);
		}
	}

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
	fOperationalRegisters = fCapabilityRegisters + ReadCapReg8(EHCI_CAPLENGTH);
	TRACE("mapped capability registers: 0x%08lx\n", (uint32)fCapabilityRegisters);
	TRACE("mapped operational registers: 0x%08lx\n", (uint32)fOperationalRegisters);

	TRACE("structural parameters: 0x%08lx\n", ReadCapReg32(EHCI_HCSPARAMS));
	TRACE("capability parameters: 0x%08lx\n", ReadCapReg32(EHCI_HCCPARAMS));

	if (EHCI_HCCPARAMS_FRAME_CACHE(ReadCapReg32(EHCI_HCCPARAMS)))
		fThreshold = 2 + 8;
	else
		fThreshold = 2 + EHCI_HCCPARAMS_IPT(ReadCapReg32(EHCI_HCCPARAMS));

	// read port count from capability register
	fPortCount = ReadCapReg32(EHCI_HCSPARAMS) & 0x0f;

	uint32 extendedCapPointer = ReadCapReg32(EHCI_HCCPARAMS) >> EHCI_ECP_SHIFT;
	extendedCapPointer &= EHCI_ECP_MASK;
	if (extendedCapPointer > 0) {
		TRACE("extended capabilities register at %ld\n", extendedCapPointer);

		uint32 legacySupport = sPCIModule->read_pci_config(fPCIInfo->bus,
			fPCIInfo->device, fPCIInfo->function, extendedCapPointer, 4);
		if ((legacySupport & EHCI_LEGSUP_CAPID_MASK) == EHCI_LEGSUP_CAPID) {
			if ((legacySupport & EHCI_LEGSUP_BIOSOWNED) != 0) {
				TRACE_ALWAYS("the host controller is bios owned, claiming"
					" ownership\n");

				sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
					fPCIInfo->function, extendedCapPointer + 3, 1, 1);

				for (int32 i = 0; i < 20; i++) {
					legacySupport = sPCIModule->read_pci_config(fPCIInfo->bus,
						fPCIInfo->device, fPCIInfo->function,
						extendedCapPointer, 4);

					if ((legacySupport & EHCI_LEGSUP_BIOSOWNED) == 0)
						break;

					TRACE_ALWAYS("controller is still bios owned, waiting\n");
					snooze(50000);
				}
			}

			if (legacySupport & EHCI_LEGSUP_BIOSOWNED) {
				TRACE_ERROR("bios won't give up control over the host controller (ignoring)\n");
			} else if (legacySupport & EHCI_LEGSUP_OSOWNED) {
				TRACE_ALWAYS("successfully took ownership of the host controller\n");
			}

			// Force off the BIOS owned flag, and clear all SMIs. Some BIOSes
			// do indicate a successful handover but do not remove their SMIs
			// and then freeze the system when interrupts are generated.
			sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
				fPCIInfo->function, extendedCapPointer + 2, 1, 0);
			sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
				fPCIInfo->function, extendedCapPointer + 4, 4, 0);
		} else {
			TRACE_ALWAYS("extended capability is not a legacy support register\n");
		}
	} else {
		TRACE_ALWAYS("no extended capabilities register\n");
	}

	// disable interrupts
	WriteOpReg(EHCI_USBINTR, 0);

	// reset the host controller
	if (ControllerReset() < B_OK) {
		TRACE_ERROR("host controller failed to reset\n");
		return;
	}

	// reset the segment register
	WriteOpReg(EHCI_CTRDSSEGMENT, 0);

	// create semaphores the finisher thread will wait for
	fAsyncAdvanceSem = create_sem(0, "EHCI Async Advance");
	fFinishTransfersSem = create_sem(0, "EHCI Finish Transfers");
	fCleanupSem = create_sem(0, "EHCI Cleanup");
	if (fFinishTransfersSem < B_OK || fAsyncAdvanceSem < B_OK
		|| fCleanupSem < B_OK) {
		TRACE_ERROR("failed to create semaphores\n");
		return;
	}

	// create finisher service thread
	fFinishThread = spawn_kernel_thread(FinishThread, "ehci finish thread",
		B_NORMAL_PRIORITY, (void *)this);
	resume_thread(fFinishThread);

	// Create a lock for the isochronous transfer list
	mutex_init(&fIsochronousLock, "EHCI isochronous lock");

	// Create semaphore the isochronous finisher thread will wait for
	fFinishIsochronousTransfersSem = create_sem(0,
		"EHCI Isochronous Finish Transfers");
	if (fFinishIsochronousTransfersSem < B_OK) {
		TRACE_ERROR("failed to create isochronous finisher semaphore\n");
		return;
	}

	// Create the isochronous finisher service thread
	fFinishIsochronousThread = spawn_kernel_thread(FinishIsochronousThread,
		"ehci isochronous finish thread", B_URGENT_DISPLAY_PRIORITY,
		(void *)this);
	resume_thread(fFinishIsochronousThread);

	// create cleanup service thread
	fCleanupThread = spawn_kernel_thread(CleanupThread, "ehci cleanup thread",
		B_NORMAL_PRIORITY, (void *)this);
	resume_thread(fCleanupThread);

	// set up interrupts or interrupt polling now that the controller is ready
	bool polling = false;
	void *settings = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
	if (settings != NULL) {
		polling = get_driver_boolean_parameter(settings, "ehci_polling", false,
			false);
		unload_driver_settings(settings);
	}

	if (polling) {
		// create and run the polling thread
		TRACE_ALWAYS("enabling ehci polling\n");
		fInterruptPollThread = spawn_kernel_thread(InterruptPollThread,
			"ehci interrupt poll thread", B_NORMAL_PRIORITY, (void *)this);
		resume_thread(fInterruptPollThread);
	} else {
		// install the interrupt handler and enable interrupts
		install_io_interrupt_handler(fPCIInfo->u.h0.interrupt_line,
			InterruptHandler, (void *)this, 0);
	}

	// ensure that interrupts are en-/disabled on the PCI device
	command = sPCIModule->read_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2);
	if (polling == ((command & PCI_command_int_disable) == 0)) {
		if (polling)
			command &= ~PCI_command_int_disable;
		else
			command |= PCI_command_int_disable;

		sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
			fPCIInfo->function, PCI_command, 2, command);
	}

	fEnabledInterrupts = EHCI_USBINTR_HOSTSYSERR | EHCI_USBINTR_USBERRINT
		| EHCI_USBINTR_USBINT | EHCI_USBINTR_INTONAA;
	WriteOpReg(EHCI_USBINTR, fEnabledInterrupts);

	// structures don't span page boundaries
	size_t itdListSize = EHCI_VFRAMELIST_ENTRIES_COUNT
		/ (B_PAGE_SIZE / sizeof(itd_entry)) * B_PAGE_SIZE;
	size_t sitdListSize = EHCI_VFRAMELIST_ENTRIES_COUNT
		/ (B_PAGE_SIZE / sizeof(sitd_entry)) * B_PAGE_SIZE;
	size_t frameListSize = B_PAGE_SIZE + B_PAGE_SIZE + itdListSize
		+ sitdListSize;

	// allocate the periodic frame list
	fPeriodicFrameListArea = fStack->AllocateArea((void **)&fPeriodicFrameList,
		(void **)&physicalAddress, frameListSize, "USB EHCI Periodic Framelist");
	if (fPeriodicFrameListArea < B_OK) {
		TRACE_ERROR("unable to allocate periodic framelist\n");
		return;
	}

	if ((physicalAddress & 0xfff) != 0)
		panic("EHCI_PERIODICLISTBASE not aligned on 4k: 0x%lx\n", physicalAddress);
	// set the periodic frame list base on the controller
	WriteOpReg(EHCI_PERIODICLISTBASE, (uint32)physicalAddress);

	// create the interrupt entries to support different polling intervals
	TRACE("creating interrupt entries\n");
	addr_t physicalBase = physicalAddress + B_PAGE_SIZE;
	uint8 *logicalBase = (uint8 *)fPeriodicFrameList + B_PAGE_SIZE;
	memset(logicalBase, 0, B_PAGE_SIZE);

	fInterruptEntries = (interrupt_entry *)logicalBase;
	for (int32 i = 0; i < EHCI_INTERRUPT_ENTRIES_COUNT; i++) {
		ehci_qh *queueHead = &fInterruptEntries[i].queue_head;
		queueHead->this_phy = physicalBase | EHCI_ITEM_TYPE_QH;
		queueHead->current_qtd_phy = EHCI_ITEM_TERMINATE;
		queueHead->overlay.next_phy = EHCI_ITEM_TERMINATE;
		queueHead->overlay.alt_next_phy = EHCI_ITEM_TERMINATE;
		queueHead->overlay.token = EHCI_QTD_STATUS_HALTED;

		// set dummy endpoint information
		queueHead->endpoint_chars = EHCI_QH_CHARS_EPS_HIGH
			| (3 << EHCI_QH_CHARS_RL_SHIFT) | (64 << EHCI_QH_CHARS_MPL_SHIFT)
			| EHCI_QH_CHARS_TOGGLE;
		queueHead->endpoint_caps = (1 << EHCI_QH_CAPS_MULT_SHIFT)
			| (0xff << EHCI_QH_CAPS_ISM_SHIFT);

		physicalBase += sizeof(interrupt_entry);
	}

	// create the itd and sitd entries
	TRACE("build up iso entries\n");
	addr_t itdPhysicalBase = physicalAddress + B_PAGE_SIZE + B_PAGE_SIZE;
	itd_entry* itds = (itd_entry *)((uint8 *)fPeriodicFrameList + B_PAGE_SIZE
		+ B_PAGE_SIZE);
	memset(itds, 0, itdListSize);

	addr_t sitdPhysicalBase = itdPhysicalBase + itdListSize;
	sitd_entry* sitds = (sitd_entry *)((uint8 *)fPeriodicFrameList + B_PAGE_SIZE
		+ B_PAGE_SIZE + itdListSize);
	memset(sitds, 0, sitdListSize);

	fItdEntries = new(std::nothrow) ehci_itd *[EHCI_VFRAMELIST_ENTRIES_COUNT];
	fSitdEntries = new(std::nothrow) ehci_sitd *[EHCI_VFRAMELIST_ENTRIES_COUNT];

	for (int32 i = 0; i < EHCI_VFRAMELIST_ENTRIES_COUNT; i++) {
		ehci_sitd *sitd = &sitds[i].sitd;
		sitd->this_phy = sitdPhysicalBase | EHCI_ITEM_TYPE_SITD;
		sitd->back_phy = EHCI_ITEM_TERMINATE;
		fSitdEntries[i] = sitd;
		TRACE("sitd entry %ld %p 0x%lx\n", i, sitd, sitd->this_phy);

		ehci_itd *itd = &itds[i].itd;
		itd->this_phy = itdPhysicalBase | EHCI_ITEM_TYPE_ITD;
		itd->next_phy = sitd->this_phy;
		fItdEntries[i] = itd;
		TRACE("itd entry %ld %p 0x%lx\n", i, itd, itd->this_phy);

		sitdPhysicalBase += sizeof(sitd_entry);
		itdPhysicalBase += sizeof(itd_entry);
		if ((sitdPhysicalBase & 0x10) != 0 || (itdPhysicalBase & 0x10) != 0)
			panic("physical base for entry %ld not aligned on 32\n", i);
	}

	// build flat interrupt tree
	TRACE("build up interrupt links\n");
	uint32 interval = EHCI_VFRAMELIST_ENTRIES_COUNT;
	uint32 intervalIndex = EHCI_INTERRUPT_ENTRIES_COUNT - 1;
	while (interval > 1) {
		for (uint32 insertIndex = interval / 2;
			insertIndex < EHCI_VFRAMELIST_ENTRIES_COUNT;
			insertIndex += interval) {
			fSitdEntries[insertIndex]->next_phy =
				fInterruptEntries[intervalIndex].queue_head.this_phy;
		}

		intervalIndex--;
		interval /= 2;
	}

	// setup the empty slot in the list and linking of all -> first
	ehci_qh *firstLogical = &fInterruptEntries[0].queue_head;
	fSitdEntries[0]->next_phy = firstLogical->this_phy;
	for (int32 i = 1; i < EHCI_INTERRUPT_ENTRIES_COUNT; i++) {
		fInterruptEntries[i].queue_head.next_phy = firstLogical->this_phy;
		fInterruptEntries[i].queue_head.next_log = firstLogical;
		fInterruptEntries[i].queue_head.prev_log = NULL;
	}

	// terminate the first entry
	firstLogical->next_phy = EHCI_ITEM_TERMINATE;
	firstLogical->next_log = NULL;
	firstLogical->prev_log = NULL;

	for (int32 i = 0; i < EHCI_FRAMELIST_ENTRIES_COUNT; i++) {
		fPeriodicFrameList[i] =
			fItdEntries[i & (EHCI_VFRAMELIST_ENTRIES_COUNT - 1)]->this_phy;
		TRACE("periodic entry %ld linked to 0x%lx\n", i, fPeriodicFrameList[i]);
	}

	// Create the array that will keep bandwidth information
	fFrameBandwidth = new(std::nothrow) uint16[EHCI_VFRAMELIST_ENTRIES_COUNT];
	for (int32 i = 0; i < EHCI_VFRAMELIST_ENTRIES_COUNT; i++) {
		fFrameBandwidth[i] = MAX_AVAILABLE_BANDWIDTH;
	}

	// allocate a queue head that will always stay in the async frame list
	fAsyncQueueHead = CreateQueueHead();
	if (!fAsyncQueueHead) {
		TRACE_ERROR("unable to allocate stray async queue head\n");
		return;
	}

	fAsyncQueueHead->next_phy = fAsyncQueueHead->this_phy;
	fAsyncQueueHead->next_log = fAsyncQueueHead;
	fAsyncQueueHead->prev_log = fAsyncQueueHead;
	fAsyncQueueHead->endpoint_chars = EHCI_QH_CHARS_EPS_HIGH | EHCI_QH_CHARS_RECHEAD;
	fAsyncQueueHead->endpoint_caps = 1 << EHCI_QH_CAPS_MULT_SHIFT;
	fAsyncQueueHead->current_qtd_phy = EHCI_ITEM_TERMINATE;
	fAsyncQueueHead->overlay.next_phy = EHCI_ITEM_TERMINATE;

	WriteOpReg(EHCI_ASYNCLISTADDR, (uint32)fAsyncQueueHead->this_phy);
	TRACE("set the async list addr to 0x%08lx\n", ReadOpReg(EHCI_ASYNCLISTADDR));

	fInitOK = true;
	TRACE("EHCI host controller driver constructed\n");
}


EHCI::~EHCI()
{
	TRACE("tear down EHCI host controller driver\n");

	WriteOpReg(EHCI_USBCMD, 0);
	WriteOpReg(EHCI_CONFIGFLAG, 0);
	CancelAllPendingTransfers();

	int32 result = 0;
	fStopThreads = true;
	delete_sem(fAsyncAdvanceSem);
	delete_sem(fFinishTransfersSem);
	delete_sem(fFinishIsochronousTransfersSem);
	wait_for_thread(fFinishThread, &result);
	wait_for_thread(fCleanupThread, &result);
	wait_for_thread(fFinishIsochronousThread, &result);

	if (fInterruptPollThread >= 0)
		wait_for_thread(fInterruptPollThread, &result);

	LockIsochronous();
	isochronous_transfer_data *isoTransfer = fFirstIsochronousTransfer;
	while (isoTransfer) {
		isochronous_transfer_data *next = isoTransfer->link;
		delete isoTransfer;
		isoTransfer = next;
	}
	mutex_destroy(&fIsochronousLock);

	delete fRootHub;
	delete [] fFrameBandwidth;
	delete [] fItdEntries;
	delete [] fSitdEntries;
	delete_area(fPeriodicFrameListArea);
	delete_area(fRegisterArea);
	put_module(B_PCI_MODULE_NAME);
}


status_t
EHCI::Start()
{
	TRACE("starting EHCI host controller\n");
	TRACE("usbcmd: 0x%08lx; usbsts: 0x%08lx\n", ReadOpReg(EHCI_USBCMD),
		ReadOpReg(EHCI_USBSTS));

	bool hasPerPortChangeEvent = (ReadCapReg32(EHCI_HCCPARAMS)
		& EHCI_HCCPARAMS_PPCEC) != 0;

	uint32 config = ReadOpReg(EHCI_USBCMD);
	config &= ~((EHCI_USBCMD_ITC_MASK << EHCI_USBCMD_ITC_SHIFT)
		| EHCI_USBCMD_PPCEE);
	uint32 frameListSize = (config >> EHCI_USBCMD_FLS_SHIFT)
		& EHCI_USBCMD_FLS_MASK;

	WriteOpReg(EHCI_USBCMD, config | EHCI_USBCMD_RUNSTOP
		| (hasPerPortChangeEvent ? EHCI_USBCMD_PPCEE : 0)
		| EHCI_USBCMD_ASENABLE | EHCI_USBCMD_PSENABLE
		| (frameListSize << EHCI_USBCMD_FLS_SHIFT)
		| (1 << EHCI_USBCMD_ITC_SHIFT));

	switch (frameListSize) {
		case 0:
			TRACE("frame list size 1024\n");
			break;
		case 1:
			TRACE("frame list size 512\n");
			break;
		case 2:
			TRACE("frame list size 256\n");
			break;
		default:
			TRACE_ALWAYS("unknown frame list size\n");
	}

	bool running = false;
	for (int32 i = 0; i < 10; i++) {
		uint32 status = ReadOpReg(EHCI_USBSTS);
		TRACE("try %ld: status 0x%08lx\n", i, status);

		if (status & EHCI_USBSTS_HCHALTED) {
			snooze(10000);
		} else {
			running = true;
			break;
		}
	}

	if (!running) {
		TRACE_ERROR("host controller didn't start\n");
		return B_ERROR;
	}

	// route all ports to us
	WriteOpReg(EHCI_CONFIGFLAG, EHCI_CONFIGFLAG_FLAG);
	snooze(10000);

	fRootHubAddress = AllocateAddress();
	fRootHub = new(std::nothrow) EHCIRootHub(RootObject(), fRootHubAddress);
	if (!fRootHub) {
		TRACE_ERROR("no memory to allocate root hub\n");
		return B_NO_MEMORY;
	}

	if (fRootHub->InitCheck() < B_OK) {
		TRACE_ERROR("root hub failed init check\n");
		return fRootHub->InitCheck();
	}

	SetRootHub(fRootHub);

	TRACE_ALWAYS("successfully started the controller\n");
	return BusManager::Start();
}


status_t
EHCI::SubmitTransfer(Transfer *transfer)
{
	// short circuit the root hub
	if (transfer->TransferPipe()->DeviceAddress() == fRootHubAddress)
		return fRootHub->ProcessTransfer(this, transfer);

	Pipe *pipe = transfer->TransferPipe();
	if (pipe->Type() & USB_OBJECT_ISO_PIPE)
		return SubmitIsochronous(transfer);

	ehci_qh *queueHead = CreateQueueHead();
	if (!queueHead) {
		TRACE_ERROR("failed to allocate queue head\n");
		return B_NO_MEMORY;
	}

	status_t result = InitQueueHead(queueHead, pipe);
	if (result < B_OK) {
		TRACE_ERROR("failed to init queue head\n");
		FreeQueueHead(queueHead);
		return result;
	}

	bool directionIn;
	ehci_qtd *dataDescriptor;
	if (pipe->Type() & USB_OBJECT_CONTROL_PIPE) {
		result = FillQueueWithRequest(transfer, queueHead, &dataDescriptor,
			&directionIn);
	} else {
		result = FillQueueWithData(transfer, queueHead, &dataDescriptor,
			&directionIn);
	}

	if (result < B_OK) {
		TRACE_ERROR("failed to fill transfer queue with data\n");
		FreeQueueHead(queueHead);
		return result;
	}

	result = AddPendingTransfer(transfer, queueHead, dataDescriptor, directionIn);
	if (result < B_OK) {
		TRACE_ERROR("failed to add pending transfer\n");
		FreeQueueHead(queueHead);
		return result;
	}

#ifdef TRACE_USB
	TRACE("linking queue\n");
	print_queue(queueHead);
#endif

	if (pipe->Type() & USB_OBJECT_INTERRUPT_PIPE)
		result = LinkInterruptQueueHead(queueHead, pipe);
	else
		result = LinkQueueHead(queueHead);

	if (result < B_OK) {
		TRACE_ERROR("failed to link queue head\n");
		FreeQueueHead(queueHead);
		return result;
	}

	return B_OK;
}


status_t
EHCI::SubmitIsochronous(Transfer *transfer)
{
	Pipe *pipe = transfer->TransferPipe();
	bool directionIn = (pipe->Direction() == Pipe::In);
	usb_isochronous_data *isochronousData = transfer->IsochronousData();
	size_t packetSize = transfer->DataLength();
#ifdef TRACE_USB
	size_t restSize = packetSize % isochronousData->packet_count;
#endif
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
	ehci_itd **isoRequest
		= new(std::nothrow) ehci_itd *[isochronousData->packet_count];
	if (isoRequest == NULL) {
		TRACE("failed to create isoRequest array!\n");
		return B_NO_MEMORY;
	}

	TRACE("isochronous submitted size=%ld bytes, TDs=%ld, "
		"maxPacketSize=%ld, packetSize=%ld, restSize=%ld\n", transfer->DataLength(),
		isochronousData->packet_count, pipe->MaxPacketSize(), packetSize, restSize);

	// Find the entry where to start inserting the first Isochronous descriptor
	if (isochronousData->flags & USB_ISO_ASAP ||
		isochronousData->starting_frame_number == NULL) {

		if (fFirstIsochronousTransfer != NULL && fNextStartingFrame != -1)
			currentFrame = fNextStartingFrame;
		else {
			uint32 threshold = fThreshold;
			TRACE("threshold: %ld\n", threshold);

			// find the first available frame with enough bandwidth.
			// This should always be the case, as defining the starting frame
			// number in the driver makes no sense for many reason, one of which
			// is that frame numbers value are host controller specific, and the
			// driver does not know which host controller is running.
			currentFrame = ((ReadOpReg(EHCI_FRINDEX) + threshold) / 8)
				& (EHCI_FRAMELIST_ENTRIES_COUNT - 1);
		}

		// Make sure that:
		// 1. We are at least 5ms ahead the controller
		// 2. We stay in the range 0-127
		// 3. There is enough bandwidth in the first entry
		currentFrame &= EHCI_VFRAMELIST_ENTRIES_COUNT - 1;
	} else {
		// Find out if the frame number specified has enough bandwidth,
		// otherwise find the first next available frame with enough bandwidth
		currentFrame = *isochronousData->starting_frame_number;
	}

	TRACE("isochronous starting frame=%d\n", currentFrame);

	uint16 itdIndex = 0;
	size_t dataLength = transfer->DataLength();
	void* bufferLog;
	addr_t bufferPhy;
	if (fStack->AllocateChunk(&bufferLog, (void**)&bufferPhy, dataLength) < B_OK) {
		TRACE_ERROR("unable to allocate itd buffer\n");
		delete isoRequest;
		return B_NO_MEMORY;
	}

	memset(bufferLog, 0, dataLength);

	addr_t currentPhy = bufferPhy;
	uint32 frameCount = 0;
	while (dataLength > 0) {
		ehci_itd* itd = CreateItdDescriptor();
		isoRequest[itdIndex++] = itd;
		uint16 pg = 0;
		itd->buffer_phy[pg] = currentPhy & 0xfffff000;
		uint32 offset = currentPhy & 0xfff;
		TRACE("isochronous created itd, filling it with phy %lx\n", currentPhy);
		for (int32 i = 0; i < 8 && dataLength > 0; i++) {
			size_t length = min_c(dataLength, packetSize);
			itd->token[i] = (EHCI_ITD_STATUS_ACTIVE << EHCI_ITD_STATUS_SHIFT)
				| (length << EHCI_ITD_TLENGTH_SHIFT) | (pg << EHCI_ITD_PG_SHIFT)
				| (offset << EHCI_ITD_TOFFSET_SHIFT);
			itd->last_token = i;
			TRACE("isochronous filled slot %ld 0x%lx\n", i, itd->token[i]);
			dataLength -= length;
			offset += length;
			if (dataLength > 0 && offset > 0xfff) {
				offset -= B_PAGE_SIZE;
				currentPhy += B_PAGE_SIZE;
				itd->buffer_phy[pg + 1] = currentPhy & 0xfffff000;
				pg++;
			}
			if (dataLength <= 0)
				itd->token[i] |= EHCI_ITD_IOC;
		}

		currentPhy += (offset & 0xfff) - (currentPhy & 0xfff);

		itd->buffer_phy[0] |= (pipe->EndpointAddress() << EHCI_ITD_ENDPOINT_SHIFT)
			| (pipe->DeviceAddress() << EHCI_ITD_ADDRESS_SHIFT);
		itd->buffer_phy[1] |= (pipe->MaxPacketSize() & EHCI_ITD_MAXPACKETSIZE_MASK)
			| (directionIn << EHCI_ITD_DIR_SHIFT);
		itd->buffer_phy[2] |=
			((((pipe->MaxPacketSize() >> EHCI_ITD_MAXPACKETSIZE_LENGTH) + 1)
			& EHCI_ITD_MUL_MASK) << EHCI_ITD_MUL_SHIFT);

		TRACE("isochronous filled itd buffer_phy[0,1,2] 0x%lx, 0x%lx 0x%lx\n",
			itd->buffer_phy[0], itd->buffer_phy[1], itd->buffer_phy[2]);

		if (!LockIsochronous())
			continue;
		LinkITDescriptors(itd, &fItdEntries[currentFrame]);
		UnlockIsochronous();
		fFrameBandwidth[currentFrame] -= bandwidth;
		currentFrame = (currentFrame + 1) & (EHCI_VFRAMELIST_ENTRIES_COUNT - 1);
		frameCount++;
	}

	TRACE("isochronous filled itds count %d\n", itdIndex);

	// Add transfer to the list
	status_t result = AddPendingIsochronousTransfer(transfer, isoRequest,
		itdIndex - 1, directionIn, bufferPhy, bufferLog,
		transfer->DataLength());
	if (result < B_OK) {
		TRACE_ERROR("failed to add pending isochronous transfer\n");
		for (uint32 i = 0; i < itdIndex; i++)
			FreeDescriptor(isoRequest[i]);
		delete isoRequest;
		return result;
	}

	TRACE("appended isochronous transfer by starting at frame number %d\n",
		currentFrame);
	fNextStartingFrame = currentFrame + 1;

	// Wake up the isochronous finisher thread
	release_sem_etc(fFinishIsochronousTransfersSem, 1 /*frameCount*/, B_DO_NOT_RESCHEDULE);

	return B_OK;
}


isochronous_transfer_data *
EHCI::FindIsochronousTransfer(ehci_itd *itd)
{
	// Simply check every last descriptor of the isochronous transfer list
	isochronous_transfer_data *transfer = fFirstIsochronousTransfer;
	if (transfer) {
		while (transfer->descriptors[transfer->last_to_process]
			!= itd) {
			transfer = transfer->link;
			if (!transfer)
				break;
		}
	}
	return transfer;
}


status_t
EHCI::NotifyPipeChange(Pipe *pipe, usb_change change)
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
EHCI::AddTo(Stack *stack)
{
#ifdef TRACE_USB
	set_dprintf_enabled(true);
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	load_driver_symbols("ehci");
#endif
#endif

	if (!sPCIModule) {
		status_t status = get_module(B_PCI_MODULE_NAME, (module_info **)&sPCIModule);
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
			&& item->class_api == PCI_usb_ehci) {
			if (item->u.h0.interrupt_line == 0
				|| item->u.h0.interrupt_line == 0xFF) {
				TRACE_MODULE_ERROR("found device with invalid IRQ - check IRQ assignement\n");
				continue;
			}

			TRACE_MODULE("found device at IRQ %u\n", item->u.h0.interrupt_line);
			EHCI *bus = new(std::nothrow) EHCI(item, stack);
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
	TRACE("reset port %d\n", index);
	uint32 portRegister = EHCI_PORTSC + index * sizeof(uint32);
	uint32 portStatus = ReadOpReg(portRegister) & EHCI_PORTSC_DATAMASK;

	if (portStatus & EHCI_PORTSC_DMINUS) {
		TRACE_ALWAYS("lowspeed device connected, giving up port ownership\n");
		// there is a lowspeed device connected.
		// we give the ownership to a companion controller.
		WriteOpReg(portRegister, portStatus | EHCI_PORTSC_PORTOWNER);
		fPortResetChange |= (1 << index);
		return B_OK;
	}

	// enable reset signaling
	WriteOpReg(portRegister, (portStatus & ~EHCI_PORTSC_ENABLE)
		| EHCI_PORTSC_PORTRESET);
	snooze(50000);

	// disable reset signaling
	portStatus = ReadOpReg(portRegister) & EHCI_PORTSC_DATAMASK;
	WriteOpReg(portRegister, portStatus & ~EHCI_PORTSC_PORTRESET);
	snooze(2000);

	portStatus = ReadOpReg(portRegister) & EHCI_PORTSC_DATAMASK;
	if (portStatus & EHCI_PORTSC_PORTRESET) {
		TRACE_ERROR("port reset won't complete\n");
		return B_ERROR;
	}

	if ((portStatus & EHCI_PORTSC_ENABLE) == 0) {
		TRACE_ALWAYS("fullspeed device connected, giving up port ownership\n");
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
	static spinlock lock = B_SPINLOCK_INITIALIZER;
	acquire_spinlock(&lock);

	// check if any interrupt was generated
	uint32 status = ReadOpReg(EHCI_USBSTS) & EHCI_USBSTS_INTMASK;
	if ((status & fEnabledInterrupts) == 0) {
		if (status != 0) {
			TRACE("discarding not enabled interrupts 0x%08lx\n", status);
			WriteOpReg(EHCI_USBSTS, status);
		}

		release_spinlock(&lock);
		return B_UNHANDLED_INTERRUPT;
	}

	bool asyncAdvance = false;
	bool finishTransfers = false;
	int32 result = B_HANDLED_INTERRUPT;

	if (status & EHCI_USBSTS_USBINT) {
		TRACE("transfer finished\n");
		result = B_INVOKE_SCHEDULER;
		finishTransfers = true;
	}

	if (status & EHCI_USBSTS_USBERRINT) {
		TRACE("transfer error\n");
		result = B_INVOKE_SCHEDULER;
		finishTransfers = true;
	}

	if (status & EHCI_USBSTS_FLROLLOVER)
		TRACE("frame list rollover\n");

	if (status & EHCI_USBSTS_PORTCHANGE)
		TRACE("port change detected\n");

	if (status & EHCI_USBSTS_INTONAA) {
		TRACE("interrupt on async advance\n");
		asyncAdvance = true;
		result = B_INVOKE_SCHEDULER;
	}

	if (status & EHCI_USBSTS_HOSTSYSERR)
		TRACE_ERROR("host system error!\n");

	WriteOpReg(EHCI_USBSTS, status);
	release_spinlock(&lock);

	if (asyncAdvance)
		release_sem_etc(fAsyncAdvanceSem, 1, B_DO_NOT_RESCHEDULE);
	if (finishTransfers)
		release_sem_etc(fFinishTransfersSem, 1, B_DO_NOT_RESCHEDULE);

	return result;
}


int32
EHCI::InterruptPollThread(void *data)
{
	EHCI *ehci = (EHCI *)data;

	while (!ehci->fStopThreads) {
		// TODO: this could be handled much better by only polling when there
		// are actual transfers going on...
		snooze(1000);

		cpu_status status = disable_interrupts();
		ehci->Interrupt();
		restore_interrupts(status);
	}

	return 0;
}


status_t
EHCI::AddPendingTransfer(Transfer *transfer, ehci_qh *queueHead,
	ehci_qtd *dataDescriptor, bool directionIn)
{
	transfer_data *data = new(std::nothrow) transfer_data;
	if (!data)
		return B_NO_MEMORY;

	status_t result = transfer->InitKernelAccess();
	if (result < B_OK) {
		delete data;
		return result;
	}

	data->transfer = transfer;
	data->queue_head = queueHead;
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
	else
		fFirstTransfer = data;

	fLastTransfer = data;
	Unlock();

	return B_OK;
}


status_t
EHCI::AddPendingIsochronousTransfer(Transfer *transfer, ehci_itd **isoRequest,
	uint32 lastIndex, bool directionIn, addr_t bufferPhy, void* bufferLog,
	size_t bufferSize)
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
	data->last_to_process = lastIndex;
	data->incoming = directionIn;
	data->is_active = true;
	data->link = NULL;
	data->buffer_phy = bufferPhy;
	data->buffer_log = bufferLog;
	data->buffer_size = bufferSize;

	// Put in the isochronous transfer list
	if (!LockIsochronous()) {
		delete data;
		return B_ERROR;
	}

	if (fLastIsochronousTransfer)
		fLastIsochronousTransfer->link = data;
	else if (!fFirstIsochronousTransfer)
		fFirstIsochronousTransfer = data;

	fLastIsochronousTransfer = data;
	UnlockIsochronous();
	return B_OK;
}


status_t
EHCI::CancelQueuedTransfers(Pipe *pipe, bool force)
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
			ehci_qtd *descriptor = current->queue_head->element_log;
			while (descriptor) {
				descriptor->token &= ~EHCI_QTD_STATUS_ACTIVE;
				descriptor = descriptor->next_log;
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
EHCI::CancelQueuedIsochronousTransfers(Pipe *pipe, bool force)
{
	isochronous_transfer_data *current = fFirstIsochronousTransfer;

	while (current) {
		if (current->transfer->TransferPipe() == pipe) {
			// TODO implement

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

		TRACE("finishing transfers\n");
		transfer_data *lastTransfer = NULL;
		transfer_data *transfer = fFirstTransfer;
		Unlock();

		while (transfer) {
			bool transferDone = false;
			ehci_qtd *descriptor = transfer->queue_head->element_log;
			status_t callbackStatus = B_OK;

			while (descriptor) {
				uint32 status = descriptor->token;
				if (status & EHCI_QTD_STATUS_ACTIVE) {
					// still in progress
					TRACE("qtd (0x%08lx) still active\n", descriptor->this_phy);
					break;
				}

				if (status & EHCI_QTD_STATUS_ERRMASK) {
					// a transfer error occured
					TRACE_ERROR("qtd (0x%08lx) error: 0x%08lx\n", descriptor->this_phy, status);

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

					transferDone = true;
					break;
				}

				if (descriptor->next_phy & EHCI_ITEM_TERMINATE) {
					// we arrived at the last (stray) descriptor, we're done
					TRACE("qtd (0x%08lx) done\n", descriptor->this_phy);
					callbackStatus = B_OK;
					transferDone = true;
					break;
				}

				descriptor = descriptor->next_log;
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
					bool nextDataToggle = false;
					if (transfer->data_descriptor && transfer->incoming) {
						// data to read out
						iovec *vector = transfer->transfer->Vector();
						size_t vectorCount = transfer->transfer->VectorCount();
						transfer->transfer->PrepareKernelAccess();
						actualLength = ReadDescriptorChain(
							transfer->data_descriptor,
							vector, vectorCount,
							&nextDataToggle);
					} else if (transfer->data_descriptor) {
						// calculate transfered length
						actualLength = ReadActualLength(
							transfer->data_descriptor, &nextDataToggle);
					}

					transfer->transfer->TransferPipe()->SetDataToggle(nextDataToggle);

					if (transfer->transfer->IsFragmented()) {
						// this transfer may still have data left
						transfer->transfer->AdvanceByFragment(actualLength);
						if (transfer->transfer->VectorLength() > 0) {
							FreeDescriptorChain(transfer->data_descriptor);
							transfer->transfer->PrepareKernelAccess();
							status_t result = FillQueueWithData(
								transfer->transfer,
								transfer->queue_head,
								&transfer->data_descriptor, NULL);

							if (result == B_OK && Lock()) {
								// reappend the transfer
								if (fLastTransfer)
									fLastTransfer->link = transfer;
								if (!fFirstTransfer)
									fFirstTransfer = transfer;

								fLastTransfer = transfer;
								Unlock();

								transfer = next;
								continue;
							}
						}

						// the transfer is done, but we already set the
						// actualLength with AdvanceByFragment()
						actualLength = 0;
					}
				}

				transfer->transfer->Finished(callbackStatus, actualLength);
				fProcessingPipe = NULL;
			}

			// unlink hardware queue and delete the transfer
			UnlinkQueueHead(transfer->queue_head, &fFreeListHead);
			delete transfer->transfer;
			delete transfer;
			transfer = next;
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
			ehci_qh *next = current->next_log;
			FreeQueueHead(current);
			current = next;
		}

		lastFreeListHead = freeListHead;
	}
}


int32
EHCI::FinishIsochronousThread(void *data)
{
       ((EHCI *)data)->FinishIsochronousTransfers();
       return B_OK;
}


void
EHCI::FinishIsochronousTransfers()
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

		uint32 frame = (ReadOpReg(EHCI_FRINDEX) / 8 )
			& (EHCI_FRAMELIST_ENTRIES_COUNT - 1);
		uint32 currentFrame = (frame + EHCI_VFRAMELIST_ENTRIES_COUNT - 5)
			& (EHCI_VFRAMELIST_ENTRIES_COUNT - 1);
		uint32 loop = 0;

		// Process the frame list until one transfer is processed
		while (!transferDone && loop++ < EHCI_VFRAMELIST_ENTRIES_COUNT) {
			// wait 1ms in order to be sure to be one position behind
			// the controller
			while (currentFrame == (((ReadOpReg(EHCI_FRINDEX) / 8)
				& (EHCI_VFRAMELIST_ENTRIES_COUNT - 1)))) {
				snooze(1000);
			}

			ehci_itd *itd = fItdEntries[currentFrame];

			TRACE("FinishIsochronousTransfers itd %p phy 0x%lx prev (%p/0x%lx)"
				" at frame %ld\n", itd, itd->this_phy, itd->prev,
				itd->prev != NULL ? itd->prev->this_phy : 0, currentFrame);

			if (!LockIsochronous())
				continue;

			// Process the frame till it has isochronous descriptors in it.
			while (!(itd->next_phy & EHCI_ITEM_TERMINATE) && itd->prev != NULL) {
				TRACE("FinishIsochronousTransfers checking itd %p last_token"
					" %ld\n", itd, itd->last_token);
				TRACE("FinishIsochronousTransfers tokens 0x%lx 0x%lx 0x%lx "
					"0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n",
					itd->token[0], itd->token[1], itd->token[2], itd->token[3],
					itd->token[4], itd->token[5], itd->token[6], itd->token[7]);
				if (((itd->token[itd->last_token] >> EHCI_ITD_STATUS_SHIFT)
					& EHCI_ITD_STATUS_ACTIVE) == EHCI_ITD_STATUS_ACTIVE) {
					TRACE("FinishIsochronousTransfers unprocessed active itd\n");
				}
				UnlinkITDescriptors(itd, &fItdEntries[currentFrame]);

				// Process the transfer if we found the last descriptor
				isochronous_transfer_data *transfer
					= FindIsochronousTransfer(itd);
					// Process the descriptors only if it is still active and
					// belongs to an inbound transfer. If the transfer is not
					// active, it means the request has been removed, so simply
					// remove the descriptors.
				if (transfer && transfer->is_active) {
					TRACE("FinishIsochronousTransfers active transfer\n");
					size_t actualLength = 0;
					if (((itd->buffer_phy[1] >> EHCI_ITD_DIR_SHIFT) & 1) != 0) {
						transfer->transfer->PrepareKernelAccess();
						actualLength = ReadIsochronousDescriptorChain(transfer);
					}

					// Remove the transfer
					if (transfer == fFirstIsochronousTransfer) {
						fFirstIsochronousTransfer = transfer->link;
						if (transfer == fLastIsochronousTransfer)
							fLastIsochronousTransfer = NULL;
					} else {
						isochronous_transfer_data *temp
							= fFirstIsochronousTransfer;
						while (temp != NULL && transfer != temp->link)
							temp = temp->link;

						if (transfer == fLastIsochronousTransfer)
							fLastIsochronousTransfer = temp;
						if (temp != NULL && temp->link != NULL)
							temp->link = temp->link->link;
					}
					transfer->link = NULL;

					transfer->transfer->Finished(B_OK, actualLength);

					itd = itd->prev;

					for (uint32 i = 0; i <= transfer->last_to_process; i++)
						FreeDescriptor(transfer->descriptors[i]);

					TRACE("FinishIsochronousTransfers descriptors freed\n");

					delete [] transfer->descriptors;
					delete transfer->transfer;
					fStack->FreeChunk(transfer->buffer_log,
						(void *)transfer->buffer_phy, transfer->buffer_size);
					delete transfer;
					transferDone = true;
				} else {
					TRACE("FinishIsochronousTransfers not end of transfer\n");
					itd = itd->prev;
				}
			}

			UnlockIsochronous();

			TRACE("FinishIsochronousTransfers next frame\n");

			// Make sure to reset the frame bandwidth
			fFrameBandwidth[currentFrame] = MAX_AVAILABLE_BANDWIDTH;
			currentFrame = (currentFrame + 1) % EHCI_VFRAMELIST_ENTRIES_COUNT;
		}
	}
}


ehci_qh *
EHCI::CreateQueueHead()
{
	ehci_qh *result;
	void *physicalAddress;
	if (fStack->AllocateChunk((void **)&result, &physicalAddress,
		sizeof(ehci_qh)) < B_OK) {
		TRACE_ERROR("failed to allocate queue head\n");
		return NULL;
	}

	result->this_phy = (addr_t)physicalAddress | EHCI_ITEM_TYPE_QH;
	result->next_phy = EHCI_ITEM_TERMINATE;
	result->next_log = NULL;
	result->prev_log = NULL;

	ehci_qtd *descriptor = CreateDescriptor(0, 0);
	if (!descriptor) {
		TRACE_ERROR("failed to allocate initial qtd for queue head\n");
		fStack->FreeChunk(result, physicalAddress, sizeof(ehci_qh));
		return NULL;
	}

	descriptor->token &= ~EHCI_QTD_STATUS_ACTIVE;
	result->stray_log = descriptor;
	result->element_log = descriptor;
	result->current_qtd_phy = EHCI_ITEM_TERMINATE;
	result->overlay.next_phy = descriptor->this_phy;
	result->overlay.alt_next_phy = EHCI_ITEM_TERMINATE;
	result->overlay.token = 0;
	for (int32 i = 0; i < 5; i++) {
		result->overlay.buffer_phy[i] = 0;
		result->overlay.ext_buffer_phy[i] = 0;
	}

	return result;
}


status_t
EHCI::InitQueueHead(ehci_qh *queueHead, Pipe *pipe)
{
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
			TRACE_ERROR("unknown pipe speed\n");
			return B_ERROR;
	}

	queueHead->endpoint_chars |= (3 << EHCI_QH_CHARS_RL_SHIFT)
		| (pipe->MaxPacketSize() << EHCI_QH_CHARS_MPL_SHIFT)
		| (pipe->EndpointAddress() << EHCI_QH_CHARS_EPT_SHIFT)
		| (pipe->DeviceAddress() << EHCI_QH_CHARS_DEV_SHIFT)
		| EHCI_QH_CHARS_TOGGLE;

	queueHead->endpoint_caps = (1 << EHCI_QH_CAPS_MULT_SHIFT);
	if (pipe->Speed() != USB_SPEED_HIGHSPEED) {
		if (pipe->Type() & USB_OBJECT_CONTROL_PIPE)
			queueHead->endpoint_chars |= EHCI_QH_CHARS_CONTROL;

		queueHead->endpoint_caps |= (pipe->HubPort() << EHCI_QH_CAPS_PORT_SHIFT)
			| (pipe->HubAddress() << EHCI_QH_CAPS_HUB_SHIFT);
	}

	return B_OK;
}


void
EHCI::FreeQueueHead(ehci_qh *queueHead)
{
	if (!queueHead)
		return;

	FreeDescriptorChain(queueHead->element_log);
	FreeDescriptor(queueHead->stray_log);
	fStack->FreeChunk(queueHead, (void *)queueHead->this_phy, sizeof(ehci_qh));
}


status_t
EHCI::LinkQueueHead(ehci_qh *queueHead)
{
	if (!Lock())
		return B_ERROR;

	ehci_qh *prevHead = fAsyncQueueHead->prev_log;
	queueHead->next_phy = fAsyncQueueHead->this_phy;
	queueHead->next_log = fAsyncQueueHead;
	queueHead->prev_log = prevHead;
	fAsyncQueueHead->prev_log = queueHead;
	prevHead->next_log = queueHead;
	prevHead->next_phy = queueHead->this_phy;

	Unlock();
	return B_OK;
}


status_t
EHCI::LinkInterruptQueueHead(ehci_qh *queueHead, Pipe *pipe)
{
	if (!Lock())
		return B_ERROR;

	uint8 interval = pipe->Interval();
	if (pipe->Speed() == USB_SPEED_HIGHSPEED) {
		// Allow interrupts to be scheduled on each possible micro frame.
		queueHead->endpoint_caps |= (0xff << EHCI_QH_CAPS_ISM_SHIFT);
	} else {
		// As we do not yet support FSTNs to correctly reference low/full
		// speed interrupt transfers, we simply put them into the 1 interval
		// queue. This way we ensure that we reach them on every micro frame
		// and can do the corresponding start/complete split transactions.
		// ToDo: use FSTNs to correctly link non high speed interrupt transfers
		interval = 1;

		// For now we also force start splits to be in micro frame 0 and
		// complete splits to be in micro frame 2, 3 and 4.
		queueHead->endpoint_caps |= (0x01 << EHCI_QH_CAPS_ISM_SHIFT);
		queueHead->endpoint_caps |= (0x1c << EHCI_QH_CAPS_SCM_SHIFT);
	}

	// this should not happen
	if (interval < 1)
		interval = 1;

	// this may happen as intervals can go up to 16; we limit the value to
	// EHCI_INTERRUPT_ENTRIES_COUNT as you cannot support intervals above
	// that with a frame list of just EHCI_VFRAMELIST_ENTRIES_COUNT entries...
	if (interval > EHCI_INTERRUPT_ENTRIES_COUNT)
		interval = EHCI_INTERRUPT_ENTRIES_COUNT;

	ehci_qh *interruptQueue = &fInterruptEntries[interval - 1].queue_head;
	queueHead->next_phy = interruptQueue->next_phy;
	queueHead->next_log = interruptQueue->next_log;
	queueHead->prev_log = interruptQueue;
	if (interruptQueue->next_log)
		interruptQueue->next_log->prev_log = queueHead;
	interruptQueue->next_log = queueHead;
	interruptQueue->next_phy = queueHead->this_phy;

	Unlock();
	return B_OK;
}


status_t
EHCI::UnlinkQueueHead(ehci_qh *queueHead, ehci_qh **freeListHead)
{
	if (!Lock())
		return B_ERROR;

	ehci_qh *prevHead = queueHead->prev_log;
	ehci_qh *nextHead = queueHead->next_log;
	if (prevHead) {
		prevHead->next_phy = queueHead->next_phy;
		prevHead->next_log = queueHead->next_log;
	}

	if (nextHead)
		nextHead->prev_log = queueHead->prev_log;

	queueHead->next_phy = fAsyncQueueHead->this_phy;
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
		TRACE_ERROR("failed to allocate descriptors\n");
		FreeDescriptor(setupDescriptor);
		FreeDescriptor(statusDescriptor);
		return B_NO_MEMORY;
	}

	iovec vector;
	vector.iov_base = requestData;
	vector.iov_len = sizeof(usb_request_data);
	WriteDescriptorChain(setupDescriptor, &vector, 1);

	ehci_qtd *strayDescriptor = queueHead->stray_log;
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
	queueHead->overlay.alt_next_phy = EHCI_ITEM_TERMINATE;

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
	ehci_qtd *strayDescriptor = queueHead->stray_log;
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
	queueHead->overlay.alt_next_phy = EHCI_ITEM_TERMINATE;

	*_dataDescriptor = firstDescriptor;
	if (_directionIn)
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
		TRACE_ERROR("failed to allocate a qtd\n");
		return NULL;
	}

	result->this_phy = (addr_t)physicalAddress;
	result->next_phy = EHCI_ITEM_TERMINATE;
	result->next_log = NULL;
	result->alt_next_phy = EHCI_ITEM_TERMINATE;
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
		TRACE_ERROR("unable to allocate qtd buffer\n");
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

	fStack->FreeChunk(descriptor, (void *)descriptor->this_phy,
		sizeof(ehci_qtd));
}


void
EHCI::FreeDescriptorChain(ehci_qtd *topDescriptor)
{
	ehci_qtd *current = topDescriptor;
	ehci_qtd *next = NULL;

	while (current) {
		next = current->next_log;
		FreeDescriptor(current);
		current = next;
	}
}


ehci_itd *
EHCI::CreateItdDescriptor()
{
	ehci_itd *result;
	void *physicalAddress;
	if (fStack->AllocateChunk((void **)&result, &physicalAddress,
		sizeof(ehci_itd)) < B_OK) {
		TRACE_ERROR("failed to allocate a itd\n");
		return NULL;
	}

	memset(result, 0, sizeof(ehci_itd));
	result->this_phy = (addr_t)physicalAddress;
	result->next_phy = EHCI_ITEM_TERMINATE;

	return result;
}


ehci_sitd *
EHCI::CreateSitdDescriptor()
{
	ehci_sitd *result;
	void *physicalAddress;
	if (fStack->AllocateChunk((void **)&result, &physicalAddress,
		sizeof(ehci_sitd)) < B_OK) {
		TRACE_ERROR("failed to allocate a sitd\n");
		return NULL;
	}

	memset(result, 0, sizeof(ehci_sitd));
	result->this_phy = (addr_t)physicalAddress | EHCI_ITEM_TYPE_SITD;
	result->next_phy = EHCI_ITEM_TERMINATE;

	return result;
}


void
EHCI::FreeDescriptor(ehci_itd *descriptor)
{
	if (!descriptor)
		return;

	fStack->FreeChunk(descriptor, (void *)descriptor->this_phy,
		sizeof(ehci_itd));
}


void
EHCI::FreeDescriptor(ehci_sitd *descriptor)
{
	if (!descriptor)
		return;

	fStack->FreeChunk(descriptor, (void *)descriptor->this_phy,
		sizeof(ehci_sitd));
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
		first->alt_next_phy = EHCI_ITEM_TERMINATE;
		first->alt_next_log = NULL;
	}
}


void
EHCI::LinkITDescriptors(ehci_itd *itd, ehci_itd **_last)
{
	ehci_itd *last = *_last;
	itd->next_phy = last->next_phy;
	itd->next = NULL;
	itd->prev = last;
	last->next = itd;
	last->next_phy = itd->this_phy;
	*_last = itd;
}


void
EHCI::LinkSITDescriptors(ehci_sitd *sitd, ehci_sitd **_last)
{
	ehci_sitd *last = *_last;
	sitd->next_phy = last->next_phy;
	sitd->next = NULL;
	sitd->prev = last;
	last->next = sitd;
	last->next_phy = sitd->this_phy;
	*_last = sitd;
}

void
EHCI::UnlinkITDescriptors(ehci_itd *itd, ehci_itd **last)
{
	itd->prev->next_phy = itd->next_phy;
	itd->prev->next = itd->next;
	if (itd->next != NULL)
		itd->next->prev = itd->prev;
	if (itd == *last)
		*last = itd->prev;
}


void
EHCI::UnlinkSITDescriptors(ehci_sitd *sitd, ehci_sitd **last)
{
	sitd->prev->next_phy = sitd->next_phy;
	sitd->prev->next = sitd->next;
	if (sitd->next != NULL)
		sitd->next->prev = sitd->prev;
	if (sitd == *last)
		*last = sitd->prev;
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
					TRACE("wrote descriptor chain (%ld bytes, no more vectors)"
						"\n", actualLength);
					return actualLength;
				}

				vectorOffset = 0;
			}

			if (bufferOffset >= current->buffer_size) {
				bufferOffset = 0;
				break;
			}
		}

		if (current->next_phy & EHCI_ITEM_TERMINATE)
			break;

		current = current->next_log;
	}

	TRACE("wrote descriptor chain (%ld bytes)\n", actualLength);
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
		bufferSize -= (current->token >> EHCI_QTD_BYTES_SHIFT)
			& EHCI_QTD_BYTES_MASK;

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
					TRACE("read descriptor chain (%ld bytes, no more vectors)"
						"\n", actualLength);
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

		if (current->next_phy & EHCI_ITEM_TERMINATE)
			break;

		current = current->next_log;
	}

	TRACE("read descriptor chain (%ld bytes)\n", actualLength);
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

		if (current->next_phy & EHCI_ITEM_TERMINATE)
			break;

		current = current->next_log;
	}

	TRACE("read actual length (%ld bytes)\n", actualLength);
	*nextDataToggle = dataToggle > 0 ? true : false;
	return actualLength;
}


size_t
EHCI::WriteIsochronousDescriptorChain(isochronous_transfer_data *transfer,
	uint32 packetCount,	iovec *vector)
{
	// TODO implement
	return 0;
}


size_t
EHCI::ReadIsochronousDescriptorChain(isochronous_transfer_data *transfer)
{
	iovec *vector = transfer->transfer->Vector();
	size_t vectorCount = transfer->transfer->VectorCount();
	size_t vectorOffset = 0;
	size_t vectorIndex = 0;
	usb_isochronous_data *isochronousData
		= transfer->transfer->IsochronousData();
	uint32 packet = 0;
	size_t totalLength = 0;
	size_t bufferOffset = 0;

	size_t packetSize = transfer->transfer->DataLength();
	packetSize /= isochronousData->packet_count;

	for (uint32 i = 0; i <= transfer->last_to_process; i++) {
		ehci_itd *itd = transfer->descriptors[i];
		for (uint32 j = 0; j <= itd->last_token
			&& packet < isochronousData->packet_count; j++) {

			size_t bufferSize = (itd->token[j] >> EHCI_ITD_TLENGTH_SHIFT)
				& EHCI_ITD_TLENGTH_MASK;
			if (((itd->token[j] >> EHCI_ITD_STATUS_SHIFT)
				& EHCI_ITD_STATUS_MASK) != 0) {
				bufferSize = 0;
			}
			isochronousData->packet_descriptors[packet].actual_length =
				bufferSize;

			if (bufferSize > 0)
				isochronousData->packet_descriptors[packet].status = B_OK;
			else
				isochronousData->packet_descriptors[packet].status = B_ERROR;

			totalLength += bufferSize;

			size_t offset = bufferOffset;
			size_t skipSize = packetSize - bufferSize;
			while (bufferSize > 0) {
				size_t length = min_c(bufferSize,
					vector[vectorIndex].iov_len - vectorOffset);
				memcpy((uint8 *)vector[vectorIndex].iov_base + vectorOffset,
					(uint8 *)transfer->buffer_log + bufferOffset, length);
				offset += length;
				vectorOffset += length;
				bufferSize -= length;

				if (vectorOffset >= vector[vectorIndex].iov_len) {
					if (++vectorIndex >= vectorCount) {
						TRACE("read isodescriptor chain (%ld bytes, no more "
							"vectors)\n", totalLength);
						return totalLength;
					}

					vectorOffset = 0;
				}
			}

			// skip to next packet offset
			while (skipSize > 0) {
				size_t length = min_c(skipSize,
					vector[vectorIndex].iov_len - vectorOffset);
				vectorOffset += length;
				skipSize -= length;
				if (vectorOffset >= vector[vectorIndex].iov_len) {
					if (++vectorIndex >= vectorCount) {
						TRACE("read isodescriptor chain (%ld bytes, no more "
							"vectors)\n", totalLength);
						return totalLength;
					}

					vectorOffset = 0;
				}
			}

			bufferOffset += packetSize;
			if (bufferOffset >= transfer->buffer_size)
				return totalLength;

			packet++;
		}
	}

	TRACE("ReadIsochronousDescriptorChain packet count %ld\n", packet);

	return totalLength;
}


bool
EHCI::LockIsochronous()
{
	return (mutex_lock(&fIsochronousLock) == B_OK);
}


void
EHCI::UnlockIsochronous()
{
	mutex_unlock(&fIsochronousLock);
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
