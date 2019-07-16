/*
 * Copyright 2013, 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include <bus/PCI.h>
#include <PCI_x86.h>
#include <virtio.h>

#include "virtio_pci.h"


//#define TRACE_VIRTIO
#ifdef TRACE_VIRTIO
#	define TRACE(x...) dprintf("\33[33mvirtio_pci:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33mvirtio_pci:\33[0m " x)
#define ERROR(x...)			dprintf("\33[33mvirtio_pci:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define VIRTIO_PCI_DEVICE_MODULE_NAME "busses/virtio/virtio_pci/driver_v1"
#define VIRTIO_PCI_SIM_MODULE_NAME "busses/virtio/virtio_pci/device/v1"

#define VIRTIO_PCI_CONTROLLER_TYPE_NAME "virtio pci controller"

typedef enum {
	VIRTIO_IRQ_LEGACY,
	VIRTIO_IRQ_MSI,
	VIRTIO_IRQ_MSI_X_SHARED,
	VIRTIO_IRQ_MSI_X,
} virtio_irq_type;

typedef struct {
	virtio_sim sim;
	uint16 queue;
} virtio_pci_queue_cookie;

typedef struct {
	pci_device_module_info* pci;
	pci_device* device;
	addr_t base_addr;
	uint8 irq;
	virtio_irq_type irq_type;
	virtio_sim sim;
	uint16 queue_count;

	device_node* node;
	pci_info info;

	virtio_pci_queue_cookie *cookies;
} virtio_pci_sim_info;


device_manager_info* gDeviceManager;
virtio_for_controller_interface* gVirtio;
static pci_x86_module_info* sPCIx86Module;


int32
virtio_pci_interrupt(void *data)
{
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)data;
	uint8 isr = bus->pci->read_io_8(bus->device,
		bus->base_addr + VIRTIO_PCI_ISR);
	if (isr == 0)
		return B_UNHANDLED_INTERRUPT;

	if (isr & VIRTIO_PCI_ISR_CONFIG)
		gVirtio->config_interrupt_handler(bus->sim);

	if (isr & VIRTIO_PCI_ISR_INTR)
		gVirtio->queue_interrupt_handler(bus->sim, INT16_MAX);

	return B_HANDLED_INTERRUPT;
}


int32
virtio_pci_config_interrupt(void *data)
{
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)data;
	gVirtio->config_interrupt_handler(bus->sim);

	return B_HANDLED_INTERRUPT;
}


int32
virtio_pci_queue_interrupt(void *data)
{
	virtio_pci_queue_cookie* cookie = (virtio_pci_queue_cookie*)data;
	gVirtio->queue_interrupt_handler(cookie->sim, cookie->queue);

	return B_HANDLED_INTERRUPT;
}


status_t
virtio_pci_setup_msix_interrupts(virtio_pci_sim_info* bus)
{
	CALLED();
	uint8 irq = 0; // first irq slot
	bus->pci->write_io_16(bus->device, bus->base_addr
		+ VIRTIO_MSI_CONFIG_VECTOR, irq);
	if (bus->pci->read_io_16(bus->device, bus->base_addr
		+ VIRTIO_MSI_CONFIG_VECTOR) == VIRTIO_MSI_NO_VECTOR) {
		ERROR("msix config vector incorrect\n");
		return B_BAD_VALUE;
	}
	if (bus->irq_type == VIRTIO_IRQ_MSI_X)
		irq++;

	for (uint16 queue = 0; queue < bus->queue_count; queue++) {
		bus->pci->write_io_16(bus->device, bus->base_addr
			+ VIRTIO_PCI_QUEUE_SEL, queue);
		bus->pci->write_io_16(bus->device, bus->base_addr
			+ VIRTIO_MSI_QUEUE_VECTOR, irq);

		if (bus->pci->read_io_16(bus->device, bus->base_addr
			+ VIRTIO_MSI_QUEUE_VECTOR) == VIRTIO_MSI_NO_VECTOR) {
			ERROR("msix queue vector incorrect\n");
			return B_BAD_VALUE;
		}

		if (bus->irq_type == VIRTIO_IRQ_MSI_X)
			irq++;
	}

	return B_OK;
}


static void
set_sim(void* cookie, virtio_sim sim)
{
	CALLED();
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)cookie;
	bus->sim = sim;
}


static status_t
read_host_features(void* cookie, uint32 *features)
{
	CALLED();
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)cookie;

	TRACE("read_host_features() %p node %p pci %p device %p\n", bus,
		bus->node, bus->pci, bus->device);

	*features = bus->pci->read_io_32(bus->device,
		bus->base_addr + VIRTIO_PCI_HOST_FEATURES);
	return B_OK;
}


static status_t
write_guest_features(void* cookie, uint32 features)
{
	CALLED();
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)cookie;
	bus->pci->write_io_32(bus->device, bus->base_addr
		+ VIRTIO_PCI_GUEST_FEATURES, features);
	return B_OK;
}


uint8
get_status(void* cookie)
{
	CALLED();
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)cookie;
	return bus->pci->read_io_8(bus->device, bus->base_addr
		+ VIRTIO_PCI_STATUS);
}


void
set_status(void* cookie, uint8 status)
{
	CALLED();
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)cookie;
	bus->pci->write_io_8(bus->device, bus->base_addr + VIRTIO_PCI_STATUS,
		status);
}


status_t
read_device_config(void* cookie, uint8 _offset, void* _buffer,
	size_t bufferSize)
{
	CALLED();
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)cookie;

	addr_t offset = bus->base_addr + VIRTIO_PCI_CONFIG(bus) + _offset;
	uint8* buffer = (uint8*)_buffer;
	while (bufferSize > 0) {
		uint8 size = 4;
		if (bufferSize == 1) {
			size = 1;
			*buffer = bus->pci->read_io_8(bus->device,
			offset);
		} else if (bufferSize <= 3) {
			size = 2;
			*(uint16*)buffer = bus->pci->read_io_16(bus->device,
			offset);
		} else {
			*(uint32*)buffer = bus->pci->read_io_32(bus->device,
				offset);
		}
		buffer += size;
		bufferSize -= size;
		offset += size;
	}

	return B_OK;
}


status_t
write_device_config(void* cookie, uint8 _offset, const void* _buffer,
	size_t bufferSize)
{
	CALLED();
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)cookie;

	addr_t offset = bus->base_addr + VIRTIO_PCI_CONFIG(bus) + _offset;
	const uint8* buffer = (const uint8*)_buffer;
	while (bufferSize > 0) {
		uint8 size = 4;
		if (bufferSize == 1) {
			size = 1;
			bus->pci->write_io_8(bus->device,
				offset, *buffer);
		} else if (bufferSize <= 3) {
			size = 2;
			bus->pci->write_io_16(bus->device,
				offset, *(const uint16*)buffer);
		} else {
			bus->pci->write_io_32(bus->device,
				offset, *(const uint32*)buffer);
		}
		buffer += size;
		bufferSize -= size;
		offset += size;
	}
	return B_OK;
}


uint16
get_queue_ring_size(void* cookie, uint16 queue)
{
	CALLED();
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)cookie;
	bus->pci->write_io_16(bus->device, bus->base_addr + VIRTIO_PCI_QUEUE_SEL,
		queue);
	return bus->pci->read_io_16(bus->device, bus->base_addr
		+ VIRTIO_PCI_QUEUE_NUM);
}


status_t
setup_queue(void* cookie, uint16 queue, phys_addr_t phy)
{
	CALLED();
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)cookie;
	bus->pci->write_io_16(bus->device, bus->base_addr + VIRTIO_PCI_QUEUE_SEL,
		queue);
	bus->pci->write_io_32(bus->device, bus->base_addr + VIRTIO_PCI_QUEUE_PFN,
		(uint32)phy >> VIRTIO_PCI_QUEUE_ADDR_SHIFT);
	return B_OK;
}


status_t
setup_interrupt(void* cookie, uint16 queueCount)
{
	CALLED();
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)cookie;
	pci_info *pciInfo = &bus->info;

	bus->queue_count = queueCount;

	if (sPCIx86Module != NULL) {
		// try MSI-X
		uint8 msixCount = sPCIx86Module->get_msix_count(
			pciInfo->bus, pciInfo->device, pciInfo->function);
		if (msixCount >= 1) {
			if (msixCount >= (queueCount + 1)) {
				uint8 vector;
				bus->cookies = new(std::nothrow)
					virtio_pci_queue_cookie[queueCount];
				if (bus->cookies != NULL
					&& sPCIx86Module->configure_msix(pciInfo->bus,
						pciInfo->device, pciInfo->function, queueCount + 1,
						&vector) == B_OK
					&& sPCIx86Module->enable_msix(pciInfo->bus, pciInfo->device,
						pciInfo->function) == B_OK) {
					TRACE_ALWAYS("using MSI-X count %u starting at %d\n",
						queueCount + 1, vector);
					bus->irq = vector;
					bus->irq_type = VIRTIO_IRQ_MSI_X;
				} else {
					ERROR("couldn't use MSI-X\n");
				}
			} else {
				uint8 vector;
				if (sPCIx86Module->configure_msix(pciInfo->bus, pciInfo->device,
						pciInfo->function, queueCount + 1, &vector) == B_OK
					&& sPCIx86Module->enable_msix(pciInfo->bus, pciInfo->device,
						pciInfo->function) == B_OK) {
					TRACE_ALWAYS("using MSI-X vector shared %u\n", 1);
					bus->irq = vector;
					bus->irq_type = VIRTIO_IRQ_MSI_X_SHARED;
				} else {
					ERROR("couldn't use MSI-X SHARED\n");
				}
			}
		} else if (sPCIx86Module->get_msi_count(
			pciInfo->bus, pciInfo->device, pciInfo->function) >= 1) {
			// try MSI
			uint8 vector;
			if (sPCIx86Module->configure_msi(pciInfo->bus, pciInfo->device,
					pciInfo->function, 1, &vector) == B_OK
				&& sPCIx86Module->enable_msi(pciInfo->bus, pciInfo->device,
					pciInfo->function) == B_OK) {
				TRACE_ALWAYS("using MSI vector %u\n", vector);
				bus->irq = vector;
				bus->irq_type = VIRTIO_IRQ_MSI;
			} else {
				ERROR("couldn't use MSI\n");
			}
		}
	}
	if (bus->irq_type == VIRTIO_IRQ_LEGACY) {
		bus->irq = pciInfo->u.h0.interrupt_line;
		TRACE_ALWAYS("using legacy interrupt %u\n", bus->irq);
	}
	if (bus->irq == 0 || bus->irq == 0xff) {
		ERROR("PCI IRQ not assigned\n");
		if (sPCIx86Module != NULL) {
			put_module(B_PCI_X86_MODULE_NAME);
			sPCIx86Module = NULL;
		}
		delete bus;
		return B_ERROR;
	}

	if (bus->irq_type == VIRTIO_IRQ_MSI_X) {
		status_t status = install_io_interrupt_handler(bus->irq,
			virtio_pci_config_interrupt, bus, 0);
		if (status != B_OK) {
			ERROR("can't install interrupt handler\n");
			return status;
		}
		int32 irq = bus->irq + 1;
		for (int32 queue = 0; queue < queueCount; queue++, irq++) {
			bus->cookies[queue].sim = bus->sim;
			bus->cookies[queue].queue = queue;
			status_t status = install_io_interrupt_handler(irq,
				virtio_pci_queue_interrupt, &bus->cookies[queue], 0);
			if (status != B_OK) {
				ERROR("can't install interrupt handler\n");
				return status;
			}
		}
		TRACE("setup_interrupt() installed MSI-X interrupt handlers\n");
		virtio_pci_setup_msix_interrupts(bus);
	} else {
		// setup interrupt handler
		status_t status = install_io_interrupt_handler(bus->irq,
			virtio_pci_interrupt, bus, 0);
		if (status != B_OK) {
			ERROR("can't install interrupt handler\n");
			return status;
		}
		TRACE("setup_interrupt() installed legacy interrupt handler\n");
	}

	return B_OK;
}


status_t
free_interrupt(void* cookie)
{
	CALLED();
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)cookie;
	pci_info *pciInfo = &bus->info;

	if (bus->irq_type == VIRTIO_IRQ_MSI_X) {
		remove_io_interrupt_handler(bus->irq, virtio_pci_config_interrupt,
			bus);
		int32 irq = bus->irq + 1;
		for (int32 queue = 0; queue < bus->queue_count; queue++, irq++) {
			remove_io_interrupt_handler(irq, virtio_pci_queue_interrupt,
				&bus->cookies[queue]);
		}
		delete[] bus->cookies;

	} else
		remove_io_interrupt_handler(bus->irq, virtio_pci_interrupt, bus);

	if (sPCIx86Module != NULL && bus->irq_type != VIRTIO_IRQ_LEGACY) {
		sPCIx86Module->disable_msi(pciInfo->bus, pciInfo->device,
			pciInfo->function);
		sPCIx86Module->unconfigure_msi(pciInfo->bus, pciInfo->device,
			pciInfo->function);
	}
	return B_OK;
}


void
notify_queue(void* cookie, uint16 queue)
{
	CALLED();
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)cookie;
	bus->pci->write_io_16(bus->device, bus->base_addr
		+ VIRTIO_PCI_QUEUE_NOTIFY, queue);
}


//	#pragma mark -


static status_t
init_bus(device_node* node, void** bus_cookie)
{
	CALLED();
	status_t status = B_OK;

	virtio_pci_sim_info* bus = new(std::nothrow) virtio_pci_sim_info;
	if (bus == NULL) {
		return B_NO_MEMORY;
	}

	pci_device_module_info* pci;
	pci_device* device;
	{
		device_node* parent = gDeviceManager->get_parent_node(node);
		device_node* pciParent = gDeviceManager->get_parent_node(parent);
		gDeviceManager->get_driver(pciParent, (driver_module_info**)&pci,
			(void**)&device);
		gDeviceManager->put_node(pciParent);
		gDeviceManager->put_node(parent);
	}

	if (get_module(B_PCI_X86_MODULE_NAME, (module_info**)&sPCIx86Module)
			!= B_OK) {
		sPCIx86Module = NULL;
	}

	bus->node = node;
	bus->pci = pci;
	bus->device = device;
	bus->cookies = NULL;
	bus->irq_type = VIRTIO_IRQ_LEGACY;

	pci_info *pciInfo = &bus->info;
	pci->get_pci_info(device, pciInfo);

	// legacy interrupt
	bus->base_addr = pciInfo->u.h0.base_registers[0];

	// enable bus master and io
	uint16 pcicmd = pci->read_pci_config(device, PCI_command, 2);
	pcicmd &= ~(PCI_command_memory | PCI_command_int_disable);
	pcicmd |= PCI_command_master | PCI_command_io;
	pci->write_pci_config(device, PCI_command, 2, pcicmd);

	set_status(bus, VIRTIO_CONFIG_STATUS_RESET);
	set_status(bus, VIRTIO_CONFIG_STATUS_ACK);

	TRACE("init_bus() %p node %p pci %p device %p\n", bus, node,
		bus->pci, bus->device);

	*bus_cookie = bus;
	return status;
}


static void
uninit_bus(void* bus_cookie)
{
	virtio_pci_sim_info* bus = (virtio_pci_sim_info*)bus_cookie;
	if (bus->irq_type != VIRTIO_IRQ_LEGACY) {
		if (bus->irq_type == VIRTIO_IRQ_MSI) {
			remove_io_interrupt_handler(bus->irq, virtio_pci_interrupt, bus);

		} else {
			int32 irq = bus->irq + 1;
			for (uint16 queue = 0; queue < bus->queue_count; queue++, irq++) {
				remove_io_interrupt_handler(irq, virtio_pci_queue_interrupt,
					&bus->cookies[queue]);
			}
			remove_io_interrupt_handler(bus->irq, virtio_pci_config_interrupt,
					bus);
		}

		if (sPCIx86Module != NULL) {
			sPCIx86Module->disable_msi(bus->info.bus,
				bus->info.device, bus->info.function);
			sPCIx86Module->unconfigure_msi(bus->info.bus,
				bus->info.device, bus->info.function);
		}
	} else
		remove_io_interrupt_handler(bus->irq, virtio_pci_interrupt, bus);
	if (sPCIx86Module != NULL) {
		put_module(B_PCI_X86_MODULE_NAME);
		sPCIx86Module = NULL;
	}
	delete[] bus->cookies;
	delete bus;
}


static void
bus_removed(void* bus_cookie)
{
	return;
}


//	#pragma mark -


static status_t
register_child_devices(void* cookie)
{
	CALLED();
	device_node* node = (device_node*)cookie;
	device_node* parent = gDeviceManager->get_parent_node(node);
	pci_device_module_info* pci;
	pci_device* device;
	gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
		(void**)&device);

	uint16 pciSubDeviceId = pci->read_pci_config(device, PCI_subsystem_id,
		2);

	char prettyName[25];
	sprintf(prettyName, "Virtio Device %" B_PRIu16, pciSubDeviceId);

	device_attr attrs[] = {
		// properties of this controller for virtio bus manager
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: prettyName }},
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{ string: VIRTIO_FOR_CONTROLLER_MODULE_NAME }},

		// private data to identify the device
		{ VIRTIO_DEVICE_TYPE_ITEM, B_UINT16_TYPE,
			{ ui16: pciSubDeviceId }},
		{ VIRTIO_VRING_ALIGNMENT_ITEM, B_UINT16_TYPE,
			{ ui16: VIRTIO_PCI_VRING_ALIGN }},
		{ NULL }
	};

	return gDeviceManager->register_node(node, VIRTIO_PCI_SIM_MODULE_NAME,
		attrs, NULL, &node);
}


static status_t
init_device(device_node* node, void** device_cookie)
{
	CALLED();
	*device_cookie = node;
	return B_OK;
}


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "Virtio PCI"}},
		{}
	};

	return gDeviceManager->register_node(parent, VIRTIO_PCI_DEVICE_MODULE_NAME,
		attrs, NULL, NULL);
}


static float
supports_device(device_node* parent)
{
	CALLED();
	const char* bus;
	uint16 vendorID, deviceID;

	// make sure parent is a PCI Virtio device node
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK
		|| gDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID,
				&vendorID, false) < B_OK
		|| gDeviceManager->get_attr_uint16(parent, B_DEVICE_ID, &deviceID,
				false) < B_OK) {
		return -1;
	}

	if (strcmp(bus, "pci") != 0)
		return 0.0f;

	if (vendorID == VIRTIO_PCI_VENDORID) {
		if (deviceID < VIRTIO_PCI_DEVICEID_MIN
			|| deviceID > VIRTIO_PCI_DEVICEID_MAX) {
			return 0.0f;
		}

		pci_device_module_info* pci;
		pci_device* device;
		gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
			(void**)&device);
		uint8 pciSubDeviceId = pci->read_pci_config(device, PCI_revision,
			1);
		if (pciSubDeviceId != VIRTIO_PCI_ABI_VERSION)
			return 0.0f;

		TRACE("Virtio device found! vendor 0x%04x, device 0x%04x\n", vendorID,
			deviceID);
		return 0.8f;
	}

	return 0.0f;
}


//	#pragma mark -


module_dependency module_dependencies[] = {
	{ VIRTIO_FOR_CONTROLLER_MODULE_NAME, (module_info**)&gVirtio },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{}
};


static virtio_sim_interface gVirtioPCIDeviceModule = {
	{
		{
			VIRTIO_PCI_SIM_MODULE_NAME,
			0,
			NULL
		},

		NULL,	// supports device
		NULL,	// register device
		init_bus,
		uninit_bus,
		NULL,	// register child devices
		NULL,	// rescan
		bus_removed,
	},

	set_sim,
	read_host_features,
	write_guest_features,
	get_status,
	set_status,
	read_device_config,
	write_device_config,
	get_queue_ring_size,
	setup_queue,
	setup_interrupt,
	free_interrupt,
	notify_queue
};


static driver_module_info sVirtioDevice = {
	{
		VIRTIO_PCI_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	supports_device,
	register_device,
	init_device,
	NULL,	// uninit
	register_child_devices,
	NULL,	// rescan
	NULL,	// device removed
};

module_info* modules[] = {
	(module_info* )&sVirtioDevice,
	(module_info* )&gVirtioPCIDeviceModule,
	NULL
};
