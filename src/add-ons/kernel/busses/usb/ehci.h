/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#ifndef EHCI_H
#define EHCI_H

#include "usb_p.h"
#include "ehci_hardware.h"


struct pci_info;
struct pci_module_info;
class EHCIRootHub;


typedef struct transfer_data_s {
	Transfer		*transfer;
	ehci_qh			*queue_head;
	ehci_qtd		*data_descriptor;
	area_id			user_area;
	bool			incoming;
	transfer_data_s	*link;
} transfer_data;


class EHCI : public BusManager {
public:
									EHCI(pci_info *info, Stack *stack);
									~EHCI();

		status_t					Start();
virtual	status_t					SubmitTransfer(Transfer *transfer);
		status_t					SubmitPeriodicTransfer(Transfer *transfer);
		status_t					SubmitAsyncTransfer(Transfer *transfer);

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

private:
		// Controller resets
		status_t					ControllerReset();
		status_t					LightReset();

		// Interrupt functions
static	int32						InterruptHandler(void *data);
		int32						Interrupt();

		// Transfer management
		status_t					AddPendingTransfer(Transfer *transfer,
										ehci_qh *queueHead,
										ehci_qtd *dataDescriptor,
										bool directionIn);
		status_t					CancelPendingTransfer(Transfer *transfer);
		status_t					CancelAllPendingTransfers();

static	int32						FinishThread(void *data);
		void						FinishTransfers();
static	int32						CleanupThread(void *data);
		void						Cleanup();

		// Queue Head functions
		ehci_qh						*CreateQueueHead();
		void						FreeQueueHead(ehci_qh *queueHead);

		status_t					LinkQueueHead(ehci_qh *queueHead);
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

		// Descriptor functions
		ehci_qtd					*CreateDescriptor(size_t bufferSizeToAllocate,
										uint8 pid);
		status_t					CreateDescriptorChain(Pipe *pipe,
										ehci_qtd **firstDescriptor,
										ehci_qtd **lastDescriptor,
										ehci_qtd *strayDescriptor,
										size_t bufferSizeToAllocate,
										uint8 pid);

		void						FreeDescriptor(ehci_qtd *descriptor);
		void						FreeDescriptorChain(ehci_qtd *topDescriptor);

		void						LinkDescriptors(ehci_qtd *first,
										ehci_qtd *last, ehci_qtd *alt);

		size_t						WriteDescriptorChain(ehci_qtd *topDescriptor,
										iovec *vector, size_t vectorCount);
		size_t						ReadDescriptorChain(ehci_qtd *topDescriptor,
										iovec *vector, size_t vectorCount,
										bool *nextDataToggle);
		size_t						ReadActualLength(ehci_qtd *topDescriptor,
										bool *nextDataToggle);

		// Operational register functions
inline	void						WriteOpReg(uint32 reg, uint32 value);
inline	uint32						ReadOpReg(uint32 reg);

		// Capability register functions
inline	uint8						ReadCapReg8(uint32 reg);
inline	uint16						ReadCapReg16(uint32 reg);
inline	uint32						ReadCapReg32(uint32 reg);

static	pci_module_info				*sPCIModule;

		uint8						*fCapabilityRegisters;
		uint8						*fOperationalRegisters;
		area_id						fRegisterArea;
		pci_info					*fPCIInfo;
		Stack						*fStack;

		// Framelist memory
		area_id						fPeriodicFrameListArea;
		addr_t						*fPeriodicFrameList;

		// Async frame list management
		ehci_qh						*fAsyncQueueHead;
		sem_id						fAsyncAdvanceSem;

		// Maintain a linked list of transfers
		transfer_data				*fFirstTransfer;
		transfer_data				*fLastTransfer;
		sem_id						fFinishTransfersSem;
		thread_id					fFinishThread;
		sem_id						fCleanupSem;
		thread_id					fCleanupThread;
		bool						fStopThreads;
		ehci_qh						*fFreeListHead;

		// Root Hub
		EHCIRootHub					*fRootHub;
		uint8						fRootHubAddress;

		// Port management
		uint8						fPortCount;
		uint16						fPortResetChange;
		uint16						fPortSuspendChange;
};


class EHCIRootHub : public Hub {
public:
									EHCIRootHub(Object *rootObject,
										int8 deviceAddress);

static	status_t					ProcessTransfer(EHCI *ehci,
										Transfer *transfer);
};


#endif // !EHCI_H
