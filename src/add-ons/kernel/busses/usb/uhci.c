//------------------------------------------------------------------------------
//	Copyright (c) 2003, Niels S. Reedijk
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
#include "util.h"
#include "host_controller.h"

#define UHCI_DEBUG
#ifdef UHCI_DEBUG
#define TRACE dprintf
#else
#define TRACE silent
void silent( const char * , ... ) {}
#endif


static status_t init_hardware(void);
static status_t submit_packet( usb_transfer_t *transfer );
static status_t rh_submit_packet( usb_transfer_t *transfer );
static void     rh_update_port_status(void);
//static int32    uhci_interrupt( void *d );

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
			return init_hardware();
		case B_MODULE_UNINIT:
			TRACE( "uhci_module: uninit the module\n" );
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}	

host_controller_info uhci_module = {
	{
		"busses/usb/uhci/nielx", 
		NULL,						// No flag like B_KEEP_LOADED : the usb module does that
		uhci_std_ops
	},
	NULL ,
	submit_packet
};

module_info *modules[] = 
{ 
	(module_info *) &uhci_module,
	NULL 
};

/* ++++++++++
Internal variables
++++++++++ */
typedef struct uhci_properties 
{
	uint32		reg_base;		//Base address of the registers
	pci_info 	*pcii;			//pci-info struct
	
	//Frame list memory
	area_id		framearea;
	void 		*framelist[1024];	//The frame list struct
	void		*framelist_phy;	//The physical pointer to the frame list
	
	//Root hub:
	uint8		rh_address;		// the address of the root hub
	usb_port_status port_status[2]; // the port status (maximum of two)
} uhci_properties_t;

static pci_module_info 	*m_pcimodule = 0;
static pci_info			*m_device = 0;
static uhci_properties_t*m_data = 0;

/* ++++++++++
Utility functions
++++++++++ */

static void
uhci_globalreset( uhci_properties_t *data )
{
	m_pcimodule->write_io_16( data->reg_base + UHCI_USBCMD , UHCI_USBCMD_GRESET );
	spin( 100 );
	m_pcimodule->write_io_16( data->reg_base + UHCI_USBCMD , 0 );
}

static status_t
uhci_reset( uhci_properties_t *data )
{
	m_pcimodule->write_io_16( data->reg_base + UHCI_USBCMD , UHCI_USBCMD_HCRESET );
	spin( 100 );
	if ( m_pcimodule->read_io_16( data->reg_base + UHCI_USBCMD ) & UHCI_USBCMD_HCRESET )
		return B_ERROR;
	return B_OK;
}

/* ++++++++++
The hardware management functions
++++++++++ */
static status_t
init_hardware(void)
{
	status_t status;
	pci_info *item;
	int i;
	
	// Try if the PCI module is loaded (it would be weird if it wouldn't, but alas)
	if( ( status = get_module( B_PCI_MODULE_NAME, (module_info **)&m_pcimodule )) != B_OK) 
	{
		TRACE( "USB_ UHCI: init_hardware(): Get PCI module failed! %lu \n", status);
		return status;
	}
	
	TRACE( "usb_uhci init_hardware(): Setting up hardware\n" );
	
	// TODO: in the future we might want to support multiple host controllers.
	item = (pci_info *)malloc( sizeof( pci_info ) );
	for ( i = 0 ; m_pcimodule->get_nth_pci_info( i , item ) == B_OK ; i++ )
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
			m_device = item;
			break;
		}
	}
	
	if ( m_device == 0 )
	{
		TRACE( "USB UHCI: init hardware(): no devices found\n" );
		free( item );
		put_module( B_PCI_MODULE_NAME );
		return ENODEV;
	}
	
	//Initialise the host controller
	m_data = (uhci_properties_t *)malloc( sizeof( uhci_properties_t ) );
	m_data->pcii = m_device;
	m_data->reg_base = m_pcimodule->read_pci_config(m_data->pcii->bus, m_data->pcii->device, m_data->pcii->function, 0x20 , 4 ) ^ 1;
	TRACE( "USB UHCI: iospace offset: %lx\n" , m_data->reg_base );
	m_data->rh_address = 255; //Invalidate the RH address
	{
		/* enable pci address access */	
		unsigned char cmd;
		cmd = m_pcimodule->read_pci_config(m_data->pcii->bus, m_data->pcii->device, m_data->pcii->function, PCI_command, 2);
		cmd = cmd | PCI_command_io | PCI_command_master | PCI_command_memory;
		m_pcimodule->write_pci_config(m_data->pcii->bus, m_data->pcii->device, m_data->pcii->function, PCI_command, 2, cmd );
	}
	
	//Do a host reset
	uhci_globalreset( m_data );
	if ( uhci_reset( m_data ) == B_ERROR )
	{
		TRACE( "USB UHCI: init_hardare(): host failed to reset\n" );
		free( item );
		put_module( B_PCI_MODULE_NAME );
		return ENODEV;
	}
	
	// Poll the status of the two ports
	rh_update_port_status();
	TRACE( "USB UHCI: init_hardware(): port1: %x port2: %x\n",
	       m_data->port_status[0].status , m_data->port_status[1].status );

	//Set up the frame list
	m_data->framearea = alloc_mem( &(m_data->framelist[0]) , &(m_data->framelist_phy) ,
	                                         4096 , "uhci framelist" );
	if ( m_data->framearea < B_OK )
	{
		TRACE( "USB UHCI: init_hardware(): unable to create an area for the frame pointer list\n" );
		free( item );
		put_module( B_PCI_MODULE_NAME );
		return ENODEV;
	}
	                                         
	//Make all frame list entries invalid
	for( i = 0 ; i < 1024 ; i++ )
		(int32)(m_data->framelist[i]) = 0x1;
	
	//Set base pointer
	m_pcimodule->write_io_32( m_data->reg_base + UHCI_FRBASEADD , (int32)(m_data->framelist_phy) );	
	
	// Install the interrupt handler
	//install_io_interrupt_handler( m_data->pcii->h0.interrupt_line ,
	//							  uhci_interrupt , m_data , NULL ); 
	//m_pcimodule->write_io_16( m_data->reg_base + USBINTR ,    
	
	//This is enough for now, the most time consuming hardware tasks have been done now
	return B_OK;
}

