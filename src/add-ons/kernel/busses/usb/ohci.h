//------------------------------------------------------------------------------
//	Copyright (c) 2005, Jan-Rixt Van Hoye
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//-------------------------------------------------------------------------------


#ifndef OHCI_H
#define OHCI_H

//	---------------------------
//	OHCI::	Includes
//	---------------------------

#include "usb_p.h"
#include "ohci_software.h"

struct pci_info;
struct pci_module_info;
class OHCIRootHub;
struct Endpoint;
struct TransferDescriptor;

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
	status_t 					SubmitTransfer(Transfer *t); 	//Override from BusManager. 
	status_t					NotifyPipeChange(Pipe *pipe,    //Override from BusManager.
								                 usb_change change);

	static pci_module_info 		*pci_module; 					// Global data for the module.

	status_t GetPortStatus(uint8 index, usb_port_status *status);
	status_t ClearPortFeature(uint8 index, uint16 feature);
	status_t SetPortFeature(uint8 index, uint16 feature);
	
	uint8 PortCount() { return fNumPorts; };

private:
	inline void   WriteReg(uint32 reg, uint32 value);
	inline uint32 ReadReg(uint32 reg);

	// Global
	pci_info 					*fPcii;					// pci-info struct
	Stack 						*fStack;				// Pointer to the stack	 
	uint8						*fRegisters;			// Base address of the operational registers
	area_id						fRegisterArea;			// Area id of the 
	// HCCA
	area_id						fHccaArea;
	struct ohci_hcca			*fHcca;					// The HCCA structure for the interupt communication
	Endpoint					*fInterruptEndpoints[OHCI_NO_EDS];	// The interrupt endpoint list
	// Dummy endpoints
	Endpoint					*fDummyControl;
	Endpoint					*fDummyBulk;
	Endpoint					*fDummyIsochronous;
	// Root Hub
	OHCIRootHub 				*fRootHub;				// the root hub
	uint8						fRootHubAddress;		// the usb address of the roothub
	uint8						fNumPorts;				// the number of ports
	// functions
	Endpoint					*AllocateEndpoint(); 	// allocate memory for an endpoint
	void						FreeEndpoint(Endpoint *end); // Free endpoint
	TransferDescriptor			*AllocateTransfer();    // create a NULL transfer
	void						FreeTransfer(TransferDescriptor *trans); // Free transfer

	status_t					InsertEndpointForPipe(Pipe *p);
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
	ohci_transfer_descriptor	*td;
};

#endif // OHCI_H
