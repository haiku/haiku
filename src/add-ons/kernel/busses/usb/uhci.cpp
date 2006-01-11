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
#include "usb_p.h"


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
				TRACE( "USB UHCI::InitCheck() failed, error %li\n" , bus->InitCheck() );
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

int32 uhci_interrupt_handler( void *data )
{
	int32 retval;
	spinlock slock = 0;
	cpu_status status = disable_interrupts();
	acquire_spinlock( &slock );
	retval = ((UHCI*)data)->Interrupt();
	release_spinlock( &slock );
	restore_interrupts( status );
	return retval;
}

UHCI::UHCI( pci_info *info , Stack *stack )
{
	//Do nothing yet
	dprintf( "UHCI: constructing new BusManager\n" );
	m_pcii = info;
	m_stack = stack;
	m_reg_base = UHCI::pci_module->read_pci_config(m_pcii->bus, m_pcii->device, m_pcii->function, PCI_memory_base, 4);
	m_reg_base &= PCI_address_io_mask;
	TRACE( "USB UHCI: iospace offset: %lx\n" , m_reg_base );
	m_rh_address = 255; //Invalidate the RH address
	{
		/* enable pci address access */	
		uint16 cmd;
		cmd = UHCI::pci_module->read_pci_config(m_pcii->bus, m_pcii->device, m_pcii->function, PCI_command, 2);
		cmd = cmd | PCI_command_io | PCI_command_master | PCI_command_memory;
		UHCI::pci_module->write_pci_config(m_pcii->bus, m_pcii->device, m_pcii->function, PCI_command, 2, cmd );
		/* make sure we gain controll of the UHCI controller instead of the BIOS - function 2 */
		UHCI::pci_module->write_pci_config(m_pcii->bus, m_pcii->device, 2, PCI_LEGSUP, 2, PCI_LEGSUP_USBPIRQDEN );
	}
	
	//Do a host reset
	GlobalReset();
	if ( Reset() != B_OK )
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
	According tot the *BSD usb sources, there needs to be a stray transfer
	descriptor in order to get some chipset to work nicely (PIIX or something
	like that).
	*/
	uhci_td *straytd;
	if ( m_stack->AllocateChunk( (void **)&(straytd) , &phy , 32 ) != B_OK )
	{
		dprintf( "USB UHCI::UHCI() Failed to allocate a stray transfer descriptor\n" );
		delete_area( m_framearea );
		m_initok = false;
		return;
	}
	straytd->link_phy = TD_TERMINATE;
	straytd->this_phy = reinterpret_cast<addr_t>(phy);
	straytd->link_log = 0;
	straytd->buffer_log = 0;
	straytd->status = 0;
	straytd->token = TD_TOKEN_NULL | 0x7f << TD_TOKEN_DEVADDR_SHIFT | 0x69;
	straytd->buffer_phy = 0;
		
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
			&phy , 32 ) != B_OK )
		{
			dprintf( "USB UHCI: init_hardware(): failed allocation of skeleton qh %i, aborting\n",  i );
			delete_area( m_framearea );
			m_initok = false;
			return;
		}
		//chunk allocated
		m_qh_virtual[i]->this_phy = reinterpret_cast<addr_t>(phy);
		m_qh_virtual[i]->element_phy = QH_TERMINATE;
		m_qh_virtual[i]->element_log = 0;
		
		//Link this qh to its previous qh
		if ( i != 0 )
		{
			m_qh_virtual[i-1]->link_phy = m_qh_virtual[i]->this_phy | QH_NEXT_IS_QH ;
			m_qh_virtual[i-1]->link_log = m_qh_virtual[i];
		}
	}
	// Make sure the qh_terminate terminates
	m_qh_virtual[11]->link_phy = straytd->this_phy;
	m_qh_virtual[11]->link_log = straytd;
		
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
			m_framelist[i] = m_qh_interrupt_256->this_phy | FRAMELIST_NEXT_IS_QH;
		else if ( ( frame % 128 ) == 0 )
			m_framelist[i] = m_qh_interrupt_128->this_phy | FRAMELIST_NEXT_IS_QH;
		else if ( ( frame % 64 ) == 0 )
			m_framelist[i] = m_qh_interrupt_64->this_phy | FRAMELIST_NEXT_IS_QH;
		else if ( ( frame % 32 ) == 0 )
			m_framelist[i] = m_qh_interrupt_32->this_phy | FRAMELIST_NEXT_IS_QH;
		else if ( ( frame % 16 ) == 0 )
			m_framelist[i] = m_qh_interrupt_16->this_phy | FRAMELIST_NEXT_IS_QH;
		else if ( ( frame % 8 ) == 0 )
			m_framelist[i] = m_qh_interrupt_8->this_phy | FRAMELIST_NEXT_IS_QH;
		else if ( ( frame % 4 ) == 0 )
			m_framelist[i] = m_qh_interrupt_4->this_phy | FRAMELIST_NEXT_IS_QH;
		else if ( ( frame % 2 ) == 0 )
			m_framelist[i] = m_qh_interrupt_2->this_phy | FRAMELIST_NEXT_IS_QH;
		else
			m_framelist[i] = m_qh_interrupt_1->this_phy | FRAMELIST_NEXT_IS_QH;
	}

	//Set base pointer
	UHCI::pci_module->write_io_32( m_reg_base + UHCI_FRBASEADD , (int32)(m_framelist_phy) );	
	UHCI::pci_module->write_io_16( m_reg_base + UHCI_FRNUM , 0 );

	//Set up the root hub
	m_rh_address = AllocateAddress();
	m_rh = new UHCIRootHub( this , m_rh_address );
	SetRootHub( m_rh );
	
	//Install the interrupt handler
	install_io_interrupt_handler( m_pcii->u.h0.interrupt_line , uhci_interrupt_handler , (void *)this , 0 );
	UHCI::pci_module->write_io_16( m_reg_base + UHCI_USBSTS , 0xffff );
	UHCI::pci_module->write_io_16( m_reg_base + UHCI_USBINTR , UHCI_USBINTR_CRC | UHCI_USBINTR_RESUME | UHCI_USBINTR_IOC | UHCI_USBINTR_SHORT );
}

