/*
 * Copyright 2013, 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioPrivate.h"


device_manager_info *gDeviceManager = NULL;


//	#pragma mark -


static status_t
virtio_device_init(device_node *node, void **_device)
{
	CALLED();
	VirtioDevice *device = new(std::nothrow) VirtioDevice(node);
	if (device == NULL)
		return B_NO_MEMORY;

	status_t result = device->InitCheck();
	if (result != B_OK) {
		ERROR("failed to set up virtio device object\n");
		return result;
	}

	*_device = device;
	return B_OK;
}


static void
virtio_device_uninit(void *_device)
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)_device;
	delete device;
}


static void
virtio_device_removed(void *_device)
{
	CALLED();
	//VirtioDevice *device = (VirtioDevice *)_device;
}


//	#pragma mark -


status_t
virtio_negotiate_features(void* _device, uint32 supported,
		uint32* negotiated, const char* (*get_feature_name)(uint32))
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)_device;

	return device->NegotiateFeatures(supported, negotiated, get_feature_name);
}


status_t
virtio_read_device_config(void* _device, uint8 offset, void* buffer,
		size_t bufferSize)
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)_device;

	return device->ReadDeviceConfig(offset, buffer, bufferSize);
}


status_t
virtio_write_device_config(void* _device, uint8 offset,
		const void* buffer, size_t bufferSize)
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)_device;

	return device->WriteDeviceConfig(offset, buffer, bufferSize);
}


status_t
virtio_alloc_queues(virtio_device _device, size_t count, virtio_queue *queues)
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)_device;
	return device->AllocateQueues(count, queues);
}


void
virtio_free_queues(virtio_device _device)
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)_device;
	device->FreeQueues();
}


status_t
virtio_setup_interrupt(virtio_device _device, virtio_intr_func config_handler,
	void *driverCookie)
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)_device;
	return device->SetupInterrupt(config_handler, driverCookie);
}


status_t
virtio_free_interrupts(virtio_device _device)
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)_device;
	return device->FreeInterrupts();
}


status_t
virtio_queue_setup_interrupt(virtio_queue _queue, virtio_callback_func handler,
	void *cookie)
{
	CALLED();
	VirtioQueue *queue = (VirtioQueue *)_queue;
	return queue->SetupInterrupt(handler, cookie);
}


status_t
virtio_queue_request_v(virtio_queue _queue, const physical_entry* vector,
		size_t readVectorCount, size_t writtenVectorCount, void *cookie)
{
	CALLED();
	VirtioQueue *queue = (VirtioQueue *)_queue;
	return queue->QueueRequest(vector, readVectorCount, writtenVectorCount,
		cookie);
}


status_t
virtio_queue_request(virtio_queue _queue, const physical_entry *readEntry,
		const physical_entry *writtenEntry, void *cookie)
{
	physical_entry entries[2];
	if (readEntry != NULL) {
		entries[0] = *readEntry;
		if (writtenEntry != NULL)
			entries[1] = *writtenEntry;
	} else if (writtenEntry != NULL)
		entries[0] = *writtenEntry;

	return virtio_queue_request_v(_queue, entries, readEntry != NULL ? 1 : 0,
		writtenEntry != NULL? 1 : 0, cookie);
}


bool
virtio_queue_is_full(virtio_queue _queue)
{
	VirtioQueue *queue = (VirtioQueue *)_queue;
	return queue->IsFull();
}


bool
virtio_queue_is_empty(virtio_queue _queue)
{
	VirtioQueue *queue = (VirtioQueue *)_queue;
	return queue->IsEmpty();
}


uint16
virtio_queue_size(virtio_queue _queue)
{
	VirtioQueue *queue = (VirtioQueue *)_queue;
	return queue->Size();
}


void*
virtio_queue_dequeue(virtio_queue _queue, uint32* _usedLength)
{
	VirtioQueue *queue = (VirtioQueue *)_queue;
	return queue->Dequeue(_usedLength);
}


//	#pragma mark -


status_t
virtio_added_device(device_node *parent)
{
	CALLED();

	uint16 deviceType;
	if (gDeviceManager->get_attr_uint16(parent,
		VIRTIO_DEVICE_TYPE_ITEM, &deviceType, true) != B_OK) {
		ERROR("device type missing\n");
		return B_ERROR;
	}

	device_attr attributes[] = {
		// info about device
		{ B_DEVICE_BUS, B_STRING_TYPE, { string: "virtio" }},
		{ VIRTIO_DEVICE_TYPE_ITEM, B_UINT16_TYPE,
			{ ui16: deviceType }},
		{ NULL }
	};

	return gDeviceManager->register_node(parent, VIRTIO_DEVICE_MODULE_NAME,
		attributes, NULL, NULL);
}


status_t
virtio_queue_interrupt_handler(virtio_sim sim, uint16 queue)
{
	VirtioDevice* device = (VirtioDevice*)sim;
	return device->QueueInterrupt(queue);
}


status_t
virtio_config_interrupt_handler(virtio_sim sim)
{
	VirtioDevice* device = (VirtioDevice*)sim;
	return device->ConfigInterrupt();
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			break;
	}

	return B_ERROR;
}


//	#pragma mark -


virtio_device_interface virtio_device_module = {
	{
		{
			VIRTIO_DEVICE_MODULE_NAME,
			0,
			std_ops
		},

		NULL, // supported devices
		NULL, // register node
		virtio_device_init,
		virtio_device_uninit,
		NULL, // register child devices
		NULL, // rescan
		virtio_device_removed,
		NULL, // suspend
		NULL, // resume
	},

	virtio_negotiate_features,
	virtio_read_device_config,
	virtio_write_device_config,
	virtio_alloc_queues,
	virtio_free_queues,
	virtio_setup_interrupt,
	virtio_free_interrupts,
	virtio_queue_setup_interrupt,
	virtio_queue_request,
	virtio_queue_request_v,
	virtio_queue_is_full,
	virtio_queue_is_empty,
	virtio_queue_size,
	virtio_queue_dequeue
};

virtio_for_controller_interface virtio_for_controller_module = {
	{
		{
			VIRTIO_FOR_CONTROLLER_MODULE_NAME,
			0,
			&std_ops
		},

		NULL, // supported devices
		virtio_added_device,
		NULL,
		NULL,
		NULL
	},

	virtio_queue_interrupt_handler,
	virtio_config_interrupt_handler
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager },
	{}
};


extern struct driver_module_info sVirtioBalloonDriver;
extern struct driver_module_info sVirtioBalloonDeviceInterface;


module_info *modules[] = {
	(module_info *)&virtio_for_controller_module,
	(module_info *)&virtio_device_module,
	(module_info *)&sVirtioBalloonDriver,
	(module_info *)&sVirtioBalloonDeviceInterface,
	NULL
};

