/*
 * Copyright 2004-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */

#ifndef UHCI_H
#define UHCI_H

#include "usb_p.h"
#include "uhci_hardware.h"
#include <lock.h>

#define UHCI_INTERRUPT_QUEUE				0
#define UHCI_LOW_SPEED_CONTROL_QUEUE		1
#define UHCI_FULL_SPEED_CONTROL_QUEUE		2
#define UHCI_BULK_QUEUE						3
#define UHCI_BANDWIDTH_RECLAMATION_QUEUE	4

struct pci_info;
struct pci_module_info;
class UHCIRootHub;


class Queue {
public:
									Queue(Stack *stack);
									~Queue();

		bool						Lock();
		void						Unlock();

		status_t					InitCheck();

		status_t					LinkTo(Queue *other);
		status_t					TerminateByStrayDescriptor();

		status_t					AppendTransfer(uhci_qh *transfer);
		status_t					RemoveTransfer(uhci_qh *transfer);

		addr_t						PhysicalAddress();

		void						PrintToStream();

private:
		status_t					fStatus;
		Stack						*fStack;
		uhci_qh						*fQueueHead;
		uhci_td						*fStrayDescriptor;
		uhci_qh						*fQueueTop;
		benaphore					fLock;
};


typedef struct transfer_data_s {
	Transfer		*transfer;
	Queue			*queue;
	uhci_qh			*transfer_queue;
	uhci_td			*first_descriptor;
	uhci_td			*data_descriptor;
	bool			incoming;
	bool			canceled;
	transfer_data_s	*link;
} transfer_data;


// This structure is used to create a list of
// descriptors per isochronous transfer
typedef struct isochronous_transfer_data_s {
	Transfer					*transfer;
	// The next field is used to keep track
	// of every isochronous descriptor as they are NOT
	// linked to each other in a queue like in every other
	// transfer type
	uhci_td						**descriptors;
	uint16						last_to_process;
	bool						incoming;
	bool						is_active;
	isochronous_transfer_data_s	*link;
} isochronous_transfer_data;


class UHCI : public BusManager {
public:
									UHCI(pci_info *info, Stack *stack);
									~UHCI();

		status_t					Start();
virtual	status_t					SubmitTransfer(Transfer *transfer);
virtual	status_t					CancelQueuedTransfers(Pipe *pipe, bool force);
		status_t					CancelQueuedIsochronousTransfers(Pipe *pipe, bool force);
		status_t					SubmitRequest(Transfer *transfer);
		status_t					SubmitIsochronous(Transfer *transfer);

static	status_t					AddTo(Stack *stack);

		// Port operations
		status_t					GetPortStatus(uint8 index, usb_port_status *status);
		status_t					SetPortFeature(uint8 index, uint16 feature);
		status_t					ClearPortFeature(uint8 index, uint16 feature);

		status_t					ResetPort(uint8 index);

private:
		// Controller resets
		void						GlobalReset();
		status_t					ControllerReset();

		// Interrupt functions
static	int32						InterruptHandler(void *data);
		int32						Interrupt();

		// Transfer functions
		status_t					AddPendingTransfer(Transfer *transfer,
										Queue *queue,
										uhci_qh *transferQueue,
										uhci_td *firstDescriptor,
										uhci_td *dataDescriptor,
										bool directionIn);
		status_t					AddPendingIsochronousTransfer(
										Transfer *transfer,
										uhci_td **isoRequest,
										bool directionIn);

static	int32						FinishThread(void *data);
		void						FinishTransfers();

		status_t					CreateFilledTransfer(Transfer *transfer,
										uhci_td **_firstDescriptor,
										uhci_qh **_transferQueue);

		// Isochronous transfer functions
static int32						FinishIsochronousThread(void *data);
		void						FinishIsochronousTransfers();
		isochronous_transfer_data	*FindIsochronousTransfer(uhci_td *descriptor);

		status_t					LinkIsochronousDescriptor(
										uhci_td *descriptor,
										uint16 frame);
		uhci_td						*UnlinkIsochronousDescriptor(uint16 frame);

		// Transfer queue functions
		uhci_qh						*CreateTransferQueue(uhci_td *descriptor);
		void						FreeTransferQueue(uhci_qh *queueHead);

		bool						LockIsochronous();
		void						UnlockIsochronous();

		// Descriptor functions
		uhci_td						*CreateDescriptor(Pipe *pipe,
										uint8 direction,
										size_t bufferSizeToAllocate);
		status_t					CreateDescriptorChain(Pipe *pipe,
										uhci_td **firstDescriptor,
										uhci_td **lastDescriptor,
										uint8 direction,
										size_t bufferSizeToAllocate);

		void						FreeDescriptor(uhci_td *descriptor);
		void						FreeDescriptorChain(uhci_td *topDescriptor);

		void						LinkDescriptors(uhci_td *first,
										uhci_td *second);

		size_t						WriteDescriptorChain(uhci_td *topDescriptor,
										iovec *vector, size_t vectorCount);
		size_t						ReadDescriptorChain(uhci_td *topDescriptor,
										iovec *vector, size_t vectorCount,
										uint8 *lastDataToggle);
		size_t						ReadActualLength(uhci_td *topDescriptor,
										uint8 *lastDataToggle);
		void						WriteIsochronousDescriptorChain(
										uhci_td **isoRequest,
										uint32 packetCount,
										iovec *vector);
		void						ReadIsochronousDescriptorChain(
										isochronous_transfer_data *transfer,
										iovec *vector);

		// Register functions
inline	void						WriteReg8(uint32 reg, uint8 value);
inline	void						WriteReg16(uint32 reg, uint16 value);
inline	void						WriteReg32(uint32 reg, uint32 value);
inline	uint8						ReadReg8(uint32 reg);
inline	uint16						ReadReg16(uint32 reg);
inline	uint32						ReadReg32(uint32 reg);

static	pci_module_info				*sPCIModule; 

		uint32						fRegisterBase;
		pci_info					*fPCIInfo;
		Stack						*fStack;

		// Frame list memory
		area_id						fFrameArea;
		uint32						*fFrameList;

		// fFrameBandwidth[n] holds the available bandwidth
		// of the nth frame in microseconds
		uint16						*fFrameBandwidth;

		// fFirstIsochronousTransfer[n] and fLastIsochronousDescriptor[n]
		// keeps track of the first and last isochronous transfer descriptor
		// in the nth frame
		uhci_td						**fFirstIsochronousDescriptor;
		uhci_td						**fLastIsochronousDescriptor;

		// Queues
		int32						fQueueCount;
		Queue						**fQueues;

		// Maintain a linked list of transfers
		transfer_data				*fFirstTransfer;
		transfer_data				*fLastTransfer;
		sem_id						fFinishTransfersSem;
		thread_id					fFinishThread;
		bool						fStopFinishThread;

		// Maintain a linked list of isochronous transfers
		isochronous_transfer_data	*fFirstIsochronousTransfer;
		isochronous_transfer_data	*fLastIsochronousTransfer;
		sem_id						fFinishIsochronousTransfersSem;
		thread_id					fFinishIsochronousThread;
		benaphore					fIsochronousLock;
		bool						fStopFinishIsochronousThread;

		// Root hub
		UHCIRootHub					*fRootHub;
		uint8						fRootHubAddress;
		uint8						fPortResetChange;

};


class UHCIRootHub : public Hub {
public:
									UHCIRootHub(Object *rootObject,
										int8 deviceAddress);

static	status_t					ProcessTransfer(UHCI *uhci,
										Transfer *transfer);
};


#endif