status_t UHCI::Start()
{
	//Start the host controller, then start the Busmanager
	TRACE("USB UCHI::STart() usbcmd reg %u, usbsts reg %u\n" , UHCI::pci_module->read_io_16( m_reg_base + UHCI_USBCMD ) , UHCI::pci_module->read_io_16( m_reg_base + UHCI_USBSTS ) );
	UHCI::pci_module->write_io_16( m_reg_base + UHCI_USBCMD , UHCI_USBCMD_RS );

	bool running = false;
	uint16 status = 0;
	for ( int i = 0 ; i <= 10 ; i++ )
	{
		status = UHCI::pci_module->read_io_16( m_reg_base + UHCI_USBSTS );
		dprintf( "UHCI::Start() current loop %u, status %u\n" , i , status );
		if ( status & UHCI_USBSTS_HCHALT )
			snooze( 1000 );
		else
		{
			running = true;
			break;
		}
	}
	
	if (!running)
	{
		TRACE( "UHCI::Start() Controller won't start running\n" );
		return B_ERROR;
	}
	
	TRACE( "UHCI::Start() Controller is started. USBSTS: %u curframe: %u \n" , UHCI::pci_module->read_io_16( m_reg_base + UHCI_USBSTS ) , UHCI::pci_module->read_io_16( m_reg_base + UHCI_FRNUM ) );
	return BusManager::Start();
}

status_t UHCI::SubmitTransfer( Transfer *t )
{
	dprintf( "UHCI::SubmitPacket( Transfer &t ) called!!!\n" );
	
	//Short circuit the root hub
	if ( m_rh_address == t->GetPipe()->GetDeviceAddress() )
		return m_rh->SubmitTransfer( t );
		
	if ( t->GetPipe()->GetType() == Pipe::Control )
		return InsertControl( t );
	
	return B_ERROR;
}

void UHCI::GlobalReset()
{
	UHCI::pci_module->write_io_16( m_reg_base + UHCI_USBCMD , UHCI_USBCMD_GRESET );
	snooze( 100000 );
	UHCI::pci_module->write_io_16( m_reg_base + UHCI_USBCMD , 0 );
}

status_t UHCI::Reset()
{
	UHCI::pci_module->write_io_16( m_reg_base + UHCI_USBCMD , UHCI_USBCMD_HCRESET );
	snooze( 100000 );
	if ( UHCI::pci_module->read_io_16( m_reg_base + UHCI_USBCMD ) & UHCI_USBCMD_HCRESET )
		return B_ERROR;
	return B_OK;
}

int32 UHCI::Interrupt()
{
	uint16 status = UHCI::pci_module->read_io_16( m_reg_base + UHCI_USBSTS );
	TRACE( "USB UHCI::Interrupt()\n" );
	//Check if we really had an interrupt
	if ( !( status | UHCI_INTERRUPT_MASK ) )
		return B_UNHANDLED_INTERRUPT;
	
	//Get funky
	if ( status | UHCI_USBSTS_USBINT )
	{
		//A transfer finished
		TRACE( "USB UHCI::Interrupt() transfer finished! [party]\n" );
	}
	else if ( status | UHCI_USBSTS_ERRINT )
	{
		TRACE( "USB UHCI::Interrupt() transfer error! [cry]\n" );
	}		
	return B_HANDLED_INTERRUPT;
}

