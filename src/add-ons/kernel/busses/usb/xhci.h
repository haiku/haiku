/*
 * Copyright 2006-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Jian Chiang <j.jian.chiang@gmail.com>
 *		Jérôme Duval <jerome.duval@gmail.com>
 */
#ifndef XHCI_H
#define XHCI_H


#include "usb_private.h"
#include "xhci_hardware.h"


struct pci_info;
struct pci_module_info;
class XHCIRootHub;


enum xhci_state {
	XHCI_STATE_DISABLED = 0,
	XHCI_STATE_ENABLED,
	XHCI_STATE_ADDRESSED,
	XHCI_STATE_CONFIGURED,
};


struct xhci_device {
	enum xhci_state state;
	area_id trb_area;
	addr_t trb_addr;
	struct xhci_trb (*trbs)[XHCI_MAX_ENDPOINTS - 1][XHCI_MAX_TRANSFERS];

	area_id input_ctx_area;
	addr_t input_ctx_addr;
	struct xhci_input_device_ctx *input_ctx;

	area_id device_ctx_area;
	addr_t device_ctx_addr;
	struct xhci_device_ctx *device_ctx;
};


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

	virtual	Device *			AllocateDevice(Hub *parent,
									int8 hubAddress, uint8 hubPort,
									usb_speed speed);
			status_t			ConfigureEndpoint(uint8 slot, uint8 number,
									uint8 type, uint64 ringAddr,
									uint16 interval, uint8 maxPacketCount,
									uint8 mult, uint8 fpsShift,
									uint16 maxPacketSize, uint16 maxFrameSize);
	virtual	void				FreeDevice(Device *device);

			// Port operations for root hub
			uint8				PortCount() const { return fPortCount; }
			status_t			GetPortStatus(uint8 index,
									usb_port_status *status);
			status_t			SetPortFeature(uint8 index, uint16 feature);
			status_t			ClearPortFeature(uint8 index, uint16 feature);

			status_t			GetPortSpeed(uint8 index, usb_speed *speed);

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
			usb_speed			fPortSpeeds[XHCI_MAX_PORTS];
			uint8				fPortSlots[XHCI_MAX_PORTS];

			// Scratchpad
			uint8				fScratchpadCount;
			area_id				fScratchpadArea[XHCI_MAX_SCRATCHPADS];
			void *				fScratchpad[XHCI_MAX_SCRATCHPADS];

			// Devices
			struct xhci_device	fDevices[XHCI_MAX_DEVICES];

			uint16				fEventIdx;
			uint16				fCmdIdx;
			uint8				fEventCcs;
			uint8				fCmdCcs;

			uint32				fExitLatMax;
};


class XHCIRootHub : public Hub {
public:
									XHCIRootHub(Object *rootObject,
										int8 deviceAddress);

static	status_t					ProcessTransfer(XHCI *ehci,
										Transfer *transfer);
};


#endif // !XHCI_H
