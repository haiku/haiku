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

typedef struct transfer_data_s {
	Transfer		*transfer;
	bool			incoming;
	bool			canceled;
	transfer_data_s	*link;
} transfer_data;

// --------------------------------------
//	OHCI:: 	Software isonchronous 
//			transfer descriptor
// --------------------------------------
typedef struct hcd_soft_itransfer
{
	ohci_isochronous_descriptor	itd;
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
virtual status_t					CancelQueuedTransfers(Pipe *pipe,
										bool force);

virtual	status_t					NotifyPipeChange(Pipe *pipe,
										usb_change change);

static	status_t					AddTo(Stack *stack);

		// Port operations
		uint8 						PortCount() { return fPortCount; };
		status_t 					GetPortStatus(uint8 index,
										usb_port_status *status);
		status_t					SetPortFeature(uint8 index, uint16 feature);
		status_t					ClearPortFeature(uint8 index, uint16 feature);

		status_t					ResetPort(uint8 index);


private:
		// Interrupt functions
static	int32						_InterruptHandler(void *data);
		int32						_Interrupt();

static	int32						_FinishThread(void *data);
		void						_FinishTransfer();

		status_t					_SubmitAsyncTransfer(Transfer *transfer);
		status_t					_SubmitPeriodicTransfer(Transfer *transfer);
		
		// Endpoint related methods
		status_t					_CreateEndpoint(Pipe *pipe,
										bool isIsochronous);
		ohci_endpoint_descriptor	*_AllocateEndpoint();
		void						_FreeEndpoint(
										ohci_endpoint_descriptor *endpoint);
		status_t					_InsertEndpointForPipe(Pipe *pipe);

		// Transfer descriptor related methods
		ohci_general_descriptor		*_CreateGeneralDescriptor();
		void						_FreeGeneralDescriptor(
										ohci_general_descriptor *descriptor);
		ohci_isochronous_descriptor	*_CreateIsochronousDescriptor();
		void						_FreeIsochronousDescriptor(
										ohci_isochronous_descriptor *descriptor);

		// Register functions
inline	void						_WriteReg(uint32 reg, uint32 value);
inline	uint32						_ReadReg(uint32 reg);

static	pci_module_info				*sPCIModule;
		pci_info 					*fPCIInfo;
		Stack						*fStack;

		uint32						*fOperationalRegisters;
		area_id						fRegisterArea;

		// Host Controller Communication Area related stuff
		area_id						fHccaArea;
		ohci_hcca					*fHcca;
		ohci_endpoint_descriptor	**fInterruptEndpoints;

		// Dummy endpoints
		ohci_endpoint_descriptor	*fDummyControl;
		ohci_endpoint_descriptor	*fDummyBulk;
		ohci_endpoint_descriptor	*fDummyIsochronous;

		// Maintain a linked list of transfer
		transfer_data				*fFirstTransfer;
		transfer_data				*fFinishTransfer;
		sem_id						fFinishTransfersSem;
		thread_id					fFinishThread;
		bool						fStopFinishThread;

		// Root Hub
		OHCIRootHub 				*fRootHub;
		uint8						fRootHubAddress;

		// Port management
		uint8						fPortCount;
};


class OHCIRootHub : public Hub {
public:
									OHCIRootHub(Object *rootObject,
										int8 deviceAddress);

static	status_t	 				ProcessTransfer(OHCI *ohci,
										Transfer *transfer);
};


#endif // OHCI_H
