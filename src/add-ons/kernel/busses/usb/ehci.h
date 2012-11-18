/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef EHCI_H
#define EHCI_H

#include "usb_private.h"
#include "ehci_hardware.h"


struct pci_info;
struct pci_module_info;
class EHCIRootHub;


typedef struct transfer_data {
	Transfer *		transfer;
	ehci_qh *		queue_head;
	ehci_qtd *		data_descriptor;
	bool			incoming;
	bool			canceled;
	transfer_data *	link;
} transfer_data;


// This structure is used to create a list of
// descriptors per isochronous transfer
typedef struct isochronous_transfer_data {
	Transfer *					transfer;
	// The next field is used to keep track
	// of every isochronous descriptor as they are NOT
	// linked to each other in a queue like in every other
	// transfer type
	ehci_itd **					descriptors;
	uint16						last_to_process;
	bool						incoming;
	bool						is_active;
	isochronous_transfer_data *	link;

	size_t						buffer_size;
	void *						buffer_log;
	addr_t						buffer_phy;
} isochronous_transfer_data;


class EHCI : public BusManager {
public:
									EHCI(pci_info *info, Stack *stack);
									~EHCI();

		status_t					Start();
virtual	status_t					SubmitTransfer(Transfer *transfer);
virtual	status_t					CancelQueuedTransfers(Pipe *pipe, bool force);
		status_t					CancelQueuedIsochronousTransfers(Pipe *pipe, bool force);
		status_t					SubmitIsochronous(Transfer *transfer);

virtual	status_t					NotifyPipeChange(Pipe *pipe,
										usb_change change);

static	status_t					AddTo(Stack *stack);

		// Port operations for root hub
		uint8						PortCount() { return fPortCount; };
		status_t					GetPortStatus(uint8 index, usb_port_status *status);
		status_t					SetPortFeature(uint8 index, uint16 feature);
		status_t					ClearPortFeature(uint8 index, uint16 feature);

		status_t					ResetPort(uint8 index);
		status_t					SuspendPort(uint8 index);

virtual	const char *				TypeName() const { return "ehci"; };

private:
		// Controller resets
		status_t					ControllerReset();
		status_t					LightReset();

		// Interrupt functions
static	int32						InterruptHandler(void *data);
		int32						Interrupt();
static	int32						InterruptPollThread(void *data);

		// Transfer management
		status_t					AddPendingTransfer(Transfer *transfer,
										ehci_qh *queueHead,
										ehci_qtd *dataDescriptor,
										bool directionIn);
		status_t					AddPendingIsochronousTransfer(
										Transfer *transfer,
										ehci_itd **isoRequest, uint32 lastIndex,
										bool directionIn, addr_t bufferPhy,
										void *bufferLog, size_t bufferSize);
		status_t					CancelAllPendingTransfers();


static	int32						FinishThread(void *data);
		void						FinishTransfers();
static	int32						CleanupThread(void *data);
		void						Cleanup();

		// Isochronous transfer functions
static int32						FinishIsochronousThread(void *data);
		void						FinishIsochronousTransfers();
		isochronous_transfer_data *	FindIsochronousTransfer(ehci_itd *itd);
		void						LinkITDescriptors(ehci_itd *itd,
										ehci_itd **last);
		void						LinkSITDescriptors(ehci_sitd *sitd,
										ehci_sitd **last);
		void						UnlinkITDescriptors(ehci_itd *itd,
										ehci_itd **last);
		void						UnlinkSITDescriptors(ehci_sitd *sitd,
										ehci_sitd **last);

		// Queue Head functions
		ehci_qh *					CreateQueueHead();
		status_t					InitQueueHead(ehci_qh *queueHead,
										Pipe *pipe);
		void						FreeQueueHead(ehci_qh *queueHead);

		status_t					LinkQueueHead(ehci_qh *queueHead);
		status_t					LinkInterruptQueueHead(ehci_qh *queueHead,
										Pipe *pipe);
		status_t					UnlinkQueueHead(ehci_qh *queueHead,
										ehci_qh **freeList);

		// Queue functions
		status_t					FillQueueWithRequest(Transfer *transfer,
										ehci_qh *queueHead,
										ehci_qtd **dataDescriptor,
										bool *directionIn);
		status_t					FillQueueWithData(Transfer *transfer,
										ehci_qh *queueHead,
										ehci_qtd **dataDescriptor,
										bool *directionIn);

