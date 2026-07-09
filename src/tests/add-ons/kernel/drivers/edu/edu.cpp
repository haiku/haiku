/*
 * Copyright 2004-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hamza Dridi, <dridiha@gmail.com>
 */

/*
 * EDU Driver
 *
 * This driver implements support for the QEMU EDU educational PCI device.
 *
 * The EDU device exposes a memory-mapped register interface through PCI BAR0
 * and provides two hardware-assisted operations:
 *
 *   - DMA transfers between a fixed internal EDU memory region and system RAM.
 *   - Hardware computation of factorial values.
 *
 * DMA operations are asynchronous and notify completion through PCI
 * interrupts. Likewise, factorial computations generate an interrupt when the
 * result is available.
 *
 * Driver characteristics
 * ----------------------
 * - Uses MMIO for all device communication.
 * - Uses the Haiku DMAResource/IOScheduler framework for user I/O.
 * - Uses interrupts instead of polling to detect operation completion.
 * - Supports both DMA reads (EDU -> RAM) and DMA writes (RAM -> EDU).
 *
 * EDU hardware limitations
 * ------------------------
 * - DMA operates only on the EDU internal memory window.
 * - Internal memory window of EDU is limited to 4KB.
 * - No scatter/gather support is exposed by the hardware.
 * - EDU device can handle 28 bit addresses.
 * - Completion notification is interrupt-driven.
 * - Factorial computation is limited by the hardware register width
 *   (32-bit result).
 *
 */

#include "edu.h"

#include <ByteOrder.h>
#include <bus/PCI.h>
#include <drivers/KernelExport.h>
#include <drivers/device_manager.h>
#include <kernel.h>
#include <new>
#include <stdio.h>

#include "IORequest.h"
#include "IOScheduler.h"
#include "IOSchedulerSimple.h"
#include "OS.h"
#include "PCI.h"
#include "SupportDefs.h"
#include "util/iovec_support.h"

#define TRACE_EDU
#ifdef TRACE_EDU
#define TRACE(x...) dprintf("edu: " x)
#else
#define TRACE(x...) ;
#endif

#define TRACE_ERROR(x...) dprintf("edu: error: " x)

#define CALLED() TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

#define EDU_DRIVER_MODULE_NAME "drivers/edu/driver_v1"
#define EDU_DEVICE_MODULE_NAME "drivers/edu/device_v1"
#define EDU_DEVICE_ID_GENERATOR "edu/device_id"

static device_manager_info* sDeviceManager;

//	#pragma mark - driver module API

static float
edu_supports_device(device_node* parent)
{

	CALLED();

	const char* bus;
	uint16 vendor_id, deviceId;

	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID, &vendor_id, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_ID, &deviceId, false) != B_OK) {
		return -1.0f;
	}

	if (strcmp(bus, "pci") != 0 || vendor_id != PCI_EDU_VENDOR_ID || deviceId != PCI_EDU_DEVICE_ID)
		return -1.0;

	TRACE("EDU device found\n");
	return 1.0;
}


static status_t
edu_init_driver(device_node* node, void** cookie)
{
	CALLED();
	device_node* parent = sDeviceManager->get_parent_node(node);

	pci_device* pci_dev; // driverData
	pci_device_module_info* pci; // driverModule
	status_t status
		= sDeviceManager->get_driver(parent, (driver_module_info**)&pci, (void**)&pci_dev);

	if (status != B_OK)
		return status;

	edu_driver_data* driver_data = new(std::nothrow) edu_driver_data;

	if (driver_data == NULL)
		return B_NO_MEMORY;

	driver_data->pci_device_info = new(std::nothrow) pci_info;
	if (driver_data->pci_device_info == NULL) {
		delete driver_data;
		return B_NO_MEMORY;
	}

	driver_data->pci = pci;
	driver_data->pci_dev = pci_dev;
	driver_data->node = node;

	pci->get_pci_info(pci_dev, driver_data->pci_device_info);
	*cookie = driver_data;
	return status;
}


static void
edu_uninit_driver(void* _cookie)
{
	CALLED();
	edu_driver_data* driver_data = (edu_driver_data*)_cookie;
	delete driver_data->pci_device_info;
	delete driver_data;
	return;
}


