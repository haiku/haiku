/*
** USB.h - Version 3 USB Device Driver API
**
** Copyright 1999, Be Incorporated. All Rights Reserved.
**
*/

#ifndef _USB_V3_H
#define _USB_V3_H

#include <KernelExport.h>
#include <bus_manager.h>
#include <iovec.h>

#include <USB_spec.h>
#include <USB_rle.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usb_module_info usb_module_info;

/* these are opaque handles to internal stack objects */
typedef uint32 usb_id;
typedef usb_id usb_device;	
typedef usb_id usb_interface;
typedef usb_id usb_pipe;

typedef struct usb_endpoint_info usb_endpoint_info;
typedef struct usb_interface_info usb_interface_info;
typedef struct usb_interface_list usb_interface_list;
typedef struct usb_configuration_info usb_configuration_info;

typedef struct usb_notify_hooks {
	status_t (*device_added)(usb_device device, void **cookie);
	status_t (*device_removed)(void *cookie);	
} usb_notify_hooks;

typedef struct usb_support_descriptor {
	uint8 dev_class;
	uint8 dev_subclass;
	uint8 dev_protocol;
	uint16 vendor;
	uint16 product;
} usb_support_descriptor;

/* ie, I support any hub device: 
**   usb_support_descriptor hub_devs = { 9, 0, 0, 0, 0 };
*/

struct usb_endpoint_info {
	usb_endpoint_descriptor *descr;       /* descriptor and handle         */
	usb_pipe handle;                        /* of this endpoint/pipe         */
};

struct usb_interface_info {
	usb_interface_descriptor *descr;      /* descriptor and handle         */
	usb_interface handle;                        /* of this interface             */
		
	size_t endpoint_count;                /* count and list of endpoints   */
	usb_endpoint_info *endpoint;	      /* in this interface             */	

	size_t generic_count;                 /* unparsed descriptors in this  */
	usb_descriptor **generic;             /* interface                     */
};

struct usb_interface_list {
	size_t alt_count;                     /* count and list of alternate   */
	usb_interface_info *alt;              /* interfaces available          */
		
	usb_interface_info *active;           /* currently active alternate    */
};

struct usb_configuration_info {
	usb_configuration_descriptor *descr;  /* descriptor of this config     */
		
	size_t interface_count;               /* interfaces in this config     */
	usb_interface_list *interface;
};

	     		
typedef void (*usb_callback_func)(void *cookie, uint32 status, 
								  void *data, uint32 actual_len);
				
struct usb_module_info {
	bus_manager_info	binfo;
	
	/* inform the bus manager of our intent to support a set of devices */
	status_t (*register_driver)(const char *driver_name,
								const usb_support_descriptor *descriptors, 
								size_t count,
								const char *optional_republish_driver_name);								

	/* request notification from the bus manager for add/remove of devices we
	   support */
	status_t (*install_notify)(const char *driver_name, 
							   const usb_notify_hooks *hooks);
	status_t (*uninstall_notify)(const char *driver_name);
	
	/* get the device descriptor */
	const usb_device_descriptor *(*get_device_descriptor)(usb_device device);
	
	/* get the nth supported configuration */	
	const usb_configuration_info *(*get_nth_configuration)(usb_device device, uint index);

	/* get the active configuration */
	const usb_configuration_info *(*get_configuration)(usb_device device);
	
	/* set the active configuration */	
	status_t (*set_configuration)(usb_device device,
								  const usb_configuration_info *configuration); 

	status_t (*set_alt_interface)(usb_device device,
								  const usb_interface_info *ifc);
	
	/* standard device requests -- convenience functions        */
	/* handle may be a usb_device, usb_pipe, or usb_interface   */
	status_t (*set_feature)(usb_id handle, uint16 selector);
	status_t (*clear_feature)(usb_id handle, uint16 selector);
	status_t (*get_status)(usb_id handle, uint16 *status);
	
	status_t (*get_descriptor)(usb_device device, 
							   uint8 type, uint8 index, uint16 lang,
							   void *data, size_t len, size_t *actual_len);

	/* generic device request function */	
	status_t (*send_request)(usb_device device, 
							 uint8 request_type, uint8 request,
							 uint16 value, uint16 index, uint16 length,
							 void *data, size_t *actual_len);

	/* async request queueing */
	status_t (*queue_interrupt)(usb_pipe pipe, 
								void *data, size_t len,
								usb_callback_func notify, void *cookie);
	
	status_t (*queue_bulk)(usb_pipe pipe, 
						   void *data, size_t len,
						   usb_callback_func notify, void *cookie);
								
	status_t (*queue_bulk_v)(usb_pipe pipe, 
						   iovec *vec, size_t count,
						   usb_callback_func notify, void *cookie);

	status_t (*queue_isochronous)(usb_pipe pipe, 
								  void *data, size_t len,
								  rlea* rle_array, uint16 buffer_duration_ms,
								  usb_callback_func notify, void *cookie);

	status_t (*queue_request)(usb_device device, 
							  uint8 request_type, uint8 request,
							  uint16 value, uint16 index, uint16 length,
							  void *data, usb_callback_func notify, void *cookie);
	
	status_t (*set_pipe_policy)(usb_pipe pipe, uint8 max_num_queued_packets,
								uint16 max_buffer_duration_ms, uint16 sample_size);
							 
	/* cancel pending async requests to an endpoint */
	status_t (*cancel_queued_transfers)(usb_pipe pipe); 
	
	/* tuning, timeouts, etc */
	status_t (*usb_ioctl)(uint32 opcode, void* buf, size_t buf_size);
};

/* status code for usb callback functions */
/*#define B_USB_STATUS_SUCCESS                      0x0000
#define B_USB_STATUS_DEVICE_CRC_ERROR             0x0002
#define B_USB_STATUS_DEVICE_TIMEOUT               0x0004
#define B_USB_STATUS_DEVICE_STALLED               0x0008
#define B_USB_STATUS_IRP_CANCELLED_BY_REQUEST     0x0010
#define B_USB_STATUS_DRIVER_INTERNAL_ERROR        0x0020
#define B_USB_STATUS_ADAPTER_HARDWARE_ERROR       0x0040
#define B_USB_STATUS_ISOCH_IRP_ABORTED            0x0080
*/
/* result codes for usb bus manager functions */
/*#define B_USBD_SUCCESS                 0
#define B_USBD_BAD_HANDLE              1
#define B_USBD_BAD_ARGS                2
#define B_USBD_NO_DATA                 3
#define B_USBD_DEVICE_FAILURE          4       
#define B_USBD_COMMAND_FAILED          5
#define B_USBD_PIPE_NOT_CONFIGURED     6
#define B_USBD_DEVICE_ERROR            7
#define B_USBD_PIPE_ERROR              8
#define B_USBD_NO_MEMORY		       9
*/
#define	B_USB_MODULE_NAME			"bus_managers/usb/v3"

#ifdef __cplusplus
}
#endif

#endif // _USB_V3_H
