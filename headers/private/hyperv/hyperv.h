/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_H_
#define _HYPERV_H_


#include <device_manager.h>
#include <KernelExport.h>

#include <hyperv_spec.h>

#define HYPERV_BUS_NAME						"hyperv"

// Device attributes for the VMBus device
#define HYPERV_CHANNEL_ID_ITEM				"hyperv/channel"
#define HYPERV_DEVICE_TYPE_STRING_ITEM		"hyperv/type_string"
#define HYPERV_INSTANCE_ID_ITEM				"hyperv/instance"
#define HYPERV_MMIO_SIZE_ITEM				"hyperv/mmio_size"

#define HYPERV_PRETTYNAME_VMBUS				"Hyper-V Virtual Machine Bus"
#define HYPERV_PRETTYNAME_VMBUS_DEVICE_FMT	"Hyper-V Channel %u"
#define HYPERV_PRETTYNAME_AVMA				"Hyper-V Automatic Virtual Machine Activation"
#define HYPERV_PRETTYNAME_BALLOON			"Hyper-V Dynamic Memory"
#define HYPERV_PRETTYNAME_DISPLAY			"Hyper-V Display"
#define HYPERV_PRETTYNAME_FIBRECHANNEL		"Hyper-V Fibre Channel"
#define HYPERV_PRETTYNAME_FILECOPY			"Hyper-V File Copy"
#define HYPERV_PRETTYNAME_HEARTBEAT			"Hyper-V Heartbeat"
#define HYPERV_PRETTYNAME_IDE				"Hyper-V IDE Controller"
#define HYPERV_PRETTYNAME_INPUT				"Hyper-V Input"
#define HYPERV_PRETTYNAME_KEYBOARD			"Hyper-V Keyboard"
#define HYPERV_PRETTYNAME_KVP				"Hyper-V Data Exchange"
#define HYPERV_PRETTYNAME_NETWORK			"Hyper-V Network Adapter"
#define HYPERV_PRETTYNAME_PCI				"Hyper-V PCI Bridge"
#define HYPERV_PRETTYNAME_RDCONTROL			"Hyper-V Remote Desktop Control"
#define HYPERV_PRETTYNAME_RDMA				"Hyper-V RDMA"
#define HYPERV_PRETTYNAME_RDVIRT			"Hyper-V Remote Desktop Virtualization"
#define HYPERV_PRETTYNAME_SCSI				"Hyper-V SCSI Controller"
#define HYPERV_PRETTYNAME_SHUTDOWN			"Hyper-V Guest Shutdown"
#define HYPERV_PRETTYNAME_TIMESYNC			"Hyper-V Time Synchronization"
#define HYPERV_PRETTYNAME_VSS				"Hyper-V Volume Shadow Copy"


typedef void* hyperv_device;
typedef void (*hyperv_device_callback)(void* data);


// Interface between the VMBus device driver, and the VMBus device bus manager
typedef struct hyperv_device_interface {
	driver_module_info info;

	uint32 (*get_bus_version)(hyperv_device cookie);
	status_t (*get_reference_counter)(hyperv_device cookie, uint64* _count);
	status_t (*open)(hyperv_device cookie, uint32 txLength, uint32 rxLength,
		hyperv_device_callback callback, void* callbackData);
	void (*close)(hyperv_device cookie);
	status_t (*read_packet)(hyperv_device cookie, void* buffer, uint32* bufferLength,
		uint32* _headerLength, uint32* _dataLength);
	status_t (*write_packet)(hyperv_device cookie, uint16 type, const void* buffer,
		uint32 length, bool responseRequired, uint64 transactionID);
	status_t (*write_gpa_packet)(hyperv_device cookie, uint32 rangeCount,
		const vmbus_gpa_range* rangesList, uint32 rangesLength, const void* buffer,
		uint32 length, bool responseRequired, uint64 transactionID);
	status_t (*allocate_gpadl)(hyperv_device cookie, uint32 length, void** _buffer,
		uint32* _gpadl);
	status_t (*free_gpadl)(hyperv_device cookie, uint32 gpadl);
} hyperv_device_interface;


#endif // _HYPERV_H_
