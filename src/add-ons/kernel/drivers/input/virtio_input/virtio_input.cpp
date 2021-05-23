/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <virtio.h>
#include <virtio_defs.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <new>

#include <kernel.h>
#include <fs/devfs.h>
#include <int.h>

#include <virtio_input_driver.h>

#include <AutoDeleter.h>
#include <AutoDeleterOS.h>
#include <AutoDeleterDrivers.h>
#include <debug.h>


//#define TRACE_VIRTIO_INPUT
#ifdef TRACE_VIRTIO_INPUT
#	define TRACE(x...) dprintf("virtio_input: " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("virtio_input: " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

#define VIRTIO_INPUT_DRIVER_MODULE_NAME "drivers/input/virtio_input/driver_v1"
#define VIRTIO_INPUT_DEVICE_MODULE_NAME "drivers/input/virtio_input/device_v1"
#define VIRTIO_INPUT_DEVICE_ID_GENERATOR "virtio_input/device_id"


struct Packet {
	VirtioInputPacket data;
	int32 next;
};


struct VirtioInputDevice {
	device_node* node;
	::virtio_device virtio_device;
	virtio_device_interface* virtio;
	::virtio_queue virtio_queue;

	uint32 features;

	uint32 packetCnt;
	int32 freePackets;
	int32 readyPackets, lastReadyPacket;
	AreaDeleter packetArea;
	phys_addr_t physAdr;
	Packet* packets;

	SemDeleter sem_cb;
};


struct VirtioInputHandle {
	VirtioInputDevice*		info;
};


device_manager_info* gDeviceManager;


static void
WriteInputPacket(const VirtioInputPacket &pkt)
{
	switch (pkt.type) {
		case kVirtioInputEvSyn:
			TRACE("syn");
			break;
		case kVirtioInputEvKey:
			TRACE("key, ");
			switch (pkt.code) {
				case kVirtioInputBtnLeft:
					TRACE("left");
					break;
				case kVirtioInputBtnRight:
					TRACE("middle");
					break;
				case kVirtioInputBtnMiddle:
					TRACE("right");
					break;
				case kVirtioInputBtnGearDown:
					TRACE("gearDown");
					break;
				case kVirtioInputBtnGearUp:
					TRACE("gearUp");
					break;
				default:
					TRACE("%d", pkt.code);
			}
			break;
		case kVirtioInputEvRel:
			TRACE("rel, ");
			switch (pkt.code) {
				case kVirtioInputRelX:
					TRACE("relX");
					break;
				case kVirtioInputRelY:
					TRACE("relY");
					break;
				case kVirtioInputRelZ:
					TRACE("relZ");
					break;
				case kVirtioInputRelWheel:
					TRACE("relWheel");
					break;
				default:
					TRACE("%d", pkt.code);
			}
			break;
		case kVirtioInputEvAbs:
			TRACE("abs, ");
			switch (pkt.code) {
				case kVirtioInputAbsX:
					TRACE("absX");
					break;
				case kVirtioInputAbsY:
					TRACE("absY");
					break;
				case kVirtioInputAbsZ:
					TRACE("absZ");
					break;
				default:
					TRACE("%d", pkt.code);
			}
			break;
		case kVirtioInputEvRep:
			TRACE("rep");
			break;
		default:
			TRACE("?(%d)", pkt.type);
	}
	switch (pkt.type) {
		case kVirtioInputEvSyn:
			break;
		case kVirtioInputEvKey:
			TRACE(", ");
			if (pkt.value == 0) {
				TRACE("up");
			} else if (pkt.value == 1) {
				TRACE("down");
			} else {
				TRACE("%" B_PRId32, pkt.value);
			}
			break;
		default:
			TRACE(", %" B_PRId32, pkt.value);
	}
}


static void
InitPackets(VirtioInputDevice* dev, uint32 count)
{
	TRACE("InitPackets(%p, %" B_PRIu32 ")\n", dev, count);
	size_t size = ROUNDUP(sizeof(Packet)*count, B_PAGE_SIZE);

	dev->packetArea.SetTo(create_area("VirtIO input packets",
		(void**)&dev->packets, B_ANY_KERNEL_ADDRESS, size,
		B_CONTIGUOUS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA));
	if (!dev->packetArea.IsSet()) {
		ERROR("Unable to set packet area!");
		return;
	}

	physical_entry pe;
	if (get_memory_map(dev->packets, size, &pe, 1) < B_OK) {
		ERROR("Unable to get memory map for input packets!");
		return;
	}
	dev->physAdr = pe.address;
	memset(dev->packets, 0, size);
	dprintf("  size: 0x%" B_PRIxSIZE "\n", size);
	dprintf("  virt: %p\n", dev->packets);
	dprintf("  phys: %p\n", (void*)dev->physAdr);

	dev->packetCnt = count;

	dev->freePackets = 0;
	for (uint32 i = 0; i < dev->packetCnt - 1; i++)
		dev->packets[i].next = i + 1;
	dev->packets[dev->packetCnt - 1].next = -1;

	dev->readyPackets = -1;
	dev->lastReadyPacket = -1;
}


