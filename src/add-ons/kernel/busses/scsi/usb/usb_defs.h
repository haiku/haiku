/*
 * Copyright (c) 2003-2005 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the BSD License.
 * 
 */

/** definitions that should be in system headers but ... */

#ifndef _USB_DEFS_H_ 
  #define _USB_DEFS_H_

#define USB_EP_ATTR_CONTROL      0x00
#define USB_EP_ATTR_ISOCHRONOUS  0x01
#define USB_EP_ATTR_BULK         0x02 
#define USB_EP_ATTR_INTERRUPT    0x03 
#define USB_EP_ATTR_MASK         0x03 

#define USB_EP_ADDR_DIR_IN   0x80
#define USB_EP_ADDR_DIR_OUT  0x00

/*USB device class/subclass/protocl definitions*/
#define USB_DEV_CLASS_MASS        0x08

#define USB_DEV_SUBCLASS_RBC      0x01
#define USB_DEV_SUBCLASS_SFF8020I 0x02
#define USB_DEV_SUBCLASS_QIC157   0x03
#define USB_DEV_SUBCLASS_UFI      0x04
#define USB_DEV_SUBCLASS_SFF8070I 0x05
#define USB_DEV_SUBCLASS_SCSI     0x06

#define USB_DEV_PROTOCOL_CBI    0x00
#define USB_DEV_PROTOCOL_CB 	0x01
#define USB_DEV_PROTOCOL_BULK	0x50

#define B_DEV_STALLED 0x8000a015 /* some "forgotten" error */

#endif /*_USB_DEFS_H_*/ 
