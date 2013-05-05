/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003-2004 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_KLSI_H_
#define _USB_KLSI_H_


#include "SerialDevice.h"


/* supported vendor and product ids */
#define VENDOR_PALM		0x0830
#define VENDOR_KLSI		0x05e9

const usb_serial_device kKLSIDevices[] = {
	{VENDOR_PALM, 0x0080, "PalmConnect RS232"},
	{VENDOR_KLSI, 0x00c0, "KLSI KL5KUSB105D"}
};


/* protocol defines */
#define KLSI_SET_REQUEST			0x01
#define KLSI_POLL_REQUEST			0x02
#define KLSI_CONF_REQUEST			0x03
#define KLSI_CONF_REQUEST_READ_ON	0x03
#define KLSI_CONF_REQUEST_READ_OFF	0x02

// not sure
#define KLSI_BUFFER_SIZE			64

enum {
	klsi_sio_b115200 = 0,
	klsi_sio_b57600 = 1,
	klsi_sio_b38400 = 2,
	klsi_sio_b19200 = 4,
	klsi_sio_b9600 = 6,
	/* unchecked */
	klsi_sio_b4800 = 8,
	klsi_sio_b2400 = 9,
	klsi_sio_b1200 = 10,
	klsi_sio_b600 = 11,
	klsi_sio_b300 = 12
};


class KLSIDevice : public SerialDevice {
public:
								KLSIDevice(usb_device device, uint16 vendorID,
									uint16 productID, const char *description);

virtual	status_t				AddDevice(const usb_configuration_info *config);

virtual	status_t				ResetDevice();

virtual	status_t				SetLineCoding(usb_cdc_line_coding *coding);

virtual	void					OnRead(char **buffer, size_t *numBytes);
virtual	void					OnWrite(const char *buffer, size_t *numBytes,
									size_t *packetBytes);
virtual	void					OnClose();
};


#endif //_USB_KLSI_H_
