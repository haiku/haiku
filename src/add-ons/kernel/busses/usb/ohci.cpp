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
ohci_add_to(Stack *stack)
{
	status_t status;
	pci_info *item;
	bool found = false;
	int i;
	
#ifdef TRACE_USB
	set_dprintf_enabled(true); 
	load_driver_symbols("ohci");
#endif
	
	//
	// 	Try if the PCI module is loaded
	//
	if( ( status = get_module( B_PCI_MODULE_NAME, (module_info **)&( OHCI::pci_module ) ) ) != B_OK) {
		TRACE(("USB_ OHCI: init_hardware(): Get PCI module failed! %lu \n", status));
		return status;
	}
	TRACE(("usb_ohci init_hardware(): Setting up hardware\n"));
	item = new pci_info;
	for ( i = 0 ; OHCI::pci_module->get_nth_pci_info( i , item ) == B_OK ; i++ ) {
		if ( ( item->class_base == PCI_serial_bus ) && ( item->class_sub == PCI_usb )
			 && ( item->class_api  == PCI_usb_ohci ) )
		{
			if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) {
				TRACE(("USB OHCI: init_hardware(): found with invalid IRQ - check IRQ assignement\n"));
				continue;
			}
			TRACE(("USB OHCI: init_hardware(): found at IRQ %u \n", item->u.h0.interrupt_line));
			OHCI *bus = new(std::nothrow) OHCI(item, stack);
			
			if (!bus) {
				delete item;
				OHCI::pci_module = NULL;
				put_module(B_PCI_MODULE_NAME);
				return B_NO_MEMORY;
			}
			
			if (bus->InitCheck() != B_OK) {
				TRACE(("usb_ohci: bus failed to initialize...\n"));
				delete bus;
				continue;
			}

			bus->Start();			
			stack->AddBusManager( bus );
			found = true;
		}
	}
	
	if (!found) {
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
		"busses/usb/ohci", 
		0,						// No flag like B_KEEP_LOADED : the usb module does that
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

module_info *modules[] = { 
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

OHCI::OHCI(pci_info *info, Stack *stack)
	:	BusManager(stack),
		fRegisterArea(-1),
		fHccaArea(-1),
		fDummyControl(0),
		fDummyBulk(0),
		fDummyIsochronous(0),
		fRootHub(0),
		fRootHubAddress(0),
		fNumPorts(0)
{
	fPcii = info;
	fStack = stack;
	int i;
	TRACE(("USB OHCI: constructing new BusManager\n"));
	
	fInitOK = false;
	
	for(i = 0; i < OHCI_NO_EDS; i++) //Clear the interrupt list
		fInterruptEndpoints[i] = 0;

	// enable busmaster and memory mapped access 
	uint16 cmd = OHCI::pci_module->read_pci_config(fPcii->bus, fPcii->device, fPcii->function, PCI_command, 2);
	cmd &= ~PCI_command_io;
	cmd |= PCI_command_master | PCI_command_memory;
	OHCI::pci_module->write_pci_config(fPcii->bus, fPcii->device, fPcii->function, PCI_command, 2, cmd );

	//
	// 5.1.1.2 map the registers
	//
	addr_t registeroffset = pci_module->read_pci_config(info->bus,
		info->device, info->function, PCI_base_registers, 4);
	registeroffset &= PCI_address_memory_32_mask;	
	TRACE(("OHCI: iospace offset: %lx\n" , registeroffset));
	fRegisterArea = map_physical_memory("OHCI base registers", (void *)registeroffset,
		B_PAGE_SIZE, B_ANY_KERNEL_BLOCK_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_READ_AREA | B_WRITE_AREA, (void **)&fRegisters);
	if (fRegisterArea < B_OK) {
		TRACE(("USB OHCI: error mapping the registers\n"));
		return;
	}
	
	//	Get the revision of the controller
	uint32 rev = ReadReg(OHCI_REVISION) & 0xff;
	
	//	Check the revision of the controller. The revision should be 10xh
	TRACE((" OHCI: Version %ld.%ld%s\n", OHCI_REV_HI(rev), OHCI_REV_LO(rev),OHCI_REV_LEGACY(rev) ? ", legacy support" : ""));
	if (OHCI_REV_HI(rev) != 1 || OHCI_REV_LO(rev) != 0) {
		TRACE(("USB OHCI: Unsupported OHCI revision of the ohci device\n"));
		return;
	}
	
	// Set up the Host Controller Communications Area
	void *hcca_phy;
	fHccaArea = fStack->AllocateArea((void**)&fHcca, &hcca_phy,
										B_PAGE_SIZE, "OHCI HCCA");
	if (fHccaArea < B_OK) {
		TRACE(("USB OHCI: Error allocating HCCA block\n"));
		return;
	}
	memset((void*)fHcca, 0, sizeof(ohci_hcca));

	//
	// 5.1.1.3 Take control of the host controller
	//
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

	//	
	// 5.1.1.4 Set Up Host controller
	//
	// Dummy endpoints
	fDummyControl = AllocateEndpoint();
	if (!fDummyControl)
		return;
	fDummyBulk = AllocateEndpoint();
	if (!fDummyBulk)
		return;
	fDummyIsochronous = AllocateEndpoint();
	if (!fDummyIsochronous)
		return;
	//Create the interrupt tree
	//Algorhythm kindly borrowed from NetBSD code
	for(i = 0; i < OHCI_NO_EDS; i++) {
		fInterruptEndpoints[i] = AllocateEndpoint();
		if (!fInterruptEndpoints[i])
			return;
		if (i != 0)
			fInterruptEndpoints[i]->SetNext(fInterruptEndpoints[(i-1) / 2]);
		else
			fInterruptEndpoints[i]->SetNext(fDummyIsochronous);
	}
	for (i = 0; i < OHCI_NO_INTRS; i++)
		fHcca->hcca_interrupt_table[revbits[i]] =
			fInterruptEndpoints[OHCI_NO_EDS-OHCI_NO_INTRS+i]->physicaladdress;
	
	//Go to the hardware part of the initialisation
	uint32 frameinterval = ReadReg(OHCI_FM_INTERVAL);
	
	WriteReg(OHCI_COMMAND_STATUS, OHCI_HCR);
	for (i = 0; i < 10; i++){
		snooze(10);				//Should be okay in one run: 10 microseconds for reset
		if (!(ReadReg(OHCI_COMMAND_STATUS) & OHCI_HCR))
			break;
	}
	if (ReadReg(OHCI_COMMAND_STATUS) & OHCI_HCR) {
		TRACE(("USB OHCI: Error resetting the host controller\n"));
		return;
	}
	
	WriteReg(OHCI_FM_INTERVAL, frameinterval);
	//We now have 2 ms to finish the following sequence. 
	//TODO: maybe add spinlock protection???
	
	WriteReg(OHCI_CONTROL_HEAD_ED, (uint32)fDummyControl->physicaladdress);
	WriteReg(OHCI_BULK_HEAD_ED, (uint32)fDummyBulk->physicaladdress);
	WriteReg(OHCI_HCCA, (uint32)hcca_phy);
	
	WriteReg(OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTRS);
	WriteReg(OHCI_INTERRUPT_ENABLE, OHCI_NORMAL_INTRS);
	
	//
	// 5.1.1.5 Begin Sending SOFs
	//
	uint32 control = ReadReg(OHCI_CONTROL);
	control &= ~(OHCI_CBSR_MASK | OHCI_LES | OHCI_HCFS_MASK | OHCI_IR);
	control |= OHCI_PLE | OHCI_IE | OHCI_CLE | OHCI_BLE |
		OHCI_RATIO_1_4 | OHCI_HCFS_OPERATIONAL;
	WriteReg(OHCI_CONTROL, control);
	
	//The controller is now operational, end of 2ms block.
	uint32 interval = OHCI_GET_IVAL(frameinterval);
	WriteReg(OHCI_PERIODIC_START, OHCI_PERIODIC(interval));
	
	//Work on some Roothub settings
	uint32 desca = ReadReg(OHCI_RH_DESCRIPTOR_A);
	WriteReg(OHCI_RH_DESCRIPTOR_A, desca | OHCI_NOCP); //FreeBSD source does this to avoid a chip bug
	//Enable power
	WriteReg(OHCI_RH_STATUS, OHCI_LPSC);
	snooze(5000); //Wait for power to stabilize (5ms)
	WriteReg(OHCI_RH_DESCRIPTOR_A, desca);
	snooze(5000); //Delay required by the AMD756 because else the # of ports might be misread
	
	fInitOK = true;
}

OHCI::~OHCI()
{
	if (fHccaArea > 0)
		delete_area(fHccaArea);
	if (fRegisterArea > 0)
		delete_area(fRegisterArea);
	if (fDummyControl)
		FreeEndpoint(fDummyControl);
	if (fDummyBulk)
		FreeEndpoint(fDummyBulk);
	if (fDummyIsochronous)
		FreeEndpoint(fDummyIsochronous);
	if (fRootHub)
		delete fRootHub;
	for (int i = 0; i < OHCI_NO_EDS; i++)
		if (fInterruptEndpoints[i])
			FreeEndpoint(fInterruptEndpoints[i]);
}

status_t
OHCI::Start()
{
	if (!(ReadReg(OHCI_CONTROL) & OHCI_HCFS_OPERATIONAL)) {
		TRACE(("USB OHCI::Start(): Controller not started. TODO: find out what happens.\n"));
		return B_ERROR;
	}
	
	fRootHubAddress = AllocateAddress();
	fNumPorts = OHCI_GET_PORT_COUNT(ReadReg(OHCI_RH_DESCRIPTOR_A));
	
	fRootHub = new(std::nothrow) OHCIRootHub(this, fRootHubAddress);
	if (!fRootHub) {
		TRACE_ERROR(("USB OHCI::Start(): no memory to allocate root hub\n"));
		return B_NO_MEMORY;
	}

	if (fRootHub->InitCheck() < B_OK) {
		TRACE_ERROR(("USB OHCI::Start(): root hub failed init check\n"));
		return B_ERROR;
	}

	SetRootHub(fRootHub);
	TRACE(("USB OHCI::Start(): Succesful start\n"));
	return B_OK;
}

status_t OHCI::SubmitTransfer( Transfer *t )
{
	TRACE(("usb OHCI::SubmitTransfer: called for device %d\n", t->TransferPipe()->DeviceAddress()));

	if (t->TransferPipe()->DeviceAddress() == fRootHubAddress)
		return fRootHub->ProcessTransfer(t, this);

	return B_ERROR;
}

status_t
OHCI::GetPortStatus(uint8 index, usb_port_status *status)
{
	if (index > fNumPorts)
		return B_BAD_INDEX;
	
	status->status = status->change = 0;
	uint32 portStatus = ReadReg(OHCI_RH_PORT_STATUS(index));
	
	TRACE(("OHCIRootHub::GetPortStatus: Port %i Value 0x%lx\n", OHCI_RH_PORT_STATUS(index), portStatus));
	
	// status
	if (portStatus & OHCI_PORTSTATUS_CCS)
		status->status |= PORT_STATUS_CONNECTION;
	if (portStatus & OHCI_PORTSTATUS_PES)
		status->status |= PORT_STATUS_ENABLE;
	if (portStatus & OHCI_PORTSTATUS_PRS)
		status->status |= PORT_STATUS_RESET;
	if (portStatus & OHCI_PORTSTATUS_LSDA)
		status->status |= PORT_STATUS_LOW_SPEED;
	if (portStatus & OHCI_PORTSTATUS_PSS)
		status->status |= PORT_STATUS_SUSPEND;
	if (portStatus & OHCI_PORTSTATUS_POCI)
		status->status |= PORT_STATUS_OVER_CURRENT;
	if (portStatus & OHCI_PORTSTATUS_PPS)
		status->status |= PORT_STATUS_POWER;
	
	
	// change
	if (portStatus & OHCI_PORTSTATUS_CSC)
		status->change |= PORT_STATUS_CONNECTION;
	if (portStatus & OHCI_PORTSTATUS_PESC)
		status->change |= PORT_STATUS_ENABLE;
	if (portStatus & OHCI_PORTSTATUS_PSSC)
		status->change |= PORT_STATUS_SUSPEND;
	if (portStatus & OHCI_PORTSTATUS_OCIC)
		status->change |= PORT_STATUS_OVER_CURRENT;
	if (portStatus & OHCI_PORTSTATUS_PRSC)
		status->change |= PORT_STATUS_RESET;
	
	return B_OK;
}

status_t
OHCI::SetPortFeature(uint8 index, uint16 feature)
{
	if (index > fNumPorts)
		return B_BAD_INDEX;
	
	switch (feature) {
		case PORT_RESET:
			WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_PORTSTATUS_PRS);
			return B_OK;
		
		case PORT_POWER:
			WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_PORTSTATUS_PPS);
			return B_OK;
	}
	
	return B_BAD_VALUE;
}

