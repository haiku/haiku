/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AHCI_DEFS_H
#define _AHCI_DEFS_H

#include <bus/PCI.h>
#include <bus/SCSI.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AHCI_DEVICE_MODULE_NAME "busses/scsi/ahci/driver_v1"
#define AHCI_SIM_MODULE_NAME "busses/scsi/ahci/sim/driver_v1"

// RW1 = Write 1 to set bit (writing 0 is ignored)
// RWC = Write 1 to clear bit (writing 0 is ignored)


enum {
	CAP_S64A		= (1 << 31),	// Supports 64-bit Addressing
	CAP_SNCQ		= (1 << 30),	// Supports Native Command Queuing
	CAP_SSNTF		= (1 << 29),	// Supports SNotification Register
	CAP_SMPS		= (1 << 28),	// Supports Mechanical Presence Switch
	CAP_SSS			= (1 << 27),	// Supports Staggered Spin-up
	CAP_SALP		= (1 << 26),	// Supports Aggressive Link Power Management
	CAP_SAL			= (1 << 25),	// Supports Activity LED
	CAP_SCLO		= (1 << 24),	// Supports Command List Override
	CAP_ISS_MASK 	= 0xf, 			// Interface Speed Support
	CAP_ISS_SHIFT	= 20,
	CAP_SNZO 		= (1 << 19),	// Supports Non-Zero DMA Offsets
	CAP_SAM 		= (1 << 18),	// Supports AHCI mode only
	CAP_SPM 		= (1 << 17),	// Supports Port Multiplier
	CAP_FBSS 		= (1 << 16),	// FIS-based Switching Supported
	CAP_PMD 		= (1 << 15),	// PIO Multiple DRQ Block
	CAP_SSC 		= (1 << 14),	// Slumber State Capable
	CAP_PSC 		= (1 << 13),	// Partial State Capable
	CAP_NCS_MASK 	= 0x1f,			// Number of Command Slots (zero-based number)
	CAP_NCS_SHIFT	= 8,
	CAP_CCCS 		= (1 << 7),		// Command Completion Coalescing Supported
	CAP_EMS 		= (1 << 6),		// Enclosure Management Supported
	CAP_SXS 		= (1 << 5), 	// Supports External SATA
	CAP_NP_MASK		= 0x1f,			// Number of Ports (zero-based number)
	CAP_NP_SHIFT	= 0,
};


enum {
	GHC_AE			= (1 << 31),	// AHCI Enable
	GHC_MRSM		= (1 << 2),		// MSI Revert to Single Message
	GHC_IE			= (1 << 1),		// Interrupt Enable
	GHC_HR			= (1 << 0),		// HBA Reset **RW1**
};


enum {
	INT_CPD			= (1 << 31),	// Cold Port Detect Status/Enable
	INT_TFE			= (1 << 30),	// Task File Error Status/Enable
	INT_HBF			= (1 << 29),	// Host Bus Fatal Error Status/Enable
	INT_HBD			= (1 << 28),	// Host Bus Data Error Status/Enable
	INT_IF			= (1 << 27),	// Interface Fatal Error Status/Enable
	INT_INF			= (1 << 26),	// Interface Non-fatal Error Status/Enable
	INT_OF			= (1 << 24),	// Overflow Status/Enable
	INT_IPM			= (1 << 23),	// Incorrect Port Multiplier Status/Enable
	INT_PRC			= (1 << 22),	// PhyRdy Change Status/Enable
	INT_DMP			= (1 << 7),		// Device Mechanical Presence Status/Enable
	INT_PC			= (1 << 6),		// Port Change Interrupt Status/Enable
	INT_DP			= (1 << 5),		// Descriptor Processed Interrupt/Enable
	INT_UF			= (1 << 4),		// Unknown FIS Interrupt/Enable
	INT_SDB			= (1 << 3),		// Set Device Bits Interrupt/Enable
	INT_DS			= (1 << 2),		// DMA Setup FIS Interrupt/Enable
	INT_PS			= (1 << 1),		// PIO Setup FIS Interrupt/Enable
	INT_DHR			= (1 << 0),		// Device to Host Register FIS Interrupt/Enable
};