		bool						LockIsochronous();
		void						UnlockIsochronous();

		// Descriptor functions
		ehci_qtd *					CreateDescriptor(
										size_t bufferSizeToAllocate,
										uint8 pid);
		status_t					CreateDescriptorChain(Pipe *pipe,
										ehci_qtd **firstDescriptor,
										ehci_qtd **lastDescriptor,
										ehci_qtd *strayDescriptor,
										size_t bufferSizeToAllocate,
										uint8 pid);
		ehci_itd*					CreateItdDescriptor();
		ehci_sitd*					CreateSitdDescriptor();

		void						FreeDescriptor(ehci_qtd *descriptor);
		void						FreeDescriptorChain(ehci_qtd *topDescriptor);
		void						FreeDescriptor(ehci_itd *descriptor);
		void						FreeDescriptor(ehci_sitd *descriptor);
		void						FreeIsochronousData(
										isochronous_transfer_data *data);

		void						LinkDescriptors(ehci_qtd *first,
										ehci_qtd *last, ehci_qtd *alt);

		size_t						WriteDescriptorChain(
										ehci_qtd *topDescriptor,
										iovec *vector, size_t vectorCount);
		size_t						ReadDescriptorChain(ehci_qtd *topDescriptor,
										iovec *vector, size_t vectorCount,
										bool *nextDataToggle);
		size_t						ReadActualLength(ehci_qtd *topDescriptor,
										bool *nextDataToggle);
		size_t						WriteIsochronousDescriptorChain(
										isochronous_transfer_data *transfer,
										uint32 packetCount,
										iovec *vector);
		size_t						ReadIsochronousDescriptorChain(
										isochronous_transfer_data *transfer);

		// Operational register functions
inline	void						WriteOpReg(uint32 reg, uint32 value);
inline	uint32						ReadOpReg(uint32 reg);

		// Capability register functions
inline	uint8						ReadCapReg8(uint32 reg);
inline	uint16						ReadCapReg16(uint32 reg);
inline	uint32						ReadCapReg32(uint32 reg);

static	pci_module_info *			sPCIModule;

		uint8 *						fCapabilityRegisters;
		uint8 *						fOperationalRegisters;
		area_id						fRegisterArea;
		pci_info *					fPCIInfo;
		Stack *						fStack;
		uint32						fEnabledInterrupts;
		uint32						fThreshold;

		// Periodic transfer framelist and interrupt entries
		area_id						fPeriodicFrameListArea;
		uint32 *					fPeriodicFrameList;
		interrupt_entry *			fInterruptEntries;
		ehci_itd **					fItdEntries;
		ehci_sitd **				fSitdEntries;

		// Async transfer queue management
		ehci_qh *					fAsyncQueueHead;
		sem_id						fAsyncAdvanceSem;

		// Maintain a linked list of transfers
		transfer_data *				fFirstTransfer;
		transfer_data *				fLastTransfer;
		sem_id						fFinishTransfersSem;
		thread_id					fFinishThread;
		Pipe *						fProcessingPipe;

		ehci_qh *					fFreeListHead;
		sem_id						fCleanupSem;
		thread_id					fCleanupThread;
		bool						fStopThreads;
		int32						fNextStartingFrame;

		// fFrameBandwidth[n] holds the available bandwidth
		// of the nth frame in microseconds
		uint16 *					fFrameBandwidth;

		// Maintain a linked list of isochronous transfers
		isochronous_transfer_data *	fFirstIsochronousTransfer;
		isochronous_transfer_data *	fLastIsochronousTransfer;
		sem_id						fFinishIsochronousTransfersSem;
		thread_id					fFinishIsochronousThread;
		mutex						fIsochronousLock;

		// Root Hub
		EHCIRootHub *				fRootHub;
		uint8						fRootHubAddress;

		// Port management
		uint8						fPortCount;
		uint16						fPortResetChange;
		uint16						fPortSuspendChange;

		// Interrupt polling
		thread_id					fInterruptPollThread;
};


class EHCIRootHub : public Hub {
public:
									EHCIRootHub(Object *rootObject,
										int8 deviceAddress);

static	status_t					ProcessTransfer(EHCI *ehci,
										Transfer *transfer);
};


#endif // !EHCI_H
