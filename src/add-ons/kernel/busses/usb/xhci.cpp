/*
 * Copyright 2006-2012, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Some code borrowed from the Haiku EHCI driver
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 * 		Jian Chiang <j.jian.chiang@gmail.com>
 *		Jérôme Duval <jerome.duval@gmail.com>
 */


#include <module.h>
#include <PCI.h>
#include <USB3.h>
#include <KernelExport.h>

#include <util/AutoLock.h>

#define TRACE_USB
#include "xhci.h"

#define USB_MODULE_NAME	"xhci"

pci_module_info *XHCI::sPCIModule = NULL;


static int32
xhci_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			TRACE_MODULE("xhci init module\n");
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE_MODULE("xhci uninit module\n");
			return B_OK;
	}

	return EINVAL;
}


static const char*
xhci_error_string(uint32 error)
{
	switch (error) {
		case COMP_INVALID: return "Invalid";
		case COMP_SUCCESS: return "Success";
		case COMP_DATA_BUFFER: return "Data buffer";
		case COMP_BABBLE: return "Babble detected";
		case COMP_USB_TRANSACTION: return "USB transaction";
		case COMP_TRB: return "TRB";
		case COMP_STALL: return "Stall";
		case COMP_RESOURCE: return "Resource";
		case COMP_BANDWIDTH: return "Bandwidth";
		case COMP_NO_SLOTS: return "No slots";
		case COMP_INVALID_STREAM: return "Invalid stream";
		case COMP_SLOT_NOT_ENABLED: return "Slot not enabled";
		case COMP_ENDPOINT_NOT_ENABLED: return "Endpoint not enabled";
		case COMP_SHORT_PACKET: return "Short packet";
		case COMP_RING_UNDERRUN: return "Ring underrun";
		case COMP_RING_OVERRUN: return "Ring overrun";
		case COMP_VF_RING_FULL: return "VF Event Ring Full";
		case COMP_PARAMETER: return "Parameter";
		case COMP_BANDWIDTH_OVERRUN: return "Bandwidth overrun";
		case COMP_CONTEXT_STATE: return "Context state";
		case COMP_NO_PING_RESPONSE: return "No ping response";
		case COMP_EVENT_RING_FULL: return "Event ring full";
		case COMP_INCOMPATIBLE_DEVICE: return "Incompatible device";
		case COMP_MISSED_SERVICE: return "Missed service";
		case COMP_COMMAND_RING_STOPPED: return "Command ring stopped";
		case COMP_COMMAND_ABORTED: return "Command aborted";
		case COMP_STOPPED: return "Stopped";
		case COMP_LENGTH_INVALID: return "Length invalid";
		case COMP_MAX_EXIT_LATENCY: return "Max exit latency too large";
		case COMP_ISOC_OVERRUN: return "Isoch buffer overrun";
		case COMP_EVENT_LOST: return "Event lost";
		case COMP_UNDEFINED: return "Undefined";
		case COMP_INVALID_STREAM_ID: return "Invalid stream ID";
		case COMP_SECONDARY_BANDWIDTH: return "Secondary bandwidth";
		case COMP_SPLIT_TRANSACTION: return "Split transaction";

		default: return "Undefined";
	}
}


usb_host_controller_info xhci_module = {
	{
		"busses/usb/xhci",
		0,
		xhci_std_ops
	},
	NULL,
	XHCI::AddTo
};


module_info *modules[] = {
	(module_info *)&xhci_module,
	NULL
};


XHCI::XHCI(pci_info *info, Stack *stack)
	:	BusManager(stack),
		fCapabilityRegisters(NULL),
		fOperationalRegisters(NULL),
		fRegisterArea(-1),
		fPCIInfo(info),
		fStack(stack),
		fErstArea(-1),
		fDcbaArea(-1),
		fSpinlock(B_SPINLOCK_INITIALIZER),
		fCmdCompSem(-1),
		fFinishTransfersSem(-1),
		fFinishThread(-1),
		fStopThreads(false),
		fFinishedHead(NULL),
		fRootHub(NULL),
		fRootHubAddress(0),
		fPortCount(0),
		fSlotCount(0),
		fScratchpadCount(0),
		fEventSem(-1),
		fEventThread(-1),
		fEventIdx(0),
		fCmdIdx(0),
		fEventCcs(1),
		fCmdCcs(1)
{
	if (BusManager::InitCheck() < B_OK) {
		TRACE_ERROR("bus manager failed to init\n");
		return;
	}

	TRACE("constructing new XHCI host controller driver\n");
	fInitOK = false;

	// enable busmaster and memory mapped access
	uint16 command = sPCIModule->read_pci_config(fPCIInfo->bus,
		fPCIInfo->device, fPCIInfo->function, PCI_command, 2);
	command &= ~(PCI_command_io | PCI_command_int_disable);
	command |= PCI_command_master | PCI_command_memory;

	sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2, command);

	// map the registers
	uint32 offset = fPCIInfo->u.h0.base_registers[0] & (B_PAGE_SIZE - 1);
	phys_addr_t physicalAddress = fPCIInfo->u.h0.base_registers[0] - offset;
	size_t mapSize = (fPCIInfo->u.h0.base_register_sizes[0] + offset
		+ B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	TRACE("map physical memory 0x%08lx (base: 0x%08" B_PRIxPHYSADDR "; offset:"
		" %lx); size: %ld\n", fPCIInfo->u.h0.base_registers[0],
		physicalAddress, offset, fPCIInfo->u.h0.base_register_sizes[0]);

	fRegisterArea = map_physical_memory("XHCI memory mapped registers",
		physicalAddress, mapSize, B_ANY_KERNEL_BLOCK_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_READ_AREA | B_WRITE_AREA,
		(void **)&fCapabilityRegisters);
	if (fRegisterArea < B_OK) {
		TRACE("failed to map register memory\n");
		return;
	}

	uint32 hciCapLength = ReadCapReg32(XHCI_HCI_CAPLENGTH);
	fCapabilityRegisters += offset;
	TRACE("mapped capability length: 0x%lx\n", fCapabilityLength);
	fOperationalRegisters = fCapabilityRegisters + HCI_CAPLENGTH(hciCapLength);
	fRuntimeRegisters = fCapabilityRegisters + ReadCapReg32(XHCI_RTSOFF);
	fDoorbellRegisters = fCapabilityRegisters + ReadCapReg32(XHCI_DBOFF);
	TRACE("mapped capability registers: 0x%08lx\n", (uint32)fCapabilityRegisters);
	TRACE("mapped operational registers: 0x%08lx\n", (uint32)fOperationalRegisters);
	TRACE("mapped runtime registers: 0x%08lx\n", (uint32)fRuntimeRegisters);
	TRACE("mapped doorbell registers: 0x%08lx\n", (uint32)fDoorbellRegisters);

	TRACE("structural parameters1: 0x%08lx\n", ReadCapReg32(XHCI_HCSPARAMS1));
	TRACE("structural parameters2: 0x%08lx\n", ReadCapReg32(XHCI_HCSPARAMS2));
	TRACE("structural parameters3: 0x%08lx\n", ReadCapReg32(XHCI_HCSPARAMS3));
	TRACE("capability parameters: 0x%08lx\n", ReadCapReg32(XHCI_HCCPARAMS));

	uint32 cparams = ReadCapReg32(XHCI_HCCPARAMS);
	uint32 eec = 0xffffffff;
	uint32 eecp = HCS0_XECP(cparams) << 2;
	for (; eecp != 0 && XECP_NEXT(eec); eecp += XECP_NEXT(eec) << 2) {
		eec = ReadCapReg32(eecp);
		if (XECP_ID(eec) != XHCI_LEGSUP_CAPID)
			continue;
	}
	TRACE("eecp register: 0x%04lx\n", eecp);
	if (eec & XHCI_LEGSUP_BIOSOWNED) {
		TRACE_ALWAYS("the host controller is bios owned, claiming"
			" ownership\n");
		WriteCapReg32(eecp, eec | XHCI_LEGSUP_OSOWNED);

		for (int32 i = 0; i < 20; i++) {
			eec = ReadCapReg32(eecp);

			if ((eec & XHCI_LEGSUP_BIOSOWNED) == 0)
				break;

			TRACE_ALWAYS("controller is still bios owned, waiting\n");
			snooze(50000);
		}

		if (eec & XHCI_LEGSUP_BIOSOWNED) {
			TRACE_ERROR("bios won't give up control over the host "
				"controller (ignoring)\n");
		} else if (eec & XHCI_LEGSUP_OSOWNED) {
			TRACE_ALWAYS("successfully took ownership of the host "
				"controller\n");
		}

		// Force off the BIOS owned flag, and clear all SMIs. Some BIOSes
		// do indicate a successful handover but do not remove their SMIs
		// and then freeze the system when interrupts are generated.
		WriteCapReg32(eecp, eec & ~XHCI_LEGSUP_BIOSOWNED);
	}
	WriteCapReg32(eecp + XHCI_LEGCTLSTS, XHCI_LEGCTLSTS_DISABLE_SMI);

	// halt the host controller
	if (ControllerHalt() < B_OK) {
		return;
	}

	// reset the host controller
	if (ControllerReset() < B_OK) {
		TRACE_ERROR("host controller failed to reset\n");
		return;
	}

	fCmdCompSem = create_sem(0, "XHCI Command Complete");
	fFinishTransfersSem = create_sem(0, "XHCI Finish Transfers");
	fEventSem = create_sem(0, "XHCI Event");
	if (fFinishTransfersSem < B_OK || fCmdCompSem < B_OK || fEventSem < B_OK) {
		TRACE_ERROR("failed to create semaphores\n");
		return;
	}

	// create finisher service thread
	fFinishThread = spawn_kernel_thread(FinishThread, "xhci finish thread",
		B_NORMAL_PRIORITY, (void *)this);
	resume_thread(fFinishThread);

	// create finisher service thread
	fEventThread = spawn_kernel_thread(EventThread, "xhci event thread",
		B_NORMAL_PRIORITY, (void *)this);
	resume_thread(fEventThread);

	// Install the interrupt handler
	TRACE("installing interrupt handler\n");
	install_io_interrupt_handler(fPCIInfo->u.h0.interrupt_line,
		InterruptHandler, (void *)this, 0);

	memset(fPortSpeeds, 0, sizeof(fPortSpeeds));
	memset(fPortSlots, 0, sizeof(fPortSlots));
	memset(fDevices, 0, sizeof(fDevices));

	fInitOK = true;
	TRACE("XHCI host controller driver constructed\n");
}


