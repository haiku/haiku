/*
 * Copyright 2011, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef _USB_SILICON_H_
#define _USB_SILICON_H_


#include "SerialDevice.h"


/* supported vendor and product ids */
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

const usb_serial_device kSiliconDevices[] = {
	{VENDOR_RENESAS,	0x0053, "Renesas RX610 RX-Stick"},
	{VENDOR_AKATOM,		0x066A, "AKTAKOM ACE-1001"},
	{VENDOR_PIRELLI,	0xE000, "Pirelli DP-L10 GSM Mobile"},
	{VENDOR_PIRELLI,	0xE003, "Pirelli DP-L10 GSM Mobile"},
	{VENDOR_CYPHERLAB,	0x1000, "Cipherlab CCD Barcode Scanner"},
	{VENDOR_GEMALTO,	0x5501, "Gemalto contactless smartcard reader"},
	{VENDOR_DIGIANSWER,	0x000A, "Digianswer ZigBee MAC device"},
	{VENDOR_MEI,		0x1100, "MEI Acceptor"},
	{VENDOR_MEI,		0x1101, "MEI Acceptor"},
	{VENDOR_DYNASTREAM,	0x1003, "Dynastream ANT development board"},
	{VENDOR_DYNASTREAM,	0x1004, "Dynastream ANT development board"},
	{VENDOR_DYNASTREAM,	0x1006, "Dynastream ANT development board"},
	{VENDOR_KNOCKOFF,	0xAA26, "Knock-off DCU-11"},
	{VENDOR_SIEMENS,	0x10C5, "Siemens MC60"},
	{VENDOR_NOKIA,		0xAC70, "Nokia CA-42"},
	{VENDOR_BALTECH,	0x9999, "Balteck card reader"},
	{VENDOR_OWEN,		0x0004, "Owen AC4 USB-RS485 Converter"},
	{VENDOR_CLIPSAL,	0x0303, "Clipsal 5500PCU C-Bus USB interface"},
	{VENDOR_JABLOTRON,	0x0001, "Jablotron serial interface"},
	{VENDOR_WIENER,		0x0010, "W-IE-NE-R Plein & Baus GmbH device"},
	{VENDOR_WIENER,		0x0011, "W-IE-NE-R Plein & Baus GmbH device"},
	{VENDOR_WIENER,		0x0012, "W-IE-NE-R Plein & Baus GmbH device"},
	{VENDOR_WIENER,		0x0015, "W-IE-NE-R Plein & Baus GmbH device"},
	{VENDOR_WAVESENSE,	0xAAAA, "Wavesense Jazz blood glucose meter"},
	{VENDOR_VAISALA,	0x0200, "Vaisala USB instrument"},
	{VENDOR_ELV,		0xE00F, "ELV USB I²C interface"},
	{VENDOR_WAGO,		0x07A6, "WAGO 750-923 USB Service"},
	{VENDOR_DW700,		0x9500, "DW700 GPS USB interface"},
	{VENDOR_SILICON,	0x0F91, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x1101, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x1601, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x800A, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x803B, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8044, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x804E, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8053, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8054, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8066, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x806F, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x807A, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x80CA, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x80DD, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x80F6, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8115, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x813D, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x813F, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x814A, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x814B, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8156, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x815E, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x818B, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x819F, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x81A6, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x81AC, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x81AD, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x81C8, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x81E2, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x81E7, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x81E8, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x81F2, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8218, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x822B, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x826B, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8293, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x82F9, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8341, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8382, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x83A8, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x83D8, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8411, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8418, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x846E, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8477, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x85EA, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x85EB, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8664, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0x8665, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0xEA60, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0xEA61, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0xEA71, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0xF001, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0xF002, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0xF003, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON,	0xF004, "Silicon Labs CP210x USB UART converter"},
	{VENDOR_SILICON2,	0xEA61, "Silicon Labs GPRS USB Modem"},
	{VENDOR_SILICON3,	0xEA6A, "Silicon Labs GPRS USB Modem 100EU"}
};


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


#endif //_USB_SILICON_H_
