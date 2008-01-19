/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_SERIAL_DRIVER_H_
#define _USB_SERIAL_DRIVER_H_

#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>

#include "BeOSCompatibility.h"
#include "kernel_cpp.h"
#include "Tracing.h"
#include "USB3.h"

extern "C" {
#include <ttylayer.h>
}

#define DRIVER_NAME		"usb_serial"	// driver name for debug output
#define DEVICES_COUNT	20				// max simultaneously open devices

/* Some usefull helper defines ... */
#define SIZEOF(array) (sizeof(array) / sizeof(array[0])) /* size of array */
/* This one rounds the size to integral count of segs (segments) */
#define ROUNDUP(size, seg) (((size) + (seg) - 1) & ~((seg) - 1))
/* Default device buffer size */
#define DEF_BUFFER_SIZE	0x200

/* line coding defines ... Come from CDC USB specs? */
#define LC_STOP_BIT_1			0
#define LC_STOP_BIT_2			2

#define LC_PARITY_NONE			0
#define LC_PARITY_ODD			1
#define LC_PARITY_EVEN			2

/* struct that represents line coding */
typedef struct usb_serial_line_coding_s {
  uint32 speed;
  uint8 stopbits;
  uint8 parity;
  uint8 databits;
} usb_serial_line_coding;

/* control line states */
#define CLS_LINE_DTR			0x0001
#define CLS_LINE_RTS			0x0002

/* attributes etc ...*/
#ifndef USB_EP_ADDR_DIR_IN
#define USB_EP_ADDR_DIR_IN		0x80
#define USB_EP_ADDR_DIR_OUT		0x00
#endif

#ifndef USB_EP_ATTR_CONTROL
#define USB_EP_ATTR_CONTROL		0x00
#define USB_EP_ATTR_ISOCHRONOUS	0x01
#define USB_EP_ATTR_BULK		0x02
#define USB_EP_ATTR_INTERRUPT	0x03
#endif

/* USB class - communication devices */
#define USB_DEV_CLASS_COMM		0x02
#define USB_INT_CLASS_CDC		0x02
#define USB_INT_SUBCLASS_ACM	0x02
#define USB_INT_CLASS_CDC_DATA	0x0a
#define USB_INT_SUBCLASS_DATA	0x00

// communication device subtypes
#define FUNCTIONAL_SUBTYPE_UNION	0x06

extern usb_module_info *gUSBModule;
extern tty_module_info *gTTYModule;
extern struct ddomain gSerialDomain;

extern "C" {
status_t	usb_serial_device_added(usb_device device, void **cookie);
status_t	usb_serial_device_removed(void *cookie);

status_t	init_hardware();
void		uninit_driver();

bool		usb_serial_service(struct tty *ptty, struct ddrover *ddr, uint flags);

status_t	usb_serial_open(const char *name, uint32 flags, void **cookie);
status_t	usb_serial_read(void *cookie, off_t position, void *buffer, size_t *numBytes);
status_t	usb_serial_write(void *cookie, off_t position, const void *buffer, size_t *numBytes);
status_t	usb_serial_control(void *cookie, uint32 op, void *arg, size_t length);
status_t	usb_serial_select(void *cookie, uint8 event, uint32 ref, selectsync *sync);
status_t	usb_serial_deselect(void *coookie, uint8 event, selectsync *sync);
status_t	usb_serial_close(void *cookie);
status_t	usb_serial_free(void *cookie);

const char **publish_devices();
device_hooks *find_device(const char *name);
}

#endif //_USB_SERIAL_DRIVER_H_