XHCI::~XHCI()
{
	TRACE("tear down XHCI host controller driver\n");

	WriteOpReg(XHCI_CMD, 0);

	int32 result = 0;
	fStopThreads = true;
	delete_sem(fCmdCompSem);
	delete_sem(fFinishTransfersSem);
	delete_sem(fEventSem);
	delete_area(fRegisterArea);
	delete_area(fErstArea);
	for (uint32 i = 0; i < fScratchpadCount; i++)
		delete_area(fScratchpadArea[i]);
	delete_area(fDcbaArea);
	wait_for_thread(fFinishThread, &result);
	wait_for_thread(fEventThread, &result);
	put_module(B_PCI_MODULE_NAME);
}


status_t
XHCI::Start()
{
	TRACE("starting XHCI host controller\n");
	TRACE("usbcmd: 0x%08lx; usbsts: 0x%08lx\n", ReadOpReg(XHCI_CMD),
		ReadOpReg(XHCI_STS));

	if ((ReadOpReg(XHCI_PAGESIZE) & (1 << 0)) == 0) {
		TRACE_ERROR("Controller does not support 4K page size.\n");
		return B_ERROR;
	}

	// read port count from capability register
	uint32 capabilities = ReadCapReg32(XHCI_HCSPARAMS1);

	fPortCount = HCS_MAX_PORTS(capabilities);
	if (fPortCount == 0) {
		TRACE_ERROR("Invalid number of ports: %u\n", fPortCount);
		fPortCount = 0;
		return B_ERROR;
	}
	fSlotCount = HCS_MAX_SLOTS(capabilities);
	WriteOpReg(XHCI_CONFIG, fSlotCount);

	// find out which protocol is used for each port
	uint8 portFound = 0;
	uint32 cparams = ReadCapReg32(XHCI_HCCPARAMS);
	uint32 eec = 0xffffffff;
	uint32 eecp = HCS0_XECP(cparams) << 2;
	for (; eecp != 0 && XECP_NEXT(eec) && portFound < fPortCount;
		eecp += XECP_NEXT(eec) << 2) {
		eec = ReadCapReg32(eecp);
		if (XECP_ID(eec) != XHCI_SUPPORTED_PROTOCOLS_CAPID)
			continue;
		if (XHCI_SUPPORTED_PROTOCOLS_0_MAJOR(eec) > 3)
			continue;
		uint32 temp = ReadCapReg32(eecp + 8);
		uint32 offset = XHCI_SUPPORTED_PROTOCOLS_1_OFFSET(temp);
		uint32 count = XHCI_SUPPORTED_PROTOCOLS_1_COUNT(temp);
		if (offset == 0 || count == 0)
			continue;
		offset--;
		for (uint32 i = offset; i < offset + count; i++) {
			if (XHCI_SUPPORTED_PROTOCOLS_0_MAJOR(eec) == 0x3)
				fPortSpeeds[i] = USB_SPEED_SUPER;
			else
				fPortSpeeds[i] = USB_SPEED_HIGHSPEED;
			TRACE("speed for port %ld is %s\n", i,
				fPortSpeeds[i] == USB_SPEED_SUPER ? "super" : "high");
		}
		portFound += count;
	}

	uint32 params2 = ReadCapReg32(XHCI_HCSPARAMS2);
	fScratchpadCount = HCS_MAX_SC_BUFFERS(params2);
	if (fScratchpadCount > XHCI_MAX_SCRATCHPADS) {
		TRACE_ERROR("Invalid number of scratchpads: %u\n", fScratchpadCount);
		return B_ERROR;
	}

	uint32 params3 = ReadCapReg32(XHCI_HCSPARAMS3);
	fExitLatMax = HCS_U1_DEVICE_LATENCY(params3)
		+ HCS_U2_DEVICE_LATENCY(params3);

	WriteOpReg(XHCI_DNCTRL, 0);

	// allocate Device Context Base Address array
	addr_t dmaAddress;
	fDcbaArea = fStack->AllocateArea((void **)&fDcba, (void**)&dmaAddress,
		sizeof(*fDcba), "DCBA Area");
	if (fDcbaArea < B_OK) {
		TRACE_ERROR("unable to create the DCBA area\n");
		return B_ERROR;
	}
	memset(fDcba, 0, sizeof(*fDcba));
	memset(fScratchpadArea, 0, sizeof(fScratchpadArea));
	memset(fScratchpad, 0, sizeof(fScratchpad));

	// setting the first address to the scratchpad array address
	fDcba->baseAddress[0] = dmaAddress
		+ offsetof(struct xhci_device_context_array, scratchpad);

	// fill up the scratchpad array with scratchpad pages
	for (uint32 i = 0; i < fScratchpadCount; i++) {
		addr_t scratchDmaAddress;
		fScratchpadArea[i] = fStack->AllocateArea((void **)&fScratchpad[i],
		(void**)&scratchDmaAddress, B_PAGE_SIZE, "Scratchpad Area");
		if (fScratchpadArea[i] < B_OK) {
			TRACE_ERROR("unable to create the scratchpad area\n");
			return B_ERROR;
		}
		fDcba->scratchpad[i] = scratchDmaAddress;
	}

	TRACE("setting DCBAAP %lx\n", dmaAddress);
	WriteOpReg(XHCI_DCBAAP_LO, (uint32)dmaAddress);
	WriteOpReg(XHCI_DCBAAP_HI, /*(uint32)(dmaAddress >> 32)*/0);

	// allocate Event Ring Segment Table
	uint8 *addr;
	fErstArea = fStack->AllocateArea((void **)&addr, (void**)&dmaAddress,
		(XHCI_MAX_COMMANDS + XHCI_MAX_EVENTS) * sizeof(xhci_trb)
		+ sizeof(xhci_erst_element),
		"USB XHCI ERST CMD_RING and EVENT_RING Area");

	if (fErstArea < B_OK) {
		TRACE_ERROR("unable to create the ERST AND RING area\n");
		delete_area(fDcbaArea);
		return B_ERROR;
	}
	fErst = (xhci_erst_element *)addr;
	memset(fErst, 0, (XHCI_MAX_COMMANDS + XHCI_MAX_EVENTS) * sizeof(xhci_trb)
		+ sizeof(xhci_erst_element));

	// fill with Event Ring Segment Base Address and Event Ring Segment Size
	fErst->rs_addr = (uint64)(dmaAddress + sizeof(xhci_erst_element));
	fErst->rs_size = XHCI_MAX_EVENTS;
	fErst->rsvdz = 0;

	addr += sizeof(xhci_erst_element);
	fEventRing = (xhci_trb *)addr;
	addr += XHCI_MAX_EVENTS * sizeof(xhci_trb);
	fCmdRing = (xhci_trb *)addr;

	TRACE("setting ERST size\n");
	WriteRunReg32(XHCI_ERSTSZ(0), XHCI_ERSTS_SET(1));

	TRACE("setting ERDP addr = 0x%llx\n", fErst->rs_addr);
	WriteRunReg32(XHCI_ERDP_LO(0), (uint32)fErst->rs_addr);
	WriteRunReg32(XHCI_ERDP_HI(0), /*(uint32)(fErst->rs_addr >> 32)*/0);

	TRACE("setting ERST base addr = 0x%lx\n", dmaAddress);
	WriteRunReg32(XHCI_ERSTBA_LO(0), (uint32)dmaAddress);
	WriteRunReg32(XHCI_ERSTBA_HI(0), /*(uint32)(dmaAddress >> 32)*/0);

	dmaAddress += sizeof(xhci_erst_element) + XHCI_MAX_EVENTS
		* sizeof(xhci_trb);
	TRACE("setting CRCR addr = 0x%lx\n", dmaAddress);
	WriteOpReg(XHCI_CRCR_LO, (uint32)dmaAddress | CRCR_RCS);
	WriteOpReg(XHCI_CRCR_HI, /*(uint32)(dmaAddress >> 32)*/0);
	//link trb
	fCmdRing[XHCI_MAX_COMMANDS - 1].qwtrb0 = dmaAddress;

	TRACE("setting interrupt rate\n");
	WriteRunReg32(XHCI_IMOD(0), 160); // 25000 irq/s

	TRACE("enabling interrupt\n");
	WriteRunReg32(XHCI_IMAN(0), ReadRunReg32(XHCI_IMAN(0)) | IMAN_INTR_ENA);

	WriteOpReg(XHCI_CMD, CMD_RUN | CMD_EIE | CMD_HSEIE);

	// wait for start up state
	int32 tries = 100;
	while ((ReadOpReg(XHCI_STS) & STS_HCH) != 0) {
		snooze(1000);
		if (tries-- < 0) {
			TRACE_ERROR("start up timeout\n");
			break;
		}
	}

	fRootHubAddress = AllocateAddress();
	fRootHub = new(std::nothrow) XHCIRootHub(RootObject(), fRootHubAddress);
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
#ifdef TRACE_USB
	TRACE("No-Op test\n");
	Noop();
#endif
	return BusManager::Start();
}


