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
#define XHCI_HCSPARAMS2		0x08		// Structural Parameters 2
#define XHCI_HCSPARAMS3		0x0C		// Structural Parameters 3
#define XHCI_HCCPARAMS		0x10		// Capability Parameters
#define XHCI_RTSOFF			0x18		// Runtime Register Space offset


// Host Controller Operational Registers
#define XHCI_CMD			0x00		// USB Command
#define XHCI_STS			0x04		// USB Status


// USB Command Register
#define CMD_HCRST			(1 << 1)	// Host Controller Reset


// USB Status Register
#define STS_HCH				(1<<0)
#define STS_CNR				(1<<11)





// Extended Capabilities
#define XHCI_LEGSUP_CAPID_MASK	0xff
#define XHCI_LEGSUP_CAPID		0x01
#define XHCI_LEGSUP_OSOWNED		(1 << 24)	// OS Owned Semaphore
#define XHCI_LEGSUP_BIOSOWNED	(1 << 16)	// BIOS Owned Semaphore

#define XHCI_LEGCTLSTS			0x04
#define XHCI_LEGCTLSTS_DISABLE_SMI	((0x3 << 1) + (0xff << 5) + (0x7 << 17))


#endif // !XHCI_HARDWARE_H

