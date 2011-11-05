/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#ifndef EHCI_HARDWARE_H
#define EHCI_HARDWARE_H

// Host Controller Capability Registers (EHCI Spec 2.2)
#define EHCI_CAPLENGTH			0x00		// Capability Register Length
#define EHCI_HCIVERSION			0x02		// Interface Version Number
#define EHCI_HCSPARAMS			0x04		// Structural Parameters
#define EHCI_HCCPARAMS			0x08		// Capability Parameters
#define EHCI_HCSP_PORTROUTE		0x0c		// Companion Port Route Description

// Host Controller Operational Registers (EHCI Spec 2.3)
#define EHCI_USBCMD				0x00		// USB Command
#define EHCI_USBSTS				0x04		// USB Status
#define EHCI_USBINTR			0x08		// USB Interrupt Enable
#define EHCI_FRINDEX			0x0c		// USB Frame Index
#define EHCI_CTRDSSEGMENT		0x10		// 4GB Segment Selector
#define EHCI_PERIODICLISTBASE	0x14		// Frame List Base Address
#define EHCI_ASYNCLISTADDR		0x18		// Next Asynchronous List Address
#define EHCI_CONFIGFLAG			0x40		// Configured Flag Register
#define EHCI_PORTSC				0x44		// Port Status/Control


// USB Command Register (EHCI Spec 2.3.1)
#define EHCI_USBCMD_ITC_SHIFT	16			// Interrupt Threshold Control
#define EHCI_USBCMD_ITC_MASK	0xff
#define EHCI_USBCMD_PPCEE		(1 << 15)	// Per-Port Change Events Enable
#define EHCI_USBCMD_FSP			(1 << 14)	// Fully Synchronized Prefetch
#define EHCI_USBCMD_ASPE		(1 << 13)	// Async Schedule Prefetch Enable
#define EHCI_USBCMD_PSPE		(1 << 12)	// Periodic Schedule Prefetch Enable
#define EHCI_USBCMD_ASPME		(1 << 11)	// Async Schedule Park Mode Enable
#define EHCI_USBCMD_ASPMC_SHIFT	8			// Async Schedule Park Mode Count
#define EHCI_USBCMD_ASPMC_MASK	0x03
#define EHCI_USBCMD_LHCRESET	(1 << 7)	// Light Host Controller Reset
#define EHCI_USBCMD_INTONAAD	(1 << 6)	// Interrupt on Async Advance Doorbell
#define EHCI_USBCMD_ASENABLE	(1 << 5)	// Asynchronous Schedule Enable
#define EHCI_USBCMD_PSENABLE	(1 << 4)	// Periodic Schedule Enable
#define EHCI_USBCMD_FLS_SHIFT	2			// Frame List Size
#define EHCI_USBCMD_FLS_MASK	0x03
#define EHCI_USBCMD_HCRESET		(1 << 1)	// Host Controller Reset
#define EHCI_USBCMD_RUNSTOP		(1 << 0)	// Run/Stop


// USB Status Register (EHCI Spec 2.3.2)
#define EHCI_USBSTS_ASSTATUS	(1 << 15)	// Asynchronous Schedule Status
#define EHCI_USBSTS_PSSTATUS	(1 << 14)	// Periodic Schedule Status
#define EHCI_USBSTS_RECLAMATION	(1 << 13)	// Reclamation
#define EHCI_USBSTS_HCHALTED	(1 << 12)	// Host Controller Halted
#define EHCI_USBSTS_INTONAA		(1 << 5)	// Interrupt on Async Advance
#define EHCI_USBSTS_HOSTSYSERR	(1 << 4)	// Host System Error
#define EHCI_USBSTS_FLROLLOVER	(1 << 3)	// Frame List Rollover
#define EHCI_USBSTS_PORTCHANGE	(1 << 2)	// Port Change Detected
#define EHCI_USBSTS_USBERRINT	(1 << 1)	// USB Error Interrupt
#define EHCI_USBSTS_USBINT		(1 << 0)	// USB Interrupt
#define EHCI_USBSTS_INTMASK		0x3f


