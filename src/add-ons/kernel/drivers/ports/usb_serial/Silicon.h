/*
 * Copyright 2011, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_SILICON_H_
#define _USB_SILICON_H_

#include "SerialDevice.h"

class SiliconDevice : public SerialDevice {
public:
					SiliconDevice(usb_device device, uint16 vendorID,
						uint16 productID, const char *description);

virtual	status_t	AddDevice(const usb_configuration_info *config);

virtual	status_t	ResetDevice();

virtual	status_t	SetLineCoding(usb_cdc_line_coding *coding);
virtual	status_t	SetControlLineState(uint16 state);

private:
enum CP210XRequest {
	ENABLE_UART = 0,
		/* 1 to enable the UART function, 0 to disable
		 * (some Silicon Labs chips have other functions such as GPIOs) */


	SET_BAUDRATE_DIVIDER = 1,
	GET_BAUDRATE_DIVIDER = 2,
		/*
		 Baudrate base clock is 3686400

		 3686400 / 32 = 115200
		 ...
		 3686400 / 384 = 9600
		*/ 

	SET_LINE_FORMAT = 3,
	GET_LINE_FORMAT = 4,
		/*
		 DataBits << 0x100 | Parity << 0x10 | StopBits

		 Databits in [5,9]
		 Parity :
		 	0 = none
			1 = odd
			2 = even
			3 = mark
			4 = space
		Stop bits:
			0 = 1 stop bit
			1 = 1.5 stop bits
			2 = 2 stop bits
		*/

	SET_BREAK = 5,
		/* 1 to enable, 0 to disable */

	IMMEDIATE_CHAR = 6,

	SET_STATUS = 7,
	GET_STATUS = 8,
		/*
		 bit 0 = DTR
		 bit 1 = RTS

		 bit 4 = CTS
		 bit 5 = DSR
		 bit 6 = RING
		 bit 7 = DCD
		 bit 8 = WRITE_DTR (unset to not touch DTR)
		 bit 9 = WRITE_RTS (unset to not touch RTS)
		*/

	SET_XON = 9,
	SET_XOFF = 10,
	SET_EVENTMASK = 11,
	GET_EVENTMASK = 12,
	SET_CHAR = 13,
	GET_CHARS = 14,
	GET_PROPS = 15,
	GET_COMM_STATUS = 16,
	RESET = 17,
	PURGE = 18,

	SET_FLOW = 19, 
	GET_FLOW = 20,
		/* Hardware flow control setup */

	EMBED_EVENTS = 21,
	GET_EVENTSTATE = 22,
	SET_CHARS = 0x19
};

private:
status_t 			WriteConfig(CP210XRequest request, uint16_t* data,
						size_t size);
};

#define VENDOR_RENESAS		0x045B
#define VENDOR_AKATOM		0x0471
#define VENDOR_PIRELLI		0x0489
#define VENDOR_CYPHERLAB	0x0745
#define VENDOR_GEMALTO		0x08E6
#define VENDOR_DIGIANSWER	0x08FD
#define VENDOR_MEI			0x0BED
#define VENDOR_DYNASTREAM	0x0FCF
#define VENDOR_KNOCKOFF		0x10A6
#define VENDOR_SIEMENS		0x10AB
#define VENDOR_NOKIA		0x10B5
#define VENDOR_SILICON		0x10C4
#define VENDOR_SILICON2		0x10C5
#define VENDOR_SILICON3		0x10CE
#define VENDOR_BALTECH		0x13AD
#define VENDOR_OWEN			0x1555
#define VENDOR_CLIPSAL		0x166A
#define VENDOR_JABLOTRON	0x16D6
#define VENDOR_WIENER		0x16DC
#define VENDOR_WAVESENSE	0x17F4
#define VENDOR_VAISALA		0x1843
#define VENDOR_ELV			0x18EF
#define VENDOR_WAGO			0x1BE3
#define VENDOR_DW700		0x413C


#endif //_USB_SILICON_H_
