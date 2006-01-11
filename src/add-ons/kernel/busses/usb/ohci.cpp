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
//
//  explanation of this file:
//  -------------------------
//
//	This is the implementation of the OHCI module for the Haiku USB stack
//  It is bases on the hcir1_0a documentation that can be found on www.usb.org
//  Some parts are derive from source-files from openbsd, netbsd and linux. 
//
//	------------------------------------------------------------------------

//	---------------------------
//	OHCI::	Includes
//	---------------------------
#include <module.h>
#include <PCI.h>
#include <USB.h>
#include <KernelExport.h>
#include <stdlib.h>

//	---------------------------
//	OHCI::	Local includes
//	---------------------------

#include "ohci.h"
#include "ohci_hardware.h"
#include "usb_p.h"

//------------------------------------------------------
//	OHCI:: 	Reverse the bits in a value between 0 and 31
//			(Section 3.3.2) 
//------------------------------------------------------

static uint8 revbits[OHCI_NO_INTRS] =
  { 0x00, 0x10, 0x08, 0x18, 0x04, 0x14, 0x0c, 0x1c,
    0x02, 0x12, 0x0a, 0x1a, 0x06, 0x16, 0x0e, 0x1e,
    0x01, 0x11, 0x09, 0x19, 0x05, 0x15, 0x0d, 0x1d,
    0x03, 0x13, 0x0b, 0x1b, 0x07, 0x17, 0x0f, 0x1f };
    
//------------------------------------------------------------------------
//	OHCI::	These are the OHCI operations that can be done.
//		
//		parameters:	
//					- op: operation
//------------------------------------------------------------------------

static int32
ohci_std_ops( int32 op , ... )
{
	switch (op)
	{
		case B_MODULE_INIT:
			TRACE( "ohci_module: init the module\n" );
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE( "ohci_module: uninit the module\n" );
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}	

//------------------------------------------------------------------------
//	OHCI::	Give an reference of a stack instance to the OHCI module
//		
//		parameters:	
//					- &stack: reference to a stack instance form stack.cpp
//------------------------------------------------------------------------

static bool
ohci_add_to( Stack &stack )
{
	status_t status;
	pci_info *item;
	bool found = false;
	int i;
	
	#ifdef OHCI_DEBUG
	set_dprintf_enabled( true ); 
	load_driver_symbols( "ohci" );
	#endif
	//
	// 	Try if the PCI module is loaded
	//
	if( ( status = get_module( B_PCI_MODULE_NAME, (module_info **)&( OHCI::pci_module ) ) ) != B_OK) 
	{
		TRACE( "USB_ OHCI: init_hardware(): Get PCI module failed! %lu \n", status);
		return status;
	}
	TRACE( "usb_ohci init_hardware(): Setting up hardware\n" );
	//
	// 	TODO: in the future we might want to support multiple host controllers.
	//
	item = new pci_info;
	for ( i = 0 ; OHCI::pci_module->get_nth_pci_info( i , item ) == B_OK ; i++ )
	{
		//
		//	class_base = 0C (serial bus) class_sub = 03 (usb) prog_int: 10 (OHCI)
		//
		if ( ( item->class_base == 0x0C ) && ( item->class_sub == 0x03 ) &&
			 ( item->class_api  == 0x10 ) )
		{
			if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) 
			{
				TRACE( "USB OHCI: init_hardware(): found with invalid IRQ - check IRQ assignement\n");
				continue;
			}
			TRACE("USB OHCI: init_hardware(): found at IRQ %u \n", item->u.h0.interrupt_line);
			OHCI *bus = new OHCI( item , &stack );
			if ( bus->InitCheck() != B_OK )
			{
				delete bus;
				break;
			}
			
			stack.AddBusManager( bus );
			bus->Start();
			found = true;
			break;
		}
	}
	
	if ( found == false )
	{
		TRACE( "USB OHCI: init hardware(): no devices found\n" );
		free( item );
		put_module( B_PCI_MODULE_NAME );
		return ENODEV;
	}
	return B_OK; //Hardware found
}

//------------------------------------------------------------------------
//	OHCI::	Host controller information
//		
//		parameters:	none
//------------------------------------------------------------------------

host_controller_info ohci_module = {
	{
		"busses/usb/ohci/jixt", 
		NULL,						// No flag like B_KEEP_LOADED : the usb module does that
		ohci_std_ops
	},
	NULL ,
	ohci_add_to
};

//------------------------------------------------------------------------
//	OHCI::	Module information
//		
//		parameters:	none
//------------------------------------------------------------------------