typedef struct {
	uint32		clb;			// Command List Base Address (alignment 1024 byte)
	uint32		clbu;			// Command List Base Address Upper 32-Bits
	uint32		fb;				// FIS Base Address (alignment 256 byte)
	uint32		fbu;			// FIS Base Address Upper 32-Bits
	uint32		is;				// Interrupt Status **RWC**
	uint32		ie;				// Interrupt Enable
	uint32		cmd;			// Command and Status
	uint32 		res1;			// Reserved
	uint32		tfd;			// Task File Data
	uint32		sig;			// Signature
	uint32		ssts;			// Serial ATA Status (SCR0: SStatus)
	uint32		sctl;			// Serial ATA Control (SCR2: SControl)
	uint32		serr;			// Serial ATA Error (SCR1: SError) **RWC**
	uint32		sact;			// Serial ATA Active (SCR3: SActive) **RW1**
	uint32		ci;				// Command Issue **RW1**
	uint32		sntf;			// SNotification
	uint32		res2;			// Reserved for FIS-based Switching Definition
	uint32		res[11];		// Reserved
	uint32		vendor[4];		// Vendor Specific
} _PACKED ahci_port;


enum {
	PORT_CMD_ICC_ACTIVE	 = (1 << 28),	// Interface Communication control
	PORT_CMD_ICC_SLUMBER = (6 << 28),	// Interface Communication control
	PORT_CMD_ICC_MASK    = (0xf<<28),	// Interface Communication control
	PORT_CMD_ATAPI	= (1 << 24),	// Device is ATAPI
	PORT_CMD_CR		= (1 << 15),	// Command List Running (DMA active)
	PORT_CMD_FR		= (1 << 14),	// FIS Receive Running
	PORT_CMD_FER	= (1 << 4),		// FIS Receive Enable
	PORT_CMD_CLO	= (1 << 3),		// Command List Override
	PORT_CMD_POD	= (1 << 2),		// Power On Device
	PORT_CMD_SUD	= (1 << 1),		// Spin-up Device
	PORT_CMD_ST		= (1 << 0),		// Start DMA
};


enum {
	PORT_INT_CPD	= (1 << 31),	// Cold Presence Detect Status/Enable
	PORT_INT_TFE	= (1 << 30),	// Task File Error Status/Enable
	PORT_INT_HBF	= (1 << 29),	// Host Bus Fatal Error Status/Enable
	PORT_INT_HBD	= (1 << 28),	// Host Bus Data Error Status/Enable
	PORT_INT_IF		= (1 << 27),	// Interface Fatal Error Status/Enable
	PORT_INT_INF	= (1 << 26),	// Interface Non-fatal Error Status/Enable
	PORT_INT_OF		= (1 << 24),	// Overflow Status/Enable
	PORT_INT_IPM	= (1 << 23),	// Incorrect Port Multiplier Status/Enable
	PORT_INT_PRC	= (1 << 22),	// PhyRdy Change Status/Enable
	PORT_INT_DI		= (1 << 7),		// Device Interlock Status/Enable
	PORT_INT_PC		= (1 << 6),		// Port Change Status/Enable
	PORT_INT_DP		= (1 << 5),		// Descriptor Processed Interrupt
	PORT_INT_UF		= (1 << 4),		// Unknown FIS Interrupt 
	PORT_INT_SDB	= (1 << 3),		// Set Device Bits FIS Interrupt 
	PORT_INT_DS		= (1 << 2),		// DMA Setup FIS Interrupt 
	PORT_INT_PS		= (1 << 1),		// PIO Setup FIS Interrupt
	PORT_INT_DHR	= (1 << 0),		// Device to Host Register FIS Interrupt
};

#define PORT_INT_ERROR 	(PORT_INT_TFE | PORT_INT_HBF | PORT_INT_HBD \
						 | PORT_INT_IF | PORT_INT_INF | PORT_INT_OF \
						 | PORT_INT_IPM | PORT_INT_PRC | PORT_INT_PC \
						 | PORT_INT_UF)

#define PORT_INT_MASK	(PORT_INT_ERROR | PORT_INT_DP | PORT_INT_SDB \
						 | PORT_INT_DS | PORT_INT_PS | PORT_INT_DHR)

enum {
	ATA_BSY 		= 0x80,
	ATA_DF 			= 0x20,
	ATA_DRQ			= 0x08,
	ATA_ERR			= 0x01,
};


typedef struct {
	uint32		cap;			// Host Capabilities
	uint32		ghc;			// Global Host Control
	uint32		is;				// Interrupt Status
	uint32		pi;				// Ports Implemented
	uint32		vs;				// Version
	uint32		ccc_ctl;		// Command Completion Coalescing Control
	uint32		ccc_ports;		// Command Completion Coalsecing Ports
	uint32		em_loc;			// Enclosure Management Location
	uint32		em_ctl;			// Enclosure Management Control
	uint32		res[31];		// Reserved
	uint32		vendor[24];		// Vendor Specific registers
	ahci_port	port[32];
} _PACKED ahci_hba;


typedef struct {
	uint8		dsfis[0x1c];	// DMA Setup FIS
	uint8		res1[0x04];
	uint8		psfis[0x14];	// PIO Setup FIS
	uint8		res2[0x0c];
	uint8		rfis[0x14];		// D2H Register FIS
	uint8		res3[0x04];
	uint8		sdbfis[0x08];	// Set Device Bits FIS
	uint8		ufis[0x40];		// Unknown FIS
	uint8		res4[0x60];
} _PACKED fis;


