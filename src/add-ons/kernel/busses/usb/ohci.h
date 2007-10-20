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

struct Endpoint;
struct TransferDescriptor;

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

#define OHCI_SITD_SIZE ((sizeof (struct hcd_soft_itransfer) + OHCI_ITD_ALIGN - 1) / OHCI_ITD_ALIGN * OHCI_ITD_ALIGN)
#define OHCI_SITD_CHUNK 64

// ------------------------------------------
//	OHCI:: Number of enpoint descriptors (63)
// ------------------------------------------

#define OHCI_NO_EDS (2 * OHCI_NUMBER_OF_INTERRUPTS - 1)

//
// Endpoint: wrapper around the hardware endpoints
//

struct Endpoint
{
	addr_t						physical_address;//Point to the physical address
	ohci_endpoint_descriptor	*ed;			//Logical 'endpoint'
	Endpoint					*next;			//Pointer to the 'next' endpoint
	TransferDescriptor			*head, *tail;	//Pointers to the 'head' and 'tail' transfer descriptors

	//Utility functions
	void SetNext(Endpoint *end) {
		next = end;
		if (end == 0)
			ed->next_endpoint = 0;
		else
			ed->next_endpoint = end->physical_address;
	}; 
	
	//Constructor (or better: initialiser)
	Endpoint() : physical_address(0), ed(0), next(0), head(0), tail(0) {};
};


struct TransferDescriptor
{
	addr_t						physical_address;
	ohci_general_transfer_descriptor	*td;
};


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
inline	void						WriteReg(uint32 reg, uint32 value);
inline	uint32						ReadReg(uint32 reg);

		// Global
static	pci_module_info				*sPCIModule;

		uint32						*fOperationalRegisters;
		pci_info 					*fPCIInfo;
		Stack						*fStack;

		area_id						fRegisterArea;

		// HCCA
		area_id						fHccaArea;
		struct ohci_hcca			*fHcca;	
		Endpoint					*fInterruptEndpoints[OHCI_NO_EDS];

		// Dummy endpoints
		Endpoint					*fDummyControl;
		Endpoint					*fDummyBulk;
		Endpoint					*fDummyIsochronous;
		// functions
		Endpoint					*_AllocateEndpoint(); 
		void						_FreeEndpoint(Endpoint *end);
		TransferDescriptor			*_AllocateTransfer();
		void						_FreeTransfer(TransferDescriptor *trans);

		status_t					_InsertEndpointForPipe(Pipe *p);

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
