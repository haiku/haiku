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

#ifndef USB_P
#define USB_P

#include <KernelExport.h>
#include <util/Vector.h>
#include <USB.h>
#include <util/kernel_cpp.h>


/* ++++++++++
Forward declarations
++++++++++ */
class Stack;
class Device;
class Transfer;
class BusManager;
class ControlPipe;


#define USB_MAX_AREAS 8
struct memory_chunk
{
	void *next_item;
	void *physical;
};

/* ++++++++++
Important data from the USB spec (not interesting for drivers)
++++++++++ */

#define POWER_DELAY 

/* ++++++++++
The host_controller_info
++++++++++ */
struct host_controller_info 
{
	module_info info;
	status_t (*control)(uint32 op, void *data, size_t length);
	bool (*add_to)(Stack &stack);
};

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


/* ++++++++++
Internal classes
++++++++++ */

class Stack
{
public:
	Stack();
	~Stack();
	status_t InitCheck();
	
	void Lock();
	void Unlock();
	
	void AddBusManager( BusManager *bus );
	
	status_t AllocateChunk( void **log , void **phy , uint8 size );
	status_t FreeChunk( void *log , void *phy , uint8 size );
	area_id AllocateArea( void **log , void **phy , size_t size , const char *name ); 

private:
	Vector<BusManager *>	m_busmodules;//Stores all the bus modules
	sem_id 					m_master;			//The master lock
	sem_id					m_datalock;			//Lock that controls data transfers
	area_id					m_areas[USB_MAX_AREAS]; //Stores the area_id for the memory pool
	void 				   *m_logical[USB_MAX_AREAS];//Stores the logical base addresses
	void				   *m_physical[USB_MAX_AREAS];//Stores the physical base addresses
	uint16					m_areafreecount[USB_MAX_AREAS];	//Keeps track of how many free chunks there are
	void				   *m_8_listhead;
	void				   *m_16_listhead;
	void                   *m_32_listhead;
};

extern "C" Stack *data;

class Device
{
	friend class Pipe;
public:
	Device( BusManager *bus , Device *parent , usb_device_descriptor &desc , int8 devicenum , bool speed);
	virtual bool IsHub()	{ return false; };
	status_t InitCheck();
	
	uint8 GetAddress() { return m_devicenum; };
	int16 GetDescriptor( uint8 descriptor_type , uint16 index , void *buffer ,
	                        size_t size );
	BusManager *GetBusManager() { return m_bus; };
	status_t SetConfiguration( uint8 value );
protected:
	usb_device_descriptor 	  m_device_descriptor;
	usb_configuration_descriptor  * m_configurations;
	usb_configuration_descriptor  * m_current_configuration;
	ControlPipe            *  m_defaultPipe;
	bool					  m_initok;
	bool					  m_lowspeed;
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
	Hub( BusManager *bus , Device *parent , usb_device_descriptor &desc , int8 devicenum , bool lowspeed );
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
	friend class ControlPipe;
public:
	BusManager();
	virtual ~BusManager();
	
	virtual status_t InitCheck();
	
	Device *AllocateNewDevice( Device *parent , bool speed );
	int8 AllocateAddress();
	
	virtual status_t Start();
//	virtual status_t Stop();
	virtual status_t SubmitTransfer( Transfer &t );
protected:
	void SetRootHub( Hub *hub ) { m_roothub = hub; };
	Device* GetRootHub() { return m_roothub; };
	bool m_initok;
private:
	bool m_devicemap[128];
	ControlPipe *m_defaultLowSpeedPipe;
	ControlPipe *m_defaultPipe;
	sem_id m_lock;
	Device *m_roothub;
	thread_id m_explore_thread;
};

// Located in the BusManager.cpp
int32 usb_explore_thread( void *data );

/*
 * The Pipe class is the communication management between the hardware and\
 * the stack. It creates packets, manages these and performs callbacks.
 */
 
class Pipe
{
	friend class Transfer;
public:
	enum Direction 	{ In , Out , Default };
	enum Speed		{ LowSpeed , NormalSpeed };

	Pipe( Device *dev , Direction &dir , uint8 &endpointaddress );
	virtual ~Pipe();
	
	virtual int8 GetDeviceAddress();
protected:
	Device   *m_device;
	BusManager *m_bus;
	Direction	m_direction;
	uint8		m_endpoint;
};

class ControlPipe : public Pipe 
{
public:
	ControlPipe( Device *dev , Direction dir , uint8 endpointaddress ); 
	//Special constructor for default control pipe
	ControlPipe( BusManager *bus , int8 dev_address , Direction dir , Speed speed , uint8 endpointaddress ); 
	virtual int8 GetDeviceAddress();
	status_t SendRequest( uint8 request_type , uint8 request , 
	                  uint16 value , uint16 index , uint16 length , void *data ,
	                  size_t data_len , size_t *actual_len );
	status_t SendControlMessage( usb_request_data *command , void *data ,
	           size_t data_length , size_t *actual_length , bigtime_t timeout );
private:
	int8		m_deviceaddress;
	bool		m_lowspeed;
};

/*
 * This class is more like an utility class that performs all functions on
 * packets. The class is only used in the bus_manager: the host controllers
 * receive the internal data structures.
 */

class Transfer
{
public:
	Transfer( Pipe *pipe );
	~Transfer();
	void SetRequestData( usb_request_data *data );
	void SetBuffer( uint8 *buffer );
	void SetBufferLength( size_t length );
	void SetActualLength( size_t * );
	
	Pipe *GetPipe() { return m_pipe; };
	uint8 *GetBuffer() { return m_buffer; };
	size_t GetBufferLength() { return m_bufferlength; };
	size_t *GetActualLength() { return m_actual_length; };
	usb_request_data *GetRequestData() {return m_request;};
private:
	//Data that is related to the transfer
	Pipe		*m_pipe;
	uint8 		*m_buffer;
	size_t 		m_bufferlength;
	size_t 		*m_actual_length;
	bigtime_t 	m_timeout;
	status_t	m_status;

	//For control transfers
	usb_request_data	*m_request;
};

#endif

