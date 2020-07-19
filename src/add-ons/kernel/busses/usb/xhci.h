/*
 * Copyright 2011-2019, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
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
struct pci_x86_module_info;
struct xhci_td;
struct xhci_device;
struct xhci_endpoint;
class XHCIRootHub;


/* Each transfer requires 2 TRBs on the endpoint "ring" (one for the link TRB,
 * and one for the Event Data TRB), plus one more at the end for the link TRB
 * to the start. */
#define XHCI_ENDPOINT_RING_SIZE	(XHCI_MAX_TRANSFERS * 2 + 1)


typedef struct xhci_td {
	xhci_trb*	trbs;
	phys_addr_t	trb_addr;
	uint32		trb_count;
	uint32		trb_used;

	void**		buffers;
	phys_addr_t* buffer_addrs;
	size_t		buffer_size;
	uint32		buffer_count;

	Transfer*	transfer;
	uint8		trb_completion_code;
	int32		td_transferred;
	int32		trb_left;

	xhci_td*	next;
} xhci_td;


typedef struct xhci_endpoint {
	mutex 			lock;

	xhci_device*	device;
	uint8			id;

	uint16			max_burst_payload;

	xhci_td*		td_head;
	uint8			used;
	uint8			current;

	xhci_trb*		trbs; // [XHCI_ENDPOINT_RING_SIZE]
	phys_addr_t 	trb_addr;
} xhci_endpoint;


typedef struct xhci_device {
	uint8 slot;
	uint8 address;
	area_id trb_area;
	phys_addr_t trb_addr;
	struct xhci_trb *trbs; // [XHCI_MAX_ENDPOINTS - 1][XHCI_ENDPOINT_RING_SIZE]

	area_id input_ctx_area;
	phys_addr_t input_ctx_addr;
	struct xhci_input_device_ctx *input_ctx;

	area_id device_ctx_area;
	phys_addr_t device_ctx_addr;
	struct xhci_device_ctx *device_ctx;

	xhci_endpoint endpoints[XHCI_MAX_ENDPOINTS - 1];
} xhci_device;


class XHCI : public BusManager {
public:
	static	status_t			AddTo(Stack *stack);

								XHCI(pci_info *info, Stack *stack);
								~XHCI();

	virtual	const char *		TypeName() const { return "xhci"; }

			status_t			Start();
	virtual	status_t			SubmitTransfer(Transfer *transfer);
			status_t			SubmitControlRequest(Transfer *transfer);
			status_t			SubmitNormalRequest(Transfer *transfer);
	virtual	status_t			CancelQueuedTransfers(Pipe *pipe, bool force);

	virtual	status_t			StartDebugTransfer(Transfer *transfer);
	virtual	status_t			CheckDebugTransfer(Transfer *transfer);
	virtual	void				CancelDebugTransfer(Transfer *transfer);

	virtual	status_t			NotifyPipeChange(Pipe *pipe,
									usb_change change);

	virtual	Device *			AllocateDevice(Hub *parent,
									int8 hubAddress, uint8 hubPort,
									usb_speed speed);
	virtual	void				FreeDevice(Device *device);

			// Port operations for root hub
			uint8				PortCount() const { return fPortCount; }
			status_t			GetPortStatus(uint8 index,
									usb_port_status *status);
			status_t			SetPortFeature(uint8 index, uint16 feature);
			status_t			ClearPortFeature(uint8 index, uint16 feature);

			status_t			GetPortSpeed(uint8 index, usb_speed *speed);

private:
			// Controller resets
			status_t			ControllerReset();
			status_t			ControllerHalt();

			// Interrupt functions
	static	int32				InterruptHandler(void *data);
			int32				Interrupt();

			// Endpoint management
			status_t			ConfigureEndpoint(xhci_endpoint* ep, uint8 slot,
									uint8 number, uint8 type, bool directionIn,
									uint16 interval, uint16 maxPacketSize,
									usb_speed speed, uint8 maxBurst,
									uint16 bytesPerInterval);
			status_t			_InsertEndpointForPipe(Pipe *pipe);
			status_t			_RemoveEndpointForPipe(Pipe *pipe);

			// Event management
	static	int32				EventThread(void *data);
			void				CompleteEvents();
			void				ProcessEvents();

			// Transfer management
	static	int32				FinishThread(void *data);
			void				FinishTransfers();

			// Descriptor management
			xhci_td *			CreateDescriptor(uint32 trbCount,
									uint32 bufferCount, size_t bufferSize);
			void				FreeDescriptor(xhci_td *descriptor);

			size_t				WriteDescriptor(xhci_td *descriptor,
									iovec *vector, size_t vectorCount);
			size_t				ReadDescriptor(xhci_td *descriptor,
									iovec *vector, size_t vectorCount);

			status_t			_LinkDescriptorForPipe(xhci_td *descriptor,
									xhci_endpoint *endpoint);
			status_t			_UnlinkDescriptorForPipe(xhci_td *descriptor,
									xhci_endpoint *endpoint);

			// Command
			void				DumpRing(xhci_trb *trb, uint32 size);
			void				QueueCommand(xhci_trb *trb);
			void				HandleCmdComplete(xhci_trb *trb);
			void				HandleTransferComplete(xhci_trb *trb);
			status_t			DoCommand(xhci_trb *trb);

			// Doorbell
			void				Ring(uint8 slot, uint8 endpoint);

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
	inline	status_t			WaitOpBits(uint32 reg, uint32 mask, uint32 expected);

			// Capability register functions
	inline	uint32				ReadCapReg32(uint32 reg);
	inline	void				WriteCapReg32(uint32 reg, uint32 value);

			// Runtime register functions
	inline	uint32				ReadRunReg32(uint32 reg);
	inline	void				WriteRunReg32(uint32 reg, uint32 value);

			// Doorbell register functions
	inline	uint32				ReadDoorReg32(uint32 reg);
	inline	void				WriteDoorReg32(uint32 reg, uint32 value);

			// Context functions
	inline	addr_t				_OffsetContextAddr(addr_t p);
	inline	uint32				_ReadContext(uint32* p);
	inline	void				_WriteContext(uint32* p, uint32 value);
	inline	uint64				_ReadContext(uint64* p);
	inline	void				_WriteContext(uint64* p, uint64 value);

			void				_SwitchIntelPorts();

private:
	static	pci_module_info *	sPCIModule;
	static	pci_x86_module_info *sPCIx86Module;

			area_id				fRegisterArea;
			uint8 *				fRegisters;
			uint32				fCapabilityRegisterOffset;
			uint32				fOperationalRegisterOffset;
			uint32				fRuntimeRegisterOffset;
			uint32				fDoorbellRegisterOffset;

			pci_info *			fPCIInfo;
			Stack *				fStack;
			uint8				fIRQ;
			bool				fUseMSI;

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
			uint32				fScratchpadCount;
			area_id				fScratchpadArea[XHCI_MAX_SCRATCHPADS];
			void *				fScratchpad[XHCI_MAX_SCRATCHPADS];

			// Devices
			struct xhci_device	fDevices[XHCI_MAX_DEVICES];
			int32				fContextSizeShift; // 0/1 for 32/64 bytes

			// Transfers
			mutex				fFinishedLock;
			xhci_td	*			fFinishedHead;
			sem_id				fFinishTransfersSem;
			thread_id			fFinishThread;

			// Events
			sem_id				fEventSem;
			thread_id			fEventThread;
			mutex				fEventLock;
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
