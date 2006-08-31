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
class Endpoint;

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
	status_t 					SubmitTransfer(Transfer *t); 	//Override from BusManager. 
	static pci_module_info 		*pci_module; 					// Global data for the module.

	inline void   WriteReg(uint32 reg, uint32 value);
	inline uint32 ReadReg(uint32 reg);
		
private:
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
	OHCIRootHub 				*m_roothub;				// the root hub
	addr_t						m_roothub_base;			// Base address of the root hub
	// functions
	Endpoint					*AllocateEndpoint(); 	// allocate memory for an endpoint
	void						FreeEndpoint(Endpoint *end); //Free endpoint
};

// --------------------------------
//	OHCI:	The root hub of the OHCI 
//			controller derived from 
//			the Hub class
// --------------------------------

class OHCIRootHub : public Hub
{
	public:
		OHCIRootHub( OHCI *ohci , int8 devicenum );
		status_t SubmitTransfer( Transfer *t );
		void UpdatePortStatus();
	private:
		usb_port_status 	m_hw_port_status[2];	// the port status (maximum of two)
		OHCI 				*m_ohci;				// needed because of internal data
};

//
// Endpoint: wrapper around the hardware endpoints
//

struct Endpoint
{
	addr_t						physicaladdress;//Point to the physical address
	ohci_endpoint_descriptor	*ed;			//Logical 'endpoint'
	Endpoint					*next;			//Pointer to the 'next' endpoint

	//Utility functions
	void SetNext(Endpoint *end) {
		next = end;
		ed->next_endpoint = end->physicaladdress;
	}; 
};

#define OHCI_DEBUG

#endif // OHCI_H
