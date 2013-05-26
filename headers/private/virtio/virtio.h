/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VIRTIO_H_
#define _VIRTIO_H_


#include <device_manager.h>
#include <KernelExport.h>


#define VIRTIO_DEVICE_ID_NETWORK	0x01
#define VIRTIO_DEVICE_ID_BLOCK		0x02
#define VIRTIO_DEVICE_ID_CONSOLE	0x03
#define VIRTIO_DEVICE_ID_ENTROPY	0x04
#define VIRTIO_DEVICE_ID_BALLOON	0x05
#define VIRTIO_DEVICE_ID_IOMEMORY	0x06
#define VIRTIO_DEVICE_ID_SCSI		0x08
#define VIRTIO_DEVICE_ID_9P			0x09

#define VIRTIO_FEATURE_TRANSPORT_MASK	((1 << 28) - 1)

#define VIRTIO_FEATURE_NOTIFY_ON_EMPTY 		(1 << 24)
#define VIRTIO_FEATURE_RING_INDIRECT_DESC	(1 << 28)
#define VIRTIO_FEATURE_RING_EVENT_IDX		(1 << 29)
#define VIRTIO_FEATURE_BAD_FEATURE 			(1 << 30)

#define VIRTIO_VIRTQUEUES_MAX_COUNT	8

#define VIRTIO_CONFIG_STATUS_RESET	0x00
#define VIRTIO_CONFIG_STATUS_ACK	0x01
#define VIRTIO_CONFIG_STATUS_DRIVER	0x02
#define VIRTIO_CONFIG_STATUS_DRIVER_OK	0x04
#define VIRTIO_CONFIG_STATUS_FAILED	0x80

// attributes:

// node type
#define VIRTIO_BUS_TYPE_NAME "bus/virtio/v1"
// device type (uint16)
#define VIRTIO_DEVICE_TYPE_ITEM "virtio/type"
// alignment (uint16)
#define VIRTIO_VRING_ALIGNMENT_ITEM "virtio/vring_alignment"

// sim cookie, issued by virtio bus manager
typedef void* virtio_sim;
// device cookie, issued by virtio bus manager
typedef void* virtio_device;
// queue cookie, issued by virtio bus manager
typedef void* virtio_queue;
// callback function for requests
typedef void (*virtio_callback_func)(void *cookie);
// callback function for interrupts
typedef void (*virtio_intr_func)(void *cookie);

#define	VIRTIO_DEVICE_MODULE_NAME "bus_managers/virtio/device/v1"

typedef struct {
	driver_module_info info;

	status_t	(*queue_interrupt_handler)(virtio_sim sim, uint16 queue);
	status_t	(*config_interrupt_handler)(virtio_sim sim);
} virtio_for_controller_interface;

#define VIRTIO_FOR_CONTROLLER_MODULE_NAME "bus_managers/virtio/controller/driver_v1"

// Bus manager interface used by Virtio controller drivers.
typedef struct {
	driver_module_info info;

	void (*set_sim)(void* cookie, virtio_sim sim);
	status_t (*read_host_features)(void* cookie, uint32* features);
	status_t (*write_guest_features)(void* cookie, uint32 features);
	uint8 (*get_status)(void* cookie);
	void (*set_status)(void* cookie, uint8 status);
	status_t (*read_device_config)(void* cookie, uint8 offset, void* buffer,
		size_t bufferSize);
	status_t (*write_device_config)(void* cookie, uint8 offset,
		const void* buffer, size_t bufferSize);
	
	uint16	(*get_queue_ring_size)(void* cookie, uint16 queue);
	status_t (*setup_queue)(void* cookie, uint16 queue, phys_addr_t phy);
	status_t (*setup_interrupt)(void* cookie);
	void	(*notify_queue)(void* cookie, uint16 queue);
} virtio_sim_interface;


// bus manager device interface for peripheral driver
typedef struct {
	driver_module_info info;

	status_t (*negociate_features)(virtio_device cookie, uint32 supported,
		uint32* negociated, const char* (*get_feature_name)(uint32));
	
	status_t (*read_device_config)(virtio_device cookie, uint8 offset,
		void* buffer, size_t bufferSize);
	status_t (*write_device_config)(virtio_device cookie, uint8 offset,
		const void* buffer, size_t bufferSize);

	status_t (*alloc_queues)(virtio_device cookie, size_t count,
		virtio_queue *queues);

	status_t (*setup_interrupt)(virtio_device cookie,
		virtio_intr_func config_handler, void* configCookie);

	status_t (*queue_request)(virtio_queue queue,
		const physical_entry *readEntry,
		const physical_entry *writtenEntry, virtio_callback_func callback,
		void *callbackCookie);

	status_t (*queue_request_v)(virtio_queue queue,
		const physical_entry* vector,
		size_t readVectorCount, size_t writtenVectorCount,
		virtio_callback_func callback, void *callbackCookie);

} virtio_device_interface;


#endif	/* _VIRTIO_H_ */
