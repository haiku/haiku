//------------------------------------------------------------------------------
//	Copyright (c) 2003, Niels S. Reedijk
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.

#ifndef UHCI_H
#define UHCI_H

/************************************************************
 * The Registers                                            *
 ************************************************************/


// R/W -- Read/Write
// R/WC -- Read/Write Clear
// ** -- Only writable with words!


// Registers
#define UHCI_USBCMD 0x0 		// USB Command - word - R/W
#define UHCI_USBSTS 0x2			// USB Status - word - R/WC
#define UHCI_USBINTR 0x4		// USB Interrupt Enable - word - R/W
#define UHCI_FRNUM 0x6			// Frame number - word - R/W**
#define UHCI_FRBASEADD 0x08		// Frame List BAse Address - dword - R/W
#define UHCI_SOFMOD 0xC			// Start of Frame Modify - byte - R/W
#define UHCI_PORTSC1 0x10		// Port 1 Status/Control - word - R/WC**
#define UHCI_PORTSC2 0x12		// Port 2 Status/Control - word - R/WC**

// USBCMD
#define UHCI_USBCMD_RS 0x1		// Run/Stop
#define UHCI_USBCMD_HCRESET 0x2 // Host Controller Reset
#define UHCI_USBCMD_GRESET 0x4 	// Global Reset
#define UHCI_USBCMD_EGSM 0x8	// Enter Global Suspensd mode
#define UHCI_USBCMD_FGR	0x10	// Force Global resume
#define UHCI_USBCMD_SWDBG 0x20	// Software Debug
#define UHCI_USBCMD_CF 0x40		// Configure Flag

//USBSTS
#define UHCI_USBSTS_USBINT 0x1	// USB interrupt
#define UHCI_USBSTS_ERRINT 0x2	// USB error interrupt
#define UHCI_USBSTS_RESDET 0x4	// Resume Detect
#define UHCI_USBSTS_HOSTERR 0x8	// Host System Error
#define UHCI_USBSTS_HCPRERR 0x10// Host Controller Process error
#define UHCI_USBSTS_HCHALT 0x20	// HCHalted

//USBINTR
#define UHCI_USBINTR_CRC 0x1	// Timeout/ CRC interrupt enable
#define UHCI_USBINTR_RESUME 0x2	// Resume interrupt enable
#define UHCI_USBINTR_IOC 0x4	// Interrupt on complete enable
#define UHCI_USBINTR_SHORT 0x8	// Short packet interrupt enable

//PORTSC
#define UHCI_PORTSC_CURSTAT 0x1	// Current connect status
#define UHCI_PORTSC_STATCHA 0x2 // Current connect status change
#define UHCI_PORTSC_ENABLED 0x4 // Port enabled/disabled

/************************************************************
 * Hardware structs                                         *
 ************************************************************/
 
//Represents a Transfer Descriptor (TD)

struct uhci_td
{
	void * link;			// Link to the next TD/QH
	uint32 status;			// Status field
	uint32 token;			// Contains the packet header (where it needs to be sent)
	void * buffer;			// A pointer to the buffer with the actual packet
};

//Represents a Queue Head (QH)

struct uhci_qh
{
	void * link;			//Link to the next TD/QH
	void * element;			//Link to the first element pointer in the queue
};

/************************************************************
 * Roothub Emulation                                        *
 ************************************************************/
#define RH_GET_DESCRIPTOR 6
#define RH_SET_ADDRESS 5
#define RH_SET_CONFIG 9

//Descriptors (in usb_request_data->Value)
#define RH_DEVICE_DESCRIPTOR ( 1 << 8 )
#define RH_CONFIG_DESCRIPTOR ( 2 << 8 )

#endif
