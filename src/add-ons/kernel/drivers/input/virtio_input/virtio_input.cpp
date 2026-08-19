/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2021-2026, Haiku, Inc. All rights reserved.
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

#include <condition_variable.h>
#include <util/AutoLock.h>
#include <virtio_input_driver.h>

#include <AutoDeleter.h>
#include <AutoDeleterDrivers.h>
#include <AutoDeleterOS.h>
#include <debug.h>
#include <util/AutoLock.h>

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
};


class PacketQueue {
private:
	spinlock fLock = B_SPINLOCK_INITIALIZER;

	uint32 fPacketCnt {};

	ArrayDeleter<Packet*> fReadyPackets;
	uint32 fReadyPacketRptr {};
	uint32 fReadyPacketWptr {};

	AreaDeleter fPacketArea;
	phys_addr_t fPhysAdr {};
	Packet* fPackets {};

	ConditionVariable fCanReadCond;

public:
	// `count` must be power of 2
	status_t Init(uint32 count);

	uint32 PacketCount() const { return fPacketCnt; }
	Packet* PacketAt(uint32 index) { return &fPackets[index]; }
	const physical_entry PacketPhysEntry(Packet* pkt) const;

	void Write(Packet* pkt);
	status_t Read(Packet*& pkt);
};


struct VirtioInputDevice {
	device_node* node {};

	mutex virtioConfigLock = MUTEX_INITIALIZER("virtioConfig");
	mutex virtioQueueLock = MUTEX_INITIALIZER("virtioQueue");

	virtio_device virtioDevice {};
	virtio_device_interface* virtio {};
	virtio_queue virtioQueue {};

	uint64 features {};
	VirtioInputType type;

	PacketQueue packetQueue;
};


struct VirtioInputHandle {
	VirtioInputDevice* info;
};


device_manager_info* gDeviceManager;

#ifdef TRACE_VIRTIO_INPUT
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
#endif


static status_t
QueryConfig(VirtioInputDevice* dev, uint8 select, uint8 subsel, VirtioInputConfig* config)
{
	MutexLocker(dev->virtioConfigLock);

	status_t status = dev->virtio->write_device_config(dev->virtioDevice,
		offsetof(VirtioInputConfig, select), &select, sizeof(select));
	if (status != B_OK)
		return status;

	status = dev->virtio->write_device_config(dev->virtioDevice,
		offsetof(VirtioInputConfig, subsel), &subsel, sizeof(subsel));
	if (status != B_OK)
		return status;

	return dev->virtio->read_device_config(dev->virtioDevice, 0, config, sizeof(*config));
}