static status_t
edu_register_device(device_node* node)
{
	CALLED();

	device_attr attrs[] = {{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "Edu"}}, {NULL}};

	return sDeviceManager->register_node(node, EDU_DRIVER_MODULE_NAME, attrs, NULL, NULL);
}


static status_t
edu_register_child_devices(void* _cookie)
{

	CALLED();
	edu_driver_data* driver_data = (edu_driver_data*)_cookie;

	int32 id = sDeviceManager->create_id(EDU_DEVICE_ID_GENERATOR);
	if (id < 0)
		return id;

	char name[64];
	snprintf(name, sizeof(name), "edu/%" B_PRId32 "/raw", id);

	return sDeviceManager->publish_device(driver_data->node, name, EDU_DEVICE_MODULE_NAME);
}

// device module API

static status_t
edu_init_device(void* _info, void** _cookie)
{
	CALLED();
	edu_driver_data* driver_data = (edu_driver_data*)_info;
	pci_info* pci_info = driver_data->pci_device_info;
	pci_device_module_info* pci = driver_data->pci;
	pci_device* pci_dev = driver_data->pci_dev;

	EduDeviceData* device_data = new(std::nothrow) EduDeviceData(pci, pci_info, pci_dev);

	if (device_data == NULL)
		return B_NO_MEMORY;

	status_t status = device_data->Init();
	if (status < B_OK) {
		delete device_data;
		return status;
	}

	*_cookie = device_data;

	return status;
}


static void
edu_uninit_device(void* _cookie)
{
	CALLED();
	EduDeviceData* device_data = (EduDeviceData*)_cookie;
	delete device_data;
}


static status_t
edu_open(void* _info, const char* path, int openMode, void** _cookie)
{
	CALLED();
	*_cookie = _info;

	return B_OK;
}


static status_t
edu_close(void* cookie)
{
	CALLED();
	return B_OK;
}


static status_t
edu_free(void* cookie)
{
	CALLED();
	return B_OK;
}


static status_t
edu_read(void* cookie, off_t position, void* buffer, size_t* _length)
{
	CALLED();
	size_t length = *_length;

	if (*_length + position > EDU_DMA_BUFFER_SIZE) {
		TRACE_ERROR("edu_read(): possible buffer overflow\n");
		return B_ERROR;
	}

	EduDeviceData* device_data = (EduDeviceData*)cookie;

	IOScheduler* scheduler = device_data->Scheduler();

	IORequest request;
	status_t status = request.Init(position, (addr_t)buffer, length, false, 0);

	if (status != B_OK) {
		TRACE_ERROR("edu_read(): Error initializing the request: %s\n", strerror(status));
		return status;
	}

	status = scheduler->ScheduleRequest(&request);

	if (status < B_OK) {
		TRACE_ERROR("edu_read(): Error scheduling a request %s\n", strerror(status));
		return status;
	}

	status = request.Wait(0, 0);

	if (status == B_OK)
		*_length = length;
	return status;
}


static status_t
edu_write(void* cookie, off_t position, const void* buffer, size_t* _length)
{

	CALLED();

	size_t length = *_length;

	if (*_length + position > EDU_DMA_BUFFER_SIZE) {
		TRACE_ERROR("edu_write(): possible buffer overflow\n");
		return B_ERROR;
	}

	EduDeviceData* device_data = (EduDeviceData*)cookie;
	IOScheduler* scheduler = device_data->Scheduler();

	IORequest request;
	status_t status = request.Init(position, (addr_t)buffer, length, true, 0);

	if (status != B_OK) {
		TRACE_ERROR("edu_write(): Error initializing the request: %s\n", strerror(status));
		return status;
	}

	status = scheduler->ScheduleRequest(&request);

	if (status < B_OK) {
		TRACE_ERROR("edu_write(): Error scheduling a request %s\n", strerror(status));
		return status;
	}

	status = request.Wait(0, 0);

	if (status == B_OK)
		*_length = length;

	return status;
}


static status_t
edu_ioctl(void* cookie, uint32 op, void* buffer, size_t length)
{
	EduDeviceData* device_data = (EduDeviceData*)cookie;
	switch (op) {
		case EDU_IOCTL_FACTORIAL:
			uint32 value;
			if (user_memcpy(&value, buffer, sizeof(uint32)) < B_OK)
				return B_BAD_ADDRESS;

			uint32 factorial = device_data->EduComputeFactorial(value);
			if (user_memcpy(buffer, &factorial, sizeof(uint32)) < B_OK)
				return B_BAD_ADDRESS;

			return B_OK;
	}

	return B_DEV_INVALID_IOCTL;
}


