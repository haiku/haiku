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


/* +++++++++
This is the 'main' function of the explore thread, which keeps track of
the hub statuses.
+++++++++ */
int32 usb_explore_thread( void *data )
{
	Hub *roothub = (Hub *)data;
	
	while (true )
	{
		//Go to the hubs
		roothub->Explore();
		snooze(1000000); //Wait one second before continueing
	}
	return B_OK;
}


BusManager::BusManager( host_controller_info *info )
{
	hcpointer = info;
	m_initok = false; 
	m_roothub = 0;
	
	// Set up the semaphore
	m_lock = create_sem( 1 , "buslock" );
	if( m_lock < B_OK )
		return;
	set_sem_owner( B_SYSTEM_TEAM , m_lock );
	
	// Clear the device map
	memset( &m_devicemap , false , 128 );
	
	// Set up the default pipes
	m_defaultPipe = new ControlPipe( this , 0 , Default , NormalSpeed , 0 );
	m_defaultLowSpeedPipe = new ControlPipe( this , 0 , Default , LowSpeed , 0 );
	
	// Set up the new root hub
	AllocateNewDevice( 0 , false );
	
	if( m_roothub == 0 )
		return;
		
	// Start the 'explore thread'
	m_explore_thread = spawn_kernel_thread( usb_explore_thread , "usb_busmanager_explore" ,
	                     B_LOW_PRIORITY , (void *)m_roothub );
	resume_thread( m_explore_thread );
	
	m_initok = true;
}

BusManager::~BusManager()
{
	put_module( hcpointer->info.name );
}

status_t BusManager::InitCheck()
{
	if ( m_initok == true )
		return B_OK;
	else
		return B_ERROR;
}

Device * BusManager::AllocateNewDevice( Device *parent , bool lowspeed )
{
	int8 devicenum;
	status_t retval;
	usb_device_descriptor device_descriptor;
	
	//1. Check if there is a free entry in the device map (for the device number)
	devicenum = AllocateAddress();
	if ( devicenum < 0 )
	{
		dprintf( "usb Busmanager [new device]: could not get a new address\n" );
		return 0;
	}
	
	
	//2. Set the address of the device USB 1.1 spec p202 (bepdf)
	retval = m_defaultPipe->SendRequest( USB_REQTYPE_DEVICE_OUT | USB_REQTYPE_STANDARD , //host->device
	             USB_REQUEST_SET_ADDRESS , 						//request
	             devicenum ,									//value
	             0 ,											//index
	             0 ,											//length
	             NULL ,											//data
	             0 ,											//data_len
	             0 );											//actual len
	
	if ( retval < B_OK )
	{
		dprintf( "usb busmanager [new device]: error with commmunicating the right address\n" );
		return 0; 
	}
	
	//Create a temporary pipe
	ControlPipe pipe( this , devicenum , Default , LowSpeed , 0 );
	
	//3. Get the device descriptor
	// Just retrieve the first 8 bytes of the descriptor -> minimum supported
	// size of any device, plus it is enough, because the device type is in
	// there too 
	
	size_t actual_length;
	
	pipe.SendRequest( USB_REQTYPE_DEVICE_IN | USB_REQTYPE_STANDARD , //Type
	             USB_REQUEST_GET_DESCRIPTOR ,					//Request
	             ( USB_DESCRIPTOR_DEVICE << 8 ),				//Value
	             0 ,											//Index
	             8 ,											//Length										
	             (void *)&device_descriptor ,					//Buffer
	             8 ,											//Bufferlength
	             &actual_length );								//length
	
	if ( actual_length != 8 )
	{
		dprintf( "usb busmanager [new device]: error while getting the device descriptor\n" );
		return 0;
	}

	//4. Create a new instant based on the type (Hub or Device);
	if ( device_descriptor.device_class == 0x09 )
	{
		dprintf( "usb Busmanager [new device]: New hub\n" );
		Device *ret = new Hub( this , parent , device_descriptor , devicenum , lowspeed );
		if ( parent == 0 ) //root hub!!
			m_roothub = ret;
		return ret;
	}
	
	dprintf( "usb Busmanager [new device]: New normal device\n" );
	return new Device( this , parent , device_descriptor , devicenum , lowspeed);
}

int8 BusManager::AllocateAddress()
{
	acquire_sem_etc( m_lock , 1 , B_CAN_INTERRUPT , 0 );
	int8 devicenum = -1;
	for( int i = 1 ; i < 128 ; i++ )
	{
		if ( m_devicemap[i] == false )
		{
			devicenum = i;
			m_devicemap[i] = true;
			break;
		}
	}
	release_sem( m_lock );
	return devicenum;
}

