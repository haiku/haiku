/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _AHCI_H
#define _AHCI_H

typedef struct
{
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


typedef struct
{
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

#endi