module_info *modules[] = 
{ 
	(module_info *) &ohci_module,
	NULL 
};

//------------------------------------------------------------------------
//	OHCI::	Constructor/Initialisation
//		
//		parameters:	
//					- info:		pointer to a pci information structure
//					- stack:	pointer to a stack instance
//------------------------------------------------------------------------

OHCI::OHCI( pci_info *info , Stack *stack )
{
	m_pcii = info;
	m_stack = stack;
	uint32 rev=0;
	int i;
	hcd_soft_endpoint *sed, *psed;
	dprintf( "OHCI: constructing new BusManager\n" );
	m_opreg_base = OHCI::pci_module->read_pci_config(m_pcii->bus, m_pcii->device, m_pcii->function, 0x20, 4);
	m_opreg_base &= PCI_address_io_mask;
	TRACE( "OHCI: iospace offset: %lx\n" , m_opreg_base );
	m_roothub_base = 255; //Invalidate the Root Hub address
	{
		// enable pci address access
		unsigned char cmd;
		cmd = OHCI::pci_module->read_pci_config(m_pcii->bus, m_pcii->device, m_pcii->function, PCI_command, 2);
		cmd = cmd | PCI_command_io | PCI_command_master | PCI_command_memory;
		OHCI::pci_module->write_pci_config(m_pcii->bus, m_pcii->device, m_pcii->function, PCI_command, 2, cmd );
	}
	
	//	Get the revision of the controller
	
	OHCI::pci_module->read_io_16( m_opreg_base + OHCI_REVISION );
	
	//	Check the revision of the controller. The revision should be 10xh
	
	dprintf(" OHCI: Version %ld.%ld%s\n", OHCI_REV_HI(rev), OHCI_REV_LO(rev),OHCI_REV_LEGACY(rev) ? ", legacy support" : "");

	if (OHCI_REV_HI(rev) != 1 || OHCI_REV_LO(rev) != 0) 
	{
		dprintf("OHCI: Unsupported OHCI revision of the ohci device\n");
		return;
	}
	//	Set the revision of the controller
	m_revision = rev;
	//	Set the product description of the controller
	m_product_descr = "Open Host Controller Interface";
	//	Initialize the transfer descriptors and the interupt transfer descriptors
	//
			/*		+++++++++++++++++++		*/   
	//
	//	Allocate the HCCA area.
	void *phy;
	m_hcca_area = stack->AllocateArea( (void **)&(hcca) , &(phy) ,OHCI_HCCA_SIZE , "ohci hcca area" );
	m_hcca_base = reinterpret_cast<addr_t>(phy);
	
	//	Allocate dummy Endpoint Descriptor that starts the control list.
	ed_control_tail = ohci_alloc_soft_endpoint();
	ed_control_tail->ed.ed_flags |= OHCI_ED_SKIP;
	
	//	Allocate dummy Endpoint Descriptor that starts the bulk list.
	ed_bulk_tail = ohci_alloc_soft_endpoint();
	ed_bulk_tail->ed.ed_flags |= OHCI_ED_SKIP;
	
	// Allocate dummy Endpoint Descriptor that starts the isochronous list.
	ed_isoch_tail = ohci_alloc_soft_endpoint();
	ed_isoch_tail->ed.ed_flags |= OHCI_ED_SKIP;
	
	// Allocate all the dummy Endpoint Desriptors that make up the interrupt tree.
	for (i = 0; i < OHCI_NO_EDS; i++) 
	{
		sed = ohci_alloc_soft_endpoint();
		if (sed == NULL) 
		{
			return;
		}
		/* All ED fields are set to 0. */
		sc_eds[i] = sed;
		sed->ed.ed_flags |= OHCI_ED_SKIP;
		if (i != 0)
			psed = sc_eds[(i-1) / 2];
		else
			psed= ed_isoch_tail;
		sed->next = psed;
		sed->ed.ed_nexted = psed->physaddr;
	}
	
	// Fill HCCA interrupt table.  The bit reversal is to get
	// the tree set up properly to spread the interrupts.
	for (i = 0; i < OHCI_NO_INTRS; i++)
		hcca->hcca_interrupt_table[revbits[i]] = sc_eds[OHCI_NO_EDS-OHCI_NO_INTRS+i]->physaddr;
	
	//	Determine in what context we are running.
	uint32 ctrlmsg,statusmsg;
	ctrlmsg = OHCI::pci_module->read_io_16( m_opreg_base + OHCI_CONTROL);
	if (ctrlmsg & OHCI_IR) 
	{
		/// SMM active, request change
		dprintf(("OHCI: SMM active, request owner change\n"));
		statusmsg = OHCI::pci_module->read_io_16( m_opreg_base + OHCI_COMMAND_STATUS);
		OHCI::pci_module->write_io_16( m_opreg_base + OHCI_COMMAND_STATUS,statusmsg | OHCI_OCR);
		for (i = 0; i < 100 && (ctrlmsg & OHCI_IR); i++) 
		{
			snooze(100);
			ctrlmsg = OHCI::pci_module->read_io_16( m_opreg_base + OHCI_CONTROL);
		}
		if ((ctrlmsg & OHCI_IR) == 0) 
		{
			dprintf("OHCI: SMM does not respond, resetting\n");
			OHCI::pci_module->write_io_16( m_opreg_base + OHCI_CONTROL,OHCI_HCFS_RESET);
			goto reset;
		}
	// Don't bother trying to reuse the BIOS init, we'll reset it anyway.
	} 
	else if ((ctrlmsg & OHCI_HCFS_MASK) != OHCI_HCFS_RESET) 
	{
		// BIOS started controller
		dprintf("OHCI: BIOS active\n");
		if ((ctrlmsg & OHCI_HCFS_MASK) != OHCI_HCFS_OPERATIONAL) 
		{
			OHCI::pci_module->write_io_16( m_opreg_base + OHCI_CONTROL,OHCI_HCFS_OPERATIONAL);
			snooze(250000);
		}
	} 
	else 
	{
		dprintf("OHCI: cold started\n");
	reset:
		// Controller was cold started.
		snooze(100000);
	}
	
	//	This reset should not be necessary according to the OHCI spec, but
	//	without it some controllers do not start.
	//
			/*		+++++++++++++++++++		*/
	//
	//	We now own the host controller and the bus has been reset.
	//
			/*		+++++++++++++++++++		*/
	//
	// Reset the Host Controller
	//
			/*		+++++++++++++++++++		*/
	//
	//	Nominal time for a reset is 10 us.
	//
			/*		+++++++++++++++++++		*/
	//
	//	The controller is now in SUSPEND state, we have 2ms to finish.
	//
	//	Set up the Host Controler registers.
	//
			/*		+++++++++++++++++++		*/
	//
	//	Disable all interrupts and then switch on all desired interrupts.
	//
			/*		+++++++++++++++++++		*/
	//
	//	Switch on desired functional features.
	//
			/*		+++++++++++++++++++		*/
	// 
	//	And finally start it!
	//
			/*		+++++++++++++++++++		*/
	//
	//	The controller is now OPERATIONAL.  Set a some final
	//	registers that should be set earlier, but that the
	//	controller ignores when in the SUSPEND state.
	//
			/*		+++++++++++++++++++		*/
	//
	// Fiddle the No OverCurrent Protection bit to avoid chip bug.
	//
			/*		+++++++++++++++++++		*/
	//
	// The AMD756 requires a delay before re-reading the register,
	// otherwise it will occasionally report 0 ports.
	//
			/*		+++++++++++++++++++		*/
	//
	// Set up the bus structure
	//
			/*		+++++++++++++++++++		*/
	//return B_OK;
}

