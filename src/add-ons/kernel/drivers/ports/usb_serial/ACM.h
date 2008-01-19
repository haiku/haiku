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

/* requests for CDC ACM devices */
#define SEND_ENCAPSULATED_COMMAND	0x00
#define GET_ENCAPSULATED_RESPONSE	0x01
#define SET_LINE_CODING				0x20
#define SET_CONTROL_LINE_STATE		0x22

/* notifications for CDC ACM devices */
#define NETWORK_CONNECTION			0x00
#define RESPONSE_AVAILABLE			0x01

class ACMDevice : public SerialDevice {
public:
								ACMDevice(usb_device device, uint16 vendorID,
									uint16 productID, const char *description);

virtual	status_t				AddDevice(const usb_configuration_info *config);

virtual	status_t				SetLineCoding(usb_serial_line_coding *coding);
virtual	status_t				SetControlLineState(uint16 state);

virtual	void					OnWrite(const char *buffer, size_t *numBytes);
};

#endif //_USB_ACM_H_
