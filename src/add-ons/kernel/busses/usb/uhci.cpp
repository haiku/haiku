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

#include <module.h>
#include <PCI.h>
#include <USB.h>
#include <KernelExport.h>
#include <stdlib.h>

#include "uhci.h"
#include "uhci_hardware.h"
#include <usb_p.h>


/* ++++++++++
This is the implementation of the UHCI controller for the OpenBeOS USB stack
++++++++++ */

static int32
uhci_std_ops( int32 op , ... )
{
	switch (op)
	{
		case B_MODULE_INIT:
			TRACE( "uhci_module: init the module\n" );
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE( "uhci_module: uninit the module\n" );
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}	

static bool
uhci_add_to( Stack &stack )
{
	status_t status;
	pci_info *item;
	bool found = false;
	int i;
	
	#ifdef UHCI_DEBUG
	set_dprintf_enabled( true ); 
	load_driver_symbols( "uhci" );
	#endif
	
	// Try if the PCI module is loaded (it would be weird if it wouldn't, but alas)
	if( ( status = get_module( B_PCI_MODULE_NAME, (module_info **)&( UHCI::pci_module ) ) ) != B_OK) 
	{
		TRACE( "USB_ UHCI: init_hardware(): Get PCI module failed! %lu \n", status);
		return status;
	}
	
	TRACE( "usb_uhci init_hardware(): Setting up hardware\n" );
	
	// TODO: in the future we might want to support multiple host controllers.
	item = new pci_info;
	for ( i = 0 ; UHCI::pci_module->get_nth_pci_info( i , item ) == B_OK ; i++ )
	{
		//class_base = 0C (serial bus) class_sub = 03 (usb) prog_int: 00 (UHCI)
		if ( ( item->class_base == 0x0C ) && ( item->class_sub == 0x03 ) &&
			 ( item->class_api  == 0x00 ) )
		{
			if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) 
			{
				TRACE( "USB UHCI: init_hardware(): found with invalid IRQ - check IRQ assignement\n");
				continue;
			}
			TRACE("USB UHCI: init_hardware(): found at IRQ %u \n", item->u.h0.interrupt_line);
			UHCI *bus = new UHCI( item , &stack );
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
		TRACE( "USB UHCI: init hardware(): no devices found\n" );
		free( item );
		put_module( B_PCI_MODULE_NAME );
		return ENODEV;
	}
	return B_OK; //Hardware found
}




host_controller_info uhci_module = {
	{
		"busses/usb/uhci/nielx", 
		NULL,						// No flag like B_KEEP_LOADED : the usb module does that
		uhci_std_ops
	},
	NULL ,
	uhci_add_to
};

module_info *modules[] = 
{ 
	(module_info *) &uhci_module,
	NULL 
};

/* ++++++++++
This is the implementation of the UHCI controller for the OpenBeOS USB stack
++++++++++ */

UHCI::UHCI( pci_info *info , Stack *stack )
{
	//Do nothing yet
	dprintf( "UHCI: constructing new BusManager\n" );
	m_pcii = info;
	m_stack = stack;
	m_reg_base = UHCI::pci_module->read_pci_config(m_pcii->bus, m_pcii->device, m_pcii->function, 0x20 , 4 ) ^ 1;
	TRACE( "USB UHCI: iospace offset: %lx\n" , m_reg_base );
	m_rh_address = 255; //Invalidate the RH address
	{
		/* enable pci address access */	
		unsigned char cmd;
		cmd = UHCI::pci_module->read_pci_config(m_pcii->bus, m_pcii->device, m_pcii->function, PCI_command, 2);
		cmd = cmd | PCI_command_io | PCI_command_master | PCI_command_memory;
		UHCI::pci_module->write_pci_config(m_pcii->bus, m_pcii->device, m_pcii->function, PCI_command, 2, cmd );
	}
	
	//Do a host reset
	GlobalReset();
	if ( Reset() == B_ERROR )
	{
		TRACE( "USB UHCI: init_hardare(): host failed to reset\n" );
		m_initok = false;
		return;
	}
	
	// Poll the status of the two ports
//	rh_update_port_status();
//	TRACE( "USB UHCI: init_hardware(): port1: %x port2: %x\n",
//	       m_data->port_status[0].status , m_data->port_status[1].status );

	//Set up the frame list
	void *phy;
	m_framearea = stack->AllocateArea( (void **)&(m_framelist[0]) , &(phy) ,
	                                         4096 , "uhci framelist" );
	m_framelist_phy = reinterpret_cast<addr_t>(phy);
	if ( m_framearea < B_OK )
	{
		TRACE( "USB UHCI: init_hardware(): unable to create an area for the frame pointer list\n" );
		m_initok = false;
		return;
	}
	
	/*
	Set up the virtual structure. I stole this idea from the linux usb stack,
	the idea is that for every interrupt interval there is a queue head. These
	things all link together and eventually point to the control and bulk 
	virtual queue heads.
	*/
	
	for( int i = 0 ; i < 12 ; i++ )
	{
		void *phy;
		//Must be aligned on 16-byte boundaries
		if ( m_stack->AllocateChunk( (void **)&(m_qh_virtual[i]) , 
			&phy , 16 ) != B_OK )
		{
			dprintf( "USB UHCI: init_hardware(): failed allocation of skeleton qh %i, aborting\n",  i );
			delete_area( m_framearea );
			m_initok = false;
			return;
		}
		//chunk allocated
		m_qh_virtual[i]->this_phy = reinterpret_cast<addr_t>(phy);
		m_qh_virtual[i]->element_phy = QH_TERMINATE;
		
		//Link this qh to its previous qh
		if ( i != 0 )
			m_qh_virtual[i-1]->link_phy = m_qh_virtual[i]->this_phy | QH_NEXT_IS_QH ;
	}
	// Make sure the qh_terminate terminates
	m_qh_virtual[11]->link_phy = QH_TERMINATE;
		
	//Insert the queues in the frame list. The linux developers mentioned
	// in a comment that they used some magic to distribute the elements all
	// over the place, but I don't really think that it is useful right now
	// (or do I know how I should do that), instead, I just take the frame
	// number and determine where it should begin
	
	//NOTE, in c++ this is butt-ugly. We have a addr_t *array (because with
	//an addr_t *array we can apply pointer arithmetic), uhci_qh *pointers
	//that need to be put through the logical | to make sure the pointer is
	//invalid for the hc. The result of that needs to be converted into a
	//addr_t. Get it?
	for( int i = 0 ; i < 1024 ; i++ )
	{
		int frame = i+1;
		if ( ( frame % 256 ) == 0 )
			m_framelist[i] = reinterpret_cast<addr_t*>( reinterpret_cast<addr_t>(m_qh_interrupt_256) | FRAMELIST_NEXT_IS_QH );
		else if ( ( frame % 128 ) == 0 )
			m_framelist[i] = reinterpret_cast<addr_t*>( reinterpret_cast<addr_t>(m_qh_interrupt_128) | FRAMELIST_NEXT_IS_QH );
		else if ( ( frame % 64 ) == 0 )
			m_framelist[i] = reinterpret_cast<addr_t*>( reinterpret_cast<addr_t>(m_qh_interrupt_64) | FRAMELIST_NEXT_IS_QH );
		else if ( ( frame % 32 ) == 0 )
			m_framelist[i] = reinterpret_cast<addr_t*>( reinterpret_cast<addr_t>(m_qh_interrupt_32) | FRAMELIST_NEXT_IS_QH );
		else if ( ( frame % 16 ) == 0 )
			m_framelist[i] = reinterpret_cast<addr_t*>( reinterpret_cast<addr_t>(m_qh_interrupt_16) | FRAMELIST_NEXT_IS_QH );
		else if ( ( frame % 8 ) == 0 )
			m_framelist[i] = reinterpret_cast<addr_t*>( reinterpret_cast<addr_t>(m_qh_interrupt_8) | FRAMELIST_NEXT_IS_QH );
		else if ( ( frame % 4 ) == 0 )
			m_framelist[i] = reinterpret_cast<addr_t*>( reinterpret_cast<addr_t>(m_qh_interrupt_4) | FRAMELIST_NEXT_IS_QH );
		else if ( ( frame % 2 ) == 0 )
			m_framelist[i] = reinterpret_cast<addr_t*>( reinterpret_cast<addr_t>(m_qh_interrupt_2) | FRAMELIST_NEXT_IS_QH );
		else
			m_framelist[i] = reinterpret_cast<addr_t*>( reinterpret_cast<addr_t>(m_qh_interrupt_1) | FRAMELIST_NEXT_IS_QH );
	}
	
	//Set base pointer
	UHCI::pci_module->write_io_32( m_reg_base + UHCI_FRBASEADD , (int32)(m_framelist_phy) );	

	//Set up the root hub
	m_rh_address = AllocateAddress();
	m_rh = new UHCIRootHub( this , m_rh_address );
	SetRootHub( m_rh );
}

status_t UHCI::SubmitTransfer( Transfer &t )
{
	dprintf( "UHCI::SubmitPacket( Transfer &t ) called!!!\n" );
	
	//Short circuit the root hub
	if ( m_rh_address == t.GetPipe()->GetDeviceAddress() )
		return m_rh->SubmitTransfer( t );
	return B_ERROR;
}

void UHCI::GlobalReset()
{
	UHCI::pci_module->write_io_16( m_reg_base + UHCI_USBCMD , UHCI_USBCMD_GRESET );
	spin( 100 );
	UHCI::pci_module->write_io_16( m_reg_base + UHCI_USBCMD , 0 );
}

status_t UHCI::Reset()
{
	UHCI::pci_module->write_io_16( m_reg_base + UHCI_USBCMD , UHCI_USBCMD_HCRESET );
	spin( 100 );
	if ( UHCI::pci_module->read_io_16( m_reg_base + UHCI_USBCMD ) & UHCI_USBCMD_HCRESET )
		return B_ERROR;
	return B_OK;
}

pci_module_info *UHCI::pci_module = 0;