static inline bool
IsBitSet(const uint8* bitmap, uint8 size, uint16 bit)
{
	return (bit / 8) < size && (bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}


static VirtioInputType
IdentifyDevice(VirtioInputDevice* dev)
{
	VirtioInputConfig config = {};

	// If absolute x and y are available, this is a tablet.
	if (QueryConfig(dev, kVirtioInputCfgEvBits, kVirtioInputEvAbs, &config) == B_OK
		&& IsBitSet(config.bitmap, config.size, kVirtioInputAbsX)
		&& IsBitSet(config.bitmap, config.size, kVirtioInputAbsY)) {
		return kVirtioInputTablet;
	}

	// If keys below BTN_MISC are available, this is a keyboard.
	if (QueryConfig(dev, kVirtioInputCfgEvBits, kVirtioInputEvKey, &config) == B_OK) {
		size_t bytes = min_c(config.size, kVirtioInputBtnMisc / 8);
		for (size_t i = 0; i < bytes; i++)
			if (config.bitmap[i] != 0)
				return kVirtioInputKeyboard;
	}

	return kVirtioInputUnknown;
}


static void
virtio_input_callback(void* driverCookie, void* cookie)
{
	CALLED();
	VirtioInputDevice* dev = (VirtioInputDevice*)cookie;

	Packet* pkt;
	while (dev->virtio->queue_dequeue(dev->virtioQueue, (void**)&pkt, NULL))
		dev->packetQueue.Write(pkt);
}


// #pragma mark -- PacketQueue


status_t
PacketQueue::Init(uint32 count)
{
	fReadyPackets.SetTo(new(std::nothrow) Packet*[count]);
	if (!fReadyPackets.IsSet())
		return B_NO_MEMORY;

	size_t size = ROUNDUP(sizeof(Packet) * count, B_PAGE_SIZE);

	fPacketArea.SetTo(create_area("VirtIO input packets", (void**)&fPackets, B_ANY_KERNEL_ADDRESS,
		size, B_CONTIGUOUS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA));
	if (!fPacketArea.IsSet()) {
		ERROR("Unable to set packet area!");
		return fPacketArea.Get();
	}

	physical_entry pe;
	status_t res = get_memory_map(fPackets, size, &pe, 1);
	if (res < B_OK) {
		ERROR("Unable to get memory map for input packets!");
		return res;
	}
	fPhysAdr = pe.address;
	memset(fPackets, 0, size);
	TRACE("  size: 0x%" B_PRIxSIZE "\n", size);
	TRACE("  virt: %p\n", packets);
	TRACE("  phys: %p\n", (void*)physAdr);

	fPacketCnt = count;

	fCanReadCond.Init(this, "hasReadyPacket");

	return B_OK;
}


const physical_entry
PacketQueue::PacketPhysEntry(Packet* pkt) const
{
	physical_entry pe{.address = fPhysAdr + ((uint8*)pkt - (uint8*)fPackets),
		.size = sizeof(VirtioInputPacket)};
	return pe;
}


void
PacketQueue::Write(Packet* pkt)
{
	InterruptsSpinLocker lock(&fLock);

#ifdef TRACE_VIRTIO_INPUT
	TRACE_ALWAYS("%" B_PRIdSSIZE ": ", pkt - fPackets);
	WriteInputPacket(pkt->data);
	TRACE("\n");
#endif

	fReadyPackets[fReadyPacketWptr & (fPacketCnt - 1)] = pkt;
	fReadyPacketWptr++;

	fCanReadCond.NotifyOne();
}


status_t
PacketQueue::Read(Packet*& pkt)
{
	InterruptsSpinLocker lock(&fLock);

	while (fReadyPacketRptr == fReadyPacketWptr) {
		ConditionVariableEntry entry;
		fCanReadCond.Add(&entry);

		release_spinlock(&fLock);
		enable_interrupts();
		status_t res = entry.Wait(B_CAN_INTERRUPT);
		disable_interrupts();
		acquire_spinlock(&fLock);

		if (res < B_OK)
			return res;
	}

	pkt = fReadyPackets[fReadyPacketRptr & (fPacketCnt - 1)];
	fReadyPacketRptr++;

	return B_OK;
}


//	#pragma mark - device module API


static status_t
virtio_input_init_device(void* _info, void** _cookie)
{
	CALLED();
	VirtioInputDevice* info = (VirtioInputDevice*)_info;

	DeviceNodePutter<&gDeviceManager> parent(
		gDeviceManager->get_parent_node(info->node));

	gDeviceManager->get_driver(parent.Get(), (driver_module_info**)&info->virtio,
		(void**)&info->virtioDevice);

	info->virtio->negotiate_features(info->virtioDevice, 0, &info->features, NULL);

	info->type = IdentifyDevice(info);

	status_t status = B_OK;

	info->packetQueue.Init(8);

	status = info->virtio->alloc_queues(info->virtioDevice, 1, &info->virtioQueue, NULL);
	if (status != B_OK) {
		ERROR("queue allocation failed (%s)\n", strerror(status));
		return status;
	}
	TRACE("  queue: %p\n", info->virtio_queue);

	status = info->virtio->setup_interrupt(info->virtioDevice, NULL, info);
	if (status < B_OK) {
		ERROR("interrupt setup failed (%s)\n", strerror(status));
		return status;
	}

	status = info->virtio->queue_setup_interrupt(info->virtioQueue, virtio_input_callback, info);
	if (status < B_OK)
		return status;

	for (uint32 i = 0; i < info->packetQueue.PacketCount(); i++) {
		Packet* pkt = info->packetQueue.PacketAt(i);
		physical_entry pe = info->packetQueue.PacketPhysEntry(pkt);
		info->virtio->queue_request(info->virtioQueue, NULL, &pe, pkt);
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
virtio_input_control(void* cookie, uint32 op, void* buffer, size_t length)
{
	CALLED();

	VirtioInputHandle* handle = (VirtioInputHandle*)cookie;
	VirtioInputDevice* info = handle->info;
	(void)info;

	TRACE("control(op = %" B_PRIu32 ")\n", op);

	switch (op) {
		case virtioInputRead: {
			TRACE("virtioInputRead\n");
			if (buffer == NULL || length < sizeof(VirtioInputPacket))
				return B_BAD_VALUE;

			Packet* pkt;
			status_t res = info->packetQueue.Read(pkt);
			if (res < B_OK)
				return res;

			res = user_memcpy(buffer, pkt, sizeof(VirtioInputPacket));

			physical_entry pe = info->packetQueue.PacketPhysEntry(pkt);
			mutex_lock(&info->virtioQueueLock);
			info->virtio->queue_request(info->virtioQueue, NULL, &pe, pkt);
			mutex_unlock(&info->virtioQueueLock);

			if (res < B_OK)
				return res;

			return B_OK;
		}
		case virtioInputGetType:
		{
			TRACE("virtioInputGetType\n");
			if (buffer == NULL || length < sizeof(VirtioInputType))
				return B_BAD_VALUE;

			return user_memcpy(buffer, &info->type, sizeof(VirtioInputType));
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

	// check whether it's really a Virtio input device
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
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { .string = "VirtIO input" }},
		{ NULL }
	};

	return gDeviceManager->register_node(node, VIRTIO_INPUT_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
virtio_input_init_driver(device_node *node, void **cookie)
{
	CALLED();

	ObjectDeleter<VirtioInputDevice> info(new(std::nothrow) VirtioInputDevice());

	if (!info.IsSet())
		return B_NO_MEMORY;

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
	.info = {
		.name = VIRTIO_INPUT_DEVICE_MODULE_NAME,
	},

	.init_device = virtio_input_init_device,
	.uninit_device = virtio_input_uninit_device,

	.open = virtio_input_open,
	.close = virtio_input_close,
	.free = virtio_input_free,
	.read = virtio_input_read,
	.write = virtio_input_write,
	.control = virtio_input_control,
};

struct driver_module_info sVirtioInputDriver = {
	.info = {
		.name = VIRTIO_INPUT_DRIVER_MODULE_NAME,
	},

	.supports_device = virtio_input_supports_device,
	.register_device = virtio_input_register_device,
	.init_driver = virtio_input_init_driver,
	.uninit_driver = virtio_input_uninit_driver,
	.register_child_devices = virtio_input_register_child_devices,
};

module_info* modules[] = {
	(module_info*)&sVirtioInputDriver,
	(module_info*)&sVirtioInputDevice,
	NULL
};
