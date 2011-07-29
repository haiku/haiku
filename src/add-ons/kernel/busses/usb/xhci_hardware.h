/*
 * Copyright 2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jian Chiang <j.jian.chiang@gmail.com>
 */
#ifndef XHCI_HARDWARE_H
#define XHCI_HARDWARE_H


// Host Controller Capability Registers
#define XHCI_CAPLENGTH		0x00		// Capability Register Length
#define XHCI_HCIVERSION		0x02		// Interface Version Number
#define XHCI_HCSPARAMS1		0x04		// Structural Parameters 1
// HCSPARAMS1
#define HCS_MAX_SLOTS(p)        (((p) >> 0) & 0xff)
#define HCS_MAX_PORTS(p)        (((p) >> 24) & 0x7f)
#define XHCI_HCSPARAMS2		0x08		// Structural Parameters 2
#define XHCI_HCSPARAMS3		0x0C		// Structural Parameters 3
#define XHCI_HCCPARAMS		0x10		// Capability Parameters
#define XHCI_DBOFF			0x14		// Doorbell Register offset
#define XHCI_RTSOFF			0x18		// Runtime Register Space offset


// Host Controller Operational Registers
#define XHCI_CMD			0x00		// USB Command
// USB Command Register
#define CMD_RUN				(1 << 0)
#define CMD_HCRST			(1 << 1)	// Host Controller Reset
#define CMD_EIE				(1 << 2)
#define CMD_HSEIE			(1 << 3)

#define XHCI_STS			0x04		// USB Status
// USB Status Register
#define STS_HCH				(1 << 0)
#define STS_HSE				(1 << 2)
#define STS_PCD				(1 << 4)
#define STS_CNR				(1<<11)
#define STS_HCE				(1 << 12)
#define XHCI_PAGESIZE		0x08		// PAGE SIZE
// Section 5.4.5
#define XHCI_CRCR_LO		0x18
#define XHCI_CRCR_HI		0x1C
#define CRCR_RCS		(1<<0)
// Section 5.4.6
#define XHCI_DCBAAP_LO		0x30
#define XHCI_DCBAAP_HI		0x34
// Section 5.4.7
#define XHCI_CONFIG			0x38


// Host Controller Runtime Registers
// Section 5.5.2.1
#define XHCI_IMAN(n)		(0x0020 + (0x20 * (n)))
// IMAN
#define IMAN_INTR_ENA		0x00000002
// Section 5.5.2.2
#define XHCI_IMOD(n)		(0x0024 + (0x20 * (n)))
// Section 5.5.2.3.1
#define XHCI_ERSTSZ(n)		(0x0028 + (0x20 * (n)))
// ERSTSZ
#define XHCI_ERSTS_SET(x)	((x) & 0xFFFF)
// Section 5.5.2.3.2
#define XHCI_ERSTBA_LO(n)	(0x0030 + (0x20 * (n)))
#define XHCI_ERSTBA_HI(n)	(0x0034 + (0x20 * (n)))
// Section 5.5.2.3.3
#define XHCI_ERDP_LO(n)		(0x0038 + (0x20 * (n)))
#define XHCI_ERDP_HI(n)		(0x003C + (0x20 * (n)))
// Event Handler Busy (EHB)
#define ERST_EHB			(1 << 3)


// Host Controller Doorbell Registers
#define XHCI_DOORBELL(n)        (0x0000 + (4 * (n)))

// Extended Capabilities
#define XECP_ID(x)				((x) & 0xff)
#define HCS0_XECP(x)			(((x) >> 16) & 0xffff)
#define XECP_NEXT(x)			(((x) >> 8) & 0xff)
#define XHCI_LEGSUP_CAPID		0x01
#define XHCI_LEGSUP_OSOWNED		(1 << 24)	// OS Owned Semaphore
#define XHCI_LEGSUP_BIOSOWNED	(1 << 16)	// BIOS Owned Semaphore

#define XHCI_LEGCTLSTS			0x04
#define XHCI_LEGCTLSTS_DISABLE_SMI	((0x3 << 1) + (0xff << 5) + (0x7 << 17))


// Port status Registers
// Section 5.4.8
#define XHCI_PORTSC(n)			(0x3F0 + (0x10 * (n)))
#define PS_CCS					(1 << 0)
#define PS_PED					(1 << 1)
#define PS_OCA					(1 << 3)
#define PS_PR					(1 << 4)
#define PS_PP					(1 << 9)
#define PS_SPEED_GET(x)			(((x) >> 10) & 0xF)
#define PS_LWS					(1 << 16)
#define PS_CSC					(1 << 17)
#define PS_PEC					(1 << 18)
#define PS_WRC					(1 << 19)
#define PS_OCC					(1 << 20)
#define PS_PRC					(1 << 21)
#define PS_PLC					(1 << 22)
#define PS_CEC					(1 << 23)
#define PS_CAS					(1 << 24)
#define PS_WCE					(1 << 25)
#define PS_WDE					(1 << 26)
#define PS_WPR					(1 << 30)

#define PS_CLEAR				0x80FF00F7U

#define PS_PLS_MASK				(0xf << 5)
#define PS_XDEV_U0				(0x0 << 5)
#define PS_XDEV_U3				(0x3 << 5)


// Completion Code
#define COMP_CODE_GET(x)		(((x) >> 24) & 0xff)
#define COMP_SUCCESS			0x01


// TRB Type
#define TRB_TYPE(x)				((x) << 10)
#define TRB_TYPE_GET(x)			(((x) >> 10) & 0x3F)
#define TRB_LINK				6
#define TRB_TR_NOOP				8
#define TRB_TRANSFER			32
#define TRB_COMPLETION			33
#define TRB_3_CYCLE_BIT			(1U << 0)
#define TRB_3_TC_BIT			(1U << 1)

#endif // !XHCI_HARDWARE_H
