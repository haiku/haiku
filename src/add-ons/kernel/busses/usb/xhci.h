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

			status_t			ResetPort(uint8 index);
			status_t			SuspendPort(uint8 index);

	virtual	const char *		TypeName() const { return "xhci"; };

private:
		// Controller resets
			status_t			ControllerReset();
			status_t			ControllerHalt();
			status_t			LightReset();

		// Interrupt functions
	static	int32				InterruptHandler(void *data);
			int32				Interrupt();


		// Operational register functions
	inline	void				WriteOpReg(uint32 reg, uint32 value);
	inline	uint32				ReadOpReg(uint32 reg);

		// Capability register functions
	inline	uint8				ReadCapReg8(uint32 reg);
	inline	uint16				ReadCapReg16(uint32 reg);
	inline	uint32				ReadCapReg32(uint32 reg);
	inline	void				WriteCapReg32(uint32 reg, uint32 value);

	static	pci_module_info *	sPCIModule;

			uint8 *				fCapabilityRegisters;
			uint8 *				fOperationalRegisters;
			uint8 *				fRuntimeRegisters;
			area_id				fRegisterArea;
			pci_info *			fPCIInfo;
			Stack *				fStack;

		// Root Hub

		// Port management
			uint8				fPortCount;
};


#endif // !XHCI_H
