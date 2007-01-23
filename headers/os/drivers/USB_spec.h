/* 
** USB_spec.h
**
** Copyright 1999, Be Incorporated. All Rights Reserved.
**
** This file contains structures and constants based on the USB Specification 1.1
**
*/

#ifndef _USB_SPEC_H
#define _USB_SPEC_H

#ifdef __cplusplus
extern "C" {
#endif

/* request types (target & direction) for  send_request() */
/* cf USB Spec Rev 1.1, table 9-2, p 183 */
#define USB_REQTYPE_DEVICE_IN         0x80
#define USB_REQTYPE_DEVICE_OUT        0x00
#define USB_REQTYPE_INTERFACE_IN      0x81
#define USB_REQTYPE_INTERFACE_OUT     0x01
#define USB_REQTYPE_ENDPOINT_IN       0x82
#define USB_REQTYPE_ENDPOINT_OUT      0x02
#define USB_REQTYPE_OTHER_OUT         0x03
#define USB_REQTYPE_OTHER_IN          0x83

/* request types for send_request() */
/* cf USB Spec Rev 1.1, table 9-2, p 183 */
#define USB_REQTYPE_STANDARD          0x00
#define USB_REQTYPE_CLASS             0x20
#define USB_REQTYPE_VENDOR            0x40
#define USB_REQTYPE_RESERVED          0x60
#define USB_REQTYPE_MASK              0x9F

/* standard request values for send_request() */
/* cf USB Spec Rev 1.1, table 9-4, p 187 */
#define USB_REQUEST_GET_STATUS           0
#define USB_REQUEST_CLEAR_FEATURE        1
#define USB_REQUEST_SET_FEATURE          3
#define USB_REQUEST_SET_ADDRESS          5
#define USB_REQUEST_GET_DESCRIPTOR       6
#define USB_REQUEST_SET_DESCRIPTOR       7
#define USB_REQUEST_GET_CONFIGURATION    8
#define USB_REQUEST_SET_CONFIGURATION    9
#define USB_REQUEST_GET_INTERFACE       10
#define USB_REQUEST_SET_INTERFACE       11
#define USB_REQUEST_SYNCH_FRAME         12

/* used by {set,get}_descriptor() */
/* cf USB Spec Rev 1.1, table 9-5, p 187 */
#define USB_DESCRIPTOR_DEVICE            1
#define USB_DESCRIPTOR_CONFIGURATION     2
#define USB_DESCRIPTOR_STRING            3
#define USB_DESCRIPTOR_INTERFACE         4
#define USB_DESCRIPTOR_ENDPOINT          5

/* used by {set,clear}_feature() */
/* cf USB Spec Rev 1.1, table 9-6, p 188 */
#define USB_FEATURE_DEVICE_REMOTE_WAKEUP	1
#define USB_FEATURE_ENDPOINT_HALT			0

#define USB_ENDPOINT_ATTR_CONTROL		0x00
#define USB_ENDPOINT_ATTR_ISOCHRONOUS	0x01
#define USB_ENDPOINT_ATTR_BULK			0x02 
#define USB_ENDPOINT_ATTR_INTERRUPT		0x03 
#define USB_ENDPOINT_ATTR_MASK			0x03 

#define USB_ENDPOINT_ADDR_DIR_IN	 	0x80
#define USB_ENDPOINT_ADDR_DIR_OUT		0x00


typedef struct { 
	/* cf USB Spec Rev 1.1, table 9-7, p 197 */
	uint8 length;
	uint8 descriptor_type;          /* USB_DESCRIPTOR_DEVICE */
	uint16 usb_version;             /* USB_DESCRIPTOR_DEVICE_LENGTH */
	uint8 device_class;
	uint8 device_subclass;
	uint8 device_protocol;
	uint8 max_packet_size_0;
	uint16 vendor_id;
	uint16 product_id;
	uint16 device_version;
	uint8 manufacturer;
	uint8 product;
	uint8 serial_number;
	uint8 num_configurations;
} _PACKED usb_device_descriptor;

typedef struct {
	/* cf USB Spec Rev 1.1, table 9-8, p 199 */
	uint8 length;
	uint8 descriptor_type;          /* USB_DESCRIPTOR_CONFIGURATION */
	uint16 total_length;            /* USB_DESCRIPTOR_CONFIGURATION_LENGTH */
	uint8 number_interfaces;
	uint8 configuration_value;
	uint8 configuration;
	uint8 attributes;
	uint8 max_power;
} _PACKED usb_configuration_descriptor;

typedef struct {
	/* cf USB Spec Rev 1.1, table 9-9, p 202 */
	uint8 length;
	uint8 descriptor_type;          /* USB_DESCRIPTOR_INTERFACE */
	uint8 interface_number;         /* USB_DESCRIPTOR_INTERFACE_LENGTH  */
	uint8 alternate_setting;
	uint8 num_endpoints;
	uint8 interface_class;
	uint8 interface_subclass;
	uint8 interface_protocol;
	uint8 interface;
} _PACKED usb_interface_descriptor;

typedef struct {
	/* cf USB Spec Rev 1.1, table 9-10, p 203 */
	uint8 length;
	uint8 descriptor_type;          /* USB_DESCRIPTOR_ENDPOINT */
	uint8 endpoint_address;         /* USB_DESCRIPTOR_ENDPOINT_LENGTH */
	uint8 attributes;
	uint16 max_packet_size;
	uint8 interval;
} _PACKED usb_endpoint_descriptor;

typedef struct {
	/* cf USB Spec Rev 1.1, table 9-12, p 205 */
	uint8 length;                   /* USB_DESCRIPTOR_STRING */
	uint8 descriptor_type;
	uchar string[1];
} _PACKED usb_string_descriptor;

typedef struct {
	uint8 length;
	uint8 descriptor_type;
	uint8 data[1];
} _PACKED usb_generic_descriptor;

typedef union {
	usb_generic_descriptor generic;
	usb_device_descriptor device;
	usb_interface_descriptor interface;
	usb_endpoint_descriptor endpoint;
	usb_configuration_descriptor configuration;
	usb_string_descriptor string;
} usb_descriptor;

#ifdef __cplusplus
}
#endif

#endif
