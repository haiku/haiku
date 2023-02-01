/*
 * Copyright 2022, Gerasim Troeglazov <3dEyes@gmail.com>
 * Distributed under the terms of the MIT License.
 */

#ifndef _USB_WINCHIPHEAD_H_
#define _USB_WINCHIPHEAD_H_

#include "SerialDevice.h"

#define CH34X_DEFAULT_BAUD_RATE	9600

#define CH34X_INPUT_BUF_SIZE	8

#define CH34X_VER_20			0x20
#define CH34X_VER_30			0x30

#define CH34X_BPS_PRE_IMM		0x80

#define CH34X_BIT_CTS			0x01
#define CH34X_BIT_DSR			0x02
#define CH34X_BIT_RI			0x04
#define CH34X_BIT_DCD			0x08
#define CH34X_BIT_DTR			0x20
#define CH34X_BIT_RTS			0x40
#define CH34X_BITS_MODEM_STAT	0x0f

#define CH34X_MULT_STAT 		0x04

#define CH34X_REQ_READ_VERSION	0x5F
#define CH34X_REQ_WRITE_REG		0x9A
#define CH34X_REQ_READ_REG		0x95
#define CH34X_REQ_SERIAL_INIT	0xA1
#define CH34X_REQ_MODEM_CTRL	0xA4

#define CH34X_REG_BREAK			0x05
#define CH34X_REG_STAT1			0x06
#define CH34X_REG_STAT2			0x07
#define CH34X_REG_BPS_PRE		0x12
#define CH34X_REG_BPS_DIV		0x13
#define CH34X_REG_LCR			0x18
#define CH34X_REG_LCR2			0x25

#define CH34X_LCR_CS5			0x00
#define CH34X_LCR_CS6			0x01
#define CH34X_LCR_CS7			0x02
#define CH34X_LCR_CS8			0x03
#define CH34X_LCR_STOP_BITS_2	0x04
#define CH34X_LCR_ENABLE_PAR	0x08
#define CH34X_LCR_PAR_EVEN		0x10
#define CH34X_LCR_MARK_SPACE	0x20
#define CH34X_LCR_ENABLE_TX		0x40
#define CH34X_LCR_ENABLE_RX		0x80

#define CH34X_SIO_600			0x6401
#define CH34X_SIO_1200			0xB201
#define CH34X_SIO_1800			0xCC01
#define CH34X_SIO_2400			0xD901
#define CH34X_SIO_4800			0x6402
#define CH34X_SIO_9600			0xB202
#define CH34X_SIO_19200			0xD902
#define CH34X_SIO_31250			0x4003
#define CH34X_SIO_38400			0x6403
#define CH34X_SIO_57600			0xF302
#define CH34X_SIO_115200		0xCC03
#define CH34X_SIO_230400		0xE603

/* supported vendor and product ids */
#define VENDOR_WCH				0x4348	// WinChipHead
#define VENDOR_QIN_HENG			0x1a86	// QinHeng Electronics
#define VENDOR_GW_INSTEK		0x2184	// GW Instek

const usb_serial_device kWCHDevices[] = {
	{VENDOR_WCH,		0x5523, "CH341 serial converter"},
	{VENDOR_QIN_HENG,	0x5523, "QinHeng CH341A serial converter"},
	{VENDOR_QIN_HENG,	0x7522, "QinHeng CH340 serial converter"},
	{VENDOR_QIN_HENG,	0x7523, "QinHeng CH340 serial converter"},
	{VENDOR_GW_INSTEK,	0x0057, "GW Instek Oscilloscope"}
};

class WCHDevice : public SerialDevice {
public:
						WCHDevice(usb_device device, uint16 vendorID,
							uint16 productID, const char *description);

	virtual	status_t	AddDevice(const usb_configuration_info *config);

	virtual	status_t	ResetDevice();

	virtual	status_t	SetLineCoding(usb_cdc_line_coding *coding);
	virtual	status_t	SetControlLineState(uint16 state);

private:
	status_t			WriteConfig(uint16 dataRate, uint8 lcr, uint8 mcr);

	uint8				fChipVersion;
	uint8				fStatusMCR;
	uint8				fStatusLCR;
	uint16				fDataRate;
};


#endif //_USB_WINCHIPHEAD_H_
