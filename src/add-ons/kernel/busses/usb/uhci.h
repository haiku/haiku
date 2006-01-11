//------------------------------------------------------------------------------
//	Copyright (c) 2004, Niels S. Reedijk
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

#ifndef UHCI_H
#define UHCI_H

#include "usb_p.h"
#include "uhci_hardware.h"

struct pci_info;
struct pci_module_info;
class UHCIRootHub;

class UHCI : public BusManager
{
	friend class UHCIRootHub;
	friend int32 uhci_interrupt_handler( void *data );
public:
	UHCI( pci_info *info , Stack *stack );
	
	//Override from BusManager
	status_t Start();
	status_t SubmitTransfer( Transfer *t );
	
	// Global data for the module.
	static pci_module_info *pci_module; 
private:
	//Utility functions
	void GlobalReset();
	status_t Reset();
	int32 Interrupt();
	
	//Functions for the actual functioning of transfers
	status_t InsertControl( Transfer *t );

	uint32		m_reg_base;		//Base address of the registers
	pci_info 	*m_pcii;		//pci-info struct
	Stack 		*m_stack;		//Pointer to the stack
	
	//Frame list memory
	area_id		m_framearea;
	addr_t 		m_framelist[1024];	//The frame list struct
	addr_t		m_framelist_phy;	//The physical pointer to the frame list
	
	// Virtual frame
	uhci_qh 	*m_qh_virtual[12];	 //
#define m_qh_interrupt_256 m_qh_virtual[0]
#define m_qh_interrupt_128 m_qh_virtual[1]
#define m_qh_interrupt_64  m_qh_virtual[2]
#define m_qh_interrupt_32  m_qh_virtual[3]
#define m_qh_interrupt_16  m_qh_virtual[4]
#define m_qh_interrupt_8   m_qh_virtual[5]
#define m_qh_interrupt_4   m_qh_virtual[6]
#define m_qh_interrupt_2   m_qh_virtual[7]
#define m_qh_interrupt_1   m_qh_virtual[8]
#define m_qh_control       m_qh_virtual[9]
#define m_qh_bulk          m_qh_virtual[10]
#define m_qh_terminate     m_qh_virtual[11]
	
	//Maintain a list of transfers
	Vector<Transfer *>     m_transfers;
	
	//Root hub:
	UHCIRootHub *m_rh;				// the root hub
	uint8		m_rh_address;		// the address of the root hub
};

class UHCIRootHub : public Hub
{
public:
	UHCIRootHub( UHCI *uhci , int8 devicenum );
	status_t SubmitTransfer( Transfer *t );
	void UpdatePortStatus();
private:
	usb_port_status m_hw_port_status[2]; // the port status (maximum of two)
	UHCI *m_uhci;					// needed because of internal data
};

struct hostcontroller_priv
{
	uhci_qh		*topqh;
	uhci_td		*firsttd;
	uhci_td		*lasttd;
};

#define UHCI_DEBUG
#ifdef UHCI_DEBUG
#define TRACE dprintf
#else
#define TRACE silent
void silent( const char * , ... ) {}
#endif

#endif
