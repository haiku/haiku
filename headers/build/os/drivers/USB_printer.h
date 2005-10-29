/* 
** USB_printer.h
**
** Copyright 1999, Be Incorporated. All Rights Reserved.
**
*/

#ifndef _USB_PRINTER_H
#define _USB_PRINTER_H

#include <Drivers.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ioctl() opcodes for usb_printer driver */

enum
{
	USB_PRINTER_GET_DEVICE_ID = B_DEVICE_OP_CODES_END+1
};


/* Maximum length of the DEVICE_ID. User MUST allocate this size
when calling USB_PRINTER_GET_DEVICE_ID ioctl() */

#define USB_PRINTER_DEVICE_ID_LENGTH	256



#ifdef __cplusplus
}
#endif

#endif