// USB Interrupt Enable Register (EHCI Spec 2.3.3)
#define EHCI_USBINTR_INTONAA	(1 << 5)	// Interrupt on Async Advance Enable
#define EHCI_USBINTR_HOSTSYSERR	(1 << 4)	// Host System Error Enable
#define EHCI_USBINTR_FLROLLOVER	(1 << 3)	// Frame List Rollover Enable
#define EHCI_USBINTR_PORTCHANGE	(1 << 2)	// Port Change Interrupt Enable
#define EHCI_USBINTR_USBERRINT	(1 << 1)	// USB Error Interrupt Enable
#define EHCI_USBINTR_USBINT		(1 << 0)	// USB Interrupt Enable


// Configure Flag Register (EHCI Spec 2.3.8)
#define EHCI_CONFIGFLAG_FLAG	(1 << 0)	// Configure Flag


// Port Status and Control (EHCI Spec 2.3.9)
#define EHCI_PORTSC_WAKEOVERCUR	(1 << 22)	// Wake on Over-Current Enable
#define EHCI_PORTSC_WAKEDISCON	(1 << 21)	// Wake on Disconnect Enable
#define EHCI_PORTSC_WAKECONNECT	(1 << 20)	// Wake on Connect Enable
#define EHCI_PORTSC_PTC_SHIFT	16			// Port Test Control
#define EHCI_PORTSC_PTC_MASK	0x07
#define EHCI_PORTSC_PIC_SHIFT	14			// Port Indicator Control
#define EHCI_PORTSC_PIC_MASK	0x03
#define EHCI_PORTSC_PORTOWNER	(1 << 13)	// Port Owner
#define EHCI_PORTSC_PORTPOWER	(1 << 12)	// Port Power
#define EHCI_PORTSC_DPLUS		(1 << 11)	// Logical Level of D+
#define EHCI_PORTSC_DMINUS		(1 << 10)	// Logical Level of D-
#define EHCI_PORTSC_PORTRESET	(1 << 8)	// Port Reset
#define EHCI_PORTSC_SUSPEND		(1 << 7)	// Suspend
#define EHCI_PORTSC_FORCERESUME	(1 << 6)	// Force Port Resume
#define EHCI_PORTSC_OCCHANGE	(1 << 5)	// Over-Current Change
#define EHCI_PORTSC_OCACTIVE	(1 << 4)	// Over-Current Active
#define EHCI_PORTSC_ENABLECHANGE (1 << 3)	// Port Enable/Disable Change
#define EHCI_PORTSC_ENABLE		(1 << 2)	// Port Enabled/Disabled
#define EHCI_PORTSC_CONNCHANGE	(1 << 1)	// Connect Status Change
#define EHCI_PORTSC_CONNSTATUS	(1 << 0)	// Current Connect Status

#define EHCI_PORTSC_DATAMASK	0xffffffd5


// Extended Capabilities
#define EHCI_ECP_SHIFT			8			// Extended Capability Pointer
#define EHCI_ECP_MASK			0xff
#define EHCI_LEGSUP_CAPID_MASK	0xff
#define EHCI_LEGSUP_CAPID		0x01
#define EHCI_LEGSUP_OSOWNED		(1 << 24)	// OS Owned Semaphore
#define EHCI_LEGSUP_BIOSOWNED	(1 << 16)	// BIOS Owned Semaphore

#define EHCI_HCCPARAMS_FPLC		(1 << 19)	// 32 Frames Period List
#define EHCI_HCCPARAMS_PPCEC	(1 << 18)	// Per-Port Change Event
#define EHCI_HCCPARAMS_LPM		(1 << 17)	// Link Power Management
#define EHCI_HCCPARAMS_HP		(1 << 16)	// Hardware Prefetch
#define EHCI_HCCPARAMS_FRAME_CACHE(x)	((x >> 7) & 0x1)	// Isochronous Periodic Threshold
#define EHCI_HCCPARAMS_IPT(x)	((x >> 4) & 0x7)	// Isochronous Periodic Threshold


