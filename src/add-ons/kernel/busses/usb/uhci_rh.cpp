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


#include "uhci.h"
#include <PCI.h>

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

//Implementation
UHCIRootHub::UHCIRootHub( UHCI *uhci , int8 devicenum )
		   : Hub( uhci , NULL , uhci_devd , devicenum , false )
{
	m_uhci = uhci;
}

status_t UHCIRootHub::SubmitTransfer( Transfer &t )
{
	status_t retval;
	usb_request_data *request = t.GetRequestData();
	uint16 port; //used in RH_CLEAR/SET_FEATURE
	
	TRACE( "USB UHCI: rh_submit_packet called. Request: %u\n" , t.GetRequestData()->Request );
	
	switch( request->Request )
	{
	case RH_GET_STATUS:
		if ( request->Index == 0 )
		{
			//Get the hub status -- everything as 0 means that it is all-rigth
			memset( t.GetBuffer() , NULL , sizeof(get_status_buffer) );
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
		UpdatePortStatus();
		memcpy( t.GetBuffer() , (void *)&(m_hw_port_status[request->Index - 1]) , t.GetBufferLength());
		*(t.GetActualLength()) = t.GetBufferLength();
		retval = B_OK;
		break;

	case RH_SET_ADDRESS:
		if ( request->Value >= 128 )
		{
			retval = EINVAL;
			break;
		}
		TRACE( "USB UHCI: rh_submit_packet RH_ADDRESS: %d\n" , request->Value );
		retval = B_OK;
		break;
	
	case RH_GET_DESCRIPTOR:
		{
			TRACE( "USB UHCI: rh_submit_packet GET_DESC: %d\n" , request->Value );
			switch ( request->Value )
			{
			case RH_DEVICE_DESCRIPTOR:
				memcpy( t.GetBuffer() , (void *)&uhci_devd , t.GetBufferLength());
				*(t.GetActualLength()) = t.GetBufferLength(); 
				retval = B_OK;
				break;
			case RH_CONFIG_DESCRIPTOR:
				memcpy( t.GetBuffer() , (void *)&uhci_confd , t.GetBufferLength());
				*(t.GetActualLength()) = t.GetBufferLength();
				retval =  B_OK;
				break;
			case RH_INTERFACE_DESCRIPTOR:
				memcpy( t.GetBuffer() , (void *)&uhci_intd , t.GetBufferLength());
				*(t.GetActualLength()) = t.GetBufferLength();
				retval = B_OK ;
				break;
			case RH_ENDPOINT_DESCRIPTOR:
				memcpy( t.GetBuffer() , (void *)&uhci_endd , t.GetBufferLength());
				*(t.GetActualLength()) = t.GetBufferLength();
				retval = B_OK ;
				break;
			case RH_HUB_DESCRIPTOR:
				memcpy( t.GetBuffer() , (void *)&uhci_hubd , t.GetBufferLength());
				*(t.GetActualLength()) = t.GetBufferLength();
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
			port = UHCI::pci_module->read_io_16( m_uhci->m_reg_base + UHCI_PORTSC1 + (request->Index - 1 ) * 2 );
			port &= ~UHCI_PORTSC_RESET;
			TRACE( "UHCI rh: port %x Clear RESET\n" , port );
			UHCI::pci_module->write_io_16( m_uhci->m_reg_base + UHCI_PORTSC1 + (request->Index - 1 ) * 2 , port );
		case C_PORT_CONNECTION:
			port = UHCI::pci_module->read_io_16( m_uhci->m_reg_base + UHCI_PORTSC1 + (request->Index - 1 ) * 2 );
			port = port & UHCI_PORTSC_DATAMASK;
			port |= UHCI_PORTSC_STATCHA;
			TRACE( "UHCI rh: port: %x\n" , port );
			UHCI::pci_module->write_io_16( m_uhci->m_reg_base + UHCI_PORTSC1 + (request->Index - 1 ) * 2 , port );
			retval = B_OK;
			break;
		default:
			retval = EINVAL;
			break;
		} //switch( t.value) 
		break;
		
	default: 
		retval = EINVAL;
		break;
	}
	
	return retval;
}

void UHCIRootHub::UpdatePortStatus(void)
{
	int i;
	for ( i = 0; i <= 1 ; i++ )
	{
		uint16 newstatus = 0;
		uint16 newchange = 0;
		
		uint16 portsc = UHCI::pci_module->read_io_16( m_uhci->m_reg_base + UHCI_PORTSC1 + i * 2 );
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
		m_hw_port_status[i].status = newstatus;
		m_hw_port_status[i].change = newchange;
	}
}
