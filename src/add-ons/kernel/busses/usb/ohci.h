/*
 * Copyright 2005-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *             Jan-Rixt Van Hoye
 *             Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */

#ifndef OHCI_H
#define OHCI_H

#include "usb_p.h"
#include "ohci_hardware.h"
#include <lock.h>

struct pci_info;
struct pci_module_info;
class OHCIRootHub;

// --------------------------------------
//	OHCI:: 	Software isonchronous 
//			transfer descriptor
// --------------------------------------
typedef struct hcd_soft_itransfer
{
	ohci_isochronous_transfer_descriptor	itd;
	struct hcd_soft_itransfer			*nextitd; 	// mirrors nexttd in ITD
	struct hcd_soft_itransfer			*dnext; 	// next in done list
	addr_t 								physaddr;	// physical address to the host controller isonchronous transfer
	//LIST_ENTRY(hcd_soft_itransfer) 		hnext;
	uint16								flags;		// flags
#ifdef DIAGNOSTIC
	char 								isdone;		// is the transfer done?
#endif
}hcd_soft_itransfer;

#define	OHCI_SITD_SIZE	((sizeof (struct hcd_soft_itransfer) + OHCI_ITD_ALIGN - 1) / OHCI_ITD_ALIGN * OHCI_ITD_ALIGN)
#define	OHCI_SITD_CHUNK	64

#define	OHCI_NUMBER_OF_ENDPOINTS	(2 * OHCI_NUMBER_OF_INTERRUPTS - 1)


class OHCI : public BusManager
{
public:

									OHCI(pci_info *info, Stack *stack);
									~OHCI();

		status_t					Start();
virtual	status_t 					SubmitTransfer(Transfer *transfer);
virtual status_t					CancelQueuedTransfers(Pipe *pipe);

virtual	status_t					NotifyPipeChange(Pipe *pipe,
										usb_change change);

static	status_t					AddTo(Stack *stack);

		// Port operations
		status_t 					GetPortStatus(uint8 index, usb_port_status *status);
		status_t					SetPortFeature(uint8 index, uint16 feature);
		status_t					ClearPortFeature(uint8 index, uint16 feature);

		uint8 						PortCount() { return fNumPorts; };

private:
		// Register functions
inline	void						_WriteReg(uint32 reg, uint32 value);
inline	uint32						_ReadReg(uint32 reg);

		// Global
static	pci_module_info				*sPCIModule;

		uint32						*fOperationalRegisters;
		pci_info 					*fPCIInfo;
		Stack						*fStack;

		area_id						fRegisterArea;

		// Host Controller Communication Area related stuff
		area_id						fHccaArea;
		struct ohci_hcca			*fHcca;
		Endpoint					*fInterruptEndpoints[OHCI_NUMBER_OF_ENDPOINTS];

		// Dummy endpoints
		Endpoint					*fDummyControl;
		Endpoint					*fDummyBulk;
		Endpoint					*fDummyIsochronous;

		// Endpoint related methods
		Endpoint					*_AllocateEndpoint(); 
		void						_FreeEndpoint(Endpoint *end);
		TransferDescriptor			*_AllocateTransfer();
		void						_FreeTransfer(TransferDescriptor *trans);
		status_t					_InsertEndpointForPipe(Pipe *p);
		void						_SetNext(ohci_endpoint_descriptor *endpoint);

		// Root Hub
		OHCIRootHub 				*fRootHub;
		uint8						fRootHubAddress;
		uint8						fNumPorts;
};


class OHCIRootHub : public Hub {
public:
									OHCIRootHub(Object *rootObject,
										int8 deviceAddress);

static	status_t	 				ProcessTransfer(OHCI *ohci,
										Transfer *transfer);
};


#endif // OHCI_H