// Data Structures (EHCI Spec 3)


// Applies to ehci_qh.next_phy, ehci_sitd.next_phy, ehci_itd.next_phy
#define EHCI_ITEM_TYPE_ITD		(0 << 1)	// Isochronous Transfer Descriptor
#define EHCI_ITEM_TYPE_QH		(1 << 1)	// Queue Head
#define EHCI_ITEM_TYPE_SITD		(2 << 1)	// Split Transaction Isochronous TD
#define EHCI_ITEM_TYPE_FSTN		(3 << 1)	// Frame Span Traversal Node
#define EHCI_ITEM_TERMINATE		(1 << 0)	// Terminate


// Isochronous (High-Speed) Transfer Descriptors (iTD, EHCI Spec 3.2)
typedef struct ehci_itd {
	// Hardware Part
	addr_t		next_phy;
	uint32		token[8];
	addr_t		buffer_phy[7];
	addr_t		ext_buffer_phy[7];

	// Software Part
	addr_t		this_phy;
	struct ehci_itd	*next;
	struct ehci_itd	*prev;
	uint32		last_token;
} ehci_itd;

#define EHCI_ITD_TOFFSET_SHIFT	0
#define EHCI_ITD_TOFFSET_MASK	0x0fff
#define EHCI_ITD_IOC			(1 << 15)
#define EHCI_ITD_PG_SHIFT		12
#define EHCI_ITD_PG_MASK		0x07
#define EHCI_ITD_TLENGTH_SHIFT	16
#define EHCI_ITD_TLENGTH_MASK	0x0fff
#define EHCI_ITD_STATUS_SHIFT	28
#define EHCI_ITD_STATUS_MASK	0xf
#define EHCI_ITD_STATUS_ACTIVE	(1 << 3)	// Active
#define EHCI_ITD_STATUS_BUFFER	(1 << 2)	// Data Buffer Error
#define EHCI_ITD_STATUS_BABBLE	(1 << 1)	// Babble Detected
#define EHCI_ITD_STATUS_TERROR	(1 << 0)	// Transaction Error
#define EHCI_ITD_ADDRESS_SHIFT	0
#define EHCI_ITD_ADDRESS_MASK	0x7f
#define EHCI_ITD_ENDPOINT_SHIFT	8
#define EHCI_ITD_ENDPOINT_MASK	0xf
#define EHCI_ITD_DIR_SHIFT	11
#define EHCI_ITD_MUL_SHIFT	0
#define EHCI_ITD_MUL_MASK	0x3
#define EHCI_ITD_BUFFERPOINTER_SHIFT	12
#define EHCI_ITD_BUFFERPOINTER_MASK	0xfffff
#define EHCI_ITD_MAXPACKETSIZE_SHIFT	0
#define EHCI_ITD_MAXPACKETSIZE_MASK	0x7ff
#define EHCI_ITD_MAXPACKETSIZE_LENGTH	11


// Split Transaction Isochronous Transfer Descriptors (siTD, EHCI Spec 3.3)
typedef struct ehci_sitd {
	// Hardware Part
	addr_t		next_phy;
	uint8		port_number;
	uint8		hub_address;
	uint8		endpoint;
	uint8		device_address;
	uint16		reserved1;
	uint8		cmask;
	uint8		smask;
	uint16		transfer_length;
	uint8		cprogmask;
	uint8		status;
	addr_t		buffer_phy[2];
	addr_t		back_phy;
	addr_t		ext_buffer_phy[2];

	// Software Part
	addr_t		this_phy;
	struct ehci_sitd *next;
	struct ehci_sitd *prev;
	size_t		buffer_size;
	void		*buffer_log;
} ehci_sitd;

// Queue Element Transfer Descriptors (qTD, EHCI Spec 3.5)
typedef struct ehci_qtd {
	// Hardware Part
	addr_t		next_phy;
	addr_t		alt_next_phy;
	uint32		token;
	addr_t		buffer_phy[5];
	addr_t		ext_buffer_phy[5];

	// Software Part
	addr_t		this_phy;
	struct ehci_qtd	*next_log;
	void		*alt_next_log;
	size_t		buffer_size;
	void		*buffer_log;
} ehci_qtd;


