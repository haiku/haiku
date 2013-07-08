/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioPrivate.h"


const char *
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


const char *
virtio_get_device_type_name(uint16 type)
{
	switch (type) {
		case VIRTIO_DEVICE_ID_NETWORK:
			return "network";
		case VIRTIO_DEVICE_ID_BLOCK:
			return "block";
		case VIRTIO_DEVICE_ID_CONSOLE:
			return "console";
		case VIRTIO_DEVICE_ID_ENTROPY:
			return "entropy";
		case VIRTIO_DEVICE_ID_BALLOON:
			return "balloon";
		case VIRTIO_DEVICE_ID_IOMEMORY:
			return "io_memory";
		case VIRTIO_DEVICE_ID_SCSI:
			return "scsi";
		case VIRTIO_DEVICE_ID_9P:
			return "9p transport";
		default:
			return "unknown";
	}
}


VirtioDevice::VirtioDevice(device_node *node)
	:
	fNode(node),
	fID(0),
	fController(NULL),
	fCookie(NULL),
	fStatus(B_NO_INIT),
	fQueues(NULL),
	fFeatures(0)
{
	device_node *parent = gDeviceManager->get_parent_node(node);
	fStatus = gDeviceManager->get_driver(parent,
		(driver_module_info **)&fController, &fCookie);
	gDeviceManager->put_node(parent);

	if (fStatus != B_OK)
		return;

	fStatus = gDeviceManager->get_attr_uint16(fNode,
		VIRTIO_VRING_ALIGNMENT_ITEM, &fAlignment, true);
	if (fStatus != B_OK) {
		ERROR("alignment missing\n");
		return;
	}

	fController->set_sim(fCookie, this);

	fController->set_status(fCookie, VIRTIO_CONFIG_STATUS_DRIVER);
}


VirtioDevice::~VirtioDevice()
{
	for (size_t index = 0; index < fQueueCount; index++) {
		delete fQueues[index];
	}
	delete fQueues;
}


status_t
VirtioDevice::InitCheck()
{
	return fStatus;
}


status_t
VirtioDevice::NegociateFeatures(uint32 supported, uint32* negociated,
	const char* (*get_feature_name)(uint32))
{
	fFeatures = 0;
	status_t status = fController->read_host_features(fCookie, &fFeatures);
	if (status != B_OK)
		return status;

	DumpFeatures("read features", fFeatures, get_feature_name);

	fFeatures &= supported;

	// filter our own features
	fFeatures &= (VIRTIO_FEATURE_TRANSPORT_MASK
		/*| VIRTIO_FEATURE_RING_INDIRECT_DESC*/ | VIRTIO_FEATURE_RING_EVENT_IDX);

	*negociated = fFeatures;
	
	DumpFeatures("negociated features", fFeatures, get_feature_name);

	return fController->write_guest_features(fCookie, fFeatures);
}
	

status_t
VirtioDevice::ReadDeviceConfig(uint8 offset, void* buffer, size_t bufferSize)
{
	return fController->read_device_config(fCookie, offset, buffer,
		bufferSize);
}


status_t
VirtioDevice::WriteDeviceConfig(uint8 offset, const void* buffer,
	size_t bufferSize)
{
	return fController->write_device_config(fCookie, offset, buffer,
		bufferSize);
}


status_t
VirtioDevice::AllocateQueues(size_t count, virtio_queue *queues)
{
	if (count > VIRTIO_VIRTQUEUES_MAX_COUNT || queues == NULL)
		return B_BAD_VALUE;

	status_t status = B_OK;
	fQueues = new(std::nothrow) VirtioQueue*[count];
	if (fQueues == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}

	fQueueCount = count;
	for (size_t index = 0; index < count; index++) {
		uint16 size = fController->get_queue_ring_size(fCookie, index);
		fQueues[index] = new(std::nothrow) VirtioQueue(this, index, size);
		queues[index] = fQueues[index];
		status = B_NO_MEMORY;
		if (fQueues[index] != NULL)
			status = fQueues[index]->InitCheck();
		if (status != B_OK)
			goto err;
	}

	return B_OK;

err:
	return status;
}


status_t
VirtioDevice::SetupInterrupt(virtio_intr_func configHandler,
	void* configCookie)
{
	fConfigHandler = configHandler;
	fConfigCookie = configCookie;
	status_t status = fController->setup_interrupt(fCookie);
	if (status != B_OK)
		return status;

	// ready to go	
	fController->set_status(fCookie, VIRTIO_CONFIG_STATUS_DRIVER_OK);

	for (size_t index = 0; index < fQueueCount; index++)
		fQueues[index]->EnableInterrupt();
	return B_OK;
}


status_t
VirtioDevice::SetupQueue(uint16 queueNumber, phys_addr_t physAddr)
{
	return fController->setup_queue(fCookie, queueNumber, physAddr);
}			


void
VirtioDevice::NotifyQueue(uint16 queueNumber)
{
	fController->notify_queue(fCookie, queueNumber);
}


status_t
VirtioDevice::QueueInterrupt(uint16 queueNumber)
{
	if (queueNumber != INT16_MAX) {
		if (queueNumber >= fQueueCount)
			return B_BAD_VALUE;
		return fQueues[queueNumber]->Interrupt();
	}
	
	status_t status = B_OK;
	for (uint16 i = 0; i < fQueueCount; i++) {
		status = fQueues[i]->Interrupt();
		if (status != B_OK)
			break;
	}
	
	return status;
}


status_t
VirtioDevice::ConfigInterrupt()
{
	if (fConfigHandler != NULL)
		fConfigHandler(fConfigCookie);
	return B_OK;
}


void
VirtioDevice::DumpFeatures(const char* title, uint32 features,
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
			snprintf(features_string, sizeof(features_string), "%s[%s] ",
			features_string, name);
		}
	}
	TRACE("%s: %s\n", title, features_string);
}

