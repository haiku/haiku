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

#ifndef USB_P
#define USB_P

#include <KernelExport.h>
#include <util/Vector.h>
#include <USB.h>
#include "host_controller.h"
#include <util/kernel_cpp.h>

/* ++++++++++
Forward declarations
++++++++++ */
class Device;
class BusManager;

/* ++++++++++
Important data from the USB spec (not interesting for drivers)
++++++++++ */

#define POWER_DELAY 

/* ++++++++++
Internal classes
++++++++++ */

class Stack
{
public:
	Stack();
	~Stack();
	status_t InitCheck();
private:
	Vector<BusManager *>	m_busmodules;//Stores all the bus modules
	sem_id 					m_master;			//The master lock
	sem_id					m_datalock;			//Lock that controls data transfers
};

extern "C" Stack *data;

class Device
{
public:
	Device( BusManager *bus , Device *parent , usb_device_descriptor &desc , int8 devicenum );
	virtual bool IsHub()	{ return false; };
	status_t InitCheck();
	
	uint8 GetAddress() { return m_devicenum; };
	int16 GetDescriptor( uint8 descriptor_type , uint16 index , void *buffer ,
	                        size_t size );
	status_t SetConfiguration( uint8 value );
protected:
	usb_device_descriptor 	  m_device_descriptor;
	usb_configuration_descriptor  * m_configurations;
	usb_configuration_descriptor  * m_current_configuration;
	bool					  m_initok;
	BusManager 				* m_bus;
	Device					* m_parent;
	int8					  m_devicenum;
	uint8                     m_maxpacketin[16];		//Max. size of input
	uint8                     m_maxpacketout[16];		//Max size of output
	sem_id					  m_lock;
};

class Hub : public Device
{
public:
	Hub( BusManager *bus , Device *parent , usb_device_descriptor &desc , int8 devicenum);
	virtual bool IsHub()	{ return true; };

	void Explore();
private: 
	usb_interface_descriptor m_interrupt_interface;
	usb_endpoint_descriptor m_interrupt_endpoint;
	usb_hub_descriptor m_hub_descriptor;
	
	usb_port_status     m_port_status[8]; //Max of 8 ports, for the moment
};

/*
 * This class manages a bus. It is created by the Stack object
 * after a host controller gives positive feedback on whether the hardware
 * is found. 
 */
class BusManager
{
	friend class Device;
public:
	BusManager( host_controller_info *info );
	~BusManager();
	
	status_t InitCheck();
	
	Device *AllocateNewDevice( Device *parent );
	int8 AllocateAddress();
	status_t SendRequest( Device *dev, uint8 request_type , uint8 request , 
	                  uint16 value , uint16 index , uint16 length , void *data ,
	                  size_t data_len , size_t *actual_len );
private:
	status_t SendControlMessage( Device *dev , uint16 pipe , 
	           usb_request_data *command , void *data ,
	           size_t data_length , size_t *actual_length , 
	           bigtime_t timeout );

	host_controller_info *hcpointer;
	bool m_initok;
	bool m_devicemap[128];
	sem_id m_lock;
	Device *m_roothub;
	thread_id m_explore_thread;
};

// Located in the BusManager.cpp
int32 usb_explore_thread( void *data );

/*
 * This class is more like an utility class that performs all functions on
 * packets. The class is only used in the bus_manager: the host controllers
 * receive the internal data structures.
 */

class Packet
{
public:
	Packet();
	~Packet();
	void SetPipe( uint16 pipe );
	void SetRequestData( uint8 *data );
	void SetBuffer( uint8 *buffer );
	void SetBufferLength( int16 length );
	void SetAddress( uint8 address );
	void SetActualLength( size_t * );
	
	usb_packet_t *GetData();
private:
	usb_packet_t *d;
};

#endif
