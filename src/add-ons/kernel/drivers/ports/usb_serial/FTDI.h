/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef _USB_FTDI_H_
#define _USB_FTDI_H_


#include "SerialDevice.h"


/* supported vendor and product ids */
#define VENDOR_FTDI		0x0403

const usb_serial_device kFTDIDevices[] = {
	{VENDOR_FTDI, 0x8372, "FTDI 8U100AX serial converter"},
	{VENDOR_FTDI, 0x6001, "FTDI 8U232AM serial converter"},
	{VENDOR_FTDI, 0x6006, "FTDI 8U232AM serial converter"},
	{VENDOR_FTDI, 0x6010, "FTDI 8U2232C serial converter"},
	{VENDOR_FTDI, 0x6011, "FTDI 4232H serial converter"},
	{VENDOR_FTDI, 0x6014, "FTDI 232H serial converter"},
	{VENDOR_FTDI, 0x6015, "FTDI FT231X serial converter"},
	{VENDOR_FTDI, 0x6040, "FTDI FT2233 serial converter"},
	{VENDOR_FTDI, 0x6041, "FTDI FT4233 serial converter"},
	{VENDOR_FTDI, 0x6042, "FTDI FT2232 serial converter"},
	{VENDOR_FTDI, 0x6043, "FTDI FT4232 serial converter"},
	{VENDOR_FTDI, 0x6044, "FTDI FT233 serial converter"},
	{VENDOR_FTDI, 0x6045, "FTDI FT232 serial converter"},
	{VENDOR_FTDI, 0x6048, "FTDI FT4232 serial converter"},
};

#define FTDI_BUFFER_SIZE		64


class FTDIDevice : public SerialDevice {
public:
								FTDIDevice(usb_device device, uint16 vendorID,
									uint16 productID, const char *description);

virtual	status_t				AddDevice(const usb_configuration_info *config);

virtual	status_t				ResetDevice();

virtual	status_t				SetLineCoding(usb_cdc_line_coding *coding);
virtual	status_t				SetControlLineState(uint16 state);
virtual	status_t				SetHardwareFlowControl(bool enable);

virtual	void					OnRead(char **buffer, size_t *numBytes);
virtual	void					OnWrite(const char *buffer, size_t *numBytes,
									size_t *packetBytes);

private:
		size_t					fHeaderLength;
		uint8					fStatusMSR;
		uint8					fStatusLSR;
};


#endif //_USB_FTDI_H_
