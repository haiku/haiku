/*
 * Copyright 2004-2008, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _DEVICE_MANAGER_H
#define _DEVICE_MANAGER_H


#include <TypeConstants.h>
#include <Drivers.h>
#include <module.h>


// type of I/O resource
enum {
	IO_MEM = 1,
	IO_PORT = 2,
	ISA_DMA_CHANNEL = 3
};


// I/O resource description
typedef struct {
	uint32	type;
		// type of I/O resource

	uint32	base;
		// I/O memory: first physical address (32 bit)
		// I/O port: first port address (16 bit)
		// ISA DMA channel: channel number (0-7)

	uint32	length;
		// I/O memory: size of address range (32 bit)
		// I/O port: size of port range (16 bit)
		// ISA DMA channel: must be 1
} io_resource;

// attribute of a device node
typedef struct {
	const char		*name;
	type_code		type;			// for supported types, see value
	union {
		uint8		ui8;			// B_UINT8_TYPE
		uint16		ui16;			// B_UINT16_TYPE
		uint32		ui32;			// B_UINT32_TYPE
		uint64		ui64;			// B_UINT64_TYPE
		const char	*string;		// B_STRING_TYPE
		struct {					// B_RAW_TYPE
			const void *data;
			size_t	length;
		} raw;
	} value;
} device_attr;


typedef struct device_node device_node;
typedef struct driver_module_info driver_module_info;


// interface of the device manager

typedef struct device_manager_info {
	module_info info;

	status_t (*rescan)(device_node *node);

	status_t (*register_device)(device_node *parent, const char *moduleName,
					const device_attr *attrs, const io_resource *ioResources,
					device_node **_node);
	status_t (*unregister_device)(device_node *node);

	driver_module_info *(*driver_module)(device_node *node);
	void *(*driver_data)(device_node *node);

	device_node *(*root_device)();
	status_t (*get_next_child_device)(device_node *parent, device_node *node,
					const device_attr *attrs);
	device_node *(*get_parent)(device_node *node);
	void (*put_device_node)(device_node *node);

#if 0
	status_t (*acquire_io_resources)(io_resource *resources);
	status_t (*release_io_resources)(const io_resource *resources);

	int32 (*create_id)(const char *generator);
	status_t (*free_id)(const char *generator, uint32 id);
#endif

	status_t (*get_attr_uint8)(device_node *node, const char *name,
					uint8 *value, bool recursive);
	status_t (*get_attr_uint16)(device_node *node, const char *name,
					uint16 *value, bool recursive);
	status_t (*get_attr_uint32)(device_node *node, const char *name,
					uint32 *value, bool recursive);
	status_t (*get_attr_uint64)(device_node *node, const char *name,
					uint64 *value, bool recursive);
	status_t (*get_attr_string)(device_node *node, const char *name,
					const char **_value, bool recursive);
	status_t (*get_attr_raw)(device_node *node, const char *name,
					const void **_data, size_t *_size, bool recursive);

	status_t (*get_next_attr)(device_node *node, device_attr **_attr);
} device_manager_info;


#define B_DEVICE_MANAGER_MODULE_NAME "system/device_manager/v1"


// interface of device driver

struct driver_module_info {
	module_info info;

	float (*supports_device)(device_node *parent);
	status_t (*register_device)(device_node *parent);

	status_t (*init_driver)(device_node *node, void **_driverData);
	void (*uninit_driver)(device_node *node);
	status_t (*register_child_devices)(device_node *node);
	status_t (*rescan_child_devices)(device_node *node);
	void (*device_removed)(device_node *node);
};


// standard device node attributes

#define B_DRIVER_PRETTY_NAME		"driver/pretty name"	// string
#define B_DRIVER_MAPPING			"driver/mapping"		// string
#define B_DRIVER_IS_BUS				"driver/is_bus"			// uint8
#define B_DRIVER_BUS				"driver/bus"			// string
#define B_DRIVER_FIXED_CHILD		"fixed child"			// string
#define B_DRIVER_FIND_CHILD_FLAGS	"find child flags"		// uint32
#define B_DRIVER_UNIQUE_DEVICE_ID	"unique id"				// string
#define B_DRIVER_DEVICE_TYPE		"device type"			// string

// find child flags
#define B_FIND_CHILD_ON_DEMAND		0x01
#define B_FIND_MULTIPLE_CHILDREN	0x02

// driver types
#define B_AUDIO_DRIVER_TYPE			"audio"
#define B_BUS_DRIVER_TYPE			"bus"
#define B_DISK_DRIVER_TYPE			"disk"
#define B_GRAPHICS_DRIVER_TYPE		"graphics"
#define B_INPUT_DRIVER_TYPE			"input"
#define B_MISC_DRIVER_TYPE			"misc"
#define B_NETWORK_DRIVER_TYPE		"net"
#define B_VIDEO_DRIVER_TYPE			"video"
#define B_INTERRUPT_CONTROLLER_DRIVER_TYPE	"interrupt controller"


// interface of device

typedef struct io_request io_request;

struct device_module_info {
	module_info info;

	status_t (*init_device)(void *cookie);
	void (*uninit_device)(void *cookie);

	status_t (*device_open)(void *deviceCookie, int openMode, void **_cookie);
	status_t (*device_close)(void *cookie);
	status_t (*device_free)(void *cookie);
	status_t (*device_read)(void *cookie, off_t pos, void *buffer,
		size_t *_length);
	status_t (*device_write)(void *cookie, off_t pos, const void *buffer,
		size_t *_length);
	status_t (*device_ioctl)(void *cookie, int32 op, void *buffer,
		size_t length);
	status_t (*device_io)(void *cookie, io_request *request);
};

extern struct device_manager_info *gDeviceManager;

#endif	/* _DEVICE_MANAGER_H */