typedef struct {
  union {
   struct {
	uint16		cfl : 5;		// command FIS length
	uint16		a : 1;			// ATAPI
	uint16		w : 1;			// Write
	uint16		p : 1;			// Prefetchable
	uint16		r : 1;			// Reset 
	uint16		b : 1;			// Build In Self Test
	uint16		c : 1;			// Clear Busy upon R_OK
	uint16		: 1;
	uint16		pmp : 4;		// Port Multiplier Port
	uint16		prdtl;			// physical region description table length;		
   } _PACKED;
    uint32		prdtl_flags_cfl;
  } _PACKED;
	uint32		prdbc;			// PRD Byte Count
	uint32		ctba;			// command table desciptor base address
								// (alignment 128 byte)
	uint32		ctbau;			// command table desciptor base address upper
	uint8		res1[0x10];
} _PACKED command_list_entry;

#define COMMAND_LIST_ENTRY_COUNT 32


// Command FIS layout - Host to Device (H2D)
//  0 - FIS Type (0x27)
//  1 - C bit (0x80)
//  2 - Command
//  3 - Features
//  4 - Sector Number           (LBA Low, bits 0-7)
//  5 - Cylinder Low            (LBA Mid, bits 8-15)
//  6 - Cylinder High           (LBA High, bits 16-23)
//  7 - Device / Head           (for 28-bit LBA commands, bits 24-27)
//  8 - Sector Number expanded  (LBA Low-previous, bits 24-31)
//  9 - Cylinder Low expanded   (LBA Mid-previous, bits 32-39)
// 10 - Cylinder High expanded  (LBA High-previous, bits 40-47)
// 11 - Features expanded
// 12 - Sector Count            (Sector count, bits 0-7)
// 13 - Sector Count expanded   (Sector count, bits 8-15)
// 14 - Reserved (0)
// 15 - Control
// 16 - Reserved (0)
// 17 - Reserved (0)
// 18 - Reserved (0)
// 19 - Reserved (0)
typedef struct {
	uint8		cfis[0x40];		// command FIS
	uint8		acmd[0x20];		// ATAPI command
	uint8		res[0x20];		// reserved
} _PACKED command_table;


typedef struct {
	uint32		dba;			// Data Base Address (2-byte aligned)
	uint32		dbau;			// Data Base Address Upper
	uint32		res;
	uint32		dbc;			// Bytecount (0-based, even, max 4MB)
	#define DBC_I	0x80000000	/* Interrupt on completition */
} _PACKED prd;

#define PRD_TABLE_ENTRY_COUNT 168
#define PRD_MAX_DATA_LENGTH 0x400000 /* 4 MB */


typedef struct {
	uint16 vendor;
	uint16 device;
	const char *name;
	uint32 flags;
} device_info;

status_t get_device_info(uint16 vendorID, uint16 deviceID, const char **name,
	uint32 *flags);


extern scsi_sim_interface gAHCISimInterface;
extern device_manager_info *gDeviceManager;
extern scsi_for_sim_interface *gSCSI;


#define MAX_SECTOR_LBA_28 ((1ull << 28) - 1)
#define MAX_SECTOR_LBA_48 ((1ull << 48) - 1)


#define LO32(val) ((uint32)(addr_t)(val))
#define HI32(val) ((uint32)(((uint64)(addr_t)(val)) >> 32))
#define ASSERT(expr) if (expr) {} else panic(#expr)

#define PCI_VENDOR_INTEL	0x8086
#define PCI_VENDOR_JMICRON	0x197b
#define PCI_JMICRON_CONTROLLER_CONTROL_1	0x40

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

template <class T>
int count_bits_set(T value)
{
	int count = 0;
	for (T mask = 1; mask; mask <<= 1)
		if (value & mask)
			count++;
	return count;
}

inline
status_t
wait_until_set(volatile uint32 *reg, uint32 bits, bigtime_t timeout)
{
	int trys = (timeout + 9999) / 10000;
	while (trys--) {
		if (((*reg) & bits) == bits)
			return B_OK;
		snooze(10000);
	}
	return B_TIMED_OUT;
}

inline
status_t
wait_until_clear(volatile uint32 *reg, uint32 bits, bigtime_t timeout)
{
	int trys = (timeout + 9999) / 10000;
	while (trys--) {
		if (((*reg) & bits) == 0)
			return B_OK;
		snooze(10000);
	}
	return B_TIMED_OUT;
}

#endif	/* __cplusplus */

#endif	/* _AHCI_DEFS_H */