status_t
XHCI::SubmitTransfer(Transfer *transfer)
{
	// short circuit the root hub
	if (transfer->TransferPipe()->DeviceAddress() == fRootHubAddress)
		return fRootHub->ProcessTransfer(this, transfer);

	TRACE("SubmitTransfer()\n");
	Pipe *pipe = transfer->TransferPipe();
	if ((pipe->Type() & USB_OBJECT_ISO_PIPE) != 0)
		return B_UNSUPPORTED;
	if ((pipe->Type() & USB_OBJECT_CONTROL_PIPE) != 0)
		return SubmitControlRequest(transfer);
	return SubmitNormalRequest(transfer);
}


status_t
XHCI::SubmitControlRequest(Transfer *transfer)
{
	Pipe *pipe = transfer->TransferPipe();
	usb_request_data *requestData = transfer->RequestData();
	bool directionIn = (requestData->RequestType & USB_REQTYPE_DEVICE_IN) != 0;

	TRACE("SubmitControlRequest() length %d\n", requestData->Length);

	xhci_td *setupDescriptor = CreateDescriptor(requestData->Length);

	// set SetupStage
	uint8 index = 0;
	setupDescriptor->trbs[index].dwtrb2 = TRB_2_IRQ(0) | TRB_2_BYTES(8);
	setupDescriptor->trbs[index].dwtrb3 = TRB_3_TYPE(TRB_TYPE_SETUP_STAGE)
		| TRB_3_IDT_BIT | TRB_3_CYCLE_BIT;
	if (requestData->Length > 0) {
		setupDescriptor->trbs[index].dwtrb3 |= directionIn ? TRB_3_TRT_IN
			: TRB_3_TRT_OUT;
	}
	memcpy(&setupDescriptor->trbs[index].qwtrb0, requestData,
		sizeof(usb_request_data));

	index++;

	if (requestData->Length > 0) {
		// set DataStage if any
		setupDescriptor->trbs[index].qwtrb0 = setupDescriptor->buffer_phy[0];
		setupDescriptor->trbs[index].dwtrb2 = TRB_2_IRQ(0)
			| TRB_2_BYTES(requestData->Length)
			| TRB_2_TD_SIZE(transfer->VectorCount());
		setupDescriptor->trbs[index].dwtrb3 = TRB_3_TYPE(TRB_TYPE_DATA_STAGE)
			| (directionIn ? TRB_3_DIR_IN : 0) | TRB_3_CYCLE_BIT;

		// TODO copy data for out transfers
		index++;
	}

	// set StatusStage
	setupDescriptor->trbs[index].dwtrb2 = TRB_2_IRQ(0);
	setupDescriptor->trbs[index].dwtrb3 = TRB_3_TYPE(TRB_TYPE_STATUS_STAGE)
		| ((directionIn && requestData->Length > 0) ? 0 : TRB_3_DIR_IN)
		| TRB_3_IOC_BIT | TRB_3_CYCLE_BIT;

	setupDescriptor->trb_count = index + 1;

	xhci_endpoint *endpoint = (xhci_endpoint *)pipe->ControllerCookie();
	uint8 id = XHCI_ENDPOINT_ID(pipe);
	if (id >= XHCI_MAX_ENDPOINTS)
		return B_BAD_VALUE;
	setupDescriptor->transfer = transfer;
	_LinkDescriptorForPipe(setupDescriptor, endpoint);

	TRACE("SubmitControlRequest() request linked\n");

	Ring(endpoint->device->slot, id);

	return B_OK;
}


status_t
XHCI::SubmitNormalRequest(Transfer *transfer)
{
	TRACE("SubmitNormalRequest() length %ld\n", transfer->DataLength());
	Pipe *pipe = transfer->TransferPipe();
	uint8 id = XHCI_ENDPOINT_ID(pipe);
	if (id >= XHCI_MAX_ENDPOINTS)
		return B_BAD_VALUE;
	bool directionIn = (pipe->Direction() == Pipe::In);

	xhci_td *descriptor = CreateDescriptorChain(transfer->DataLength());
	descriptor->trb_count = descriptor->buffer_count;

	// set NormalStage
	uint8 index;
	for (index = 0; index < descriptor->buffer_count; index++) {
		descriptor->trbs[index].qwtrb0 = descriptor->buffer_phy[index];
		descriptor->trbs[index].dwtrb2 = TRB_2_IRQ(0)
			| TRB_2_BYTES(descriptor->buffer_size[index])
			| TRB_2_TD_SIZE(descriptor->trb_count);
		descriptor->trbs[index].dwtrb3 = TRB_3_TYPE(TRB_TYPE_NORMAL)
			| TRB_3_CYCLE_BIT;
	}
	if (descriptor->trb_count > 0)
		descriptor->trbs[index - 1].dwtrb3 |= TRB_3_IOC_BIT;

	if (!directionIn) {
		TRACE("copying out iov count %ld\n", transfer->VectorCount());
		WriteDescriptorChain(descriptor, transfer->Vector(),
			transfer->VectorCount());
	}
	/*	memcpy(descriptor->buffer_log[index],
				(uint8 *)transfer->Vector()[index].iov_base, transfer->VectorLength());
		}*/

	xhci_endpoint *endpoint = (xhci_endpoint *)pipe->ControllerCookie();
	descriptor->transfer = transfer;
	_LinkDescriptorForPipe(descriptor, endpoint);

	TRACE("SubmitNormalRequest() request linked\n");

	Ring(endpoint->device->slot, id);

	return B_OK;
}


status_t
XHCI::CancelQueuedTransfers(Pipe *pipe, bool force)
{
	return B_OK;
}


status_t
XHCI::NotifyPipeChange(Pipe *pipe, usb_change change)
{
	TRACE("pipe change %d for pipe %p (%d)\n", change, pipe,
		pipe->EndpointAddress());
	switch (change) {
		case USB_CHANGE_CREATED:
			_InsertEndpointForPipe(pipe);
			break;
		case USB_CHANGE_DESTROYED:
			_RemoveEndpointForPipe(pipe);
			break;

		case USB_CHANGE_PIPE_POLICY_CHANGED: {
			// ToDo: for isochronous pipes we might need to adapt to new
			// pipe policy settings here
			break;
		}
	}

	return B_OK;
}


