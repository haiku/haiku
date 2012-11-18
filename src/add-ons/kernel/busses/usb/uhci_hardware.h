/*
 * Copyright 2004-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#ifndef UHCI_HARDWARE_H
#define UHCI_HARDWARE_H

/************************************************************
 * The Registers                                            *
 ************************************************************/

// R/W -- Read/Write
// R/WC -- Read/Write Clear
// ** -- Only writable with words!

// PCI register
#define PCI_LEGSUP				0xC0
#define PCI_LEGSUP_USBPIRQDEN	0x2000
#define PCI_LEGSUP_CLEAR_SMI	0x8f00

// Registers
#define UHCI_USBCMD				0x00 	// USB Command - word - R/W
#define UHCI_USBSTS				0x02	// USB Status - word - R/WC
#define UHCI_USBINTR			0x04	// USB Interrupt Enable - word - R/W
#define UHCI_FRNUM				0x06	// Frame number - word - R/W**
#define UHCI_FRBASEADD			0x08	// Frame List BAse Address - dword - R/W
#define UHCI_SOFMOD				0x0c	// Start of Frame Modify - byte - R/W
#define UHCI_PORTSC1			0x10	// Port 1 Status/Control - word - R/WC**
#define UHCI_PORTSC2			0x12	// Port 2 Status/Control - word - R/WC**

// USBCMD
#define UHCI_USBCMD_RS			0x01	// Run/Stop
#define UHCI_USBCMD_HCRESET		0x02 	// Host Controller Reset
#define UHCI_USBCMD_GRESET		0x04 	// Global Reset
#define UHCI_USBCMD_EGSM		0x08	// Enter Global Suspensd mode
#define UHCI_USBCMD_FGR			0x10	// Force Global resume
#define UHCI_USBCMD_SWDBG		0x20	// Software Debug
#define UHCI_USBCMD_CF			0x40	// Configure Flag
#define UHCI_USBCMD_MAXP		0x80	// Max packet

//USBSTS
#define UHCI_USBSTS_USBINT		0x01	// USB interrupt
#define UHCI_USBSTS_ERRINT		0x02	// USB error interrupt
#define UHCI_USBSTS_RESDET		0x04	// Resume Detect
#define UHCI_USBSTS_HOSTERR		0x08	// Host System Error
#define UHCI_USBSTS_HCPRERR		0x10	// Host Controller Process error
#define UHCI_USBSTS_HCHALT		0x20	// HCHalted

//USBINTR
#define UHCI_USBINTR_CRC		0x01	// Timeout/ CRC interrupt enable
#define UHCI_USBINTR_RESUME		0x02	// Resume interrupt enable
#define UHCI_USBINTR_IOC		0x04	// Interrupt on complete enable
#define UHCI_USBINTR_SHORT		0x08	// Short packet interrupt enable

//PORTSC
#define UHCI_PORTSC_CURSTAT		0x0001	// Current connect status
#define UHCI_PORTSC_STATCHA		0x0002	// Current connect status change
#define UHCI_PORTSC_ENABLED		0x0004	// Port enabled/disabled
#define UHCI_PORTSC_ENABCHA		0x0008	// Change in enabled/disabled
#define UHCI_PORTSC_LINE_0		0x0010	// The status of D+
#define UHCI_PORTSC_LINE_1		0x0020	// The status of D-
#define UHCI_PORTSC_RESUME		0x0040	// Something with the suspend state ???
#define UHCI_PORTSC_LOWSPEED	0x0100	// Low speed device attached?
#define UHCI_PORTSC_RESET		0x0200	// Port is in reset
#define UHCI_PORTSC_SUSPEND		0x1000	// Set port in suspend state

#define UHCI_PORTSC_DATAMASK	0x13f5	// Mask that excludes the change bits

/************************************************************
 * Hardware structs                                         *
 ************************************************************/

// Framelist flags
#define FRAMELIST_TERMINATE    0x1
#define FRAMELIST_NEXT_IS_QH   0x2

