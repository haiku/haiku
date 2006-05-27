/*
 * Copyright 2004-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#ifndef UHCI_H
#define UHCI_H

#include "usb_p.h"
#include "uhci_hardware.h"

struct pci_info;
struct pci_module_info;
class UHCIRootHub;


class UHCI : public BusManager {
public:
									UHCI(pci_info *info, Stack *stack);

		status_t					Start();
		status_t					SubmitTransfer(Transfer *transfer);

static	bool						AddTo(Stack &stack);

private:
friend	class UHCIRootHub;

		// Utility functions
		void						GlobalReset();
		status_t					Reset();
static	int32						InterruptHandler(void *data);
		int32						Interrupt();

		// Register functions
inline	void						WriteReg16(uint32 reg, uint16 value);
inline	void						WriteReg32(uint32 reg, uint32 value);
inline	uint16						ReadReg16(uint32 reg);
inline	uint32						ReadReg32(uint32 reg);

		// Functions for the actual functioning of transfers
		status_t					InsertControl(Transfer *transfer);

static	pci_module_info				*sPCIModule; 

		uint32						fRegisterBase;
		pci_info					*fPCIInfo;
		Stack						*fStack;

		// Frame list memory
		area_id						fFrameArea;
		addr_t						*fFrameList;

		// Virtual frame
		uhci_qh						*fVirtualQueueHead[12];

#define fQueueHeadInterrupt256		fVirtualQueueHead[0]
#define fQueueHeadInterrupt128		fVirtualQueueHead[1]
#define fQueueHeadInterrupt64		fVirtualQueueHead[2]
#define fQueueHeadInterrupt32		fVirtualQueueHead[3]
#define fQueueHeadInterrupt16		fVirtualQueueHead[4]
#define fQueueHeadInterrupt8		fVirtualQueueHead[5]
#define fQueueHeadInterrupt4		fVirtualQueueHead[6]
#define fQueueHeadInterrupt2		fVirtualQueueHead[7]
#define fQueueHeadInterrupt1		fVirtualQueueHead[8]
#define fQueueHeadControl			fVirtualQueueHead[9]
#define fQueueHeadBulk				fVirtualQueueHead[10]
#define fQueueHeadTerminate			fVirtualQueueHead[11]

		// Maintain a list of transfers
		Vector<Transfer *>			fTransfers;

		// Root hub
		UHCIRootHub					*fRootHub;
		uint8						fRootHubAddress;
};


class UHCIRootHub : public Hub {
public:
									UHCIRootHub(UHCI *uhci, int8 deviceNum);

		status_t					SubmitTransfer(Transfer *transfer);
		void						UpdatePortStatus();

private:

		usb_port_status				fPortStatus[2];
		UHCI						*fUHCI;
};


struct hostcontroller_priv {
	uhci_qh		*topqh;
	uhci_td		*firsttd;
	uhci_td		*lasttd;
};

#endif
