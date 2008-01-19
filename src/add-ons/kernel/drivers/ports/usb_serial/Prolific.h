/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003-2004 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_PROLIFIC_H_
#define _USB_PROLIFIC_H_

#include "ACM.h"

/* supported vendor and product ids */
#define VENDOR_PROLIFIC		0x067b
#define VENDOR_IODATA		0x04bb
#define VENDOR_ATEN			0x0557
#define VENDOR_TDK			0x04bf
#define VENDOR_RATOC		0x0584
#define VENDOR_ELECOM		0x056e
#define	VENDOR_SOURCENEXT	0x0833
#define	VENDOR_HAL			0x0b41

#define PRODUCT_IODATA_USBRSAQ			0x0a03
#define PRODUCT_PROLIFIC_RSAQ2			0x04bb
#define PRODUCT_ATEN_UC232A				0x2008
#define PRODUCT_PROLIFIC_PL2303			0x2303
#define PRODUCT_TDK_UHA6400				0x0117
#define PRODUCT_RATOC_REXUSB60			0xb000
#define PRODUCT_ELECOM_UCSGT			0x5003
#define	PRODUCT_SOURCENEXT_KEIKAI8		0x039f
#define	PRODUCT_SOURCENEXT_KEIKAI8_CHG	0x012e
#define	PRODUCT_HAL_IMR001				0x0011

/* protocol defines */
#define PROLIFIC_SET_REQUEST	0x01

#define PROLIFIC_BUF_SIZE		256

struct request_item;


class ProlificDevice : public ACMDevice {
public:
								ProlificDevice(usb_device device,
									uint16 vendorID, uint16 productID,
									const char *description);

virtual	status_t				AddDevice(const usb_configuration_info *config);

virtual	status_t				ResetDevice();

private:
		status_t				SendRequestList(request_item *list, size_t length);

		bool					fIsHX;
};

#endif //_USB_PROLIFIC_H_