#define EHCI_QTD_DATA_TOGGLE	(1 << 31)
#define EHCI_QTD_BYTES_SHIFT	16
#define EHCI_QTD_BYTES_MASK		0x7fff
#define EHCI_QTD_IOC			(1 << 15)
#define EHCI_QTD_CPAGE_SHIFT	12
#define EHCI_QTD_CPAGE_MASK		0x07
#define EHCI_QTD_ERRCOUNT_SHIFT	10
#define EHCI_QTD_ERRCOUNT_MASK	0x03
#define EHCI_QTD_PID_SHIFT		8
#define EHCI_QTD_PID_MASK		0x03
#define EHCI_QTD_PID_OUT		0x00
#define EHCI_QTD_PID_IN			0x01
#define EHCI_QTD_PID_SETUP		0x02
#define EHCI_QTD_STATUS_SHIFT	0
#define EHCI_QTD_STATUS_MASK	0x7f
#define EHCI_QTD_STATUS_ERRMASK	0x50
#define EHCI_QTD_STATUS_ACTIVE	(1 << 7)	// Active
#define EHCI_QTD_STATUS_HALTED	(1 << 6)	// Halted
#define EHCI_QTD_STATUS_BUFFER	(1 << 5)	// Data Buffer Error
#define EHCI_QTD_STATUS_BABBLE	(1 << 4)	// Babble Detected
#define EHCI_QTD_STATUS_TERROR	(1 << 3)	// Transaction Error
#define EHCI_QTD_STATUS_MISSED	(1 << 2)	// Missed Micro-Frame
#define EHCI_QTD_STATUS_SPLIT	(1 << 1)	// Split Transaction State
#define EHCI_QTD_STATUS_PING	(1 << 0)	// Ping State
#define EHCI_QTD_PAGE_MASK		0xfffff000


// Queue Head (QH, EHCI Spec 3.6)
typedef struct ehci_qh {
	// Hardware Part
	addr_t		next_phy;
	uint32		endpoint_chars;
	uint32		endpoint_caps;
	addr_t		current_qtd_phy;

	struct {
		addr_t		next_phy;
		addr_t		alt_next_phy;
		uint32		token;
		addr_t		buffer_phy[5];
		addr_t		ext_buffer_phy[5];
	} overlay;

	// Software Part
	addr_t		this_phy;
	struct ehci_qh *next_log;
	struct ehci_qh *prev_log;
	ehci_qtd	*stray_log;
	ehci_qtd	*element_log;
} ehci_qh;


typedef struct {
	ehci_qh		queue_head;
	uint32		padding[2];
} interrupt_entry;

typedef struct {
	ehci_itd	itd;
	uint32		padding[5]; // align on 128
} itd_entry;

typedef struct {
	ehci_sitd	sitd;
	uint32		padding[2]; // align on 64
} sitd_entry;

#define EHCI_INTERRUPT_ENTRIES_COUNT	(7 + 1)		// (log 128 / log 2) + 1
#define EHCI_VFRAMELIST_ENTRIES_COUNT	128
#define EHCI_FRAMELIST_ENTRIES_COUNT	1024

#define MAX_AVAILABLE_BANDWIDTH	125	// Microseconds