/* ++++++++++
Inserting transfers in the queue
++++++++++ */
status_t submit_packet( usb_transfer_t *transfer )
{
	dprintf( "uhci submit_packet(): CALLED\n");
	//Do the following:
	//1. Check if the packet really belongs to this bus -- no way of doing that now
	//2. Acquire the spinlock of the packet -- still needs to be fixed

	if ( transfer->pipe->deviceid == 1 ) //First device of a bus has address 1 per definition
	{
		rh_submit_packet( transfer );
		goto out;
	}
	//4. Determine the type of transfer and send it to the proper function
	//5. Release the spinlock
out:
	return B_OK;
}

/* ++++++++++
Root hub emulation
++++++++++ */
usb_device_descriptor uhci_devd =
{
	0x12,					//Descriptor size
	USB_DESCRIPTOR_DEVICE ,	//Type of descriptor
	0x110,					//USB 1.1
	0x09 ,					//Hub type
	0 ,						//Subclass
	0 ,						//Protocol
	64 ,					//Max packet size
	0 ,						//Vendor
	0 ,						//Product
	0x110 ,					//Version
	1 , 2 , 0 ,				//Other data
	1						//Number of configurations
};

usb_configuration_descriptor uhci_confd =
{
	0x09,					//Size
	USB_DESCRIPTOR_CONFIGURATION ,
	25 ,					//Total size (taken from BSD source)
	1 ,						//Number interfaces
	1 ,						//Value of configuration
	0 ,						//Number of configuration
	0x40 ,					//Self powered
	0						//Max power (0, because of self power)
};

usb_interface_descriptor uhci_intd =
{
	0x09 ,					//Size
	USB_DESCRIPTOR_INTERFACE ,
	0 ,						//Interface number
	0 ,						//Alternate setting
	1 ,						//Num endpoints
	0x09 ,					//Interface class
	0 ,						//Interface subclass
	0 ,						//Interface protocol
	0 ,						//Interface
};

usb_endpoint_descriptor uhci_endd =
{
	0x07 ,					//Size
	USB_DESCRIPTOR_ENDPOINT,
	USB_REQTYPE_DEVICE_IN | 1, //1 from freebsd driver
	0x3 ,					// Interrupt
	8 ,						// Max packet size
	0xFF					// Interval 256
};

usb_hub_descriptor uhci_hubd =
{
	0x09 ,					//Including deprecated powerctrlmask
	USB_DESCRIPTOR_HUB,
	1,						//Number of ports
	0x02 | 0x01 ,			//Hub characteristics FIXME
	50 ,					//Power on to power good
	0,						// Current
	0x00					//Both ports are removable
};

