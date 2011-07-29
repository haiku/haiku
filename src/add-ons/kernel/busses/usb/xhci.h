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


#define MAX_EVENTS		(16 * 13)
#define MAX_COMMANDS		(16 * 1)
#define XHCI_MAX_SLOTS		256
#define XHCI_MAX_PORTS		127


struct pci_info;
struct pci_module_info;
class XHCIRootHub;


struct xhci_trb {
	uint64	qwtrb0;
	uint32	dwtrb2;
	uint32	dwtrb3;
};


struct xhci_segment {
	xhci_trb *		trbs;
	xhci_segment *	next;
};


struct xhci_ring {
	xhci_segment *	first_seg;
	xhci_trb *		enqueue;
	xhci_trb *		dequeue;
};


// Section 6.5
struct xhci_erst_element {
	uint64	rs_addr;
	uint32	rs_size;
	uint32	rsvdz;
} __attribute__((__aligned__(64)));


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
			uint8				PortCount() { return fPortCount; };
			status_t			GetPortStatus(uint8 index, usb_port_status *status);
			status_t			SetPortFeature(uint8 index, uint16 feature);
			status_t			ClearPortFeature(uint8 index, uint16 feature);

	virtual	const char *		TypeName() const { return "xhci"; };

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
			
			//Doorbell
			void				Ring();

			//no-op
			void				QueueNoop();
	static	int32				CmdCompThread(void *data);
			void				CmdComplete();

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
			uint8 *				fDcba;

			spinlock			fSpinlock;

			sem_id				fCmdCompSem;
			thread_id			fCmdCompThread;
			sem_id				fFinishTransfersSem;
			thread_id			fFinishThread;
			bool				fStopThreads;

			// Root Hub
			XHCIRootHub *		fRootHub;
			uint8				fRootHubAddress;

			// Port management
			uint8				fPortCount;
			uint8				fSlotCount;

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
