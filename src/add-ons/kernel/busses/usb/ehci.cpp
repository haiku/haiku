/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <module.h>
#include <PCI.h>
#include <USB3.h>
#include <KernelExport.h>

#include "ehci.h"

pci_module_info *EHCI::sPCIModule = NULL;


static int32
ehci_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			TRACE(("usb_ehci_module: init module\n"));
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE(("usb_ehci_module: uninit module\n"));
			return B_OK;
	}

	return EINVAL;
}


host_controller_info ehci_module = {
	{
		"busses/usb/ehci",
		NULL,
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


EHCI::EHCI(pci_info *info, Stack *stack)
	:	BusManager(stack),
		fPCIInfo(info),
		fStack(stack),
		fPeriodicFrameListArea(-1),
		fPeriodicFrameList(NULL),
		fFirstTransfer(NULL),
		fLastTransfer(NULL),
		fFinishTransfers(false),
		fFinishThread(-1),
		fStopFinishThread(false),
		fRootHub(NULL),
		fRootHubAddress(0),
		fPortCount(0),
		fPortResetChange(0),
		fPortSuspendChange(0)
{
	if (BusManager::InitCheck() < B_OK) {
		TRACE_ERROR(("usb_ehci: bus manager failed to init\n"));
		return;
	}

	TRACE(("usb_ehci: constructing new EHCI Host Controller Driver\n"));
	fInitOK = false;

	// make sure we take the controller away from BIOS
	sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device, 2,
		PCI_LEGSUP, 2, PCI_LEGSUP_USBPIRQDEN);

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

	TRACE(("usb_ehci: map physical memory 0x%08x (base: 0x%08x; offset: %x); size: %d -> %d\n", fPCIInfo->u.h0.base_registers[0], physicalAddress, offset, fPCIInfo->u.h0.base_register_sizes[0], mapSize));
	fRegisterArea = map_physical_memory("EHCI memory mapped registers",
		(void *)physicalAddress, mapSize, B_ANY_KERNEL_BLOCK_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_READ_AREA | B_WRITE_AREA,
		(void **)&fCapabilityRegisters);
	if (fRegisterArea < B_OK) {
		TRACE(("usb_ehci: failed to map register memory\n"));
		return;
	}

	fCapabilityRegisters += offset;
	fOperationalRegisters = fCapabilityRegisters + ReadCapReg8(EHCI_CAPLENGTH);
	TRACE(("usb_ehci: mapped capability registers: 0x%08x\n", fCapabilityRegisters));
	TRACE(("usb_ehci: mapped operational registers: 0x%08x\n", fOperationalRegisters));

	// read port count from capability register
	fPortCount = ReadCapReg32(EHCI_HCSPARAMS) & 0x0f;

	// disable interrupts
	WriteOpReg(EHCI_USBINTR, 0);

	// reset the host controller
	if (ControllerReset() < B_OK) {
		TRACE_ERROR(("usb_ehci: host controller failed to reset\n"));
		return;
	}

	// create finisher service thread
	fFinishThread = spawn_kernel_thread(FinishThread, "ehci finish thread",
		B_NORMAL_PRIORITY, (void *)this);
	resume_thread(fFinishThread);

	// install the interrupt handler and enable interrupts
	install_io_interrupt_handler(fPCIInfo->u.h0.interrupt_line,
		InterruptHandler, (void *)this, 0);
	WriteOpReg(EHCI_USBINTR, EHCI_USBINTR_HOSTSYSERR
		| EHCI_USBINTR_USBERRINT | EHCI_USBINTR_USBINT);

	// allocate the periodic frame list
	fPeriodicFrameListArea = fStack->AllocateArea((void **)&fPeriodicFrameList,
		(void **)&physicalAddress, B_PAGE_SIZE, "USB EHCI Periodic Framelist");
	if (fPeriodicFrameListArea < B_OK) {
		TRACE_ERROR(("usb_ehci: unable to allocate periodic framelist\n"));
		return;
	}

	// terminate all elements
	for (int32 i = 0; i < 1024; i++)
		fPeriodicFrameList[i] = EHCI_PFRAMELIST_TERM;

	WriteOpReg(EHCI_PERIODICLISTBASE, (uint32)physicalAddress);

	// route all ports to us
	WriteOpReg(EHCI_CONFIGFLAG, EHCI_CONFIGFLAG_FLAG);

	TRACE(("usb_ehci: EHCI Host Controller Driver constructed\n"));
	fInitOK = true;
}


EHCI::~EHCI()
{
	TRACE(("usb_ehci: tear down EHCI Host Controller Driver\n"));

	int32 result = 0;
	fStopFinishThread = true;
	wait_for_thread(fFinishThread, &result);

	CancelAllPendingTransfers();

	delete fRootHub;
	delete_area(fPeriodicFrameListArea);
	delete_area(fRegisterArea);
	put_module(B_PCI_MODULE_NAME);
}


status_t
EHCI::Start()
{
	TRACE(("usb_ehci: starting EHCI Host Controller\n"));
	TRACE(("usb_ehci: usbcmd: 0x%08x; usbsts: 0x%08x\n", ReadOpReg(EHCI_USBCMD), ReadOpReg(EHCI_USBSTS)));

	WriteOpReg(EHCI_USBCMD, ReadOpReg(EHCI_USBCMD) | EHCI_USBCMD_RUNSTOP);

	bool running = false;
	for (int32 i = 0; i < 10; i++) {
		uint32 status = ReadOpReg(EHCI_USBSTS);
		TRACE(("usb_ehci: try %ld: status 0x%08x\n", i, status));

		if (status & EHCI_USBSTS_HCHALTED) {
			snooze(10000);
		} else {
			running = true;
			break;
		}
	}

	if (!running) {
		TRACE(("usb_ehci: Host Controller didn't start\n"));
		return B_ERROR;
	}

	fRootHubAddress = AllocateAddress();
	fRootHub = new(std::nothrow) EHCIRootHub(RootObject(), fRootHubAddress);
	if (!fRootHub) {
		TRACE_ERROR(("usb_ehci: no memory to allocate root hub\n"));
		return B_NO_MEMORY;
	}

	if (fRootHub->InitCheck() < B_OK) {
		TRACE_ERROR(("usb_ehci: root hub failed init check\n"));
		return fRootHub->InitCheck();
	}

	SetRootHub(fRootHub);
	TRACE(("usb_ehci: Host Controller started\n"));
	return BusManager::Start();
}


status_t
EHCI::SubmitTransfer(Transfer *transfer)
{
	// short circuit the root hub
	if (transfer->TransferPipe()->DeviceAddress() == fRootHubAddress)
		return fRootHub->ProcessTransfer(this, transfer);

 	return B_ERROR;
}


status_t
EHCI::AddTo(Stack *stack)
{
#ifdef TRACE_USB
	set_dprintf_enabled(true); 
	load_driver_symbols("ehci");
#endif

	if (!sPCIModule) {
		status_t status = get_module(B_PCI_MODULE_NAME, (module_info **)&sPCIModule);
		if (status < B_OK) {
			TRACE_ERROR(("usb_ehci: getting pci module failed! 0x%08lx\n", status));
			return status;
		}
	}

	TRACE(("usb_ehci: searching devices\n"));
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
				TRACE_ERROR(("usb_ehci: found device with invalid IRQ - check IRQ assignement\n"));
				continue;
			}

			TRACE(("usb_ehci: found device at IRQ %u\n", item->u.h0.interrupt_line));
			EHCI *bus = new(std::nothrow) EHCI(item, stack);
			if (!bus) {
				delete item;
				sPCIModule = NULL;
				put_module(B_PCI_MODULE_NAME);
				return B_NO_MEMORY;
			}

			if (bus->InitCheck() < B_OK) {
				TRACE_ERROR(("usb_ehci: bus failed init check\n"));
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
		TRACE_ERROR(("usb_ehci: no devices found\n"));
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
	TRACE(("usb_ehci: reset port %d\n", index));
	uint32 portRegister = EHCI_PORTSC + index * sizeof(uint32);
	uint32 portStatus = ReadOpReg(portRegister) & EHCI_PORTSC_DATAMASK;

	if (portStatus & EHCI_PORTSC_DMINUS) {
		TRACE(("usb_ehci: lowspeed device connected, giving up port ownership\n"));
		// there is a lowspeed device connected.
		// we give the ownership to a companion controller.
		WriteOpReg(portRegister, portStatus | EHCI_PORTSC_PORTOWNER);
		fPortResetChange |= (1 << index);
		return B_OK;
	}

	// enable reset signaling
	WriteOpReg(portRegister, portStatus | EHCI_PORTSC_PORTRESET);
	snooze(250000);

	// disable reset signaling
	portStatus = ReadOpReg(portRegister) & EHCI_PORTSC_DATAMASK;
	WriteOpReg(portRegister, portStatus & ~EHCI_PORTSC_PORTRESET);
	snooze(2000);

	portStatus = ReadOpReg(portRegister) & EHCI_PORTSC_DATAMASK;
	if (portStatus & EHCI_PORTSC_PORTRESET) {
		TRACE(("usb_ehci: port reset won't complete\n"));
		return B_ERROR;
	}

	if ((portStatus & EHCI_PORTSC_ENABLE) == 0) {
		TRACE(("usb_ehci: fullspeed device connected, giving up port ownership\n"));
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
	cpu_status status = disable_interrupts();
	spinlock lock = 0;
	acquire_spinlock(&lock);

	int32 result = ((EHCI *)data)->Interrupt();

	release_spinlock(&lock);
	restore_interrupts(status);
	return result;
}


int32
EHCI::Interrupt()
{
	return B_UNHANDLED_INTERRUPT;
}


status_t
EHCI::AddPendingTransfer(Transfer *transfer, ehci_qh *queueHead,
	ehci_qtd *firstDescriptor, ehci_qtd *dataDescriptor,
	ehci_qtd *lastDescriptor, bool directionIn)
{
	transfer_data *data = new(std::nothrow) transfer_data();
	if (!data)
		return B_NO_MEMORY;

	data->transfer = transfer;
	data->queue_head = queueHead;
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


status_t
EHCI::CancelPendingTransfer(Transfer *transfer)
{
	if (!Lock())
		return B_ERROR;

	transfer_data *last = NULL;
	transfer_data *current = fFirstTransfer;
	while (current) {
		if (current->transfer == transfer) {
			current->transfer->Finished(B_USB_STATUS_IRP_CANCELLED_BY_REQUEST, 0);
			delete current->transfer;

			if (last)
				last->link = current->link;
			else
				fFirstTransfer = current->link;

			if (fLastTransfer == current)
				fLastTransfer = last;

			delete current;
			Unlock();
			return B_OK;
		}

		last = current;
		current = current->link;
	}

	Unlock();
	return B_BAD_VALUE;
}


status_t
EHCI::CancelAllPendingTransfers()
{
	if (!Lock())
		return B_ERROR;

	transfer_data *transfer = fFirstTransfer;
	while (transfer) {
		transfer->transfer->Finished(B_USB_STATUS_IRP_CANCELLED_BY_REQUEST, 0);
		delete transfer->transfer;

		transfer_data *next = transfer->link;
		delete transfer;
		transfer = next;
	}

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
	while (!fStopFinishThread) {
		while (!fFinishTransfers) {
			if (fStopFinishThread)
				return;
			snooze(1000);
		}

		fFinishTransfers = false;
		if (!Lock())
			continue;

		TRACE(("usb_ehci: finishing transfers\n"));
		transfer_data *lastTransfer = NULL;
		transfer_data *transfer = fFirstTransfer;
		Unlock();

		while (transfer) {
			bool transferDone = false;

			// ToDo...

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


ehci_qtd *
EHCI::CreateDescriptor(Pipe *pipe, size_t bufferSize)
{
	return NULL;
}


status_t
EHCI::CreateDescriptorChain(Pipe *pipe, ehci_qtd **firstDescriptor,
	ehci_qtd **lastDescriptor, size_t bufferSize)
{
	*firstDescriptor = NULL;
	*lastDescriptor = NULL;
	return B_ERROR;
}


void
EHCI::FreeDescriptor(ehci_qtd *descriptor)
{
}


void
EHCI::FreeDescriptorChain(ehci_qtd *topDescriptor)
{
}


void
EHCI::LinkDescriptors(ehci_qtd *first, ehci_qtd *last)
{
}


size_t
EHCI::WriteDescriptorChain(ehci_qtd *topDescriptor, iovec *vector,
	size_t vectorCount)
{
	return 0;
}


size_t
EHCI::ReadDescriptorChain(ehci_qtd *topDescriptor, iovec *vector,
	size_t vectorCount, uint8 *lastDataToggle)
{
	return 0;
}


size_t
EHCI::ReadActualLength(ehci_qtd *topDescriptor, uint8 *lastDataToggle)
{
	return 0;
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
