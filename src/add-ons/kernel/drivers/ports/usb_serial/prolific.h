/*
 * Copyright (c) 2003-2004 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 * 
 */

#ifndef _USB_PROLIFIC_H_ 
  #define _USB_PROLIFIC_H_

#define VND_PROLIFIC   0x067b
#define VND_IODATA     0x04bb
#define VND_ATEN       0x0557
#define VND_TDK        0x04bf
#define VND_RATOC      0x0584
#define VND_ELECOM     0x056e
#define	VND_SOURCENEXT 0x0833
#define	VND_HAL        0x0b41

#define PROD_IODATA_USBRSAQ  0x0a03
#define PROD_PROLIFIC_RSAQ2  0x04bb
#define PROD_ATEN_UC232A     0x2008
#define PROD_PROLIFIC_PL2303 0x2303
#define PROD_TDK_UHA6400     0x0117
#define PROD_RATOC_REXUSB60  0xb000
#define PROD_ELECOM_UCSGT    0x5003
#define	PROD_SOURCENEXT_KEIKAI8     0x039f
#define	PROD_SOURCENEXT_KEIKAI8_CHG 0x012e
#define	PROD_HAL_IMR001             0x0011

// dobavit?!!!!!!!!!!!!!
//	{ 0, 0, 0, 0x0eba, 0x1080 },		// ITEGNO
//	{ 0, 0, 0, 0x0df7, 0x2620 }		// MA620 

#define PROLIFIC_SET_REQUEST 0x01

#define PROLIFIC_BUF_SIZE  0x100

status_t add_prolific_device(usb_serial_device *usd,
                             const usb_configuration_info *uci);
status_t reset_prolific_device(usb_serial_device *usd);

#endif //_USB_PROLIFIC_H_ 
