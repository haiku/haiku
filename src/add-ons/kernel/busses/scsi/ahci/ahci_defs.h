/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AHCI_DEFS_H
#define _AHCI_DEFS_H

#include <bus/PCI.h>
#include <bus/SCSI.h>

#define AHCI_DEVICE_MODULE_NAME "busses/scsi/ahci/device_v1"
#define AHCI_SIM_MODULE_NAME "busses/scsi/ahci/sim/v1"

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
	GHC_HR			= (1 << 0),		// HBA Reset
};


typedef struct {
	uint32		clb;			// Command List Base Address
	uint32		clbu;			// Command List Base Address Upper 32-Bits
	uint32		fb;				// FIS Base Address
	uint32		fbu;			// FIS Base Address Upper 32-Bits
	uint32		is;				// Interrupt Status
	uint32		ie;				// Interrupt Enable
	uint32		cmd;			// Command and Status
	uint32 		res1;			// Reserved
	uint32		tfd;			// Task File Data
	uint32		sig;			// Signature
	uint32		ssts;			// Serial ATA Status (SCR0: SStatus)
	uint32		sctl;			// Serial ATA Control (SCR2: SControl)
	uint32		serr;			// Serial ATA Error (SCR1: SError)
	uint32		sact;			// Serial ATA Active (SCR3: SActive)
	uint32		ci;				// Command Issue
	uint32		sntf;			// SNotification
	uint32		res2;			// Reserved for FIS-based Switching Definition
	uint32		res[11];		// Reserved
	uint32		vendor[2];		// Vendor Specific
} ahci_port;


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
} ahci_hba;


extern scsi_sim_interface gAHCISimInterface;
extern device_manager_info *gDeviceManager;
extern pci_device_module_info *gPCI;
extern scsi_for_sim_interface *gSCSI;

#endif	/* _AHCI_DEFS_H */