status_t UHCI::InsertControl( Transfer *t )
{
	TRACE("USB UCHI::InsertControl() frnum %u , usbsts reg %u\n" , UHCI::pci_module->read_io_16( m_reg_base + UHCI_FRNUM ), UHCI::pci_module->read_io_16( m_reg_base + UHCI_USBSTS ) );

	//HACK: this one is to prevent rogue transfers from happening
	if ( t->GetBuffer() != 0 )
		return B_ERROR;

	//Please note that any data structures must be aligned on a 16 byte boundary
	//Also, due to the strange ways of C++' void* handling, this code is much messier
	//than it actually should be. Forgive me. Or blame the compiler.
	//First, set up a Queue Head for the transfer
	uhci_qh *topqh;
	void  *topqh_phy;
	if ( m_stack->AllocateChunk( (void **)&topqh , &topqh_phy , 32 ) < B_OK )
	{
		TRACE( "UHCI::InsertControl(): Failed to allocate a QH\n" );
		return ENOMEM;
	}
	topqh->link_phy = QH_TERMINATE;
	topqh->link_log = 0;
	topqh->this_phy = (addr_t)topqh_phy;
	

	//Allocate the transfer descriptor for the transfer
	uhci_td *firsttd;
	void *firsttd_phy;
	if ( m_stack->AllocateChunk( (void**)&firsttd , &firsttd_phy , 32 ) < B_OK )
	{
		TRACE( "UHCI::InsertControl(): Failed to allocate the first TD\n" );
		m_stack->FreeChunk( topqh , topqh_phy , 32 );
		return ENOMEM;
	}
	firsttd->this_phy = (addr_t)firsttd_phy;
	//Set the 'status' field of the td
	if ( t->GetPipe()->GetSpeed() == Pipe::LowSpeed )
		firsttd->status = TD_STATUS_LOWSPEED | TD_STATUS_ACTIVE;
	else
		firsttd->status = TD_STATUS_ACTIVE;

	//Set the 'token' field of the td
	firsttd->token = ( ( sizeof(usb_request_data) - 1 ) << 21 ) | ( t->GetPipe()->GetEndpointAddress() << 15 )
	                  | ( t->GetPipe()->GetDeviceAddress() << 8 ) | ( 0x2D );
	//Create a physical space for the setup request
	if ( m_stack->AllocateChunk( &(firsttd->buffer_log) , &(firsttd->buffer_phy) , sizeof (usb_request_data) ) )
	{
		TRACE( "UHCI::InsertControl(): Unable to allocate space for the SETUP buffer\n" );
		m_stack->FreeChunk( topqh , topqh_phy , 32 );
		m_stack->FreeChunk( firsttd , firsttd_phy , 32 );
		return ENOMEM;
	}
	memcpy( t->GetRequestData() , firsttd->buffer_log , sizeof(usb_request_data) );
	
	//Link this thing in the queue head
	topqh->element_phy = (addr_t)firsttd_phy;
	topqh->element_log = firsttd;
	
	
	//TODO: split the buffer into max transfer sizes

	
	//Finally, create a status td
	uhci_td *statustd;
	void *statustd_phy;
	if ( m_stack->AllocateChunk( (void **)&statustd , &statustd_phy , 32 ) < B_OK )
	{
		TRACE( "UHCI::InsertControl(): Failed to allocate the status TD\n" );
		return ENOMEM;
	}
	//Set the 'status' field of the td to interrupt on complete
	if ( t->GetPipe()->GetSpeed() == Pipe::LowSpeed )
		statustd->status = TD_STATUS_LOWSPEED | TD_STATUS_IOC;
	else
		statustd->status =  TD_STATUS_IOC;	
	
	//Set the 'token' field of the td (always DATA1) and a null buffer
	statustd->token = TD_TOKEN_NULL | TD_TOKEN_DATA1 | ( t->GetPipe()->GetEndpointAddress() << 15 )
	                  | ( t->GetPipe()->GetDeviceAddress() << 8 ) |  0x69 ;
	
	//Invalidate the buffer field
	statustd->buffer_phy = statustd->buffer_log = 0;
	
	//Link into the previous transfer descriptor
	firsttd->link_phy = (addr_t)statustd_phy | TD_DEPTH_FIRST;
	firsttd->link_log = statustd;
	
	//This is the end of this chain, so don't link to any next QH/TD
	statustd->link_phy = QH_TERMINATE;
	statustd->link_log = 0;


	//First, add the transfer to the list of transfers
	t->SetHostPrivate( new hostcontroller_priv );
	t->GetHostPrivate()->topqh = topqh;
	t->GetHostPrivate()->firsttd = firsttd;
	t->GetHostPrivate()->lasttd = statustd;
	m_transfers.PushBack( t );
	
	//Secondly, append the qh to the control list
	//a) if the control queue is empty, make this the first element
	if ( ( m_qh_control->element_phy & QH_TERMINATE ) != 0 )
	{
		m_qh_control->element_phy = topqh->this_phy;
		m_qh_control->link_log = (void *)topqh;
		TRACE( "USB UHCI::InsertControl() First transfer in QUeue\n" );
	}
	//b) there are control transfers linked, append to the queue
	else
	{
		uhci_qh *qh = (uhci_qh *)(m_qh_control->link_log);
		while ( ( qh->link_phy & QH_TERMINATE ) == 0 )
		{
			TRACE( "USB UHCI::InsertControl() Looping\n" );
			qh = (uhci_qh *)(qh->link_log);
		}
		qh->link_phy = topqh->this_phy;
		qh->link_log = (void *)topqh;
		TRACE( "USB UHCI::InsertControl() Appended transfers in queue\n" );
	}
	return EINPROGRESS;	
}
	
pci_module_info *UHCI::pci_module = 0;
