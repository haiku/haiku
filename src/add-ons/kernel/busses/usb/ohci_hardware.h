/*
 * Copyright 2005-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jan-Rixt Van Hoye
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#ifndef OHCI_HARDWARE_H
#define OHCI_HARDWARE_H

// --------------------------------
//	The OHCI registers
// --------------------------------

// --------------------------------
//	Revision register (section 7.1.1)
// --------------------------------

#define	OHCI_REVISION				0x00
#define	OHCI_REVISION_LOW(rev)		((rev) & 0x0f)
#define	OHCI_REVISION_HIGH(rev)		(((rev) >> 4) & 0x03)
#define	OHCI_REVISION_LEGACY(rev)	((rev) & 0x10)

// --------------------------------
//	Control register (section 7.1.2)
// --------------------------------

#define	OHCI_CONTROL							0x04
#define	OHCI_CONTROL_BULK_SERVICE_RATIO_MASK	0x00000003
#define	OHCI_CONTROL_BULK_RATIO_1_1				0x00000000
#define	OHCI_CONTROL_BULK_RATIO_1_2				0x00000001
#define	OHCI_CONTROL_BULK_RATIO_1_3				0x00000002
#define	OHCI_CONTROL_BULK_RATIO_1_4				0x00000003
#define	OHCI_PERIODIC_LIST_ENABLE				0x00000004
#define	OHCI_ISOCHRONOUS_ENABLE					0x00000008
#define	OHCI_CONTROL_LIST_ENABLE				0x00000010
#define	OHCI_BULK_LIST_ENABLE					0x00000020
#define	OHCI_HC_FUNCTIONAL_STATE_MASK			0x000000c0
#define	OHCI_HC_FUNCTIONAL_STATE_RESET			0x00000000
#define	OHCI_HC_FUNCTIONAL_STATE_RESUME			0x00000040
#define	OHCI_HC_FUNCTIONAL_STATE_OPERATIONAL	0x00000080
#define	OHCI_HC_FUNCTIONAL_STATE_SUSPEND		0x000000c0
#define	OHCI_INTERRUPT_ROUTING					0x00000100
#define	OHCI_REMOTE_WAKEUP_CONNECTED			0x00000200
#define	OHCI_REMORE_WAKEUP_ENABLED				0x00000400

// --------------------------------
//	Command status register (section 7.1.3)
// --------------------------------

#define	OHCI_COMMAND_STATUS						0x08
#define	OHCI_HOST_CONTROLLER_RESET				0x00000001
#define	OHCI_CONTROL_LIST_FILLED				0x00000002
#define	OHCI_BULK_LIST_FILLED					0x00000004
#define	OHCI_OWNERSHIP_CHANGE_REQUEST			0x00000008
#define	OHCI_SCHEDULING_OVERRUN_COUNT_MASK		0x00030000

// --------------------------------
//	Interrupt status register (section 7.1.4)
// --------------------------------

#define	OHCI_INTERRUPT_STATUS			0x0c
#define	OHCI_SCHEDULING_OVERRUN			0x00000001
#define	OHCI_WRITEBACK_DONE_HEAD		0x00000002
#define	OHCI_START_OF_FRAME				0x00000004
#define	OHCI_RESUME_DETECTED			0x00000008
#define	OHCI_UNRECOVERABLE_ERROR		0x00000010
#define	OHCI_FRAME_NUMBER_OVERFLOW		0x00000020
#define	OHCI_ROOT_HUB_STATUS_CHANGE		0x00000040
#define	OHCI_OWNERSHIP_CHANGE			0x40000000
#define	OHCI_MASTER_INTERRUPT_ENABLE	0x80000000

// --------------------------------
//	Interupt enable register (section 7.1.5)
// --------------------------------

#define OHCI_INTERRUPT_ENABLE		0x10

// --------------------------------
//	Interupt disable register (section 7.1.6)
// --------------------------------

#define OHCI_INTERRUPT_DISABLE		0x14

// -------------------------------------
//	Memory Pointer Partition (section 7.2)
// -------------------------------------

// --------------------------------
//	HCCA register (section 7.2.1)
// --------------------------------

#define OHCI_HCCA					0x18

// --------------------------------
//	Period current ED register (section 7.2.2)
// --------------------------------

#define OHCI_PERIOD_CURRENT_ED		0x1c

// --------------------------------
//	Control head ED register (section 7.2.3)
// --------------------------------

#define OHCI_CONTROL_HEAD_ED		0x20

// --------------------------------
//	Current control ED register (section 7.2.4)
// --------------------------------

#define OHCI_CONTROL_CURRENT_ED		0x24

// --------------------------------
//	Bulk head ED register (section 7.2.5)
// --------------------------------

#define OHCI_BULK_HEAD_ED			0x28

// --------------------------------
//	Current bulk ED register (section 7.2.6)
// --------------------------------

#define OHCI_BULK_CURRENT_ED		0x2c

// --------------------------------
//	Done head register (section 7.2.7)
// --------------------------------

#define OHCI_DONE_HEAD				0x30

// --------------------------------
//	Frame Counter partition (section 7.3)
// --------------------------------

// --------------------------------
//	Frame interval register (section 7.3.1)
// --------------------------------

#define	OHCI_FRAME_INTERVAL					0x34
#define	OHCI_GET_INTERVAL_VALUE(s)			((s) & 0x3fff)
#define	OHCI_GET_FS_LARGEST_DATA_PACKET(s)	(((s) >> 16) & 0x7fff)
#define	OHCI_FRAME_INTERVAL_TOGGLE			0x80000000

// --------------------------------
//	Frame remaining register (section 7.3.2)
// --------------------------------

#define	OHCI_FRAME_REMAINING			0x38

// --------------------------------
//	Frame number register	(section 7.3.3)
// --------------------------------

#define	OHCI_FRAME_NUMBER				0x3c

// --------------------------------
//	Periodic start register (section 7.3.4)
// --------------------------------

#define	OHCI_PERIODIC_START				0x40

// --------------------------------
//	Low Speed (LS) treshold register (section 7.3.5)
// --------------------------------

#define	OHCI_LOW_SPEED_THRESHOLD		0x44

// --------------------------------
//	Root Hub Partition (section 7.4)
// --------------------------------

// --------------------------------
//	Root Hub Descriptor A register (section 7.4.1)
// --------------------------------

#define	OHCI_RH_DESCRIPTOR_A						0x48
#define	OHCI_RH_GET_PORT_COUNT(s)					((s) & 0xff)
#define	OHCI_RH_POWER_SWITCHING_MODE				0x0100
#define	OHCI_RH_NO_POWER_SWITCHING					0x0200
#define	OHCI_RH_DEVICE_TYPE							0x0400
#define	OHCI_RH_OVER_CURRENT_PROTECTION_MODE		0x0800
#define	OHCI_RH_NO_OVER_CURRENT_PROTECTION			0x1000
#define	OHCI_RH_GET_POWER_ON_TO_POWER_GOOD_TIME(s)	((s) >> 24)

// --------------------------------
//	Root Hub Descriptor B register (section 7.4.2)
// --------------------------------

#define OHCI_RH_DESCRIPTOR_B		0x4c

// --------------------------------
//	Root Hub status register (section 7.4.3)
// --------------------------------

#define	OHCI_RH_STATUS							0x50
#define	OHCI_RH_LOCAL_POWER_STATUS				0x00000001
#define	OHCI_RH_OVER_CURRENT_INDICATOR			0x00000002
#define	OHCI_RH_DEVICE_REMOTE_WAKEUP_ENABLE		0x00008000
#define	OHCI_RH_LOCAL_POWER_STATUS_CHANGE		0x00010000
#define	OHCI_RH_OVER_CURRENT_INDICATOR_CHANGE	0x00020000
#define	OHCI_RH_CLEAR_REMOTE_WAKEUP_ENABLE		0x80000000

// --------------------------------
//	Root Hub port status (n) register (section 7.4.4)
// --------------------------------

#define	OHCI_RH_PORT_STATUS(n)		(0x54 + (n) * 4)// 0 based indexing
#define	OHCI_RH_PORTSTATUS_CCS		0x00000001		// Current Connection Status
#define	OHCI_RH_PORTSTATUS_PES		0x00000002		// Port Enable Status
#define	OHCI_RH_PORTSTATUS_PSS		0x00000004		// Port Suspend Status
#define	OHCI_RH_PORTSTATUS_POCI		0x00000008		// Port Overcurrent Indicator
#define	OHCI_RH_PORTSTATUS_PRS		0x00000010		// Port Reset Status
#define	OHCI_RH_PORTSTATUS_PPS		0x00000100		// Port Power Status
#define	OHCI_RH_PORTSTATUS_LSDA		0x00000200		// Low Speed Device Attached
#define	OHCI_RH_PORTSTATUS_CSC		0x00010000		// Connection Status Change
#define	OHCI_RH_PORTSTATUS_PESC		0x00020000		// Port Enable Status Change
#define	OHCI_RH_PORTSTATUS_PSSC		0x00040000		// Port Suspend Status change
#define	OHCI_RH_PORTSTATUS_OCIC		0x00080000		// Port Overcurrent Change
#define	OHCI_RH_PORTSTATUS_PRSC		0x00100000		// Port Reset Status Change

// --------------------------------
//	Enable List
// --------------------------------

#define	OHCI_ENABLE_LIST		(OHCI_PERIODIC_LIST_ENABLE \
								| OHCI_ISOCHRONOUS_ENABLE \
								| OHCI_CONTROL_LIST_ENABLE \
								| OHCI_BULK_LIST_ENABLE)

// --------------------------------
//	All interupts
// --------------------------------

#define	OHCI_ALL_INTERRUPTS		(OHCI_SCHEDULING_OVERRUN \
								| OHCI_WRITEBACK_DONE_HEAD \
								| OHCI_START_OF_FRAME \
								| OHCI_RESUME_DETECTED \
								| OHCI_UNRECOVERABLE_ERROR \
								| OHCI_FRAME_NUMBER_OVERFLOW \
								| OHCI_ROOT_HUB_STATUS_CHANGE \
								| OHCI_OWNERSHIP_CHANGE)

// --------------------------------
//	All normal interupts
// --------------------------------

#define	OHCI_NORMAL_INTERRUPTS		(OHCI_SCHEDULING_OVERRUN \
									| OHCI_WRITEBACK_DONE_HEAD \
									| OHCI_RESUME_DETECTED \
									| OHCI_UNRECOVERABLE_ERROR \
									| OHCI_ROOT_HUB_STATUS_CHANGE)

// --------------------------------
//	FSMPS
// --------------------------------

#define	OHCI_FSMPS(i)				(((i - 210) * 6 / 7) << 16)

// --------------------------------
//	Periodic
// --------------------------------

#define	OHCI_PERIODIC(i)			((i) * 9 / 10)

// --------------------------------
//	HCCA structure (section 4.4)
//	256 bytes aligned
// --------------------------------

#define OHCI_NUMBER_OF_INTERRUPTS	32
#define OHCI_STATIC_ENDPOINT_COUNT	6
#define OHCI_BIGGEST_INTERVAL		32

typedef struct {
	uint32		interrupt_table[OHCI_NUMBER_OF_INTERRUPTS];
	uint32		current_frame_number;
	uint32		done_head;
	// The following is 120 instead of 116 because the spec
	// only specifies 252 bytes
	uint8		reserved_for_hc[120];
} ohci_hcca;

#define OHCI_DONE_INTERRUPTS		1
#define OHCI_HCCA_SIZE				256
#define OHCI_HCCA_ALIGN				256
#define OHCI_PAGE_SIZE				0x1000
#define OHCI_PAGE(x)				((x) &~ 0xfff)
#define OHCI_PAGE_OFFSET(x)			((x) & 0xfff)

// --------------------------------
//	Endpoint descriptor structure (section 4.2)
// --------------------------------

typedef struct {
	// Hardware part
	uint32	flags;						// Flags field
	uint32	tail_physical_descriptor;	// Queue tail physical pointer
	uint32	head_physical_descriptor;	// Queue head physical pointer
	uint32	next_physical_endpoint;		// Physical pointer to the next endpoint
	// Software part
	uint32	physical_address;			// Physical pointer to this address
	void	*tail_logical_descriptor;	// Queue tail logical pointer
	void	*next_logical_endpoint;		// Logical pointer to the next endpoint
	mutex	*lock;						// Protects tail changes and checks
} ohci_endpoint_descriptor;

#define	OHCI_ENDPOINT_ADDRESS_MASK				0x0000007f
#define	OHCI_ENDPOINT_GET_DEVICE_ADDRESS(s)		((s) & 0x7f)
#define	OHCI_ENDPOINT_SET_DEVICE_ADDRESS(s)		(s)
#define	OHCI_ENDPOINT_GET_ENDPOINT_NUMBER(s)	(((s) >> 7) & 0xf)
#define	OHCI_ENDPOINT_SET_ENDPOINT_NUMBER(s)	((s) << 7)
#define	OHCI_ENDPOINT_DIRECTION_MASK			0x00001800
#define	OHCI_ENDPOINT_DIRECTION_DESCRIPTOR		0x00000000
#define	OHCI_ENDPOINT_DIRECTION_OUT				0x00000800
#define	OHCI_ENDPOINT_DIRECTION_IN				0x00001000
#define	OHCI_ENDPOINT_LOW_SPEED					0x00002000
#define	OHCI_ENDPOINT_FULL_SPEED				0x00000000
#define	OHCI_ENDPOINT_SKIP						0x00004000
#define	OHCI_ENDPOINT_GENERAL_FORMAT			0x00000000
#define	OHCI_ENDPOINT_ISOCHRONOUS_FORMAT		0x00008000
#define	OHCI_ENDPOINT_MAX_PACKET_SIZE_MASK		(0x7ff << 16)
#define	OHCI_ENDPOINT_GET_MAX_PACKET_SIZE(s)	(((s) >> 16) & 0x07ff)
#define	OHCI_ENDPOINT_SET_MAX_PACKET_SIZE(s)	((s) << 16)
#define	OHCI_ENDPOINT_HALTED					0x00000001
#define	OHCI_ENDPOINT_TOGGLE_CARRY				0x00000002
#define	OHCI_ENDPOINT_HEAD_MASK					0xfffffffc


// --------------------------------
//	General transfer descriptor structure (section 4.3.1)
// --------------------------------

typedef struct {
	// Hardware part 16 bytes
	uint32	flags;						// Flags field
	uint32	buffer_physical;			// Physical buffer pointer
	uint32	next_physical_descriptor;	// Physical pointer next descriptor
	uint32	last_physical_byte_address;	// Physical pointer to buffer end
	// Software part
	uint32	physical_address;			// Physical address of this descriptor
	size_t	buffer_size;				// Size of the buffer
	void	*buffer_logical;			// Logical pointer to the buffer
	void	*next_logical_descriptor;	// Logical pointer next descriptor
} ohci_general_td;

#define	OHCI_TD_BUFFER_ROUNDING			0x00040000
#define	OHCI_TD_DIRECTION_PID_MASK		0x00180000
#define	OHCI_TD_DIRECTION_PID_SETUP		0x00000000
#define	OHCI_TD_DIRECTION_PID_OUT		0x00080000
#define	OHCI_TD_DIRECTION_PID_IN		0x00100000
#define	OHCI_TD_GET_DELAY_INTERRUPT(x)	(((x) >> 21) & 7)
#define	OHCI_TD_SET_DELAY_INTERRUPT(x)	((x) << 21)
#define	OHCI_TD_INTERRUPT_MASK			0x00e00000
#define	OHCI_TD_TOGGLE_CARRY			0x00000000
#define	OHCI_TD_TOGGLE_0				0x02000000
#define	OHCI_TD_TOGGLE_1				0x03000000
#define	OHCI_TD_TOGGLE_MASK				0x03000000
#define	OHCI_TD_GET_ERROR_COUNT(x)		(((x) >> 26) & 3)
#define	OHCI_TD_GET_CONDITION_CODE(x)	((x) >> 28)
#define	OHCI_TD_SET_CONDITION_CODE(x)	((x) << 28)
#define	OHCI_TD_CONDITION_CODE_MASK		0xf0000000

#define OHCI_TD_INTERRUPT_IMMEDIATE			0x00
#define OHCI_TD_INTERRUPT_NONE				0x07

#define OHCI_TD_CONDITION_NO_ERROR			0x00
#define OHCI_TD_CONDITION_CRC_ERROR			0x01
#define OHCI_TD_CONDITION_BIT_STUFFING		0x02
#define OHCI_TD_CONDITION_TOGGLE_MISMATCH	0x03
#define OHCI_TD_CONDITION_STALL				0x04
#define OHCI_TD_CONDITION_NO_RESPONSE		0x05
#define OHCI_TD_CONDITION_PID_CHECK_FAILURE	0x06
#define OHCI_TD_CONDITION_UNEXPECTED_PID	0x07
#define OHCI_TD_CONDITION_DATA_OVERRUN		0x08
#define OHCI_TD_CONDITION_DATA_UNDERRUN		0x09
#define OHCI_TD_CONDITION_BUFFER_OVERRUN	0x0c
#define OHCI_TD_CONDITION_BUFFER_UNDERRUN	0x0d
#define OHCI_TD_CONDITION_NOT_ACCESSED		0x0f

#define OHCI_GENERAL_TD_ALIGN 16

// --------------------------------
//	Isochronous transfer descriptor structure (section 4.3.2)
// --------------------------------

#define OHCI_ITD_NOFFSET 8
typedef struct {
	// Hardware part 32 byte
	uint32		flags;
	uint32		buffer_page_byte_0;			// Physical page number of byte 0
	uint32		next_physical_descriptor;	// Next isochronous transfer descriptor
	uint32		last_byte_address;			// Physical buffer end
	uint16		offset[OHCI_ITD_NOFFSET];	// Buffer offsets
	// Software part
	uint32		physical_address;			// Physical address of this descriptor
	void		*next_logical_descriptor;	// Logical pointer next descriptor
	void		*next_done_descriptor;		// Used for collision in the hash table
} ohci_isochronous_td;

#define	OHCI_ITD_GET_STARTING_FRAME(x)			((x) & 0x0000ffff)
#define	OHCI_ITD_SET_STARTING_FRAME(x)			((x) & 0xffff)
#define	OHCI_ITD_GET_DELAY_INTERRUPT(x)			(((x) >> 21) & 7)
#define	OHCI_ITD_SET_DELAY_INTERRUPT(x)			((x) << 21)
#define	OHCI_ITD_NO_INTERRUPT					0x00e00000
#define	OHCI_ITD_GET_FRAME_COUNT(x)				((((x) >> 24) & 7) + 1)
#define	OHCI_ITD_SET_FRAME_COUNT(x)				(((x) - 1) << 24)
#define	OHCI_ITD_GET_CONDITION_CODE(x)			((x) >> 28)
#define	OHCI_ITD_NO_CONDITION_CODE				0xf0000000

// TO FIX
#define itd_pswn itd_offset						// Packet Status Word
#define OHCI_ITD_PAGE_SELECT					0x00001000
#define OHCI_ITD_MK_OFFS(len)					(0xe000 | ((len) & 0x1fff))
#define OHCI_ITD_GET_BUFFER_LENGTH(x)			((x) & 0xfff)
#define OHCI_ITD_GET_BUFFER_CONDITION_CODE(x)	((x) >> 12)

#define OHCI_ISOCHRONOUS_TD_ALIGN 32

// --------------------------------
//	Completion Codes (section 4.3.3)
// --------------------------------

#define	OHCI_NO_ERROR				0
#define	OHCI_CRC					1
#define	OHCI_BIT_STUFFING			2
#define	OHCI_DATA_TOGGLE_MISMATCH	3
#define	OHCI_STALL					4
#define	OHCI_DEVICE_NOT_RESPONDING	5
#define	OHCI_PID_CHECK_FAILURE		6
#define	OHCI_UNEXPECTED_PID			7
#define	OHCI_DATA_OVERRUN			8
#define	OHCI_DATA_UNDERRUN			9
#define	OHCI_BUFFER_OVERRUN			12
#define	OHCI_BUFFER_UNDERRUN		13
#define	OHCI_NOT_ACCESSED			15

// --------------------------------
//	Some delay needed when changing
//	certain registers.
// --------------------------------

#define	OHCI_ENABLE_POWER_DELAY			5000
#define	OHCI_READ_DESC_DELAY			5000

// Maximum port count set by OHCI
#define OHCI_MAX_PORT_COUNT				15

#endif // OHCI_HARDWARE_H
