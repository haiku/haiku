/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#ifndef EHCI_H
#define EHCI_H

#define TRACE_USB
#include "usb_p.h"
#include "ehci_hardware.h"


struct pci_info;
struct pci_module_info;
class EHCIRootHub;


typedef struct transfer_data_s {
	Transfer		*transfer;
	ehci_qh			*queue_head;
	ehci_qtd		*first_descriptor;
	ehci_qtd		*data_descriptor;
	ehci_qtd		*last_descriptor;
	bool			incoming;
	transfer_data_s	*link;
} transfer_data;


class EHCI : public BusManager {
public:
									EHCI(pci_info *info, Stack *stack);
									~EHCI();

		status_t					Start();
virtual	status_t					SubmitTransfer(Transfer *transfer);

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
										ehci_qtd *firstDescriptor,
										ehci_qtd *dataDescriptor,
										ehci_qtd *lastDescriptor,
										bool directionIn);
		status_t					CancelPendingTransfer(Transfer *transfer);
		status_t					CancelAllPendingTransfers();

static	int32						FinishThread(void *data);
		void						FinishTransfers();

		// Descriptor functions
		ehci_qtd					*CreateDescriptor(Pipe *pipe,
										size_t bufferSizeToAllocate);
		status_t					CreateDescriptorChain(Pipe *pipe,
										ehci_qtd **firstDescriptor,
										ehci_qtd **lastDescriptor,
										size_t bufferSizeToAllocate);

		void						FreeDescriptor(ehci_qtd *descriptor);
		void						FreeDescriptorChain(ehci_qtd *topDescriptor);

		void						LinkDescriptors(ehci_qtd *first,
										ehci_qtd *last);

		size_t						WriteDescriptorChain(ehci_qtd *topDescriptor,
										iovec *vector, size_t vectorCount);
		size_t						ReadDescriptorChain(ehci_qtd *topDescriptor,
										iovec *vector, size_t vectorCount,
										uint8 *lastDataToggle);
		size_t						ReadActualLength(ehci_qtd *topDescriptor,
										uint8 *lastDataToggle);

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

		// Maintain a linked list of transfers
		transfer_data				*fFirstTransfer;
		transfer_data				*fLastTransfer;
		bool						fFinishTransfers;
		thread_id					fFinishThread;
		bool						fStopFinishThread;

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
