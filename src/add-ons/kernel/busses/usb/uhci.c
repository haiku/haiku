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
#include "host_controller.h"

#define UHCI_DEBUG
#ifdef UHCI_DEBUG
#define TRACE dprintf
#else
#define TRACE silent
void silent( const char * , ... ) {}
#endif


static status_t init_hardware(void);
static status_t submit_packet( usb_packet_t *packet );
static status_t rh_submit_packet( usb_packet_t *packet );

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
	m_data->reg_base = m_device->u.h0.base_registers[0];
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
	
	//Set up the frame list
	m_data->framearea = map_physical_memory( "uhci framelist" , m_data->framelist_phy ,
	                                         4096 , B_ANY_KERNEL_BLOCK_ADDRESS ,
	                                         B_WRITE_AREA , m_data->framelist );
	                                         
	//!!!!!!!!!!!!!! Check the area
	
	//This is enough for now, the most time consuming hardware tasks have been done now
	return B_OK;
}

/* ++++++++++
Inserting packets in the queue
++++++++++ */
status_t submit_packet( usb_packet_t *packet )
{
	dprintf( "uhci submit_packet(): CALLED\n");
	//Do the following:
	//1. Check if the packet really belongs to this bus -- no way of doing that now
	//2. Acquire the spinlock of the packet -- still needs to be fixed

	if ( packet->address == 0 ) //First device of a bus has address 0 per definition
	{
		rh_submit_packet( packet );
		goto out;
	}
	//4. Determine the type of transfer and send it to the proper function
	//5. Release the spinlock
out:
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
	0xFF					// Interval
};



status_t rh_submit_packet( usb_packet_t *packet )
{
	usb_request_data *request = (usb_request_data *)packet->requestdata;
	status_t retval;
	
	TRACE( "USB UHCI: rh_submit_packet called\n" );
	
	switch( request->Request )
	{
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
				memcpy( packet->buffer , (void *)&uhci_devd , packet->bufferlength );
				*(packet->actual_length) = packet->bufferlength; 
				retval = B_OK;
				break;
			case RH_CONFIG_DESCRIPTOR:
				memcpy( packet->buffer , (void *)&uhci_confd , packet->bufferlength );
				*(packet->actual_length) = packet->bufferlength;
				retval =  B_OK;
				break;
			case RH_SET_CONFIG:
				retval = B_OK;
				break;
			default:
				retval = EINVAL;
				break;
			}
			break;
		}
	default: 
		retval = EINVAL;
		break;
	}
	
	return retval;
} 	