// Applies to ehci_qh.endpoint_chars
#define EHCI_QH_CHARS_RL_SHIFT	28			// NAK Count Reload
#define EHCI_QH_CHARS_RL_MASK	0x07
#define EHCI_QH_CHARS_CONTROL	(1 << 27)	// Control Endpoint Flag
#define EHCI_QH_CHARS_MPL_SHIFT	16			// Max Packet Length
#define EHCI_QH_CHARS_MPL_MASK	0x03ff
#define EHCI_QH_CHARS_RECHEAD	(1 << 15)	// Head of Reclamation List Flag
#define EHCI_QH_CHARS_TOGGLE	(1 << 14)	// Data Toggle Control
#define EHCI_QH_CHARS_EPS_FULL	(0 << 12)	// Endpoint is Full-Speed
#define EHCI_QH_CHARS_EPS_LOW	(1 << 12)	// Endpoint is Low-Speed
#define EHCI_QH_CHARS_EPS_HIGH	(2 << 12)	// Endpoint is High-Speed
#define EHCI_QH_CHARS_EPT_SHIFT	8			// Endpoint Number
#define EHCI_QH_CHARS_EPT_MASK	0x0f
#define EHCI_QH_CHARS_INACTIVE	(1 << 7)	// Inactive on Next Transaction
#define EHCI_QH_CHARS_DEV_SHIFT	0			// Device Address
#define EHCI_QH_CHARS_DEV_MASK	0x7f


// Applies to ehci_qh.endpoint_caps
#define EHCI_QH_CAPS_MULT_SHIFT	30			// Transactions per Micro-Frame
#define EHCI_QH_CAPS_MULT_MASK	0x03
#define EHCI_QH_CAPS_PORT_SHIFT	23			// Hub Port (Split-Transaction)
#define EHCI_QH_CAPS_PORT_MASK	0x7f
#define EHCI_QH_CAPS_HUB_SHIFT	16			// Hub Address (Split-Transaction)
#define EHCI_QH_CAPS_HUB_MASK	0x7f
#define EHCI_QH_CAPS_SCM_SHIFT	8			// Split Completion Mask
#define EHCI_QH_CAPS_SCM_MASK	0xff
#define EHCI_QH_CAPS_ISM_SHIFT	0			// Interrupt Schedule Mask
#define EHCI_QH_CAPS_ISM_MASK	0xff


// Applies to ehci_qh.overlay[EHCI_QH_OL_*_INDEX]
#define EHCI_QH_OL_NAK_INDEX	1			// NAK Counter
#define EHCI_QH_OL_NAK_SHIFT	1
#define EHCI_QH_OL_NAK_MASK		0x0f
#define EHCI_QH_OL_TOGGLE_INDEX	2			// Data Toggle
#define EHCI_QH_OL_TOGGLE		(1 << 31)
#define EHCI_QH_OL_IOC_INDEX	2			// Interrupt on Complete
#define EHCI_QH_OL_IOC			(1 << 15)
#define EHCI_QH_OL_ERRC_INDEX	2			// Error Counter
#define EHCI_QH_OL_ERRC_SHIFT	10
#define EHCI_QH_OL_ERRC_MASK	0x03
#define EHCI_QH_OL_PING_INDEX	2			// Ping State
#define EHCI_QH_OL_PING			(1 << 0)
#define EHCI_QH_OL_CPROG_INDEX	4			// Split-Transaction Complete-Split Progress
#define EHCI_QH_OL_CPROG_SHIFT	0
#define EHCI_QH_OL_CPROG_MASK	0xff
#define EHCI_QH_OL_FTAG_INDEX	5			// Split-Transaction Frame Tag
#define EHCI_QH_OL_FTAG_SHIFT	0
#define EHCI_QH_OL_FTAG_MASK	0x0f
#define EHCI_QH_OL_BYTES_INDEX	5			// Transfered Bytes
#define EHCI_QH_OL_BYTES_SHIFT	5
#define EHCI_QH_OL_BYTES_MASK	0x7f


// ToDo: Periodic Frame Span Traversal Node (FSTN, EHCI Spec 3.7)


// Quirk registers and values
#define	AMD_SBX00_VENDOR				0x1002
#define	AMD_SBX00_SMBUS_CONTROLLER		0x4385
#define	AMD_SB600_EHCI_CONTROLLER		0x4386
#define	AMD_SB700_SB800_EHCI_CONTROLLER	0x4396

#define	AMD_SBX00_EHCI_MISC_REGISTER	0x50	// Advanced config register
#define	AMD_SBX00_EHCI_MISC_DISABLE_PERIODIC_LIST_CACHE		(1 << 27)

#endif // !EHCI_HARDWARE_H
