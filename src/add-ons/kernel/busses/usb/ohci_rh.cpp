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
//------------------------------------------------------------------------------


#include "ohci.h"
#include <PCI.h>

usb_device_descriptor ohci_devd =
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

usb_configuration_descriptor ohci_confd =
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

usb_interface_descriptor ohci_intd =
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

usb_endpoint_descriptor ohci_endd =
{
	0x07 ,					//Size
	USB_DESCRIPTOR_ENDPOINT,
	USB_REQTYPE_DEVICE_IN | 1, //1 from freebsd driver
	0x3 ,					// Interrupt
	8 ,						// Max packet size
	0xFF					// Interval 256
};

usb_hub_descriptor ohci_hubd =
{
	0x09 ,					//Including deprecated powerctrlmask
	USB_DESCRIPTOR_HUB,
	1,						//Number of ports
	0x02 | 0x01 ,			//Hub characteristics FIXME
	50 ,					//Power on to power good
	0,						// Current
	0x00					//Both ports are removable
};

//Implementation
OHCIRootHub::OHCIRootHub( OHCI *ohci , int8 devicenum )
		   : Hub( ohci->RootObject() , ohci_devd , devicenum , USB_SPEED_FULLSPEED )
{
	m_ohci = ohci;
}

status_t OHCIRootHub::SubmitTransfer( Transfer *t )
{
	return B_OK;
}

void OHCIRootHub::UpdatePortStatus(void)
{
	// nothing yet
}
