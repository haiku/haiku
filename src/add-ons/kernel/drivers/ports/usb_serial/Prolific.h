/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003-2004 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef _USB_PROLIFIC_H_
#define _USB_PROLIFIC_H_


#include "ACM.h"


/* supported vendor and product ids */
#define VENDOR_PROLIFIC		0x067b
#define VENDOR_ATEN			0x0557
#define VENDOR_ELECOM		0x056e
#define VENDOR_HAL			0x0b41
#define VENDOR_IODATA		0x04bb
#define VENDOR_RATOC		0x0584
#define VENDOR_SOURCENEXT	0x0833
#define VENDOR_TDK			0x04bf

const usb_serial_device kProlificDevices[] = {
	{VENDOR_PROLIFIC,	0x04bb, "PL2303 Serial adapter (IODATA USB-RSAQ2)"},
	{VENDOR_PROLIFIC,	0x2303, "PL2303 Serial adapter (ATEN/IOGEAR UC232A)"},
	{VENDOR_ATEN,		0x2008, "Aten Serial adapter"},
	{VENDOR_ELECOM,		0x5003, "Elecom UC-SGT"},
	{VENDOR_HAL,		0x0011, "HAL Corporation Crossam2+USB"},
	{VENDOR_IODATA,		0x0a03, "I/O Data USB serial adapter USB-RSAQ1"},
	{VENDOR_RATOC,		0xb000, "Ratoc USB serial adapter REX-USB60"},
	{VENDOR_SOURCENEXT,	0x039f, "SOURCENEXT KeikaiDenwa 8"},
	{VENDOR_SOURCENEXT,	0x039f, "SOURCENEXT KeikaiDenwa 8 with charger"},
	{VENDOR_TDK,		0x0117, "TDK USB-PHS Adapter UHA6400"}
};


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
