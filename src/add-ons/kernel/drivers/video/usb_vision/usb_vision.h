/*
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#ifndef _USBVISION_DRIVER_H_ 
  #define _USBVISION_DRIVER_H_
  
#define DRIVER_NAME "usb_vision"  

#define DEVICES_COUNT 0x1f
 
#include "nt100x.h"
#include "tracing.h"

#define SIZEOF(array) (sizeof(array)/sizeof(array[0]))

/* "forgotten" attributes etc ...*/
#define USB_EP_ADDR_DIR_IN   0x80
#define USB_EP_ADDR_DIR_OUT  0x00

#define USB_EP_ATTR_CONTROL      0x00
#define USB_EP_ATTR_ISOCHRONOUS  0x01
#define USB_EP_ATTR_BULK         0x02 
#define USB_EP_ATTR_INTERRUPT    0x03 


typedef struct{
  uint32 open_count;
  const usb_device *dev;
  usb_pipe *control_pipe;
  usb_pipe *data_pipe;
} usb_vision_device;

#endif//_USB_SERIAL_DRIVER_H_ 
