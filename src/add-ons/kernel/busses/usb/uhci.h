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


class Queue {
public:
									Queue(Stack *stack);
									~Queue();

		status_t					InitCheck();

		status_t					LinkTo(Queue *other);
		status_t					TerminateByStrayDescriptor();

		status_t					AppendDescriptor(uhci_td *descriptor);
		status_t					AddTransfer(Transfer *transfer,
										bigtime_t timeout);
		status_t					RemoveInactiveDescriptors();

		addr_t						PhysicalAddress();

private:
		status_t					fStatus;
		Stack						*fStack;
		uhci_qh						*fQueueHead;
		uhci_td						*fStrayDescriptor;
		uhci_td						*fQueueTop;
};


class UHCI : public BusManager {
public:
									UHCI(pci_info *info, Stack *stack);

		status_t					Start();
		status_t					SubmitTransfer(Transfer *transfer,
										bigtime_t timeout = 0);

static	bool						AddTo(Stack &stack);

		void						GlobalReset();
		status_t					ResetController();

		// port operations
		uint16						PortStatus(int32 index);
		status_t					SetPortStatus(int32 index, uint16 status);
		status_t					ResetPort(int32 index);

private:
		// Utility functions
static	int32						InterruptHandler(void *data);
		int32						Interrupt();

		// Register functions
inline	void						WriteReg16(uint32 reg, uint16 value);
inline	void						WriteReg32(uint32 reg, uint32 value);
inline	uint16						ReadReg16(uint32 reg);
inline	uint32						ReadReg32(uint32 reg);

static	pci_module_info				*sPCIModule; 

		uint32						fRegisterBase;
		pci_info					*fPCIInfo;
		Stack						*fStack;

		// Frame list memory
		area_id						fFrameArea;
		addr_t						*fFrameList;

		// Queues
		Queue						*fQueues[4];

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
