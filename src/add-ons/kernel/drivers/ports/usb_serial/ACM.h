/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_ACM_H_
#define _USB_ACM_H_

#include "SerialDevice.h"

class ACMDevice : public SerialDevice {
public:
								ACMDevice(usb_device device, uint16 vendorID,
									uint16 productID, const char *description);

virtual	status_t				AddDevice(const usb_configuration_info *config);

virtual	status_t				SetLineCoding(usb_cdc_line_coding *coding);
virtual	status_t				SetControlLineState(uint16 state);
};

#endif //_USB_ACM_H_
