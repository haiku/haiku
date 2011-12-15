/*
 * Copyright 2006-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Some code borrowed from the Haiku EHCI driver
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 * 		Jian Chiang <j.jian.chiang@gmail.com>
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
		fRootHub(NULL),
		fRootHubAddress(0),
		fPortCount(0),
		fSlotCount(0),
		fScratchpadCount(0),
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

	fCapabilityRegisters += offset;
	fOperationalRegisters = fCapabilityRegisters + ReadCapReg8(XHCI_CAPLENGTH);
	fRuntimeRegisters = fCapabilityRegisters + ReadCapReg32(XHCI_RTSOFF);
	fDoorbellRegisters = fCapabilityRegisters + ReadCapReg32(XHCI_DBOFF);
	TRACE("mapped capability registers: 0x%08lx\n", (uint32)fCapabilityRegisters);
	TRACE("mapped operational registers: 0x%08lx\n", (uint32)fOperationalRegisters);
	TRACE("mapped rumtime registers: 0x%08lx\n", (uint32)fRuntimeRegisters);
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
	if (fFinishTransfersSem < B_OK || fCmdCompSem < B_OK) {
		TRACE_ERROR("failed to create semaphores\n");
		return;
	}

	// create finisher service thread
	fFinishThread = spawn_kernel_thread(FinishThread, "xhci finish thread",
		B_NORMAL_PRIORITY, (void *)this);
	resume_thread(fFinishThread);

	// Install the interrupt handler
	TRACE("installing interrupt handler\n");
	install_io_interrupt_handler(fPCIInfo->u.h0.interrupt_line,
		InterruptHandler, (void *)this, 0);

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
	delete_area(fRegisterArea);
	delete_area(fErstArea);
	for (uint32 i = 0; i < fScratchpadCount; i++)
		delete_area(fScratchpadArea[i]);
	delete_area(fDcbaArea);
	wait_for_thread(fFinishThread, &result);
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

	uint32 params2 = ReadCapReg32(XHCI_HCSPARAMS2);
	fScratchpadCount = HCS_MAX_SC_BUFFERS(params2);
	if (fScratchpadCount > XHCI_MAX_SCRATCHPADS) {
		TRACE_ERROR("Invalid number of scratchpads: %u\n", fScratchpadCount);
		return B_ERROR;
	}

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

	dmaAddress += sizeof(xhci_erst_element) + XHCI_MAX_EVENTS * sizeof(xhci_trb);
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
	if (pipe->Type() & USB_OBJECT_ISO_PIPE)
		return B_UNSUPPORTED;

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
XHCI::AddTo(Stack *stack)
{
#ifdef TRACE_USB
	set_dprintf_enabled(true);
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	load_driver_symbols("xhci");
#endif
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


status_t
XHCI::GetPortStatus(uint8 index, usb_port_status *status)
{
	if (index >= fPortCount)
		return B_BAD_INDEX;

	status->status = status->change = 0;
	uint32 portStatus = ReadOpReg(XHCI_PORTSC(index));
	TRACE("port status=0x%08lx\n", portStatus);

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
	if (portStatus & PS_PP)
		status->status |= PORT_STATUS_POWER;

	// build the change
	if (portStatus & PS_CSC)
		status->change |= PORT_STATUS_CONNECTION;
	if (portStatus & PS_PEC)
		status->change |= PORT_STATUS_ENABLE;
	if (portStatus & PS_OCC)
		status->change |= PORT_STATUS_OVER_CURRENT;
	if (portStatus & PS_PRC)
		status->change |= PORT_STATUS_RESET;

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
		return B_OK;
	case PORT_ENABLE:
		WriteOpReg(portRegister, portStatus | PS_PED);
		return B_OK;
	case PORT_POWER:
		WriteOpReg(portRegister, portStatus & ~PS_PP);
		return B_OK;
	case C_PORT_CONNECTION:
		WriteOpReg(portRegister, portStatus | PS_CSC);
		return B_OK;
	case C_PORT_ENABLE:
		WriteOpReg(portRegister, portStatus | PS_PEC);
		return B_OK;
	case C_PORT_OVER_CURRENT:
		WriteOpReg(portRegister, portStatus | PS_OCC);
		return B_OK;
	case C_PORT_RESET:
		WriteOpReg(portRegister, portStatus | PS_PRC);
		return B_OK;
	}

	return B_BAD_VALUE;
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
	TRACE("STS: %lx IRQ_PENDING: %lx\n", status, temp);

	int32 result = B_HANDLED_INTERRUPT;

	if (status & STS_HCH) {
		TRACE_ERROR("Host Controller halted\n");
		return result;
	}
	if (status & STS_HSE) {
		TRACE_ERROR("Host System Error\n");
		return result;
	}
	if (status & STS_HCE) {
		TRACE_ERROR("Host Controller Error\n");
		return result;
	}
	uint16 i = fEventIdx;
	uint8 j = fEventCcs;
	uint8 t = 2;

	while (1) {
		temp = fEventRing[i].dwtrb3;
		uint8 k = (temp & TRB_3_CYCLE_BIT) ? 1 : 0;
		if (j != k)
			break;

		uint8 event = TRB_3_TYPE_GET(temp);

		TRACE("event[%u] = %u (0x%016llx 0x%08lx 0x%08lx)\n", i, event,
			fEventRing[i].qwtrb0, fEventRing[i].dwtrb2, fEventRing[i].dwtrb3);
		switch (event) {
		case TRB_TYPE_COMMAND_COMPLETION:
			HandleCmdComplete(&fEventRing[i]);
			result = B_INVOKE_SCHEDULER;
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

	return result;
}


void
XHCI::Ring()
{
	TRACE("Ding Dong!\n")
	WriteDoorReg32(XHCI_DOORBELL(0), 0);
	/* Flush PCI posted writes */
	ReadDoorReg32(XHCI_DOORBELL(0));
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


status_t
XHCI::DoCommand(xhci_trb *trb)
{
	if (!Lock())
		return B_ERROR;

	QueueCommand(trb);
	Ring();

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
		TRACE_ERROR("unsuccessful no-op command %ld\n",
			TRB_2_COMP_CODE_GET(fCmdResult[0]));
		status = B_IO_ERROR;
	}

	trb->dwtrb2 = fCmdResult[0];
	trb->dwtrb3 = fCmdResult[1];

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
	return B_OK;
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
XHCI::FinishThread(void *data)
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

		TRACE("finishing transfers\n");
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


inline uint8
XHCI::ReadCapReg8(uint32 reg)
{
	return *(volatile uint8 *)(fCapabilityRegisters + reg);
}


inline uint16
XHCI::ReadCapReg16(uint32 reg)
{
	return *(volatile uint16 *)(fCapabilityRegisters + reg);
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