status_t
XHCI::AddTo(Stack *stack)
{
#ifdef TRACE_USB
	set_dprintf_enabled(true);
#endif

	if (!sPCIModule) {
		status_t status = get_module(B_PCI_MODULE_NAME,
			(module_info **)&sPCIModule);
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
			&& item->class_api == PCI_usb_xhci) {
			if (item->u.h0.interrupt_line == 0
				|| item->u.h0.interrupt_line == 0xFF) {
				TRACE_MODULE_ERROR("found device with invalid IRQ - check IRQ "
					"assignment\n");
				continue;
			}

			TRACE_MODULE("found device at IRQ %u\n",
				item->u.h0.interrupt_line);
			XHCI *bus = new(std::nothrow) XHCI(item, stack);
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


xhci_td *
XHCI::CreateDescriptorChain(size_t bufferSize)
{
	size_t packetSize = B_PAGE_SIZE * 16;
	int32 trbCount = (bufferSize + packetSize - 1) / packetSize;
	// keep one trb for linking
	int32 tdCount = (trbCount + XHCI_MAX_TRBS_PER_TD - 2) / (XHCI_MAX_TRBS_PER_TD - 1);

	xhci_td *first = NULL;
	xhci_td *last = NULL;
	for (int32 i = 0; i < tdCount; i++) {
		xhci_td *descriptor = CreateDescriptor(0);
		if (!descriptor) {
			//FreeDescriptorChain(firstDescriptor);
			return NULL;
		} else if (first == NULL)
			first = descriptor;

		uint8 trbs = min_c(trbCount, XHCI_MAX_TRBS_PER_TD);
		TRACE("CreateDescriptorChain trbs %d for td %ld\n", trbs, i);
		for (int j = 0; j < trbs; j++) {
			if (fStack->AllocateChunk(&descriptor->buffer_log[j],
				(void **)&descriptor->buffer_phy[j],
				min_c(packetSize, bufferSize)) < B_OK) {
				TRACE_ERROR("unable to allocate space for the buffer (size %ld)\n",
					bufferSize);
				return NULL;
			}

			descriptor->buffer_size[j] = min_c(packetSize, bufferSize);
			bufferSize -= descriptor->buffer_size[j];
			TRACE("CreateDescriptorChain allocated %ld for trb %d\n",
				descriptor->buffer_size[j], j);
		}

		descriptor->buffer_count = trbs;
		trbCount -= trbs;
		if (last != NULL)
			last->next = descriptor;
		last = descriptor;
	}

	return first;
}


xhci_td *
XHCI::CreateDescriptor(size_t bufferSize)
{
	xhci_td *result;
	addr_t physicalAddress;

	if (fStack->AllocateChunk((void **)&result, (void**)&physicalAddress,
		sizeof(xhci_td)) < B_OK) {
		TRACE_ERROR("failed to allocate a transfer descriptor\n");
		return NULL;
	}

	result->this_phy = (addr_t)physicalAddress;
	result->buffer_size[0] = bufferSize;
	result->trb_count = 0;
	result->buffer_count = 1;
	if (bufferSize <= 0) {
		result->buffer_log[0] = NULL;
		result->buffer_phy[0] = 0;
		return result;
	}

	if (fStack->AllocateChunk(&result->buffer_log[0],
		(void **)&result->buffer_phy[0], bufferSize) < B_OK) {
		TRACE_ERROR("unable to allocate space for the buffer (size %ld)\n",
			bufferSize);
		fStack->FreeChunk(result, (void *)result->this_phy, sizeof(xhci_td));
		return NULL;
	}

	return result;
}


void
XHCI::FreeDescriptor(xhci_td *descriptor)
{
	if (!descriptor)
		return;

	for (int i = 0; i < descriptor->buffer_count; i++) {
		if (descriptor->buffer_size[i] == 0)
			continue;
		TRACE("FreeDescriptor buffer %d buffer_size %ld\n", i,
			descriptor->buffer_size[i]);
		fStack->FreeChunk(descriptor->buffer_log[i],
			(void *)descriptor->buffer_phy[i], descriptor->buffer_size[i]);
	}

	fStack->FreeChunk(descriptor, (void *)descriptor->this_phy,
		sizeof(xhci_td));
}


size_t
XHCI::WriteDescriptorChain(xhci_td *descriptor, iovec *vector,
	size_t vectorCount)
{
	xhci_td *current = descriptor;
	uint8 trbIndex = 0;
	size_t actualLength = 0;
	uint8 vectorIndex = 0;
	size_t vectorOffset = 0;
	size_t bufferOffset = 0;

	while (current != NULL) {
		if (current->buffer_log == NULL)
			break;

		while (true) {
			size_t length = min_c(current->buffer_size[trbIndex] - bufferOffset,
				vector[vectorIndex].iov_len - vectorOffset);

			TRACE("copying %ld bytes to bufferOffset %ld from"
				" vectorOffset %ld at index %d of %ld\n", length, bufferOffset,
				vectorOffset, vectorIndex, vectorCount);
			memcpy((uint8 *)current->buffer_log[trbIndex] + bufferOffset,
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

			if (bufferOffset >= current->buffer_size[trbIndex]) {
				if (++trbIndex >= current->buffer_count)
					break;
				bufferOffset = 0;
			}
		}

		current = current->next;
		trbIndex = 0;
	}

	TRACE("wrote descriptor chain (%ld bytes)\n", actualLength);
	return actualLength;
}


size_t
XHCI::ReadDescriptorChain(xhci_td *descriptor, iovec *vector,
	size_t vectorCount)
{
	xhci_td *current = descriptor;
	uint8 trbIndex = 0;
	size_t actualLength = 0;
	uint8 vectorIndex = 0;
	size_t vectorOffset = 0;
	size_t bufferOffset = 0;

	while (current != NULL) {
		if (current->buffer_log == NULL)
			break;

		while (true) {
			size_t length = min_c(current->buffer_size[trbIndex] - bufferOffset,
				vector[vectorIndex].iov_len - vectorOffset);

			TRACE("copying %ld bytes to vectorOffset %ld from"
				" bufferOffset %ld at index %d of %ld\n", length, vectorOffset,
				bufferOffset, vectorIndex, vectorCount);
			memcpy((uint8 *)vector[vectorIndex].iov_base + vectorOffset,
				(uint8 *)current->buffer_log[trbIndex] + bufferOffset, length);

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

			if (bufferOffset >= current->buffer_size[trbIndex]) {
				if (++trbIndex >= current->buffer_count)
					break;
				bufferOffset = 0;
			}
		}

		current = current->next;
		trbIndex = 0;
	}

	TRACE("read descriptor chain (%ld bytes)\n", actualLength);
	return actualLength;
}


Device *
XHCI::AllocateDevice(Hub *parent, int8 hubAddress, uint8 hubPort,
	usb_speed speed)
{
	TRACE("AllocateDevice hubAddress %d hubPort %d speed %d\n", hubAddress,
		hubPort, speed);
	GetPortSpeed(hubPort - 1, &speed);
	TRACE("speed %d\n", speed);

	uint8 slot = XHCI_MAX_SLOTS;
	if (EnableSlot(&slot) != B_OK) {
		TRACE_ERROR("AllocateDevice() failed enable slot\n");
		return NULL;
	}

	if (slot == 0 || slot > fSlotCount) {
		TRACE_ERROR("AllocateDevice() bad slot\n");
		return NULL;
	}

	if (fDevices[slot].state != XHCI_STATE_DISABLED) {
		TRACE_ERROR("AllocateDevice() slot already used\n");
		return NULL;
	}

	struct xhci_device *device = &fDevices[slot];
	memset(device, 0, sizeof(struct xhci_device));
	device->state = XHCI_STATE_ENABLED;
	device->slot = slot;

	device->input_ctx_area = fStack->AllocateArea((void **)&device->input_ctx,
		(void**)&device->input_ctx_addr, sizeof(*device->input_ctx),
		"XHCI input context");
	if (device->input_ctx_area < B_OK) {
		TRACE_ERROR("unable to create a input context area\n");
		return NULL;
	}

	memset(device->input_ctx, 0, sizeof(*device->input_ctx));
	device->input_ctx->input.dropFlags = 0;
	device->input_ctx->input.addFlags = 3;

	uint32 route = 0;
	uint8 routePort = hubPort;
	uint8 rhPort = 0;
	for (Device *hubDevice = parent; hubDevice != RootObject();
		hubDevice = (Device *)hubDevice->Parent()) {
		route *= 16;
		if (hubPort > 15)
			route += 15;
		else
			route += routePort;
		rhPort = routePort;
		routePort = hubDevice->HubPort();
	}

	device->input_ctx->slot.dwslot0 = SLOT_0_NUM_ENTRIES(1) | SLOT_0_ROUTE(route);
	//device->input_ctx->slot.dwslot0 =
	//	SLOT_0_NUM_ENTRIES(XHCI_MAX_ENDPOINTS - 1) | SLOT_0_ROUTE(route);

	// add the speed
	switch (speed) {
	case USB_SPEED_LOWSPEED:
		device->input_ctx->slot.dwslot0 |= SLOT_0_SPEED(2);
		break;
	case USB_SPEED_HIGHSPEED:
		device->input_ctx->slot.dwslot0 |= SLOT_0_SPEED(3);
		break;
	case USB_SPEED_FULLSPEED:
		device->input_ctx->slot.dwslot0 |= SLOT_0_SPEED(1);
		break;
	case USB_SPEED_SUPER:
		device->input_ctx->slot.dwslot0 |= SLOT_0_SPEED(4);
		break;
	default:
		TRACE_ERROR("unknown usb speed\n");
		break;
	}

	device->input_ctx->slot.dwslot1 = SLOT_1_RH_PORT(rhPort); // TODO enable power save
	device->input_ctx->slot.dwslot2 = SLOT_2_IRQ_TARGET(0);
	if (0)
		device->input_ctx->slot.dwslot2 |= SLOT_2_PORT_NUM(hubPort);
	device->input_ctx->slot.dwslot3 = SLOT_3_SLOT_STATE(0) | SLOT_3_DEVICE_ADDRESS(0);

	TRACE("slot 0x%lx 0x%lx 0x%lx 0x%lx\n", device->input_ctx->slot.dwslot0,
		device->input_ctx->slot.dwslot1, device->input_ctx->slot.dwslot2,
		device->input_ctx->slot.dwslot3);

	device->device_ctx_area = fStack->AllocateArea((void **)&device->device_ctx,
		(void**)&device->device_ctx_addr, sizeof(*device->device_ctx), "XHCI device context");
	if (device->device_ctx_area < B_OK) {
		TRACE_ERROR("unable to create a device context area\n");
		delete_area(device->input_ctx_area);
		return NULL;
	}
	memset(device->device_ctx, 0, sizeof(*device->device_ctx));

	device->trb_area = fStack->AllocateArea((void **)&device->trbs,
		(void**)&device->trb_addr, sizeof(*device->trbs), "XHCI endpoint trbs");
	if (device->trb_area < B_OK) {
		TRACE_ERROR("unable to create a device trbs area\n");
		delete_area(device->input_ctx_area);
		delete_area(device->device_ctx_area);
		return NULL;
	}

	for (uint32 i = 0; i < XHCI_MAX_ENDPOINTS; i++) {
		struct xhci_trb *linkTrb = device->trbs + (i + 1) * XHCI_MAX_TRANSFERS - 1;
		linkTrb->qwtrb0 = device->trb_addr
			+ i * XHCI_MAX_TRANSFERS * sizeof(xhci_trb);
		linkTrb->dwtrb2 = TRB_2_IRQ(0);
		linkTrb->dwtrb3 = TRB_3_CYCLE_BIT | TRB_3_TYPE(TRB_TYPE_LINK);
	}

	// set up slot pointer to device context
	fDcba->baseAddress[slot] = device->device_ctx_addr;

	size_t maxPacketSize;
	switch (speed) {
	case USB_SPEED_LOWSPEED:
	case USB_SPEED_FULLSPEED:
		maxPacketSize = 8;
		break;
	case USB_SPEED_HIGHSPEED:
		maxPacketSize = 64;
		break;
	default:
		maxPacketSize = 512;
		break;
	}

	// configure the Control endpoint 0 (type 4)
	if (ConfigureEndpoint(slot, 0, 4, device->trb_addr, 0, 1, 1, 0,
		maxPacketSize, maxPacketSize, speed) != B_OK) {
		TRACE_ERROR("unable to configure default control endpoint\n");
		return NULL;
	}

	device->endpoints[0].device = device;
	device->endpoints[0].td_head = NULL;
	device->endpoints[0].trbs = device->trbs;
	device->endpoints[0].used = 0;
	device->endpoints[0].current = 0;
	device->endpoints[0].trb_addr = device->trb_addr;
	mutex_init(&device->endpoints[0].lock, "xhci endpoint lock");

	// device should get to addressed state (bsr = 0)
	if (SetAddress(device->input_ctx_addr, false, slot) != B_OK) {
		TRACE_ERROR("unable to set address\n");
		return NULL;
	}

	device->state = XHCI_STATE_ADDRESSED;
	device->address = SLOT_3_DEVICE_ADDRESS_GET(device->device_ctx->slot.dwslot3);

	TRACE("device: address 0x%x state 0x%lx\n", device->address,
		SLOT_3_SLOT_STATE_GET(device->device_ctx->slot.dwslot3));
	TRACE("endpoint0 state 0x%lx\n",
		ENDPOINT_0_STATE_GET(device->device_ctx->endpoints[0].dwendpoint0));

	// Create a temporary pipe with the new address
	ControlPipe pipe(parent);
	pipe.SetControllerCookie(&device->endpoints[0]);
	pipe.InitCommon(device->address + 1, 0, speed, Pipe::Default, 8, 0, hubAddress,
		hubPort);

	// Get the device descriptor
	// Just retrieve the first 8 bytes of the descriptor -> minimum supported
	// size of any device. It is enough because it includes the device type.

	size_t actualLength = 0;
	usb_device_descriptor deviceDescriptor;

	TRACE("getting the device descriptor\n");
	pipe.SendRequest(
		USB_REQTYPE_DEVICE_IN | USB_REQTYPE_STANDARD,		// type
		USB_REQUEST_GET_DESCRIPTOR,							// request
		USB_DESCRIPTOR_DEVICE << 8,							// value
		0,													// index
		8,													// length
		(void *)&deviceDescriptor,							// buffer
		8,													// buffer length
		&actualLength);										// actual length

	if (actualLength != 8) {
		TRACE_ERROR("error while getting the device descriptor\n");
		return NULL;
	}

	TRACE("device_class: %d device_subclass %d device_protocol %d\n",
		deviceDescriptor.device_class, deviceDescriptor.device_subclass,
		deviceDescriptor.device_protocol);

	TRACE("creating new device\n");
	Device *deviceObject = new(std::nothrow) Device(parent, hubAddress, hubPort,
		deviceDescriptor, device->address + 1, speed, false, device);
	if (!deviceObject) {
		TRACE_ERROR("no memory to allocate device\n");
		return NULL;
	}
	fPortSlots[hubPort] = slot;
	TRACE("AllocateDevice() port %d slot %d\n", hubPort, slot);
	return deviceObject;
}


void
XHCI::FreeDevice(Device *device)
{
	uint8 slot = fPortSlots[device->HubPort()];
	TRACE("FreeDevice() port %d slot %d\n", device->HubPort(), slot);
	DisableSlot(slot);
	fDcba->baseAddress[slot] = 0;
	fPortSlots[device->HubPort()] = 0;
	delete_area(fDevices[slot].trb_area);
	delete_area(fDevices[slot].input_ctx_area);
	delete_area(fDevices[slot].device_ctx_area);
	fDevices[slot].state = XHCI_STATE_DISABLED;
	delete device;
}


status_t
XHCI::_InsertEndpointForPipe(Pipe *pipe)
{
	if (pipe->ControllerCookie() != NULL
		|| pipe->Parent()->Type() != USB_OBJECT_DEVICE) {
		// default pipe is already referenced
		return B_OK;
	}

	Device* usbDevice = (Device *)pipe->Parent();
	struct xhci_device *device = (struct xhci_device *)
		usbDevice->ControllerCookie();
	if (usbDevice->Parent() == RootObject())
		return B_OK;
	if (device == NULL) {
		panic("_InsertEndpointForPipe device is NULL\n");
		return B_OK;
	}

	uint8 id = XHCI_ENDPOINT_ID(pipe) - 1;
	if (id >= XHCI_MAX_ENDPOINTS)
		return B_BAD_VALUE;

	if (id > 0) {
		if (SLOT_0_NUM_ENTRIES_GET(device->device_ctx->slot.dwslot0) == 1) {
			device->input_ctx->slot.dwslot0 &= ~(SLOT_0_NUM_ENTRIES(0x1f));
			device->input_ctx->slot.dwslot0 |=
				SLOT_0_NUM_ENTRIES(XHCI_MAX_ENDPOINTS - 1);
			EvaluateContext(device->input_ctx_addr, device->slot);
		}

		device->endpoints[id].device = device;
		device->endpoints[id].trbs = device->trbs
			+ id * XHCI_MAX_TRANSFERS;
		device->endpoints[id].td_head = NULL;
		device->endpoints[id].used = 0;
		device->endpoints[id].trb_addr = device->trb_addr
			+ id * XHCI_MAX_TRANSFERS * sizeof(xhci_trb);
		mutex_init(&device->endpoints[id].lock, "xhci endpoint lock");

		TRACE("_InsertEndpointForPipe trbs device %p endpoint %p\n",
			device->trbs, device->endpoints[id].trbs);
		TRACE("_InsertEndpointForPipe trb_addr device 0x%lx endpoint 0x%lx\n",
			device->trb_addr, device->endpoints[id].trb_addr);

		uint8 endpoint = id + 1;

		StopEndpoint(false, endpoint, device->slot);

		ResetEndpoint(false, endpoint, device->slot);

		SetTRDequeue(device->endpoints[id].trb_addr, 0, endpoint,
			device->slot);

		device->input_ctx->input.dropFlags = 0;
		device->input_ctx->input.addFlags = (1 << endpoint) | (1 << 0);

		// configure the Control endpoint 0 (type 4)
		uint32 type = 4;
		if ((pipe->Type() & USB_OBJECT_INTERRUPT_PIPE) != 0)
			type = 3;
		if ((pipe->Type() & USB_OBJECT_BULK_PIPE) != 0)
			type = 2;
		if ((pipe->Type() & USB_OBJECT_ISO_PIPE) != 0)
			type = 1;
		type |= (pipe->Direction() == Pipe::In) ? (1 << 2) : 0;

		TRACE("trb_addr 0x%lx\n", device->endpoints[id].trb_addr);

		if (ConfigureEndpoint(device->slot, id, type,
			device->endpoints[id].trb_addr, pipe->Interval(),
			1, 1, 0, pipe->MaxPacketSize(), pipe->MaxPacketSize(),
			usbDevice->Speed()) != B_OK) {
			TRACE_ERROR("unable to configure endpoint\n");
			return B_ERROR;
		}

		EvaluateContext(device->input_ctx_addr, device->slot);

		ConfigureEndpoint(device->input_ctx_addr, false, device->slot);
		TRACE("device: address 0x%x state 0x%lx\n", device->address,
			SLOT_3_SLOT_STATE_GET(device->device_ctx->slot.dwslot3));
		TRACE("endpoint[0] state 0x%lx\n",
			ENDPOINT_0_STATE_GET(device->device_ctx->endpoints[0].dwendpoint0));
		TRACE("endpoint[%d] state 0x%lx\n", id,
			ENDPOINT_0_STATE_GET(device->device_ctx->endpoints[id].dwendpoint0));
		device->state = XHCI_STATE_CONFIGURED;
	}
	pipe->SetControllerCookie(&device->endpoints[id]);

	TRACE("_InsertEndpointForPipe for pipe %p at id %d\n", pipe, id);

	return B_OK;
}


status_t
XHCI::_RemoveEndpointForPipe(Pipe *pipe)
{
	if (pipe->Parent()->Type() != USB_OBJECT_DEVICE)
		return B_OK;
	//Device* device = (Device *)pipe->Parent();

	return B_OK;
}


status_t
XHCI::_LinkDescriptorForPipe(xhci_td *descriptor, xhci_endpoint *endpoint)
{
	TRACE("_LinkDescriptorForPipe\n");
	MutexLocker endpointLocker(endpoint->lock);
	if (endpoint->used >= XHCI_MAX_TRANSFERS)
		return B_BAD_VALUE;

	endpoint->used++;
	if (endpoint->td_head == NULL)
		descriptor->next = NULL;
	else
		descriptor->next = endpoint->td_head;
	endpoint->td_head = descriptor;

	uint8 current = endpoint->current;
	uint8 next = (current + 1) % (XHCI_MAX_TRANSFERS - 1);

	TRACE("_LinkDescriptorForPipe current %d, next %d\n", current, next);

	// compute next link
	addr_t addr = endpoint->trb_addr + next * sizeof(struct xhci_trb);
	descriptor->trbs[descriptor->trb_count].qwtrb0 = addr;
	descriptor->trbs[descriptor->trb_count].dwtrb2 = TRB_2_IRQ(0);
	descriptor->trbs[descriptor->trb_count].dwtrb3 = TRB_3_TYPE(TRB_TYPE_LINK)
		| TRB_3_IOC_BIT | TRB_3_CYCLE_BIT;

	endpoint->trbs[next].qwtrb0 = 0;
	endpoint->trbs[next].dwtrb2 = 0;
	endpoint->trbs[next].dwtrb3 = 0;

	endpoint->trbs[current].qwtrb0 = descriptor->this_phy;
	endpoint->trbs[current].dwtrb2 = TRB_2_IRQ(0);
	endpoint->trbs[current].dwtrb3 = TRB_3_TYPE(TRB_TYPE_LINK)
		| TRB_3_CYCLE_BIT;

	TRACE("_LinkDescriptorForPipe pCurrent %p phys 0x%lx 0x%llx 0x%lx\n",
		&endpoint->trbs[current],
		endpoint->trb_addr + current * sizeof(struct xhci_trb),
		endpoint->trbs[current].qwtrb0, endpoint->trbs[current].dwtrb3);
	endpoint->current = next;

	return B_OK;
}


status_t
XHCI::_UnlinkDescriptorForPipe(xhci_td *descriptor, xhci_endpoint *endpoint)
{
	TRACE("_UnlinkDescriptorForPipe\n");
	MutexLocker endpointLocker(endpoint->lock);
	endpoint->used--;
	if (descriptor == endpoint->td_head) {
		endpoint->td_head = descriptor->next;
		descriptor->next = NULL;
		return B_OK;
	} else {
		for (xhci_td *td = endpoint->td_head; td->next != NULL; td = td->next) {
			if (td->next == descriptor) {
				td->next = descriptor->next;
				descriptor->next = NULL;
				return B_OK;
			}
		}
	}

	endpoint->used++;
	return B_ERROR;
}


status_t
XHCI::ConfigureEndpoint(uint8 slot, uint8 number, uint8 type, uint64 ringAddr, uint16 interval,
	uint8 maxPacketCount, uint8 mult, uint8 fpsShift, uint16 maxPacketSize,
	uint16 maxFrameSize, usb_speed speed)
{
	struct xhci_device *device = &fDevices[slot];
	struct xhci_endpoint_ctx *endpoint = &device->input_ctx->endpoints[number];

	if (mult == 0 || maxPacketCount == 0)
		return B_BAD_VALUE;

	maxPacketCount--;

	endpoint->dwendpoint0 = ENDPOINT_0_STATE(0) | ENDPOINT_0_MAXPSTREAMS(0);
	// add mult for isochronous and interrupt types
	switch (speed) {
		case USB_SPEED_LOWSPEED:
		case USB_SPEED_FULLSPEED:
			fpsShift += 3;
		break;
		default:
			break;
	}
	switch (type) {
		case 1:
		case 5:
			if (fpsShift > 3)
				fpsShift--;
		case 3:
		case 7:
			endpoint->dwendpoint0 |= ENDPOINT_0_INTERVAL(fpsShift);
			break;
		default:
			break;
	}
	// add interval
	endpoint->dwendpoint1 = ENDPOINT_1_EPTYPE(type)
		| ENDPOINT_1_MAXBURST(maxPacketCount)
		| ENDPOINT_1_MAXPACKETSIZE(maxPacketSize)
		| ENDPOINT_1_CERR(3);
	endpoint->qwendpoint2 = ENDPOINT_2_DCS_BIT | ringAddr;
	// 8 for Control endpoint
	switch (type) {
		case 4:
			endpoint->dwendpoint4 =	ENDPOINT_4_AVGTRBLENGTH(8);
			break;
		case 1:
		case 3:
		case 5:
		case 7:
			endpoint->dwendpoint4 =	ENDPOINT_4_AVGTRBLENGTH(min_c(maxFrameSize,
				B_PAGE_SIZE)) | ENDPOINT_4_MAXESITPAYLOAD(maxFrameSize);
			break;
		default:
			endpoint->dwendpoint4 =	ENDPOINT_4_AVGTRBLENGTH(B_PAGE_SIZE);
	}

	TRACE("endpoint 0x%lx 0x%lx 0x%llx 0x%lx\n", endpoint->dwendpoint0,
		endpoint->dwendpoint1, endpoint->qwendpoint2, endpoint->dwendpoint4);

	return B_OK;
}


status_t
XHCI::GetPortSpeed(uint8 index, usb_speed *speed)
{
	if (fPortSpeeds[index] == USB_SPEED_SUPER)
		*speed = USB_SPEED_SUPER;
	else {
		uint32 portStatus = ReadOpReg(XHCI_PORTSC(index));

		switch (PS_SPEED_GET(portStatus)) {
		case 3:
			*speed = USB_SPEED_HIGHSPEED;
			break;
		case 2:
			*speed = USB_SPEED_LOWSPEED;
			break;
		case 1:
			*speed = USB_SPEED_FULLSPEED;
			break;
		default:
			*speed = USB_SPEED_SUPER;
		}
	}
	return B_OK;
}


status_t
XHCI::GetPortStatus(uint8 index, usb_port_status *status)
{
	if (index >= fPortCount)
		return B_BAD_INDEX;

	status->status = status->change = 0;
	uint32 portStatus = ReadOpReg(XHCI_PORTSC(index));
	//TRACE("port status=0x%08lx\n", portStatus);

	// build the status
	switch (PS_SPEED_GET(portStatus)) {
	case 3:
		status->status |= PORT_STATUS_HIGH_SPEED;
		break;
	case 2:
		status->status |= PORT_STATUS_LOW_SPEED;
		break;
	default:
		break;
	}

	if (portStatus & PS_CCS)
		status->status |= PORT_STATUS_CONNECTION;
	if (portStatus & PS_PED)
		status->status |= PORT_STATUS_ENABLE;
	if (portStatus & PS_OCA)
		status->status |= PORT_STATUS_OVER_CURRENT;
	if (portStatus & PS_PR)
		status->status |= PORT_STATUS_RESET;
	if (portStatus & PS_PP) {
		if (fPortSpeeds[index] == USB_SPEED_SUPER)
			status->status |= PORT_STATUS_SS_POWER;
		else
			status->status |= PORT_STATUS_POWER;
	}

	// build the change
	if (portStatus & PS_CSC)
		status->change |= PORT_STATUS_CONNECTION;
	if (portStatus & PS_PEC)
		status->change |= PORT_STATUS_ENABLE;
	if (portStatus & PS_OCC)
		status->change |= PORT_STATUS_OVER_CURRENT;
	if (portStatus & PS_PRC)
		status->change |= PORT_STATUS_RESET;

	if (fPortSpeeds[index] == USB_SPEED_SUPER) {
		if (portStatus & PS_PLC)
			status->change |= PORT_LINK_STATE;
		if (portStatus & PS_WRC)
			status->change |= PORT_BH_PORT_RESET;
	}

	return B_OK;
}


status_t
XHCI::SetPortFeature(uint8 index, uint16 feature)
{
	TRACE("set port feature index %u feature %u\n", index, feature);
	if (index >= fPortCount)
		return B_BAD_INDEX;

	uint32 portRegister = XHCI_PORTSC(index);
	uint32 portStatus = ReadOpReg(portRegister) & ~PS_CLEAR;

	switch (feature) {
	case PORT_SUSPEND:
		if ((portStatus & PS_PED) == 0 || (portStatus & PS_PR)
			|| (portStatus & PS_PLS_MASK) >= PS_XDEV_U3) {
			TRACE_ERROR("USB core suspending device not in U0/U1/U2.\n");
			return B_BAD_VALUE;
		}
		portStatus &= ~PS_PLS_MASK;
		WriteOpReg(portRegister, portStatus | PS_LWS | PS_XDEV_U3);
		break;

	case PORT_RESET:
		WriteOpReg(portRegister, portStatus | PS_PR);
		break;

	case PORT_POWER:
		WriteOpReg(portRegister, portStatus | PS_PP);
		break;
	default:
		return B_BAD_VALUE;
	}
	ReadOpReg(portRegister);
	return B_OK;
}


status_t
XHCI::ClearPortFeature(uint8 index, uint16 feature)
{
	TRACE("clear port feature index %u feature %u\n", index, feature);
	if (index >= fPortCount)
		return B_BAD_INDEX;

	uint32 portRegister = XHCI_PORTSC(index);
	uint32 portStatus = ReadOpReg(portRegister) & ~PS_CLEAR;

	switch (feature) {
	case PORT_SUSPEND:
		portStatus = ReadOpReg(portRegister);
		if (portStatus & PS_PR)
			return B_BAD_VALUE;
		if (portStatus & PS_XDEV_U3) {
			if ((portStatus & PS_PED) == 0)
				return B_BAD_VALUE;
			portStatus &= ~PS_PLS_MASK;
			WriteOpReg(portRegister, portStatus | PS_XDEV_U0 | PS_LWS);
		}
		break;
	case PORT_ENABLE:
		WriteOpReg(portRegister, portStatus | PS_PED);
		break;
	case PORT_POWER:
		WriteOpReg(portRegister, portStatus & ~PS_PP);
		break;
	case C_PORT_CONNECTION:
		WriteOpReg(portRegister, portStatus | PS_CSC);
		break;
	case C_PORT_ENABLE:
		WriteOpReg(portRegister, portStatus | PS_PEC);
		break;
	case C_PORT_OVER_CURRENT:
		WriteOpReg(portRegister, portStatus | PS_OCC);
		break;
	case C_PORT_RESET:
		WriteOpReg(portRegister, portStatus | PS_PRC);
		break;
	default:
		return B_BAD_VALUE;
	}

	ReadOpReg(portRegister);
	return B_OK;
}


status_t
XHCI::ControllerHalt()
{
	WriteOpReg(XHCI_CMD, 0);

	int32 tries = 100;
	while ((ReadOpReg(XHCI_STS) & STS_HCH) == 0) {
		snooze(1000);
		if (tries-- < 0)
			return B_ERROR;
	}

	return B_OK;
}


status_t
XHCI::ControllerReset()
{
	TRACE("ControllerReset() cmd: 0x%lx sts: 0x%lx\n", ReadOpReg(XHCI_CMD),
		ReadOpReg(XHCI_STS));
	WriteOpReg(XHCI_CMD, ReadOpReg(XHCI_CMD) | CMD_HCRST);

	int32 tries = 250;
	while (ReadOpReg(XHCI_CMD) & CMD_HCRST) {
		snooze(1000);
		if (tries-- < 0) {
			TRACE("ControllerReset() failed CMD_HCRST\n");
			return B_ERROR;
		}
	}

	tries = 250;
	while (ReadOpReg(XHCI_STS) & STS_CNR) {
		snooze(1000);
		if (tries-- < 0) {
			TRACE("ControllerReset() failed STS_CNR\n");
			return B_ERROR;
		}
	}

	return B_OK;
}


int32
XHCI::InterruptHandler(void *data)
{
	return ((XHCI *)data)->Interrupt();
}


int32
XHCI::Interrupt()
{
	SpinLocker _(&fSpinlock);

	uint32 status = ReadOpReg(XHCI_STS);
	uint32 temp = ReadRunReg32(XHCI_IMAN(0));
	WriteOpReg(XHCI_STS, status);
	WriteRunReg32(XHCI_IMAN(0), temp);

	int32 result = B_HANDLED_INTERRUPT;

	if ((status & STS_HCH) != 0) {
		TRACE_ERROR("Host Controller halted\n");
		return result;
	}
	if ((status & STS_HSE) != 0) {
		TRACE_ERROR("Host System Error\n");
		return result;
	}
	if ((status & STS_HCE) != 0) {
		TRACE_ERROR("Host Controller Error\n");
		return result;
	}

	if ((status & STS_EINT) == 0) {
		TRACE("STS: %lx IRQ_PENDING: %lx\n", status, temp);
		return B_UNHANDLED_INTERRUPT;
	}

	TRACE("Event Interrupt\n");
	release_sem_etc(fEventSem, 1, B_DO_NOT_RESCHEDULE);
	return B_INVOKE_SCHEDULER;
}


void
XHCI::Ring(uint8 slot, uint8 endpoint)
{
	TRACE("Ding Dong! slot:%d endpoint %d\n", slot, endpoint)
	if ((slot == 0 && endpoint > 0) || (slot > 0 && endpoint == 0))
		panic("Ring() invalid slot/endpoint combination\n");
	if (slot > fSlotCount || endpoint > XHCI_MAX_ENDPOINTS)
		panic("Ring() invalid slot or endpoint\n");
	WriteDoorReg32(XHCI_DOORBELL(slot), XHCI_DOORBELL_TARGET(endpoint)
		| XHCI_DOORBELL_STREAMID(0));
	/* Flush PCI posted writes */
	ReadDoorReg32(XHCI_DOORBELL(slot));
}


void
XHCI::QueueCommand(xhci_trb *trb)
{
	uint8 i, j;
	uint32 temp;

	i = fCmdIdx;
	j = fCmdCcs;

	TRACE("command[%u] = %lx (0x%016llx, 0x%08lx, 0x%08lx)\n",
		i, TRB_3_TYPE_GET(trb->dwtrb3),
		trb->qwtrb0, trb->dwtrb2, trb->dwtrb3);

	fCmdRing[i].qwtrb0 = trb->qwtrb0;
	fCmdRing[i].dwtrb2 = trb->dwtrb2;
	temp = trb->dwtrb3;

	if (j)
		temp |= TRB_3_CYCLE_BIT;
	else
		temp &= ~TRB_3_CYCLE_BIT;
	temp &= ~TRB_3_TC_BIT;
	fCmdRing[i].dwtrb3 = temp;

	fCmdAddr = fErst->rs_addr + (XHCI_MAX_EVENTS + i) * sizeof(xhci_trb);

	i++;

	if (i == (XHCI_MAX_COMMANDS - 1)) {
		temp = TRB_3_TYPE(TRB_TYPE_LINK) | TRB_3_TC_BIT;
		if (j)
			temp |= TRB_3_CYCLE_BIT;
		fCmdRing[i].dwtrb3 = temp;

		i = 0;
		j ^= 1;
	}

	fCmdIdx = i;
	fCmdCcs = j;
}


void
XHCI::HandleCmdComplete(xhci_trb *trb)
{
	if (fCmdAddr == trb->qwtrb0) {
		TRACE("Received command event\n");
		fCmdResult[0] = trb->dwtrb2;
		fCmdResult[1] = trb->dwtrb3;
		release_sem_etc(fCmdCompSem, 1, B_DO_NOT_RESCHEDULE);
	}

}


void
XHCI::HandleTransferComplete(xhci_trb *trb)
{
	TRACE("HandleTransferComplete trb %p\n", trb);
	addr_t source = trb->qwtrb0;
	//uint8 completionCode = TRB_2_COMP_CODE_GET(trb->dwtrb2);
	//uint32 remainder = TRB_2_REM_GET(trb->dwtrb2);
	uint8 endpointNumber = TRB_3_ENDPOINT_GET(trb->dwtrb3);
	uint8 slot = TRB_3_SLOT_GET(trb->dwtrb3);

	if (slot > fSlotCount)
		TRACE_ERROR("invalid slot\n");
	if (endpointNumber == 0 || endpointNumber > XHCI_MAX_ENDPOINTS)
		TRACE_ERROR("invalid endpoint\n");

	xhci_device *device = &fDevices[slot];
	xhci_endpoint *endpoint = &device->endpoints[endpointNumber - 1];
	for (xhci_td *td = endpoint->td_head; td != NULL; td = td->next) {
		int64 offset = source - td->this_phy;
		TRACE("HandleTransferComplete td %p offset %lld\n", td, offset);
		_UnlinkDescriptorForPipe(td, endpoint);

		// add descriptor to finished list (to be processed and freed)
		Lock();
		td->next = fFinishedHead;
		fFinishedHead = td;
		Unlock();
		release_sem(fFinishTransfersSem);
		break;
	}
}


status_t
XHCI::DoCommand(xhci_trb *trb)
{
	if (!Lock())
		return B_ERROR;

	QueueCommand(trb);
	Ring(0, 0);

	if (acquire_sem(fCmdCompSem) < B_OK) {
		Unlock();
		return B_ERROR;
	}
	// eat up sems that have been released by multiple interrupts
	int32 semCount = 0;
	get_sem_count(fCmdCompSem, &semCount);
	if (semCount > 0)
		acquire_sem_etc(fCmdCompSem, semCount, B_RELATIVE_TIMEOUT, 0);

	status_t status = B_OK;
	TRACE("Command Complete\n");
	if (TRB_2_COMP_CODE_GET(fCmdResult[0]) != COMP_SUCCESS) {
		uint32 errorCode = TRB_2_COMP_CODE_GET(fCmdResult[0]);
		TRACE_ERROR("unsuccessful command %s (%ld)\n",
			xhci_error_string(errorCode), errorCode);
		status = B_IO_ERROR;
	}

	trb->dwtrb2 = fCmdResult[0];
	trb->dwtrb3 = fCmdResult[1];
	TRACE("Storing trb 0x%08lx 0x%08lx\n", trb->dwtrb2, trb->dwtrb3);

	Unlock();
	return status;
}


status_t
XHCI::Noop()
{
	xhci_trb trb;
	trb.qwtrb0 = 0;
	trb.dwtrb2 = 0;
	trb.dwtrb3 = TRB_3_TYPE(TRB_TYPE_CMD_NOOP);

	return DoCommand(&trb);
}


status_t
XHCI::EnableSlot(uint8 *slot)
{
	xhci_trb trb;
	trb.qwtrb0 = 0;
	trb.dwtrb2 = 0;
	trb.dwtrb3 = TRB_3_TYPE(TRB_TYPE_ENABLE_SLOT);

	status_t status = DoCommand(&trb);
	if (status != B_OK)
		return status;

	*slot = TRB_3_SLOT_GET(trb.dwtrb3);
	return *slot != 0 ? B_OK : B_BAD_VALUE;
}


status_t
XHCI::DisableSlot(uint8 slot)
{
	xhci_trb trb;
	trb.qwtrb0 = 0;
	trb.dwtrb2 = 0;
	trb.dwtrb3 = TRB_3_TYPE(TRB_TYPE_DISABLE_SLOT) | TRB_3_SLOT(slot);

	return DoCommand(&trb);
}


status_t
XHCI::SetAddress(uint64 inputContext, bool bsr, uint8 slot)
{
	xhci_trb trb;
	trb.qwtrb0 = inputContext;
	trb.dwtrb2 = 0;
	trb.dwtrb3 = TRB_3_TYPE(TRB_TYPE_ADDRESS_DEVICE) | TRB_3_SLOT(slot);

	if (bsr)
		trb.dwtrb3 |= TRB_3_BSR_BIT;

	return DoCommand(&trb);
}


status_t
XHCI::ConfigureEndpoint(uint64 inputContext, bool deconfigure, uint8 slot)
{
	xhci_trb trb;
	trb.qwtrb0 = inputContext;
	trb.dwtrb2 = 0;
	trb.dwtrb3 = TRB_3_TYPE(TRB_TYPE_CONFIGURE_ENDPOINT) | TRB_3_SLOT(slot);

	if (deconfigure)
		trb.dwtrb3 |= TRB_3_DCEP_BIT;

	return DoCommand(&trb);
}


status_t
XHCI::EvaluateContext(uint64 inputContext, uint8 slot)
{
	xhci_trb trb;
	trb.qwtrb0 = inputContext;
	trb.dwtrb2 = 0;
	trb.dwtrb3 = TRB_3_TYPE(TRB_TYPE_EVALUATE_CONTEXT) | TRB_3_SLOT(slot);

	return DoCommand(&trb);
}


status_t
XHCI::ResetEndpoint(bool preserve, uint8 endpoint, uint8 slot)
{
	xhci_trb trb;
	trb.qwtrb0 = 0;
	trb.dwtrb2 = 0;
	trb.dwtrb3 = TRB_3_TYPE(TRB_TYPE_RESET_ENDPOINT) | TRB_3_SLOT(slot)
		| TRB_3_ENDPOINT(endpoint);
	if (preserve)
		trb.dwtrb3 |= TRB_3_PRSV_BIT;

	return DoCommand(&trb);
}


status_t
XHCI::StopEndpoint(bool suspend, uint8 endpoint, uint8 slot)
{
	xhci_trb trb;
	trb.qwtrb0 = 0;
	trb.dwtrb2 = 0;
	trb.dwtrb3 = TRB_3_TYPE(TRB_TYPE_STOP_ENDPOINT) | TRB_3_SLOT(slot)
		| TRB_3_ENDPOINT(endpoint);
	if (suspend)
		trb.dwtrb3 |= TRB_3_SUSPEND_ENDPOINT_BIT;

	return DoCommand(&trb);
}


status_t
XHCI::SetTRDequeue(uint64 dequeue, uint16 stream, uint8 endpoint, uint8 slot)
{
	xhci_trb trb;
	trb.qwtrb0 = dequeue;
	trb.dwtrb2 = TRB_2_STREAM(stream);
	trb.dwtrb3 = TRB_3_TYPE(TRB_TYPE_SET_TR_DEQUEUE) | TRB_3_SLOT(slot)
		| TRB_3_ENDPOINT(endpoint);

	return DoCommand(&trb);
}


status_t
XHCI::ResetDevice(uint8 slot)
{
	xhci_trb trb;
	trb.qwtrb0 = 0;
	trb.dwtrb2 = 0;
	trb.dwtrb3 = TRB_3_TYPE(TRB_TYPE_RESET_DEVICE) | TRB_3_SLOT(slot);

	return DoCommand(&trb);
}


int32
XHCI::EventThread(void* data)
{
	((XHCI *)data)->CompleteEvents();
	return B_OK;
}


void
XHCI::CompleteEvents()
{
	while (!fStopThreads) {
		if (acquire_sem(fEventSem) < B_OK)
			continue;

		// eat up sems that have been released by multiple interrupts
		int32 semCount = 0;
		get_sem_count(fEventSem, &semCount);
		if (semCount > 0)
			acquire_sem_etc(fEventSem, semCount, B_RELATIVE_TIMEOUT, 0);

		uint16 i = fEventIdx;
		uint8 j = fEventCcs;
		uint8 t = 2;

		while (1) {
			uint32 temp = fEventRing[i].dwtrb3;
			uint8 k = (temp & TRB_3_CYCLE_BIT) ? 1 : 0;
			if (j != k)
				break;

			uint8 event = TRB_3_TYPE_GET(temp);

			TRACE("event[%u] = %u (0x%016llx 0x%08lx 0x%08lx)\n", i, event,
				fEventRing[i].qwtrb0, fEventRing[i].dwtrb2, fEventRing[i].dwtrb3);
			switch (event) {
			case TRB_TYPE_COMMAND_COMPLETION:
				HandleCmdComplete(&fEventRing[i]);
				break;
			case TRB_TYPE_TRANSFER:
				HandleTransferComplete(&fEventRing[i]);
				break;
			case TRB_TYPE_PORT_STATUS_CHANGE:
				TRACE("port change detected\n");
				break;
			default:
				TRACE_ERROR("Unhandled event = %u\n", event);
				break;
			}

			i++;
			if (i == XHCI_MAX_EVENTS) {
				i = 0;
				j ^= 1;
				if (!--t)
					break;
			}
		}

		fEventIdx = i;
		fEventCcs = j;

		uint64 addr = fErst->rs_addr + i * sizeof(xhci_trb);
		addr |= ERST_EHB;
		WriteRunReg32(XHCI_ERDP_LO(0), (uint32)addr);
		WriteRunReg32(XHCI_ERDP_HI(0), (uint32)(addr >> 32));
	}
}


int32
XHCI::FinishThread(void* data)
{
	((XHCI *)data)->FinishTransfers();
	return B_OK;
}


void
XHCI::FinishTransfers()
{
	while (!fStopThreads) {
		if (acquire_sem(fFinishTransfersSem) < B_OK)
			continue;

		// eat up sems that have been released by multiple interrupts
		int32 semCount = 0;
		get_sem_count(fFinishTransfersSem, &semCount);
		if (semCount > 0)
			acquire_sem_etc(fFinishTransfersSem, semCount, B_RELATIVE_TIMEOUT, 0);

		Lock();
		TRACE("finishing transfers\n");
		while (fFinishedHead != NULL) {
			xhci_td* td = fFinishedHead;
			fFinishedHead = td->next;
			td->next = NULL;
			Unlock();

			Transfer* transfer = td->transfer;
			bool directionIn = (transfer->TransferPipe()->Direction() != Pipe::Out);
			usb_request_data *requestData = transfer->RequestData();

			// TODO check event
			status_t callbackStatus = B_OK;
			size_t actualLength = requestData ? requestData->Length
				: transfer->DataLength();
			TRACE("finishing transfer td %p\n", td);
			if (directionIn && actualLength > 0) {
				if (requestData) {
					TRACE("copying in data %d bytes\n", requestData->Length);
					memcpy((uint8 *)transfer->Vector()[0].iov_base,
						td->buffer_log[0], requestData->Length);
				} else {
					TRACE("copying in iov count %ld\n", transfer->VectorCount());
					ReadDescriptorChain(td, transfer->Vector(),
						transfer->VectorCount());
				}
			}
			transfer->Finished(callbackStatus, actualLength);

			FreeDescriptor(td);
			Lock();
		}
		Unlock();

	}
}

inline void
XHCI::WriteOpReg(uint32 reg, uint32 value)
{
	*(volatile uint32 *)(fOperationalRegisters + reg) = value;
}


inline uint32
XHCI::ReadOpReg(uint32 reg)
{
	return *(volatile uint32 *)(fOperationalRegisters + reg);
}


inline uint32
XHCI::ReadCapReg32(uint32 reg)
{
	return *(volatile uint32 *)(fCapabilityRegisters + reg);
}


inline void
XHCI::WriteCapReg32(uint32 reg, uint32 value)
{
	*(volatile uint32 *)(fCapabilityRegisters + reg) = value;
}


inline uint32
XHCI::ReadRunReg32(uint32 reg)
{
	return *(volatile uint32 *)(fRuntimeRegisters + reg);
}


inline void
XHCI::WriteRunReg32(uint32 reg, uint32 value)
{
	*(volatile uint32 *)(fRuntimeRegisters + reg) = value;
}


inline uint32
XHCI::ReadDoorReg32(uint32 reg)
{
	return *(volatile uint32 *)(fDoorbellRegisters + reg);
}


inline void
XHCI::WriteDoorReg32(uint32 reg, uint32 value)
{
	*(volatile uint32 *)(fDoorbellRegisters + reg) = value;
}
