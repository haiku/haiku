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
#include <USB3.h>
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
			TRACE(("ohci_module: init the module\n"));
			return B_OK;
		case B_MODULE_UNINIT:
			TRACE(("ohci_module: uninit the module\n"));
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

static status_t
ohci_add_to( Stack *stack )
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
		TRACE(("USB_ OHCI: init_hardware(): Get PCI module failed! %lu \n", status));
		return status;
	}
	TRACE(("usb_ohci init_hardware(): Setting up hardware\n"));
	//
	// 	TODO: in the future we might want to support multiple host controllers.
	//
	item = new pci_info;
	for ( i = 0 ; OHCI::pci_module->get_nth_pci_info( i , item ) == B_OK ; i++ )
	{
		if ( ( item->class_base == PCI_serial_bus ) && ( item->class_sub == PCI_usb ) &&
			 ( item->class_api  == PCI_usb_ohci ) )
		{
			if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) 
			{
				TRACE(("USB OHCI: init_hardware(): found with invalid IRQ - check IRQ assignement\n"));
				continue;
			}
			TRACE(("USB OHCI: init_hardware(): found at IRQ %u \n", item->u.h0.interrupt_line));
			OHCI *bus = new OHCI( item , stack );
			if ( bus->InitCheck() != B_OK )
			{
				delete bus;
				break;
			}
			
			stack->AddBusManager( bus );
			bus->Start();
			found = true;
			break;
		}
	}
	
	if ( found == false )
	{
		TRACE(("USB OHCI: init hardware(): no devices found\n"));
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
	:	BusManager(stack)
{
	m_pcii = info;
	m_stack = stack;
	int i;
	hcd_soft_endpoint *sed, *psed;
	TRACE(("USB OHCI: constructing new BusManager\n"));
	
	fInitOK = false;

	// enable busmaster and memory mapped access 
	uint16 cmd = OHCI::pci_module->read_pci_config(m_pcii->bus, m_pcii->device, m_pcii->function, PCI_command, 2);
	cmd &= ~PCI_command_io;
	cmd |= PCI_command_master | PCI_command_memory;
	OHCI::pci_module->write_pci_config(m_pcii->bus, m_pcii->device, m_pcii->function, PCI_command, 2, cmd );

	// 5.1.1.2 map the registers
	addr_t registeroffset = pci_module->read_pci_config(info->bus,
		info->device, info->function, PCI_base_registers, 4);
	registeroffset &= PCI_address_memory_32_mask;	
	TRACE(("OHCI: iospace offset: %lx\n" , registeroffset));
	m_register_area = map_physical_memory("OHCI base registers", (void *)registeroffset,
		B_PAGE_SIZE, B_ANY_KERNEL_BLOCK_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_READ_AREA | B_WRITE_AREA, (void **)&m_registers);
	if (m_register_area < B_OK) {
		TRACE(("USB OHCI: error mapping the registers\n"));
		return;
	}
	
	//	Get the revision of the controller
	
	uint32 rev = ReadReg(OHCI_REVISION) & 0xff;
	
	//	Check the revision of the controller. The revision should be 10xh
	
	TRACE((" OHCI: Version %ld.%ld%s\n", OHCI_REV_HI(rev), OHCI_REV_LO(rev),OHCI_REV_LEGACY(rev) ? ", legacy support" : ""));
	if (OHCI_REV_HI(rev) != 1 || OHCI_REV_LO(rev) != 0) {
		TRACE(("USB OHCI: Unsupported OHCI revision of the ohci device\n"));
		delete_area(m_register_area);
		return;
	}
	
	// Set up the Host Controller Communications Area
	void *hcca_phy;
	m_hcca_area = m_stack->AllocateArea((void**)&m_hcca, &hcca_phy,
										B_PAGE_SIZE, "OHCI HCCA");
	if (m_hcca_area < B_OK) {
		TRACE(("USB OHCI: Error allocating HCCA block\n"));
		delete_area(m_register_area);
		return;
	}

	// 5.1.1.3 Take control of the host controller
	if (ReadReg(OHCI_CONTROL) & OHCI_IR) {
		TRACE(("USB OHCI: SMM is in control of the host controller\n"));
		WriteReg(OHCI_COMMAND_STATUS, OHCI_OCR);
		for (int i = 0; i < 100 && (ReadReg(OHCI_CONTROL) & OHCI_IR); i++)
			snooze(1000);
		if (ReadReg(OHCI_CONTROL) & OHCI_IR)
			TRACE(("USB OHCI: SMM doesn't respond... continueing anyway...\n"));
	} else if (!(ReadReg(OHCI_CONTROL) & OHCI_HCFS_RESET)) {
		TRACE(("USB OHCI: BIOS is in control of the host controller\n"));
		if (!(ReadReg(OHCI_CONTROL) & OHCI_HCFS_OPERATIONAL)) {
			WriteReg(OHCI_CONTROL, OHCI_HCFS_RESUME);
			snooze(USB_DELAY_BUS_RESET);
		}
	} else if (ReadReg(OHCI_CONTROL) & OHCI_HCFS_RESET) //Only if no BIOS/SMM control
		snooze(USB_DELAY_BUS_RESET);
	
	// 5.1.1.4 Set Up Host controller
	// initialize the data structures for the HCCA
	

	uint32 framevalue = ReadReg(OHCI_FM_NUMBER);
		

	m_roothub_base = 255; //Invalidate the Root Hub address

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

void
OHCI::WriteReg(uint32 reg, uint32 value)
{
	*(volatile uint32 *)(m_registers + reg) = value;
}

uint32
OHCI::ReadReg(uint32 reg)
{
	return *(volatile uint32 *)(m_registers + reg);
}
