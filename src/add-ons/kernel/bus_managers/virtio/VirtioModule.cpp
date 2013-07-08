/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioPrivate.h"


device_manager_info *gDeviceManager = NULL;


//	#pragma mark -


static status_t
virtio_device_init(device_node *node, void **cookie)
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

	*cookie = device;
	return B_OK;
}


static void
virtio_device_uninit(void *cookie)
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)cookie;
	delete device;
}


static void
virtio_device_removed(void *cookie)
{
	CALLED();
	//VirtioDevice *device = (VirtioDevice *)cookie;
}


//	#pragma mark -


status_t
virtio_negociate_features(void* cookie, uint32 supported,
		uint32* negociated, const char* (*get_feature_name)(uint32))
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)cookie;
	
	return device->NegociateFeatures(supported, negociated, get_feature_name);
}

	
status_t
virtio_read_device_config(void* cookie, uint8 offset, void* buffer,
		size_t bufferSize)
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)cookie;
	
	return device->ReadDeviceConfig(offset, buffer, bufferSize);
}


status_t
virtio_write_device_config(void* cookie, uint8 offset,
		const void* buffer, size_t bufferSize)
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)cookie;
	
	return device->WriteDeviceConfig(offset, buffer, bufferSize);
}


status_t
virtio_alloc_queues(virtio_device cookie, size_t count, virtio_queue *queues)
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)cookie;
	return device->AllocateQueues(count, queues);
}


status_t
virtio_setup_interrupt(virtio_device cookie, virtio_intr_func config_handler,
	void* configCookie)
{
	CALLED();
	VirtioDevice *device = (VirtioDevice *)cookie;
	return device->SetupInterrupt(config_handler, configCookie); 
}


status_t
virtio_queue_request_v(virtio_queue cookie, const physical_entry* vector,
		size_t readVectorCount, size_t writtenVectorCount,
		virtio_callback_func callback, void *callbackCookie)
{
	CALLED();
	VirtioQueue *queue = (VirtioQueue *)cookie;
	return queue->QueueRequest(vector, readVectorCount, writtenVectorCount,
		callback, callbackCookie);
}


status_t
virtio_queue_request(virtio_queue cookie, const physical_entry *readEntry,
		const physical_entry *writtenEntry, virtio_callback_func callback,
		void *callbackCookie)
{
	physical_entry entries[2];
	if (readEntry != NULL) {
		entries[0] = *readEntry;
		if (writtenEntry != NULL)
			entries[1] = *writtenEntry;
	} else if (writtenEntry != NULL)
		entries[0] = *writtenEntry;
	
	return virtio_queue_request_v(cookie, entries, readEntry != NULL ? 1 : 0,
		writtenEntry != NULL? 1 : 0, callback, callbackCookie);
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

	virtio_negociate_features,
	virtio_read_device_config,
	virtio_write_device_config,
	virtio_alloc_queues,
	virtio_setup_interrupt,
	virtio_queue_request,
	virtio_queue_request_v
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

module_info *modules[] = {
	(module_info *)&virtio_for_controller_module,
	(module_info *)&virtio_device_module,
	NULL
};