static const physical_entry
PacketPhysEntry(VirtioInputDevice* dev, Packet* pkt)
{
	physical_entry pe;
	pe.address = dev->physAdr + (uint8*)pkt - (uint8*)dev->packets;
	pe.size = sizeof(VirtioInputPacket);
	return pe;
}


static Packet*
AllocPacket(VirtioInputDevice* dev)
{
	int32 idx = dev->freePackets;
	if (idx < 0)
		return NULL;
	dev->freePackets = dev->packets[idx].next;

	return &dev->packets[idx];
}


static void
FreePacket(VirtioInputDevice* dev, Packet* pkt)
{
	pkt->next = dev->freePackets;
	dev->freePackets = pkt - dev->packets;
}


static void
ScheduleReadyPacket(VirtioInputDevice* dev, Packet* pkt)
{
	if (dev->readyPackets < 0)
		dev->readyPackets = pkt - dev->packets;
	else
		dev->packets[dev->lastReadyPacket].next = pkt - dev->packets;

	dev->lastReadyPacket = pkt - dev->packets;
}


static Packet*
ConsumeReadyPacket(VirtioInputDevice* dev)
{
	if (dev->readyPackets < 0)
		return NULL;
	Packet* pkt = &dev->packets[dev->readyPackets];
	dev->readyPackets = pkt->next;
	if (dev->readyPackets < 0)
		dev->lastReadyPacket = -1;
	return pkt;
}


static void
virtio_input_callback(void* driverCookie, void* cookie)
{
	CALLED();
	VirtioInputDevice* dev = (VirtioInputDevice*)cookie;

	Packet* pkt;
	while (dev->virtio->queue_dequeue(dev->virtio_queue, (void**)&pkt, NULL)) {
#ifdef TRACE_VIRTIO_INPUT
		TRACE("%" B_PRIdSSIZE ": ", pkt - dev->packets);
		WriteInputPacket(pkt->data);
		TRACE("\n");
#endif
		ScheduleReadyPacket(dev, pkt);
		release_sem_etc(dev->sem_cb.Get(), 1, B_DO_NOT_RESCHEDULE);
	}
}


//	#pragma mark - device module API


static status_t
virtio_input_init_device(void* _info, void** _cookie)
{
	CALLED();
	VirtioInputDevice* info = (VirtioInputDevice*)_info;

	DeviceNodePutter<&gDeviceManager> parent(
		gDeviceManager->get_parent_node(info->node));

	gDeviceManager->get_driver(parent.Get(),
		(driver_module_info **)&info->virtio,
		(void **)&info->virtio_device);

	info->virtio->negotiate_features(info->virtio_device, 0,
		&info->features, NULL);

	status_t status = B_OK;
/*
	status = info->virtio->read_device_config(
		info->virtio_device, 0, &info->config,
		sizeof(struct virtio_blk_config));
	if (status != B_OK)
		return status;
*/

	InitPackets(info, 8);

	status = info->virtio->alloc_queues(info->virtio_device, 1,
		&info->virtio_queue);
	if (status != B_OK) {
		ERROR("queue allocation failed (%s)\n", strerror(status));
		return status;
	}
	TRACE("  queue: %p\n", info->virtio_queue);

	status = info->virtio->queue_setup_interrupt(info->virtio_queue,
		virtio_input_callback, info);
	if (status < B_OK)
		return status;

	for (uint32 i = 0; i < info->packetCnt; i++) {
		Packet* pkt = &info->packets[i];
		physical_entry pe = PacketPhysEntry(info, pkt);
		info->virtio->queue_request(info->virtio_queue, NULL, &pe, pkt);
	}

	*_cookie = info;
	return B_OK;
}


static void
virtio_input_uninit_device(void* _cookie)
{
	CALLED();
	VirtioInputDevice* info = (VirtioInputDevice*)_cookie;
	(void)info;
}


static status_t
virtio_input_open(void* _info, const char* path, int openMode, void** _cookie)
{
	CALLED();
	VirtioInputDevice* info = (VirtioInputDevice*)_info;

	ObjectDeleter<VirtioInputHandle>
		handle(new(std::nothrow) (VirtioInputHandle));

	if (!handle.IsSet())
		return B_NO_MEMORY;

	handle->info = info;

	*_cookie = handle.Detach();
	return B_OK;
}


static status_t
virtio_input_close(void* cookie)
{
	CALLED();
	return B_OK;
}


