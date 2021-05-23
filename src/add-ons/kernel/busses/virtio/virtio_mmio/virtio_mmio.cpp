/*
 * Copyright 2013, 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include <ByteOrder.h>
#include <KernelExport.h>
#include <device_manager.h>

#include <drivers/bus/FDT.h>

#include <debug.h>
#include <AutoDeleterDrivers.h>

#include <virtio.h>
#include <virtio_defs.h>
#include "VirtioDevice.h"


#define VIRTIO_MMIO_DEVICE_MODULE_NAME "busses/virtio/virtio_mmio/driver_v1"
#define VIRTIO_MMIO_CONTROLLER_TYPE_NAME "virtio MMIO controller"


device_manager_info* gDeviceManager;


static const char *
virtio_get_feature_name(uint32 feature)
{
	switch (feature) {
		case VIRTIO_FEATURE_NOTIFY_ON_EMPTY:
			return "notify on empty";
		case VIRTIO_FEATURE_RING_INDIRECT_DESC:
			return "ring indirect";
		case VIRTIO_FEATURE_RING_EVENT_IDX:
			return "ring event index";
		case VIRTIO_FEATURE_BAD_FEATURE:
			return "bad feature";
	}
	return NULL;
}


static void
virtio_dump_features(const char* title, uint32 features,
	const char* (*get_feature_name)(uint32))
{
	char features_string[512] = "";
	for (uint32 i = 0; i < 32; i++) {
		uint32 feature = features & (1 << i);
		if (feature == 0)
			continue;
		const char* name = virtio_get_feature_name(feature);
		if (name == NULL)
			name = get_feature_name(feature);
		if (name != NULL) {
			strlcat(features_string, "[", sizeof(features_string));
			strlcat(features_string, name, sizeof(features_string));
			strlcat(features_string, "] ", sizeof(features_string));
		}
	}
	TRACE("%s: %s\n", title, features_string);
}


//#pragma mark Device


static float
virtio_device_supports_device(device_node* parent)
{
	TRACE("supports_device(%p)\n", parent);

	const char* name;
	const char* bus;
	const char* compatible;

	status_t status = gDeviceManager->get_attr_string(parent,
		B_DEVICE_PRETTY_NAME, &name, false);

	if (status >= B_OK)
		dprintf("  name: %s\n", name);

	status = gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false);

	if (status < B_OK)
		return -1.0f;

	status = gDeviceManager->get_attr_string(parent, "fdt/compatible",
		&compatible, false);

	if (status < B_OK)
		return -1.0f;

	if (strcmp(bus, "fdt") != 0)
		return 0.0f;

	if (strcmp(compatible, "virtio,mmio") != 0)
		return 0.0f;

	return 1.0f;
}


static status_t
virtio_device_register_device(device_node* parent)
{
	TRACE("register_device(%p)\n", parent);

	fdt_device_module_info *parentModule;
	fdt_device* parentDev;
	if (gDeviceManager->get_driver(parent, (driver_module_info**)&parentModule,
			(void**)&parentDev)) {
		ERROR("can't get parent node driver");
		return B_ERROR;
	}

	uint64 regs, regsLen;
	if (!parentModule->get_reg(parentDev, 0, &regs, &regsLen)) {
		ERROR("no regs");
		return B_ERROR;
	}

	VirtioRegs *volatile mappedRegs;
	AreaDeleter fRegsArea(map_physical_memory("Virtio MMIO", regs, regsLen,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void **)&mappedRegs));

	if (!fRegsArea.IsSet()) {
		ERROR("cant't map regs");
		return B_ERROR;
	}

	if (mappedRegs->signature != kVirtioSignature) {
		ERROR("bad signature: 0x%08" B_PRIx32 ", should be 0x%08" B_PRIx32 "\n",
			mappedRegs->signature, (uint32)kVirtioSignature);
		return B_ERROR;
	}

	dprintf("  version: 0x%08" B_PRIx32 "\n",   mappedRegs->version);
	dprintf("  deviceId: 0x%08" B_PRIx32 "\n",  mappedRegs->deviceId);
	dprintf("  vendorId: 0x%08" B_PRIx32 "\n",  mappedRegs->vendorId);

	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "Virtio MMIO"} },
		{ B_DEVICE_BUS,         B_STRING_TYPE, {string: "virtio"} },
		{ "virtio/version",     B_UINT32_TYPE, {ui32: mappedRegs->version} },
		{ "virtio/device_id",   B_UINT32_TYPE, {ui32: mappedRegs->deviceId} },
		{ "virtio/type",        B_UINT16_TYPE,
			{ui16: (uint16)mappedRegs->deviceId} },
		{ "virtio/vendor_id",   B_UINT32_TYPE, {ui32: mappedRegs->vendorId} },
		{ NULL }
	};

	return gDeviceManager->register_node(parent, VIRTIO_MMIO_DEVICE_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
virtio_device_init_device(device_node* node, void** cookie)
{
	TRACE("init_device(%p)\n", node);

	DeviceNodePutter<&gDeviceManager>
		parent(gDeviceManager->get_parent_node(node));

	fdt_device_module_info *parentModule;
	fdt_device* parentDev;
	if (gDeviceManager->get_driver(parent.Get(),
			(driver_module_info**)&parentModule, (void**)&parentDev))
		panic("can't get parent node driver");

	dprintf("  bus: %p\n", parentModule->get_bus(parentDev));
	dprintf("  compatible: %s\n", (const char*)parentModule->get_prop(parentDev,
		"compatible", NULL));

	uint64 regs;
	uint64 regsLen;
	for (uint32 i = 0; parentModule->get_reg(parentDev, i, &regs, &regsLen);
			i++) {
		dprintf("  reg[%" B_PRIu32 "]: (0x%" B_PRIx64 ", 0x%" B_PRIx64 ")\n",
			i, regs, regsLen);
	}

	device_node* interruptController;
	uint64 interrupt;
	for (uint32 i = 0; parentModule->get_interrupt(parentDev,
			i, &interruptController, &interrupt); i++) {

		const char* name;
		if (interruptController == NULL
			|| gDeviceManager->get_attr_string(interruptController, "fdt/name",
				&name, false) < B_OK) {
			name = NULL;
		}

		dprintf("  interrupt[%" B_PRIu32 "]: ('%s', 0x%" B_PRIx64 ")\n", i,
			name, interrupt);
	}

	if (!parentModule->get_reg(parentDev, 0, &regs, &regsLen)) {
		dprintf("  no regs\n");
		return B_ERROR;
	}

	if (!parentModule->get_interrupt(parentDev, 0, &interruptController,
			&interrupt)) {
		dprintf("  no interrupts\n");
		return B_ERROR;
	}

	ObjectDeleter<VirtioDevice> dev(new(std::nothrow) VirtioDevice());
	if (!dev.IsSet())
		return B_NO_MEMORY;

	status_t res = dev->Init(regs, regsLen, interrupt, 1);
	if (res < B_OK)
		return res;

	*cookie = dev.Detach();
	return B_OK;
}


static void
virtio_device_uninit_device(void* cookie)
{
	TRACE("uninit_device(%p)\n", cookie);
	ObjectDeleter<VirtioDevice> dev((VirtioDevice*)cookie);
}


static status_t
virtio_device_register_child_devices(void* cookie)
{
	TRACE("register_child_devices(%p)\n", cookie);
	return B_OK;
}


//#pragma mark driver API


static status_t
virtio_device_negotiate_features(virtio_device cookie, uint32 supported,
	uint32* negotiated, const char* (*get_feature_name)(uint32))
{
	TRACE("virtio_device_negotiate_features(%p)\n", cookie);
	VirtioDevice* dev = (VirtioDevice*)cookie;

	dev->fRegs->status |= kVirtioConfigSAcknowledge;
	dev->fRegs->status |= kVirtioConfigSDriver;

	uint32 features = dev->fRegs->deviceFeatures;
	virtio_dump_features("read features", features, get_feature_name);
	features &= supported;

	// filter our own features
	features &= (VIRTIO_FEATURE_TRANSPORT_MASK
		| VIRTIO_FEATURE_RING_INDIRECT_DESC | VIRTIO_FEATURE_RING_EVENT_IDX);
	*negotiated = features;

	virtio_dump_features("negotiated features", features, get_feature_name);

	dev->fRegs->driverFeatures = features;
	dev->fRegs->status |= kVirtioConfigSFeaturesOk;
	dev->fRegs->status |= kVirtioConfigSDriverOk;
	dev->fRegs->guestPageSize = B_PAGE_SIZE;

	return B_OK;
}


static status_t
virtio_device_clear_feature(virtio_device cookie, uint32 feature)
{
	panic("not implemented");
	return B_ERROR;
}


static status_t
virtio_device_read_device_config(virtio_device cookie, uint8 offset,
	void* buffer, size_t bufferSize)
{
	TRACE("virtio_device_read_device_config(%p, %d, %" B_PRIuSIZE ")\n", cookie,
		offset, bufferSize);

	VirtioDevice* dev = (VirtioDevice*)cookie;
	memcpy(buffer, (void*)(dev->fRegs->config + offset), bufferSize);

	return B_OK;
}


static status_t
virtio_device_write_device_config(virtio_device cookie, uint8 offset,
	const void* buffer, size_t bufferSize)
{
	TRACE("virtio_device_write_device_config(%p, %d, %" B_PRIuSIZE ")\n",
		cookie, offset, bufferSize);
	VirtioDevice* dev = (VirtioDevice*)cookie;
	memcpy((void*)(dev->fRegs->config + offset), buffer, bufferSize);
	return B_OK;
}


static status_t
virtio_device_alloc_queues(virtio_device cookie, size_t count,
	virtio_queue* queues)
{
	TRACE("virtio_device_alloc_queues(%p, %" B_PRIuSIZE ")\n", cookie, count);
	VirtioDevice* dev = (VirtioDevice*)cookie;

	ArrayDeleter<ObjectDeleter<VirtioQueue> > newQueues(new(std::nothrow)
		ObjectDeleter<VirtioQueue>[count]);

	if (!newQueues.IsSet())
		return B_NO_MEMORY;

	for (size_t i = 0; i < count; i++) {
		newQueues[i].SetTo(new(std::nothrow) VirtioQueue(dev, i));

		if (!newQueues[i].IsSet())
			return B_NO_MEMORY;

		status_t res = newQueues[i]->Init();
		if (res < B_OK)
			return res;
	}

	dev->fQueueCnt = count;
	dev->fQueues.SetTo(newQueues.Detach());

	for (size_t i = 0; i < count; i++)
		queues[i] = dev->fQueues[i].Get();

	return B_OK;
}


static void
virtio_device_free_queues(virtio_device cookie)
{
	TRACE("virtio_device_free_queues(%p)\n", cookie);
	VirtioDevice* dev = (VirtioDevice*)cookie;

	dev->fQueues.Unset();
	dev->fQueueCnt = 0;
}


static status_t
virtio_device_setup_interrupt(virtio_device cookie,
	virtio_intr_func config_handler, void* driverCookie)
{
	VirtioDevice* dev = (VirtioDevice*)cookie;
	TRACE("virtio_device_setup_interrupt(%p, %#" B_PRIxADDR ")\n", dev,
		(addr_t)config_handler);

	dev->fConfigHandler = config_handler;
	dev->fConfigHandlerCookie = driverCookie;
	dev->fConfigHandlerRef.SetTo((config_handler == NULL)
		? NULL : &dev->fIrqHandler);

	return B_OK;
}


static status_t
virtio_device_free_interrupts(virtio_device cookie)
{
	VirtioDevice* dev = (VirtioDevice*)cookie;
	TRACE("virtio_device_free_interrupts(%p)\n", dev);

	for (int32 i = 0; i < dev->fQueueCnt; i++) {
		VirtioQueue* queue = dev->fQueues[i].Get();
		queue->fQueueHandler = NULL;
		queue->fQueueHandlerCookie = NULL;
		queue->fQueueHandlerRef.Unset();
	}

	dev->fConfigHandler = NULL;
	dev->fConfigHandlerCookie = NULL;
	dev->fConfigHandlerRef.Unset();

	return B_OK;
}


static status_t
virtio_device_queue_setup_interrupt(virtio_queue aQueue,
	virtio_callback_func handler, void* cookie)
{
	TRACE("virtio_device_queue_setup_interrupt(%p, %p)\n", aQueue, handler);

	VirtioQueue* queue = (VirtioQueue*)aQueue;
	VirtioDevice* dev = queue->fDev;

	queue->fQueueHandler = handler;
	queue->fQueueHandlerCookie = cookie;
	queue->fQueueHandlerRef.SetTo((handler == NULL) ? NULL : &dev->fIrqHandler);

	return B_OK;
}


static status_t
virtio_device_queue_request_v(virtio_queue aQueue,
	const physical_entry* vector,
	size_t readVectorCount, size_t writtenVectorCount,
	void* cookie)
{
	// TRACE("virtio_device_queue_request_v(%p, %" B_PRIuSIZE ", %" B_PRIuSIZE
	//	", %p)\n", aQueue, readVectorCount, writtenVectorCount, cookie);
	VirtioQueue* queue = (VirtioQueue*)aQueue;

	return queue->Enqueue(vector, readVectorCount, writtenVectorCount, cookie);
}


static status_t
virtio_device_queue_request(virtio_queue aQueue,
	const physical_entry* readEntry,
	const physical_entry* writtenEntry, void* cookie)
{
	VirtioQueue* queue = (VirtioQueue*)aQueue;

	physical_entry vector[2];
	physical_entry* vectorEnd = vector;

	if (readEntry != NULL)
		*vectorEnd++ = *readEntry;

	if (writtenEntry != NULL)
		*vectorEnd++ = *writtenEntry;

	return queue->Enqueue(vector, (readEntry != NULL) ? 1 : 0,
		(writtenEntry != NULL) ? 1 : 0, cookie);
}


static bool
virtio_device_queue_is_full(virtio_queue queue)
{
	panic("not implemented");
	return false;
}


static bool
virtio_device_queue_is_empty(virtio_queue aQueue)
{
	VirtioQueue *queue = (VirtioQueue *)aQueue;
	return queue->fUsed->idx == queue->fLastUsed;
}


static uint16
virtio_device_queue_size(virtio_queue aQueue)
{
	VirtioQueue *queue = (VirtioQueue *)aQueue;
	return (uint16)queue->fQueueLen;
}


static bool
virtio_device_queue_dequeue(virtio_queue aQueue, void** _cookie,
	uint32* _usedLength)
{
	// TRACE("virtio_device_queue_dequeue(%p)\n", aQueue);
	VirtioQueue* queue = (VirtioQueue*)aQueue;
	return queue->Dequeue(_cookie, _usedLength);
}


//#pragma mark -


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{ NULL }
};


static virtio_device_interface sVirtioDevice = {
	{
		{
			VIRTIO_MMIO_DEVICE_MODULE_NAME,
			0,
			NULL
		},

		virtio_device_supports_device,
		virtio_device_register_device,
		virtio_device_init_device,
		virtio_device_uninit_device,
		virtio_device_register_child_devices,
		NULL,	// rescan
		NULL,	// device removed
	},
	virtio_device_negotiate_features,
	virtio_device_clear_feature,
	virtio_device_read_device_config,
	virtio_device_write_device_config,
	virtio_device_alloc_queues,
	virtio_device_free_queues,
	virtio_device_setup_interrupt,
	virtio_device_free_interrupts,
	virtio_device_queue_setup_interrupt,
	virtio_device_queue_request,
	virtio_device_queue_request_v,
	virtio_device_queue_is_full,
	virtio_device_queue_is_empty,
	virtio_device_queue_size,
	virtio_device_queue_dequeue,
};


module_info* modules[] = {
	(module_info* )&sVirtioDevice,
	NULL
};