static int32
edu_handle_interrupt(void* data)
{

	CALLED();
	TRACE("EDU handle interrupt has been triggered\n");
	EduDeviceData* device_data = (EduDeviceData*)data;
	uint32 irqStatus = device_data->EduReadRegister(EDU_INTERRUPT_STATUS_REG);

	if (irqStatus == 0)
		return B_UNHANDLED_INTERRUPT;

	uint32 status = device_data->EduReadRegister(EDU_STATUS_REG);

	if (status & EDU_FACTORIAL_INTERRUPT) {
		status &= ~EDU_FACTORIAL_INTERRUPT;
		device_data->EduWriteRegister(EDU_STATUS_REG, status);
	}

	device_data->EduWriteRegister(EDU_INTERRUPT_ACK_REG, irqStatus);
	release_sem_etc(device_data->Notify(), 1, B_DO_NOT_RESCHEDULE);

	return B_HANDLED_INTERRUPT;
}


static status_t
edu_do_io(void* data, IOOperation* operation)
{

	CALLED();
	EduDeviceData* device_data = (EduDeviceData*)data;
	ASSERT(operation->VecCount() == 1);

	phys_addr_t physaddr = operation->Vecs()[0].base;

	if (operation->IsWrite())
		device_data->EduDoDma(physaddr, operation->Length(), operation->Offset(), true);
	else if (operation->IsRead())
		device_data->EduDoDma(physaddr, operation->Length(), 0, false);

	device_data->Scheduler()->OperationCompleted(operation, B_OK, operation->Length());

	return B_OK;
}

// EduDeviceData implementation

EduDeviceData::EduDeviceData(pci_device_module_info* pci, struct pci_info* pci_info,
	pci_device* pci_dev)
	:
	fRegisterArea(-1),
	fRegisters(NULL),
	fNotify(-1),
	fDMAResource(NULL),
	fScheduler(NULL),
	fIRQ(0),
	fPci(pci),
	fPciInfo(pci_info),
	fPciDev(pci_dev)
{
}


status_t
EduDeviceData::Init()
{

	uint16 command = fPci->read_pci_config(fPciDev, PCI_command, 2);
	command |= PCI_command_memory | PCI_command_master;
	command &= ~EDU_DISABLE_INTERRUPT;

	fPci->write_pci_config(fPciDev, PCI_command, 2, command);

	// 64 bit addresses are not supported.
	// Edu device exposes 32 bit address through BAR0
	phys_addr_t physicalAddress = fPciInfo->u.h0.base_registers[0];
	size_t size = fPciInfo->u.h0.base_register_sizes[0];

	TRACE("map registers %08" B_PRIxPHYSADDR ", size: %" B_PRIuSIZE "\n", physicalAddress, size);

	area_id area = map_physical_memory("EDU memory mapped registers", physicalAddress, size,
		B_ANY_KERNEL_BLOCK_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fRegisters);

	if (area < B_OK) {
		TRACE_ERROR("edu_init_device(): Error mapping memory registers\n");
		return area;
	}

	fRegisterArea = area;
	fDMAResource = new(std::nothrow) DMAResource;

	if (fDMAResource == NULL) {
		TRACE_ERROR("edu_init_device(): Error creating DMAResouce\n");
		return B_NO_MEMORY;
	}

	dma_restrictions restrictions = {0};

	restrictions.low_address = EDU_LOW_ADDRESS;
	restrictions.high_address = EDU_HIGH_ADDRESS;

	status_t status = fDMAResource->Init(restrictions, B_PAGE_SIZE, 1, 1);
	if (status < B_OK) {
		TRACE_ERROR("edu_init_device(): Error initializing dma_resource\n");
		return status;
	}

	fScheduler = new(std::nothrow) IOSchedulerSimple(fDMAResource);

	if (fScheduler == NULL) {
		TRACE_ERROR("edu_init_device(): Error creating IOScheduler due to memory\n");
		return B_NO_MEMORY;
	}

	status = fScheduler->Init("EDU DMA Scheduler");

	if (status < B_OK) {
		TRACE_ERROR("edu_init_device(): Error initializing IOScheduler\n");
		return status;
	}

	fScheduler->SetCallback(edu_do_io, this);
	fNotify = create_sem(0, "EDU Interrupt Callback");

	if (fNotify < B_OK) {
		TRACE_ERROR("edu_init_device(): Error creating semaphore: %s\n", strerror(fNotify));
		return fNotify;
	}

	uchar irq = fPciInfo->u.h0.interrupt_line;
	status = install_io_interrupt_handler(irq, edu_handle_interrupt, (void*)this, 0);

	if (status != B_OK) {
		TRACE_ERROR("edu_init_device(): Error installing EDU interrupt handler: %s\n",
			strerror(status));
		return status;
	}

	fIRQ = irq;

	return status;
}


