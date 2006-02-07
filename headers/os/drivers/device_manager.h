/*
 * Copyright 2004-2005, Haiku Inc. All Rights Reserved.
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
			void	*data;
			size_t	length;
		} raw;
	} value;
} device_attr;


typedef struct device_node_info *device_node_handle;
typedef struct io_resource_info *io_resource_handle;
typedef struct device_attr_info *device_attr_handle;

typedef struct driver_module_info driver_module_info;


// interface of the device manager

typedef struct device_manager_info {
	module_info info;

	status_t (*init_driver)(device_node_handle node, void *userCookie,
					driver_module_info **interface, void **cookie);
	status_t (*uninit_driver)(device_node_handle node);

	status_t (*rescan)(device_node_handle node);

	status_t (*register_device)(device_node_handle parent,
					const device_attr *attrs,
					const io_resource_handle *io_resources,
					device_node_handle *node);
	status_t (*unregister_device)(device_node_handle node);

	device_node_handle (*get_root)();
	status_t (*get_next_child_device)(device_node_handle parent,
		device_node_handle *_node, const device_attr *attrs);
	device_node_handle (*get_parent)(device_node_handle node);
	void (*put_device_node)(device_node_handle node);

	status_t (*acquire_io_resources)(io_resource *resources,
					io_resource_handle *handles);
	status_t (*release_io_resources)(const io_resource_handle *handles);

	int32 (*create_id)(const char *generator);
	status_t (*free_id)(const char *generator, uint32 id);

	status_t (*get_attr_uint8)(device_node_handle node,
					const char *name, uint8 *value, bool recursive);
	status_t (*get_attr_uint16)(device_node_handle node,
					const char *name, uint16 *value, bool recursive);
	status_t (*get_attr_uint32)(device_node_handle node,
					const char *name, uint32 *value, bool recursive);
	status_t (*get_attr_uint64)(device_node_handle node,
					const char *name, uint64 *value, bool recursive);
	status_t (*get_attr_string)(device_node_handle node,
					const char *name, char **value, bool recursive);
	status_t (*get_attr_raw)(device_node_handle node,
					const char *name, void **data, size_t *_size,
					bool recursive);

	status_t (*get_next_attr)(device_node_handle node,
					device_attr_handle *attrHandle);
	status_t (*release_attr)(device_node_handle node,
					device_attr_handle attr_handle);
	status_t (*retrieve_attr)(device_attr_handle attr_handle,
					const device_attr **attr);
	status_t (*write_attr)(device_node_handle node,
					const device_attr *attr);
	status_t (*remove_attr)(device_node_handle node, const char *name);
} device_manager_info;


#define B_DEVICE_MANAGER_MODULE_NAME "system/device_manager/v1"


// interface of device driver

struct driver_module_info {
	module_info info;

	float (*supports_device)(device_node_handle parent, bool *_noConnection);
	status_t (*register_device)(device_node_handle parent);

	status_t (*init_driver)(device_node_handle node, void *user_cookie, void **_cookie);
	status_t (*uninit_driver)(void *cookie);

	void (*device_removed)(device_node_handle node, void *cookie);
	void (*device_cleanup)(device_node_handle node);

	void (*get_supported_paths)(const char ***_busses, const char ***_devices);
};


// standard device node attributes

#define PNP_MANAGER_ID_GENERATOR "id_generator"
	// if you are using an id generator (see create_id), you can let the 
	// manager automatically free the id when the node is deleted by setting 
	// the following attributes:
	// name of generator (string)
#define PNP_MANAGER_AUTO_ID "auto_id"
	// generated id (uint32)
	
#define B_DRIVER_MODULE				"driver/module"
#define B_DRIVER_PRETTY_NAME		"driver/pretty name"
#define B_DRIVER_FIXED_CHILD		"driver/fixed child"
#define B_DRIVER_MAPPING			"driver/mapping"
#define B_DRIVER_BUS				"driver/bus"
#define B_DRIVER_FIND_DEVICES_ON_DEMAND "driver/on demand"
#define B_DRIVER_EXPLORE_LAST		"driver/explore last"

#define B_DRIVER_UNIQUE_DEVICE_ID	"unique id"
#define B_DRIVER_DEVICE_TYPE		"device type"

#define B_AUDIO_DRIVER_TYPE			"audio"
#define B_BUS_DRIVER_TYPE			"bus"
#define B_DISK_DRIVER_TYPE			"disk"
#define B_GRAPHICS_DRIVER_TYPE		"graphics"
#define B_INPUT_DRIVER_TYPE			"input"
#define B_MISC_DRIVER_TYPE			"misc"
#define B_NETWORK_DRIVER_TYPE		"net"
#define B_VIDEO_DRIVER_TYPE			"video"
#define B_INTERRUPT_CONTROLLER_DRIVER_TYPE	"interrupt controller"

#define PNP_DRIVER_CONNECTION "connection"
	// connection of parent the device is attached to (optional, string)
	// there can be only one device per connection


// interface of a bus device driver

typedef struct bus_module_info {
	driver_module_info info;

	// scan the bus and register all devices.
	status_t (*register_child_devices)(void *cookie);

	// user initiated rescan of the bus - only propagate changes
	// you only need to implement this, if you cannot detect device changes yourself
	// driver is always loaded during this call, but other hooks may
	// be called concurrently
	status_t (*rescan_bus)(void *cookie);
} bus_module_info;


// standard attributes

// PnP bus identification (required, uint8)
// define this to let the PnP manager know that this is a PnP bus
// the actual content is ignored
#define PNP_BUS_IS_BUS "bus/is_bus"

#endif	/* _DEVICE_MANAGER_H */