status_t 
rh_submit_packet( usb_transfer_t *transfer )
{
	usb_request_data *request = transfer->request;
	status_t retval;
	
	uint16 port; //used in RH_CLEAR/SET_FEATURE
	
	TRACE( "USB UHCI: rh_submit_packet called. Request: %u\n" , request->Request );
	
	switch( request->Request )
	{
	case RH_GET_STATUS:
		if ( request->Index == 0 )
		{
			//Get the hub status -- everything as 0 means that it is all-rigth
			memset( transfer->buffer , NULL , sizeof(get_status_buffer) );
			retval = B_OK;
			break;
		}
		else if (request->Index > uhci_hubd.bNbrPorts )
		{
			//This port doesn't exist
			retval = EINVAL;
			break;
		}
		//Get port status
		rh_update_port_status();
		memcpy( transfer->buffer , (void *)&(m_data->port_status[request->Index - 1]) , transfer->bufferlength );
		*(transfer->actual_length) = transfer->bufferlength;
		retval = B_OK;
		break;

	case RH_SET_ADDRESS:
		if ( request->Value >= 128 )
		{
			retval = EINVAL;
			break;
		}
		TRACE( "USB UHCI: rh_submit_packet RH_ADDRESS: %d\n" , request->Value );
		m_data->rh_address = request->Value;
		retval = B_OK;
		break;
	
	case RH_GET_DESCRIPTOR:
		{
			TRACE( "USB UHCI: rh_submit_packet GET_DESC: %d\n" , request->Value );
			switch ( request->Value )
			{
			case RH_DEVICE_DESCRIPTOR:
				memcpy( transfer->buffer , (void *)&uhci_devd , transfer->bufferlength );
				*(transfer->actual_length) = transfer->bufferlength; 
				retval = B_OK;
				break;
			case RH_CONFIG_DESCRIPTOR:
				memcpy( transfer->buffer , (void *)&uhci_confd , transfer->bufferlength );
				*(transfer->actual_length) = transfer->bufferlength;
				retval =  B_OK;
				break;
			case RH_INTERFACE_DESCRIPTOR:
				memcpy( transfer->buffer , (void *)&uhci_intd , transfer->bufferlength );
				*(transfer->actual_length) = transfer->bufferlength;
				retval = B_OK ;
				break;
			case RH_ENDPOINT_DESCRIPTOR:
				memcpy( transfer->buffer , (void *)&uhci_endd , transfer->bufferlength );
				*(transfer->actual_length) = transfer->bufferlength;
				retval = B_OK ;
				break;
			case RH_HUB_DESCRIPTOR:
				memcpy( transfer->buffer , (void *)&uhci_hubd , transfer->bufferlength );
				*(transfer->actual_length) = transfer->bufferlength;
				retval = B_OK;
				break;
			default:
				retval = EINVAL;
				break;
			}
			break;
		}

	case RH_SET_CONFIG:
		retval = B_OK;
		break;

	case RH_CLEAR_FEATURE:
		if ( request->Index == 0 )
		{
			//We don't support any hub changes
			TRACE( "UHCI: RH_CLEAR_FEATURE no hub changes!\n" );
			retval = EINVAL;
			break;
		}
		else if ( request->Index > uhci_hubd.bNbrPorts )
		{
			//Invalid port number
			TRACE( "UHCI: RH_CLEAR_FEATURE invalid port!\n" );
			retval = EINVAL;
			break;
		}
		
		TRACE("UHCI: RH_CLEAR_FEATURE called. Feature: %u!\n" , request->Value );
		switch( request->Value )
		{
		case PORT_RESET:
			port = m_pcimodule->read_io_16( m_data->reg_base + UHCI_PORTSC1 + (request->Index - 1 ) * 2 );
			port &= ~UHCI_PORTSC_RESET;
			TRACE( "UHCI rh: port %x Clear RESET\n" , port );
			m_pcimodule->write_io_16( m_data->reg_base + UHCI_PORTSC1 + (request->Index - 1 ) * 2 , port );
		case C_PORT_CONNECTION:
			port = m_pcimodule->read_io_16( m_data->reg_base + UHCI_PORTSC1 + (request->Index - 1 ) * 2 );
			port = port & UHCI_PORTSC_DATAMASK;
			port |= UHCI_PORTSC_STATCHA;
			TRACE( "UHCI rh: port: %x\n" , port );
			m_pcimodule->write_io_16( m_data->reg_base + UHCI_PORTSC1 + (request->Index - 1 ) * 2 , port );
			retval = B_OK;
			break;
		default:
			retval = EINVAL;
			break;
		} //switch( transfer->value) 
		break;
		
	default: 
		retval = EINVAL;
		break;
	}
	
	return retval;
} 	

void
rh_update_port_status(void)
{
	int i;
	for ( i = 0; i <= 1 ; i++ )
	{
		uint16 newstatus = 0;
		uint16 newchange = 0;
		
		uint16 portsc = m_pcimodule->read_io_16( m_data->reg_base + UHCI_PORTSC1 + i * 2 );
		dprintf( "USB UHCI: port: %x status: 0x%x\n" , UHCI_PORTSC1 + i * 2 , portsc );
		//Set all individual bits
		if ( portsc & UHCI_PORTSC_CURSTAT )
			newstatus |= PORT_STATUS_CONNECTION;

		if ( portsc & UHCI_PORTSC_STATCHA )
			newchange |= PORT_STATUS_CONNECTION;
		
		if ( portsc & UHCI_PORTSC_ENABLED )
			newstatus |= PORT_STATUS_ENABLE;
		
		if ( portsc & UHCI_PORTSC_ENABCHA )
			newchange |= PORT_STATUS_ENABLE;
			
		// TODO: work out suspended/resume
		
		if ( portsc & UHCI_PORTSC_RESET )
			newstatus |= PORT_STATUS_RESET;
		
		//TODO: work out reset change...
		
		//The port is automagically powered on
		newstatus |= PORT_POWER;
		
		if ( portsc & UHCI_PORTSC_LOWSPEED )
			newstatus |= PORT_LOW_SPEED;
			
		//Update the stored port status
		m_data->port_status[i].status = newstatus;
		m_data->port_status[i].change = newchange;
	}
}
