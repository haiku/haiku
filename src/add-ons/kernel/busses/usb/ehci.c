//------------------------------------------------------------------------------
//	Copyright (c) 2003-2004, Niels S. Reedijk
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

#define UHCI_DEBUG
#ifdef UHCI_DEBUG
#define TRACE dprintf
#else
#define TRACE silent
void silent( const char * , ... ) {}
#endif


static status_t init_hardware();


/* ++++++++++
This is the implementation of the UHCI controller for the OpenBeOS USB stack
++++++++++ */
static int32
ehci_std_ops( int32 op , ... )
{
	switch (op)
	{
		case B_MODULE_INIT:
			TRACE( "ehci_module: init the module\n" );
			return init_hardware();
		case B_MODULE_UNINIT:
			TRACE( "ehci_module: uninit the module\n" );
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}	

struct module_info ehci_module = {
	"busses/usb/ehci/nielx", 
	NULL,						// No flag like B_KEEP_LOADED : the usb module does that
	ehci_std_ops
};

module_info *modules[] = {
	(module_info *)&ehci_module ,
	NULL
};

/* ++++++++++
Internal variables
++++++++++ */
typedef struct ehci_properties 
{
	uint32		reg_base;		//Base address of the registers
	pci_info 	*pcii;			//pci-info struct
} ehci_properties_t;

static pci_module_info 	*m_pcimodule = 0;
static pci_info			*m_device = 0;
static ehci_properties_t*m_data = 0;

/* ++++++++++
Register/Bit constants
++++++++++ */

enum {
	USBCMD = 0x0				//Command register (2byte)
};

enum USBCMD_commands {
	RS = 0x1 ,					//Run=1 Stop=0
	HCRESET = 0x2				//Host controller reset
};

/* ++++++++++
The hardware management functions
++++++++++ */
static status_t
init_hardware()
{
	status_t status;
	pci_info *item;
	int i;
	
	// Try if the PCI module is loaded (it would be weird if it wouldn't, but alas)
	if( ( status = get_module( B_PCI_MODULE_NAME, (module_info **)&m_pcimodule )) != B_OK) 
	{
		TRACE( "ehci_nielx init_hardware(): Get PCI module failed! %lu \n", status);
		return status;
	}

	// TODO: in the future we might want to support multiple host controllers.
	item = (pci_info *)malloc( sizeof( pci_info ) );
	for ( i = 0 ; m_pcimodule->get_nth_pci_info( i , item ) == B_OK ; i++ )
	{
		//class_base = 0C (serial bus) class_sub = 03 (usb) prog_int: 20 (EHCI)
		if ( ( item->class_base == 0x0C ) && ( item->class_sub == 0x03 ) &&
			 ( item->class_api  == 0x20 ) )
		{
			if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) 
			{
				TRACE( "ehci_nielx init_hardware(): found with invalid IRQ - check IRQ assignement\n");
				continue;
			}
			TRACE("ehci_nielx init_hardware(): found at IRQ %u \n", item->u.h0.interrupt_line);
			m_device = item;
			break;
		}
	}
	
	if ( m_device == 0 )
	{
		TRACE( "ehci_nielx init hardware(): no devices found\n" );
		put_module( B_PCI_MODULE_NAME );
		return ENODEV;
	}
	
	//Initialise the host controller
	m_data = (ehci_properties_t *)malloc( sizeof( ehci_properties_t ) );
	m_data->pcii = m_device;
	m_data->reg_base = m_device->u.h0.base_registers[0];
	
	{
		/* enable pci address access */	
		unsigned char cmd;
		cmd = m_pcimodule->read_pci_config(m_data->pcii->bus, m_data->pcii->device, m_data->pcii->function, PCI_command, 2);
		cmd = cmd | PCI_command_io | PCI_command_master | PCI_command_memory;
		m_pcimodule->write_pci_config(m_data->pcii->bus, m_data->pcii->device, m_data->pcii->function, PCI_command, 2, cmd );
	}
	
	//Do a host reset
	//m_pcimodule->write_io_16( data->reg_base + USBCMD , HCRESET );
	//usleep(50);
	
	
	return B_OK;
}