hcd_soft_endpoint * OHCI::ohci_alloc_soft_endpoint()
{
	hcd_soft_endpoint *sed;
	int i;
	void *phy;
	if (sc_freeeds == NULL) 
	{
		dprintf("OHCI: ohci_alloc_soft_endpoint(): allocating chunk\n");
		for(i = 0; i < OHCI_SED_CHUNK; i++) 
		{
			if(m_stack->AllocateChunk( (void **)&(sed) ,&phy , OHCI_ED_ALIGN )!= B_OK)
			{
				dprintf( "USB OHCI: ohci_alloc_soft_endpoint(): failed allocation of skeleton qh %i, aborting\n",  i );
				return (0);
			}
			// chunk has been allocated
			sed->physaddr = reinterpret_cast<addr_t>(phy);
			sed->next = sc_freeeds;
			sc_freeeds = sed;
		}
	}
	sed = sc_freeeds;
	sc_freeeds = sed->next;
	memset(&sed->ed, 0, sizeof(hc_endpoint_descriptor));
	sed->next = 0;
	return (sed);
}

//------------------------------------------------------------------------
//	OHCI::	Submit a transfer
//		
//		parameters:	
//					- t: pointer to a transfer instance
//------------------------------------------------------------------------

status_t OHCI::SubmitTransfer( Transfer *t )
{
	return B_OK;
}
pci_module_info *OHCI::pci_module = 0;
