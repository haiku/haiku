/*
	Driver for USB Ethernet Control Model devices
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#ifndef _USB_ECM_DRIVER_H_
#define _USB_ECM_DRIVER_H_

#include <device_manager.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>
#include <USB3.h>

#include <util/kernel_cpp.h>

#define DRIVER_NAME	"usb_ecm"
#define MAX_DEVICES	8

/* class and subclass codes */
#define USB_INTERFACE_CLASS_CDC			0x02
#define USB_INTERFACE_SUBCLASS_ECM		0x06
#define USB_INTERFACE_CLASS_CDC_DATA	0x0a
#define USB_INTERFACE_SUBCLASS_DATA		0x00

/* communication device descriptor subtypes */
#define FUNCTIONAL_SUBTYPE_UNION		0x06
#define FUNCTIONAL_SUBTYPE_ETHERNET		0x0f

typedef struct ethernet_functional_descriptor_s {
	uint8	functional_descriptor_subtype;
	uint8	mac_address_index;
	uint32	ethernet_statistics;
	uint16	max_segment_size;
	uint16	num_multi_cast_filters;
	uint8	num_wakeup_pattern_filters;
} _PACKED ethernet_functional_descriptor;

/* notification definitions */
#define CDC_NOTIFY_NETWORK_CONNECTION		0x00
#define CDC_NOTIFY_CONNECTION_SPEED_CHANGE	0x2a

typedef struct cdc_notification_s {
	uint8	request_type;
	uint8	notification_code;
	uint16	value;
	uint16	index;
	uint16	data_length;
	uint8	data[0];
} _PACKED cdc_notification;

typedef struct cdc_connection_speed_s {
	uint32	upstream_speed; /* in bits/s */
	uint32	downstream_speed; /* in bits/s */
} _PACKED cdc_connection_speed;

extern usb_module_info *gUSBModule;
extern device_manager_info *gDeviceManager;

class ECMDevice;

// bus manager device interface for peripheral driver
typedef struct {
        driver_module_info info;

} usb_device_interface;


typedef struct {
	device_node*			node;
	::usb_device			usb_device;
	usb_device_interface*	usb;
	ECMDevice *				device;
} usb_ecm_driver_info;



#define	TRACE(x...)			dprintf(DRIVER_NAME ": " x)
#define TRACE_ALWAYS(x...)	dprintf(DRIVER_NAME ": " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#endif //_USB_ECM_DRIVER_H_