EduDeviceData::~EduDeviceData()
{
	if (fIRQ > 0)
		remove_io_interrupt_handler(fIRQ, edu_handle_interrupt, (void*)this);

	if (fNotify >= B_OK)
		delete_sem(fNotify);

	delete fScheduler;
	delete fDMAResource;

	if (fRegisterArea >= B_OK)
		delete_area(fRegisterArea);
}

/*
 * Writes a 32-bit value to an EDU MMIO register.
 *
 * All EDU registers are 32-bit aligned and are accessed through the
 * memory-mapped BAR0 register space.
 */

void
EduDeviceData::EduWriteRegister(uint8 offset, uint32 value)
{
	value = B_HOST_TO_LENDIAN_INT32(value);
	*(volatile uint32*)(fRegisters + offset) = value;
}

/*
 * Reads a 32-bit value from an EDU MMIO register.
 */
uint32
EduDeviceData::EduReadRegister(uint8 offset)
{
	uint32 value = *(volatile uint32*)(fRegisters + offset);
	return B_HOST_TO_LENDIAN_INT32(value);
}

/*
 * Starts a hardware factorial computation.
 *
 * The requested value is written to the factorial register after enabling
 * factorial-completion interrupts. The function blocks until the interrupt
 * handler signals completion.
 *
 */
uint32
EduDeviceData::EduComputeFactorial(uint32 value)
{
	CALLED();
	uint32 status = EduReadRegister(EDU_STATUS_REG);
	status |= EDU_FACTORIAL_INTERRUPT;

	EduWriteRegister(EDU_STATUS_REG, status);
	EduWriteRegister(EDU_FACTORIAL_REG, value);

	acquire_sem(fNotify);

	uint32 factorial = EduReadRegister(EDU_FACTORIAL_REG);
	TRACE("factorial of %d is %d\n", value, factorial);

	return factorial;
}


void
EduDeviceData::EduDoDma(phys_addr_t physaddr, uint32 length, off_t offset, bool isWrite)
{
	CALLED();

	uint32 command = EDU_DMA_START | EDU_DMA_INTERRUPT;
	if (isWrite) {
		EduWriteRegister(EDU_DMA_SRC_REG, physaddr);
		EduWriteRegister(EDU_DMA_DST_REG, EDU_DMA_ADDR + offset);
		command |= EDU_DMA_RAM_TO_EDU;

	} else {
		EduWriteRegister(EDU_DMA_SRC_REG, EDU_DMA_ADDR);
		EduWriteRegister(EDU_DMA_DST_REG, physaddr);
		command |= EDU_DMA_EDU_TO_RAM;
	}

	EduWriteRegister(EDU_DMA_COUNT_REG, length);
	EduWriteRegister(EDU_DMA_CMD_REG, command);

	acquire_sem(fNotify);
}


module_dependency module_dependencies[]
	= {{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager}, {NULL}};

struct device_module_info sEduDevice = {
	{EDU_DEVICE_MODULE_NAME, 0, NULL},

	edu_init_device,
	edu_uninit_device,
	NULL, // remove,

	edu_open,
	edu_close,
	edu_free,
	edu_read,
	edu_write,
	NULL,
	edu_ioctl,
	NULL,
	NULL,
};

struct driver_module_info sEduDriver = {
	{EDU_DRIVER_MODULE_NAME, 0, NULL},
	edu_supports_device,
	edu_register_device,
	edu_init_driver,
	edu_uninit_driver,
	edu_register_child_devices,
	NULL, //
	NULL,
};

module_info* modules[] = {(module_info*)&sEduDriver, (module_info*)&sEduDevice, NULL};
