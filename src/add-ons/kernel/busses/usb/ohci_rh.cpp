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

static usb_device_descriptor sOHCIRootHubDevice =
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

struct ohci_root_hub_configuration_s {
	usb_configuration_descriptor	configuration;
	usb_interface_descriptor		interface;
	usb_endpoint_descriptor			endpoint;
	usb_hub_descriptor				hub;
} _PACKED;

static ohci_root_hub_configuration_s sOHCIRootHubConfig = {
	{ // configuration descriptor
		9,						//Size
		USB_DESCRIPTOR_CONFIGURATION,
		34,						//Total size of the configuration
		1,						//Number interfaces
		1,						//Value of configuration
		0,						//Number of configuration
		0x40,					//Self powered
		0						//Max power (0, because of self power)
	},

	{ // interface descriptor
		9,						//Size
		USB_DESCRIPTOR_INTERFACE,
		0,						//Interface number
		0,						//Alternate setting
		1,						//Num endpoints
		0x09,					//Interface class
		0,						//Interface subclass
		0,						//Interface protocol
		0						//Interface
	},

	{ // endpoint descriptor
		7,						//Size
		USB_DESCRIPTOR_ENDPOINT,
		USB_REQTYPE_DEVICE_IN | 1, //1 from freebsd driver
		0x3,					// Interrupt
		8,						// Max packet size
		0xFF					// Interval 256
	},
	
	{
		9,						//Including deprecated powerctrlmask
		USB_DESCRIPTOR_HUB,
		0,						//Number of ports
		0x0000,					//Hub characteristics FIXME
		50,						//Power on to power good
		0,						// Current
		0x00,					//Both ports are removable
		0xff					// Depricated power control mask
	}
};

struct ohci_root_hub_string_s {
	uint8	length;
	uint8	descriptor_type;
	uint16	unicode_string[12];
} _PACKED;


static ohci_root_hub_string_s sOHCIRootHubStrings[3] = {
	{
		4,								// Descriptor length
		USB_DESCRIPTOR_STRING,			// Descriptor type
		{
			0x0409						// Supported language IDs (English US)
		}
	},

	{
		22,								// Descriptor length
		USB_DESCRIPTOR_STRING,			// Descriptor type
		{
			'H', 'A', 'I', 'K', 'U',	// Characters
			' ', 'I', 'n', 'c', '.'
		}
	},

	{
		26,								// Descriptor length
		USB_DESCRIPTOR_STRING,			// Descriptor type
		{
			'O', 'H', 'C', 'I', ' ',	// Characters
			'R', 'o', 'o', 't', 'H',
			'u', 'b'
		}
	}
};


OHCIRootHub::OHCIRootHub(OHCI *ohci, int8 deviceAddress)
		   : Hub(ohci->RootObject(), sOHCIRootHubDevice, deviceAddress , USB_SPEED_FULLSPEED )
{
}

status_t
OHCIRootHub::ProcessTransfer(Transfer *t, OHCI *ohci)
{
	if ((t->TransferPipe()->Type() & USB_OBJECT_CONTROL_PIPE) == 0)
		return B_ERROR;

	usb_request_data *request = t->RequestData();
	TRACE(("OHCIRootHub::ProcessTransfer(): request: %d\n", request->Request));

	uint32 status = B_TIMED_OUT;
	size_t actualLength = 0;
	switch (request->Request) {
		case USB_REQUEST_GET_STATUS: {
			if (request->Index == 0) {
				// get hub status
				actualLength = MIN(sizeof(usb_port_status),
					t->DataLength());
				// the hub reports whether the local power failed (bit 0)
				// and if there is a over-current condition (bit 1).
				// everything as 0 means all is ok.
				// TODO (?) actually check for the value
				memset(t->Data(), 0, actualLength);
				status = B_OK;
				break;
			}

			usb_port_status portStatus;
			if (ohci->GetPortStatus(request->Index, &portStatus) >= B_OK) {
				actualLength = MIN(sizeof(usb_port_status), t->DataLength());
				memcpy(t->Data(), (void *)&portStatus, actualLength);
				status = B_OK;
			}

			break;
		}

		case USB_REQUEST_SET_ADDRESS:
			if (request->Value >= 128) {
				status = B_TIMED_OUT;
				break;
			}

			TRACE(("OHCIRootHub::ProcessTransfer():  set address: %d\n", request->Value));
			status = B_OK;
			break;

		case USB_REQUEST_GET_DESCRIPTOR:
			TRACE(("OHCIRootHub::ProcessTransfer(): get descriptor: %d\n", request->Value >> 8));

			switch (request->Value >> 8) {
				case USB_DESCRIPTOR_DEVICE: {
					actualLength = MIN(sizeof(usb_device_descriptor),
						t->DataLength());
					memcpy(t->Data(), (void *)&sOHCIRootHubDevice,
						actualLength);
					status = B_OK;
					break;
				}

				case USB_DESCRIPTOR_CONFIGURATION: {
					//Make sure we have the correct number of ports
					sOHCIRootHubConfig.hub.num_ports = ohci->PortCount();
					
					actualLength = MIN(sizeof(sOHCIRootHubConfig),
						t->DataLength());
					memcpy(t->Data(), (void *)&(sOHCIRootHubConfig),
						actualLength);
					status = B_OK;
					break;
				}

				case USB_DESCRIPTOR_STRING: {
					uint8 index = request->Value & 0x00ff;
					if (index > 2)
						break;

					actualLength = MIN(sOHCIRootHubStrings[index].length,
						t->DataLength());
					memcpy(t->Data(), (void *)&sOHCIRootHubStrings[index],
						actualLength);
					status = B_OK;
					break;
				}

				case USB_DESCRIPTOR_HUB: {
					//Make sure we have the correct number of ports
					sOHCIRootHubConfig.hub.num_ports = ohci->PortCount();
					
					actualLength = MIN(sizeof(usb_hub_descriptor),
						t->DataLength());
					memcpy(t->Data(), (void *)&sOHCIRootHubConfig.hub,
						actualLength);
					status = B_OK;
					break;
				}
			}
			break;

		case USB_REQUEST_SET_CONFIGURATION:
			status = B_OK;
			break;

		case USB_REQUEST_CLEAR_FEATURE: {
			if (request->Index == 0) {
				// we don't support any hub changes
				TRACE_ERROR(("OHCIRootHub::ProcessTransfer(): clear feature: no hub changes\n"));
				break;
			}

			TRACE(("OHCIRootHub::ProcessTransfer(): clear feature: %d\n", request->Value));
			if (ohci->ClearPortFeature(request->Index, request->Value) >= B_OK)
				status = B_OK;
			break;
		}

		case USB_REQUEST_SET_FEATURE: {
			if (request->Index == 0) {
				// we don't support any hub changes
				TRACE_ERROR(("OHCIRootHub::ProcessTransfer(): set feature: no hub changes\n"));
				break;
			}

			TRACE(("OHCIRootHub::ProcessTransfer(): set feature: %d\n", request->Value));
			if (ohci->SetPortFeature(request->Index, request->Value) >= B_OK)
				status = B_OK;
			break;
		}
	}

	t->Finished(status, actualLength);
	delete t;
	return B_OK;
} 