static status_t
virtio_input_free(void* cookie)
{
	CALLED();
	ObjectDeleter<VirtioInputHandle> handle((VirtioInputHandle*)cookie);
	return B_OK;
}


static status_t
virtio_input_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	return B_ERROR;
}


static status_t
virtio_input_write(void* cookie, off_t pos, const void* buffer,
	size_t* _length)
{
	*_length = 0;
	return B_ERROR;
}


static status_t
virtio_input_ioctl(void* cookie, uint32 op, void* buffer, size_t length)
{
	CALLED();

	VirtioInputHandle* handle = (VirtioInputHandle*)cookie;
	VirtioInputDevice* info = handle->info;
	(void)info;

	TRACE("ioctl(op = %" B_PRIu32 ")\n", op);

	switch (op) {
		case virtioInputRead: {
			TRACE("virtioInputRead\n");
			if (buffer == NULL || length < sizeof(VirtioInputPacket))
				return B_BAD_VALUE;

			status_t res = acquire_sem(info->sem_cb.Get());
			if (res < B_OK)
				return res;

			Packet* pkt = ConsumeReadyPacket(info);
			TRACE("  pkt: %" B_PRIdSSIZE "\n", pkt - info->packets);

			physical_entry pe = PacketPhysEntry(info, pkt);
			info->virtio->queue_request(info->virtio_queue, NULL, &pe, pkt);

			res = user_memcpy(buffer, pkt, sizeof(Packet));
			if (res < B_OK)
				return res;

			return B_OK;
		}
	}

	return B_DEV_INVALID_IOCTL;
}


//	#pragma mark - driver module API


static float
virtio_input_supports_device(device_node *parent)
{
	CALLED();

	const char *bus;
	uint16 deviceType;

	// make sure parent is really the Virtio bus manager
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "virtio"))
		return 0.0;

	// check whether it's really a Direct Access Device
	if (gDeviceManager->get_attr_uint16(parent, VIRTIO_DEVICE_TYPE_ITEM,
			&deviceType, true) != B_OK || deviceType != kVirtioDevInput)
		return 0.0;

	TRACE("Virtio input device found!\n");

	return 0.6;
}


static status_t
virtio_input_register_device(device_node *node)
{
	CALLED();

	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "VirtIO input" }},
		{ NULL }
	};

	return gDeviceManager->register_node(node, VIRTIO_INPUT_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
virtio_input_init_driver(device_node *node, void **cookie)
{
	CALLED();

	ObjectDeleter<VirtioInputDevice>
		info(new(std::nothrow) VirtioInputDevice());

	if (!info.IsSet())
		return B_NO_MEMORY;

	memset(info.Get(), 0, sizeof(*info.Get()));

	info->sem_cb.SetTo(create_sem(0, "virtio_input_cb"));
	if (!info->sem_cb.IsSet())
		return info->sem_cb.Get();

	info->node = node;

	*cookie = info.Detach();
	return B_OK;
}


static void
virtio_input_uninit_driver(void *_cookie)
{
	CALLED();
	ObjectDeleter<VirtioInputDevice> info((VirtioInputDevice*)_cookie);
}


static status_t
virtio_input_register_child_devices(void* _cookie)
{
	CALLED();
	VirtioInputDevice* info = (VirtioInputDevice*)_cookie;
	status_t status;

	int32 id = gDeviceManager->create_id(VIRTIO_INPUT_DEVICE_ID_GENERATOR);
	if (id < 0)
		return id;

	char name[64];
	snprintf(name, sizeof(name), "input/virtio/%" B_PRId32 "/raw", id);

	status = gDeviceManager->publish_device(info->node, name,
		VIRTIO_INPUT_DEVICE_MODULE_NAME);

	if (status < B_OK) {
		ERROR("publish_device error: 0x%" B_PRIx32 "(%s) \n", status,
			strerror(status));
	}

	return status;
}


//	#pragma mark -


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{ NULL }
};


struct device_module_info sVirtioInputDevice = {
	{
		VIRTIO_INPUT_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	virtio_input_init_device,
	virtio_input_uninit_device,
	NULL, // remove,

	virtio_input_open,
	virtio_input_close,
	virtio_input_free,
	virtio_input_read,
	virtio_input_write,
	NULL,
	virtio_input_ioctl,

	NULL,	// select
	NULL,	// deselect
};

struct driver_module_info sVirtioInputDriver = {
	{
		VIRTIO_INPUT_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	virtio_input_supports_device,
	virtio_input_register_device,
	virtio_input_init_driver,
	virtio_input_uninit_driver,
	virtio_input_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};

module_info* modules[] = {
	(module_info*)&sVirtioInputDriver,
	(module_info*)&sVirtioInputDevice,
	NULL
};
