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

// --------------------------------
//	OHCI: 	The OHCI class derived 
//			from the BusManager
// --------------------------------

class OHCI : public BusManager
{
	friend class OHCIRootHub;
public:
		
	OHCI( pci_info *info , Stack *stack );
	status_t 					SubmitTransfer( Transfer *t ); 	//Override from BusManager. 
	static pci_module_info 		*pci_module; 					// Global data for the module.

	inline void   WriteReg(uint32 reg, uint32 value);
	inline uint32 ReadReg(uint32 reg);
		
private:
	// Global
	pci_info 					*m_pcii;				// pci-info struct
	Stack 						*m_stack;				// Pointer to the stack	 
	uint8						*m_registers;			// Base address of the operational registers
	area_id						m_register_area;		// Area id of the 
	// HCCA
	area_id						m_hcca_area;
	struct hc_hcca				*m_hcca;					// The HCCA structure for the interupt communication
	hcd_soft_endpoint			*sc_eds[OHCI_NO_EDS];	// number of interupt endpiont descriptors
	// Registers
	struct hcd_soft_endpoint	*ed_bulk_tail;			// last in the bulk list
	struct hcd_soft_endpoint	*ed_control_tail;		// last in the control list
	struct hcd_soft_endpoint	*ed_isoch_tail;			// lest in the isochrnonous list
	struct hcd_soft_endpoint	*ed_periodic[32];		// shadow of the interupt table
	// free list structures
	struct hcd_soft_endpoint	*sc_freeeds;			// list of free endpoint descriptors
	struct hcd_soft_transfer	*sc_freetds;			// list of free general transfer descriptors
	struct hcd_soft_itransfer	*sc_freeitds; 			// list of free isonchronous transfer descriptors
	// Done queue
	struct hcd_soft_transfer	*sc_sdone;				// list of done general transfer descriptors 
	struct hcd_soft_itransfer 	*sc_sidone;				// list of done isonchronous transefer descriptors
	// memory management for queue data structures.
	struct hcd_soft_transfer  	sc_hash_tds[OHCI_HASH_SIZE];
	struct hcd_soft_itransfer 	sc_hash_itds[OHCI_HASH_SIZE];			
	// Root Hub
	OHCIRootHub 				*m_roothub;			// the root hub
	addr_t						m_roothub_base;		// Base address of the root hub
	// functions
	hcd_soft_endpoint			*ohci_alloc_soft_endpoint(); // allocate memory for an endpoint
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

#define OHCI_DEBUG

#endif // OHCI_H
