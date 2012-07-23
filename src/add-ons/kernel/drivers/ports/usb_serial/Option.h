/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef _USB_OPTION_H_
#define _USB_OPTION_H_


#include "ACM.h"


/* supported vendor and product ids */
#define VENDOR_AIRPLUS		0x1011
#define VENDOR_ALCATEL		0x1bbb
#define VENDOR_ALINK		0x1e0e
#define VENDOR_AMOI			0x1614
#define VENDOR_ANYDATA		0x16d5
#define VENDOR_AXESSTEL		0x1726
#define VENDOR_BANDRICH		0x1A8D
#define VENDOR_BENQ			0x04a5
#define VENDOR_CELOT		0x211f
#define VENDOR_CMOTECH		0x16d8
#define VENDOR_DELL			0x413C
#define VENDOR_DLINK		0x1186
#define VENDOR_HAIER		0x201e
#define VENDOR_HUAWEI		0x12D1
#define VENDOR_KYOCERA		0x0c88
#define VENDOR_LG			0x1004
#define VENDOR_LONGCHEER	0x1c9e
#define VENDOR_MEDIATEK		0x0e8d
#define VENDOR_NOVATEL		0x1410
#define VENDOR_OLIVETTI		0x0b3c
#define VENDOR_ONDA			0x1ee8
#define VENDOR_OPTION		0x0AF0
#define VENDOR_QISDA		0x1da5
#define VENDOR_QUALCOMM		0x05C6
#define VENDOR_SAMSUNG		0x04e8
#define VENDOR_TELIT		0x1bc7
#define VENDOR_TLAYTECH		0x20B9
#define VENDOR_TOSHIBA		0x0930
#define VENDOR_VIETTEL		0x2262
#define VENDOR_YISO			0x0EAB
#define VENDOR_YUGA			0x257A
#define VENDOR_ZD			0x0685
#define VENDOR_ZTE			0x19d2

const usb_serial_device kOptionDevices[] = {
	{VENDOR_CMOTECH, 0x6008, "CMOTECH CDMA Modem"},
	{VENDOR_CMOTECH, 0x5553, "CMOTECH CDU550"},
	{VENDOR_CMOTECH, 0x6512, "CMOTECH CDX650"}
};


class OptionDevice : public ACMDevice {
public:
							OptionDevice(usb_device device,
								uint16 vendorID, uint16 productID,
								const char *description);

	virtual	status_t		AddDevice(const usb_configuration_info *config);
	virtual	status_t		ResetDevice();
};


#endif /*_USB_OPTION_H_ */
