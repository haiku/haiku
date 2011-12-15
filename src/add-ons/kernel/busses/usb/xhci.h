/*
 * Copyright 2006-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Jian Chiang <j.jian.chiang@gmail.com>
 */
#ifndef XHCI_H
#define XHCI_H


#include "usb_private.h"
#include "xhci_hardware.h"


struct pci_info;
struct pci_module_info;
class XHCIRootHub;


class XHCI : public BusManager {
public:
								XHCI(pci_info *info, Stack *stack);
								~XHCI();

			status_t			Start();
	virtual	status_t			SubmitTransfer(Transfer *transfer);
	virtual	status_t			CancelQueuedTransfers(Pipe *pipe, bool force);

	virtual	status_t			NotifyPipeChange(Pipe *pipe,
									usb_change change);

	static	status_t			AddTo(Stack *stack);

			// Port operations for root hub
			uint8				PortCount() const { return fPortCount; }
			status_t			GetPortStatus(uint8 index, usb_port_status *status);
			status_t			SetPortFeature(uint8 index, uint16 feature);
			status_t			ClearPortFeature(uint8 index, uint16 feature);

	virtual	const char *		TypeName() const { return "xhci"; }

private:
			// Controller resets
			status_t			ControllerReset();
			status_t			ControllerHalt();

			// Interrupt functions
	static	int32				InterruptHandler(void *data);
			int32				Interrupt();

			// Transfer management
	static	int32				FinishThread(void *data);
			void				FinishTransfers();

			// Command
			void				QueueCommand(xhci_trb *trb);
			void				HandleCmdComplete(xhci_trb *trb);
			status_t			DoCommand(xhci_trb *trb);
			//Doorbell
			void				Ring();

			// Commands
			status_t			Noop();
			status_t			EnableSlot(uint8 *slot);
			status_t			DisableSlot(uint8 slot);
			status_t			SetAddress(uint64 inputContext, bool bsr,
									uint8 slot);
			status_t			ConfigureEndpoint(uint64 inputContext,
									bool deconfigure, uint8 slot);
			status_t			EvaluateContext(uint64 inputContext,
									uint8 slot);
			status_t			ResetEndpoint(bool preserve, uint8 endpoint,
									uint8 slot);
			status_t			StopEndpoint(bool suspend, uint8 endpoint,
									uint8 slot);
			status_t			SetTRDequeue(uint64 dequeue, uint16 stream,
									uint8 endpoint, uint8 slot);
			status_t			ResetDevice(uint8 slot);

			// Operational register functions
	inline	void				WriteOpReg(uint32 reg, uint32 value);
	inline	uint32				ReadOpReg(uint32 reg);

			// Capability register functions
	inline	uint8				ReadCapReg8(uint32 reg);
	inline	uint16				ReadCapReg16(uint32 reg);
	inline	uint32				ReadCapReg32(uint32 reg);
	inline	void				WriteCapReg32(uint32 reg, uint32 value);

			// Runtime register functions
	inline	uint32				ReadRunReg32(uint32 reg);
	inline	void				WriteRunReg32(uint32 reg, uint32 value);

			// Doorbell register functions
	inline	uint32				ReadDoorReg32(uint32 reg);
	inline	void				WriteDoorReg32(uint32 reg, uint32 value);

	static	pci_module_info *	sPCIModule;

			uint8 *				fCapabilityRegisters;
			uint8 *				fOperationalRegisters;
			uint8 *				fRuntimeRegisters;
			uint8 *				fDoorbellRegisters;
			area_id				fRegisterArea;
			pci_info *			fPCIInfo;
			Stack *				fStack;

			area_id				fErstArea;
			xhci_erst_element *	fErst;
			xhci_trb *			fEventRing;
			xhci_trb *			fCmdRing;
			uint64				fCmdAddr;
			uint32				fCmdResult[2];

			area_id				fDcbaArea;
			struct xhci_device_context_array * fDcba;

			spinlock			fSpinlock;

			sem_id				fCmdCompSem;
			sem_id				fFinishTransfersSem;
			thread_id			fFinishThread;
			bool				fStopThreads;

			// Root Hub
			XHCIRootHub *		fRootHub;
			uint8				fRootHubAddress;

			// Port management
			uint8				fPortCount;
			uint8				fSlotCount;

			// Scratchpad
			uint8				fScratchpadCount;
			area_id				fScratchpadArea[XHCI_MAX_SCRATCHPADS];
			void *				fScratchpad[XHCI_MAX_SCRATCHPADS];

			uint16				fEventIdx;
			uint16				fCmdIdx;
			uint8				fEventCcs;
			uint8				fCmdCcs;
};


class XHCIRootHub : public Hub {
public:
									XHCIRootHub(Object *rootObject,
										int8 deviceAddress);

static	status_t					ProcessTransfer(XHCI *ehci,
										Transfer *transfer);
};


#endif // !XHCI_H
