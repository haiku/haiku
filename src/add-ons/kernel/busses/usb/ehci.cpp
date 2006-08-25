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
		fAsyncFrameListArea(-1),
		fAsyncFrameList(NULL),
		fFirstTransfer(NULL),
		fLastTransfer(NULL),
		fFinishTransfers(false),
		fFinishThread(-1),
		fStopFinishThread(false),
		fRootHub(NULL),
		fRootHubAddress(0)
{
	if (!fInitOK) {
		TRACE_ERROR(("usb_ehci: bus manager failed to init\n"));
		return;
	}

	TRACE(("usb_ehci: constructing new EHCI Host Controller Driver\n"));
	fInitOK = false;

	fRegisterBase = sPCIModule->read_pci_config(fPCIInfo->bus,
		fPCIInfo->device, fPCIInfo->function, PCI_memory_base, 4);
	fRegisterBase &= PCI_address_io_mask;
	TRACE(("usb_ehci: register base: 0x%08x\n", fRegisterBase));

	// enable pci address access
	uint16 command = PCI_command_io | PCI_command_master | PCI_command_memory;
	command |= sPCIModule->read_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2);

	sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2, command);

	// make sure we take the controller away from BIOS
	sPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device, 2,
		PCI_LEGSUP, 2, PCI_LEGSUP_USBPIRQDEN);

	// disable interrupts
	WriteReg16(EHCI_USBINTR, 0);

	// reset the host controller
	// ToDo...

	// allocate the periodic frame list
	void *physicalAddress;
	fPeriodicFrameListArea = fStack->AllocateArea((void **)&fPeriodicFrameList,
		&physicalAddress, B_PAGE_SIZE, "USB EHCI Periodic Framelist");
	if (fPeriodicFrameListArea < B_OK) {
		TRACE_ERROR(("usb_ehci: unable to allocate periodic framelist\n"));
		return;
	}

	WriteReg32(EHCI_PERIODICLISTBASE, (uint32)physicalAddress);

	// allocate the async frame list
	fAsyncFrameListArea = fStack->AllocateArea((void **)&fAsyncFrameList,
		&physicalAddress, B_PAGE_SIZE, "USB EHCI Async Framelist");
	if (fAsyncFrameListArea < B_OK) {
		TRACE_ERROR(("usb_ehci: unable to allocate async framelist\n"));
	}

	WriteReg32(EHCI_ASYNCLISTADDR, (uint32)physicalAddress);

	// create finisher service thread
	fFinishThread = spawn_kernel_thread(FinishThread, "ehci finish thread",
		B_NORMAL_PRIORITY, (void *)this);
	resume_thread(fFinishThread);

	// install the interrupt handler and enable interrupts
	install_io_interrupt_handler(fPCIInfo->u.h0.interrupt_line,
		InterruptHandler, (void *)this, 0);
	WriteReg16(EHCI_USBINTR, EHCI_USBINTR_HOSTSYSERR
		| EHCI_USBINTR_USBERRINT | EHCI_USBINTR_USBINT);

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
	delete_area(fAsyncFrameListArea);

	put_module(B_PCI_MODULE_NAME);
}


status_t
EHCI::Start()
{
	TRACE(("usb_ehci: starting EHCI Host Controller\n"));
	TRACE(("usb_ehci: usbcmd: 0x%08x; usbsts: 0x%08x\n", ReadReg32(EHCI_USBCMD), ReadReg32(EHCI_USBSTS)));

	WriteReg32(EHCI_USBCMD, ReadReg32(EHCI_USBCMD) | EHCI_USBCMD_RUNSTOP);

	bool running = false;
	for (int32 i = 0; i < 10; i++) {
		uint32 status = ReadReg32(EHCI_USBSTS);
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
	fRootHub = new(std::nothrow) EHCIRootHub(this, fRootHubAddress);
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
	return B_ERROR;
}


status_t
EHCI::SubmitRequest(Transfer *transfer)
{
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
		// class_base: 0C (Serial Bus); class_sub: 03 (USB); class_api: 20 (EHCI)
		if (item->class_base == 0x0C && item->class_sub == 0x03
			&& item->class_api == 0x20) {
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
EHCI::ResetPort(int32 index)
{
	return B_ERROR;
}


status_t
EHCI::ControllerReset()
{
	return B_ERROR;
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
EHCI::WriteReg8(uint32 reg, uint8 value)
{
	sPCIModule->write_io_8(fRegisterBase + reg, value);
}


inline void
EHCI::WriteReg16(uint32 reg, uint16 value)
{
	sPCIModule->write_io_16(fRegisterBase + reg, value);
}


inline void
EHCI::WriteReg32(uint32 reg, uint32 value)
{
	sPCIModule->write_io_32(fRegisterBase + reg, value);
}


inline uint8
EHCI::ReadReg8(uint32 reg)
{
	return sPCIModule->read_io_8(fRegisterBase + reg);
}


inline uint16
EHCI::ReadReg16(uint32 reg)
{
	return sPCIModule->read_io_16(fRegisterBase + reg);
}


inline uint32
EHCI::ReadReg32(uint32 reg)
{
	return sPCIModule->read_io_32(fRegisterBase + reg);
}