// Number of frames
#define NUMBER_OF_FRAMES		1024
#define MAX_AVAILABLE_BANDWIDTH	900	// Microseconds

// Represents a Transfer Descriptor (TD)
typedef struct
{
	// Hardware part
	uint32	link_phy;		// Link to the next TD/QH
	uint32	status;			// Status field
	uint32	token;			// Contains the packet header (where it needs to be sent)
	uint32	buffer_phy;		// A pointer to the buffer with the actual packet
	// Software part
	uint32	this_phy;		// A physical pointer to this address
	void	*link_log;		// Pointer to the next logical TD/QT
	void	*buffer_log;	// Pointer to the logical buffer
	size_t	buffer_size;	// Size of the buffer
} uhci_td;

#define	TD_NEXT_IS_QH				0x02

// Control and Status
#define TD_CONTROL_SPD				(1 << 29)
#define TD_CONTROL_3_ERRORS			(3 << 27)
#define TD_CONTROL_LOWSPEED			(1 << 26)
#define TD_CONTROL_ISOCHRONOUS		(1 << 25)
#define TD_CONTROL_IOC				(1 << 24)

#define TD_STATUS_ACTIVE			(1 << 23)
#define TD_STATUS_ERROR_STALLED		(1 << 22)
#define TD_STATUS_ERROR_BUFFER		(1 << 21)
#define TD_STATUS_ERROR_BABBLE		(1 << 20)
#define TD_STATUS_ERROR_NAK			(1 << 19)
#define TD_STATUS_ERROR_CRC			(1 << 18)
#define TD_STATUS_ERROR_TIMEOUT		(1 << 18)
#define TD_STATUS_ERROR_BITSTUFF	(1 << 17)

#define TD_STATUS_ACTLEN_MASK		0x07ff
#define TD_STATUS_ACTLEN_NULL		0x07ff

// Token
#define TD_TOKEN_MAXLEN_SHIFT		21
#define TD_TOKEN_NULL_DATA			(0x07ff << TD_TOKEN_MAXLEN_SHIFT)
#define TD_TOKEN_DATA_TOGGLE_SHIFT	19
#define TD_TOKEN_DATA1				(1 << TD_TOKEN_DATA_TOGGLE_SHIFT)

#define TD_TOKEN_SETUP				0x2d
#define TD_TOKEN_IN					0x69
#define TD_TOKEN_OUT				0xe1

#define TD_TOKEN_ENDPTADDR_SHIFT	15
#define TD_TOKEN_DEVADDR_SHIFT		8

#define TD_DEPTH_FIRST				0x04
#define TD_TERMINATE				0x01
#define TD_ERROR_MASK				0x440000
#define TD_ERROR_COUNT_SHIFT		27
#define TD_ERROR_COUNT_MASK			0x03
#define TD_LINK_MASK				0xfffffff0


static inline size_t
uhci_td_maximum_length(uhci_td *descriptor)
{
	size_t length = (descriptor->token >> TD_TOKEN_MAXLEN_SHIFT) + 1;
	if (length == TD_STATUS_ACTLEN_NULL + 1)
		return 0;
	return length;
}


static inline size_t
uhci_td_actual_length(uhci_td *descriptor)
{
	size_t length = (descriptor->status & TD_STATUS_ACTLEN_MASK) + 1;
	if (length == TD_STATUS_ACTLEN_NULL + 1)
		return 0;
	return length;
}


// Represents a Queue Head (QH)
typedef struct
{
	// Hardware part
	uint32	link_phy;		// Link to the next TD/QH
	uint32	element_phy;	// Pointer to the first element in the queue
	// Software part
	uint32	this_phy;		// The physical pointer to this address
	void	*link_log;		// Pointer to the next logical TD/QH
} uhci_qh;

#define QH_TERMINATE			0x01
#define QH_NEXT_IS_QH  			0x02
#define QH_LINK_MASK			0xfffffff0

#endif
