/*
 * Copyright 2005-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jan-Rixt Van Hoye
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
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

// --------------------------------
//	OHCI: 	The OHCI class derived 
//			from the BusManager
// --------------------------------

class OHCI : public BusManager
{
	friend class OHCIRootHub;
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

		uint32						*fRegisterBase;
		pci_info 					*fPCIInfo;
		Stack						*fStack;

		area_id						fRegisterArea;

	// HCCA
	area_id						fHccaArea;
	struct ohci_hcca			*fHcca;					// The HCCA structure for the interupt communication
	Endpoint					*fInterruptEndpoints[OHCI_NO_EDS];	// The interrupt endpoint list
	// Dummy endpoints
	Endpoint					*fDummyControl;
	Endpoint					*fDummyBulk;
	Endpoint					*fDummyIsochronous;
	// functions
	Endpoint					*AllocateEndpoint(); 	// allocate memory for an endpoint
	void						FreeEndpoint(Endpoint *end); // Free endpoint
	TransferDescriptor			*AllocateTransfer();    // create a NULL transfer
	void						FreeTransfer(TransferDescriptor *trans); // Free transfer

	status_t					InsertEndpointForPipe(Pipe *p);

		// Root Hub
		OHCIRootHub 				*fRootHub;
		uint8						fRootHubAddress;
		uint8						fNumPorts;
};

// --------------------------------
//	OHCI:	The root hub of the OHCI 
//			controller derived from 
//			the Hub class
// --------------------------------

class OHCIRootHub : public Hub
{
	public:
		OHCIRootHub(OHCI *ohci, int8 deviceAddress);
		status_t 			ProcessTransfer(Transfer *t, OHCI *ohci);
};

//
// Endpoint: wrapper around the hardware endpoints
//

struct Endpoint
{
	addr_t						physicaladdress;//Point to the physical address
	ohci_endpoint_descriptor	*ed;			//Logical 'endpoint'
	Endpoint					*next;			//Pointer to the 'next' endpoint
	TransferDescriptor			*head, *tail;	//Pointers to the 'head' and 'tail' transfer descriptors

	//Utility functions
	void SetNext(Endpoint *end) {
		next = end;
		if (end == 0)
			ed->next_endpoint = 0;
		else
			ed->next_endpoint = end->physicaladdress;
	}; 
	
	//Constructor (or better: initialiser)
	Endpoint() : physicaladdress(0), ed(0), next(0), head(0), tail(0) {};
};

struct TransferDescriptor
{
	addr_t						physicaladdress;
	ohci_general_transfer_descriptor	*td;
};

#endif // OHCI_H
