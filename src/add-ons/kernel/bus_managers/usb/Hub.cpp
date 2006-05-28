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

#include "usb_p.h"

Hub::Hub(  BusManager *bus , Device *parent , usb_device_descriptor &desc , int8 devicenum , bool lowspeed )
   : Device ( bus , parent , desc , devicenum , lowspeed )
{
	dprintf( "USB Hub is being initialised\n" );
	size_t actual_length;

	if ( m_initok == false )
	{
		dprintf( "Hub::Hub() Device failed to initialize\n" );
		return;
	}
	
	//Set to false again for the hub init.
	m_initok = false; 
	
	if( m_device_descriptor.device_subclass != 0 || m_device_descriptor.device_protocol != 0 )
	{
		dprintf( "USB Hub: wrong class/subclass/protocol! Bailing out\n" );
		return;
	}
	
	if ( m_current_configuration->number_interfaces > 1 )
	{
		dprintf( "USB Hub: too much interfaces! Bailing out\n" );
		return;
	}
	
	dprintf( "USB Hub this: %p" , this );
	
	if ( GetDescriptor( USB_DESCRIPTOR_INTERFACE , 0 , 
	                    (void *)&m_interrupt_interface ,
	                    sizeof (usb_interface_descriptor) ) != sizeof( usb_interface_descriptor ) )
	{
		dprintf( "USB Hub: error getting the interrupt interface! Bailing out.\n" );
		return;
	}
	
	if ( m_interrupt_interface.num_endpoints > 1 )
	{
		dprintf( "USB Hub: too much endpoints! Bailing out.\n" );
		return;
	}
	
	if ( GetDescriptor( USB_DESCRIPTOR_ENDPOINT , 0 ,
	                    (void *)&m_interrupt_endpoint ,
	                    sizeof (usb_endpoint_descriptor) ) != sizeof (usb_endpoint_descriptor ) )
	{
		dprintf( "USB Hub: Error getting the endpoint. Bailing out\n" );
		return;
	}
	
	if ( m_interrupt_endpoint.attributes != 0x03 ) //interrupt transfer
	{
		dprintf( "USB Hub: Not an interrupt endpoint. Bailing out\n" );
		return;
	}
	
	dprintf( "USB Hub: Getting hub descriptor...\n" );
	if ( GetDescriptor( USB_DESCRIPTOR_HUB , 0 ,
						(void *)&m_hub_descriptor ,
						sizeof (usb_hub_descriptor) ) != sizeof (usb_hub_descriptor ) )
	{
		dprintf( "USB Hub: Error getting hub descriptor\n" );
		return;
	}
	
	// Enable port power on all ports
	for ( int i = 0 ; i < m_hub_descriptor.bNbrPorts ; i++ )
		m_defaultPipe->SendRequest( USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT ,
		                    USB_REQUEST_SET_FEATURE ,
		                    PORT_POWER , 
		                    i + 1 , //index
		                    0 ,
		                    NULL ,
		                    0 ,
		                    &actual_length );
	//Wait for power to stabilize
	snooze( m_hub_descriptor.bPwrOn2PwrGood * 2 );
	
	//We're basically done now
	m_initok = true;
	dprintf( "USB Hub: initialised ok\n" );
}

void Hub::Explore()
{
	size_t actual_length;  
	
	for ( int i = 0 ; i < m_hub_descriptor.bNbrPorts ; i++ )
	{
		// Get the current port status
		m_defaultPipe->SendRequest( USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_IN ,
		                    USB_REQUEST_GET_STATUS ,
		                    0 , //Value
		                    i + 1 , //Index
		                    4 ,  //length
		                    (void *)&m_port_status[i],
		                    4 ,
		                    &actual_length );
		if ( actual_length < 4 )
		{
			dprintf( "USB Hub: ERROR getting port status\n" );
			return;
		}
		dprintf("status: 0x%04x; change: 0x%04x\n", m_port_status[i].status, m_port_status[i].change);
	
		//We need to test the port change against a number of things
		
		if ( m_port_status[i].status & PORT_RESET )
		{
			//We didn't do this!
			m_defaultPipe->SendRequest( USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT ,
		                    USB_REQUEST_CLEAR_FEATURE ,
		                    PORT_RESET , 
		                    i + 1 , //index
		                    0 ,
		                    NULL ,
		                    0 ,
		                    &actual_length );
		}
		
					
		if ( m_port_status[i].change & PORT_STATUS_CONNECTION )
		{
			if ( m_port_status[i].status & PORT_STATUS_CONNECTION )
			{
				//New device attached!

				if ((m_port_status[i].status & PORT_STATUS_ENABLE) == 0) {
					// enable the port if it isn't
					m_defaultPipe->SendRequest( USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT ,
		                    USB_REQUEST_SET_FEATURE ,
		                    PORT_ENABLE , 
		                    i + 1 , //index
		                    0 ,
		                    NULL ,
		                    0 ,
		                    &actual_length );
		        }

				dprintf( "USB Hub Explore(): New device connected\n" );
				Device *newdev;
				if ( m_port_status[i].status & PORT_STATUS_LOW_SPEED )
					newdev = m_bus->AllocateNewDevice( this , true );
				else
					newdev = m_bus->AllocateNewDevice( this , false );
				if ( newdev != 0 )
					m_children[i] = newdev;
			}
			else
			{
				//Device removed...
				//DO something
				dprintf( "USB Hub Explore(): Device removed\n" );
			
			}
		
			//Clear status
			m_defaultPipe->SendRequest( USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_OUT ,
		                    USB_REQUEST_CLEAR_FEATURE ,
		                    C_PORT_CONNECTION , 
		                    i + 1 , //index
		                    0 ,
		                    NULL ,
		                    0 ,
		                    &actual_length );
		} // PORT_STATUS_CONNECTION
	}//for (...)
}
