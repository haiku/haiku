//------------------------------------------------------------------------------
//	Copyright (c) 2003-2005, Niels S. Reedijk
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

#ifndef USBSPEC_P
#define USBSPEC_P

#include <KernelExport.h>
#include <util/Vector.h>
#include <USB.h>
#include <util/kernel_cpp.h>

#define USB_MAX_AREAS 8
struct memory_chunk
{
	addr_t next_item;
	addr_t physical;
};

/* ++++++++++
Important data from the USB spec (not interesting for drivers)
++++++++++ */

#define POWER_DELAY 

struct usb_request_data
{
	uint8   RequestType; 
	uint8   Request; 
	uint16  Value; 
	uint16  Index; 
	uint16  Length; 
};

struct usb_hub_descriptor
{
	uint8 bDescLength;
	uint8 bDescriptorType;
	uint8 bNbrPorts;
	uint16 wHubCharacteristics;
	uint8 bPwrOn2PwrGood;
	uint8 bHucContrCurrent;
	uint8 DeviceRemovable; //Should be variable!!!
	uint8 PortPwrCtrlMask;  //Deprecated
};

#define USB_DESCRIPTOR_HUB 0x29

// USB Spec 1.1 page 273
struct usb_port_status
{
	uint16 status;
	uint16 change;
};

//The bits in the usb_port_status struct
// USB 1.1 spec page 274
#define PORT_STATUS_CONNECTION    0x1
#define PORT_STATUS_ENABLE        0x2
#define PORT_STATUS_SUSPEND       0x4
#define PORT_STATUS_OVER_CURRENT  0x8
#define PORT_STATUS_RESET         0x10
#define PORT_STATUS_POWER         0x100
#define PORT_STATUS_LOW_SPEED     0x200

//The feature requests with ports
// USB 1.1 spec page 268
#define PORT_CONNECTION    0
#define PORT_ENABLE        1
#define PORT_SUSPEND       2
#define PORT_OVER_CURRENT  3
#define PORT_RESET         4
#define PORT_POWER         8
#define PORT_LOW_SPEED     9
#define C_PORT_CONNECTION  16
#define C_PORT_ENABLE      17
#define C_PORT_SUSPEND     18
#define C_PORT_OVER_CURRENT 19
#define C_PORT_RESET       20

#endif