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

#include "usb_p.h"

Device::Device( BusManager *bus , Device *parent , usb_device_descriptor &desc , int8 devicenum )
{
	status_t retval;
	
	m_bus = bus;
	m_parent = parent;
	m_initok = false;
	m_configurations = 0;
	m_current_configuration = 0;
	
	dprintf( "USB Device: new device\n" );
	
	//1. Create the semaphore
	m_lock = create_sem( 1 , "device_sem" );
	if ( m_lock < B_OK )
	{
		dprintf( "usb Device: could not create semaphore\n" );
		return;
	}
	set_sem_owner( m_lock , B_SYSTEM_TEAM );
	
	//2. Set the free entry free entry in the device map (for the device number)
	m_devicenum = devicenum;
	
	//3. Get the device descriptor
	// We already have a part of it, but we want it all
	m_device_descriptor = desc;
	
	//Set the maximum transfer numbers for the incoming and the outgoing packets on endpoint 0
	m_maxpacketin[0] = m_maxpacketout[0] = m_device_descriptor.max_packet_size_0; 
	
	retval = GetDescriptor( USB_DESCRIPTOR_DEVICE , 0 ,
	                        (void *)&m_device_descriptor , sizeof(m_device_descriptor ) );
	
	if ( retval != sizeof( m_device_descriptor ) )
	{
		dprintf( "usb Device: error while getting the device descriptor\n" );
		return;
	}
	
	dprintf( "usb Device %d: Vendor id: %d , Product id: %d\n" , devicenum ,
	         m_device_descriptor.vendor_id , m_device_descriptor.product_id );
		
	
	// 4. Get the configurations
	m_configurations = (usb_configuration_descriptor *)malloc( m_device_descriptor.num_configurations * sizeof (usb_configuration_descriptor) );
	if ( m_configurations == 0 )
	{
		dprintf( "usb Device: out of memory during config creations!\n" );
		return;
	}
	
	for ( int i = 0 ; i < m_device_descriptor.num_configurations ; i++ )
	{
		if ( GetDescriptor( USB_DESCRIPTOR_CONFIGURATION , i , 
		     (void *)( m_configurations + i ) , sizeof (usb_configuration_descriptor ) ) != sizeof( usb_configuration_descriptor ) )
		{
			dprintf( "usb Device %d: error fetching configuration %d ...\n" , m_devicenum , i );
			return;
		}
	}
	
	// 5. Set the default configuration
	dprintf( "usb Device %d: setting configuration %d\n" , m_devicenum , 0 );
	SetConfiguration( 0 );
	
	//6. TODO: Find drivers for the device
}

//Returns the length that was copied (index gives the number of the config)
int16 Device::GetDescriptor( uint8 descriptor_type , uint16 index , 
                                void *buffer , size_t size )
{
	size_t actual_length;
	m_bus->SendRequest( this ,
	             USB_REQTYPE_DEVICE_IN | USB_REQTYPE_STANDARD , //Type
	             USB_REQUEST_GET_DESCRIPTOR ,					//Request
	             ( descriptor_type << 8 ) | index ,				//Value
	             0 ,											//Index
	             size ,											//Length										
	             buffer ,										//Buffer
	             size ,											//Bufferlength
	             &actual_length );								//length
	return actual_length;
}			
	             
status_t Device::SetConfiguration( uint8 value )
{
	if ( value >= m_device_descriptor.num_configurations )
		return EINVAL;
	
	m_bus->SendRequest( this ,
	             USB_REQTYPE_DEVICE_OUT | USB_REQTYPE_STANDARD , //Type
	             USB_REQUEST_SET_CONFIGURATION ,				//Request
	             value ,										//Value
	             0 ,											//Index
	             0 ,											//Length										
	             NULL ,											//Buffer
	             0 ,											//Bufferlength
	             0 );											//length
}