status_t
OHCI::ClearPortFeature(uint8 index, uint16 feature)
{
	if (index > fNumPorts)
		return B_BAD_INDEX;
	
	switch (feature) {
		case C_PORT_RESET:
			WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_PORTSTATUS_CSC);
			return B_OK;
		
		case C_PORT_CONNECTION:
			WriteReg(OHCI_RH_PORT_STATUS(index), OHCI_PORTSTATUS_CSC);
			return B_OK;
	}
	
	return B_BAD_VALUE;
}

Endpoint *
OHCI::AllocateEndpoint()
{
	Endpoint *endpoint = new Endpoint;
	void *phy;
	if (fStack->AllocateChunk((void **)&endpoint->ed, &phy, sizeof(ohci_endpoint_descriptor)) != B_OK) {
		TRACE(("OHCI::AllocateEndpoint(): Error Allocating Endpoint\n"));
		return 0;
	}
	endpoint->physicaladdress = (addr_t)phy;
	
	memset((void *)endpoint->ed, 0, sizeof(ohci_endpoint_descriptor));
	endpoint->ed->ed_flags = OHCI_ED_SKIP;
	return endpoint;
}

void
OHCI::FreeEndpoint(Endpoint *end)
{
	fStack->FreeChunk((void *)end->ed, (void *) end->physicaladdress, sizeof(ohci_endpoint_descriptor));
	delete end;
}


pci_module_info *OHCI::pci_module = 0;

void
OHCI::WriteReg(uint32 reg, uint32 value)
{
	*(volatile uint32 *)(fRegisters + reg) = value;
}

uint32
OHCI::ReadReg(uint32 reg)
{
	return *(volatile uint32 *)(fRegisters + reg);
}
