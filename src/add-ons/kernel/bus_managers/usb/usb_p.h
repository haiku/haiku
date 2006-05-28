/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#ifndef _USB_P_
#define _USB_P_

#include "usbspec_p.h"


class Stack;
class Device;
class Transfer;
class BusManager;
class ControlPipe;


struct host_controller_info {
	module_info		info;
	status_t		(*control)(uint32 op, void *data, size_t length);
	bool			(*add_to)(Stack &stack);
};


class Stack {
public:
										Stack();
										~Stack();

		status_t						InitCheck();

		void							Lock();
		void							Unlock();

		void							AddBusManager(BusManager *bus);

		status_t						AllocateChunk(void **logicalAddress,
											void **physicalAddress, uint8 size);
		status_t						FreeChunk(void *logicalAddress,
											void *physicalAddress, uint8 size);

		area_id							AllocateArea(void **logicalAddress,
											void **physicalAddress,
											size_t size, const char *name);

private:
		Vector<BusManager *>			fBusManagers;

		sem_id							fMasterLock;
		sem_id							fDataLock;

		area_id							fAreas[USB_MAX_AREAS];
		void							*fLogical[USB_MAX_AREAS];
		void							*fPhysical[USB_MAX_AREAS];
		uint16							fAreaFreeCount[USB_MAX_AREAS];

		addr_t							fListhead8;
		addr_t							fListhead16;
		addr_t							fListhead32;
};

extern "C" Stack *usb_stack;

class Device {
public:
										Device(BusManager *bus, Device *parent,
											usb_device_descriptor &desc,
											int8 deviceAddress, bool lowSpeed);

		status_t						InitCheck();

virtual	bool							IsHub()	{ return false; };
		int8							Address() { return fDeviceAddress; };
		BusManager						*Manager() { return fBus; };

		size_t							GetDescriptor(uint8 descriptorType,
											uint16 index, void *buffer,
											size_t bufferSize);

		status_t						SetConfiguration(uint8 value);

protected:
friend	class Pipe;

		usb_device_descriptor			fDeviceDescriptor;
		usb_configuration_descriptor	*fConfigurations;
		usb_configuration_descriptor	*fCurrentConfiguration;
		ControlPipe						*fDefaultPipe;
		bool							fInitOK;
		bool							fLowSpeed;
		BusManager						*fBus;
		Device							*fParent;
		int8							fDeviceAddress;
		uint8							fMaxPacketIn[16];
		uint8							fMaxPacketOut[16];
		sem_id							fLock;
};


class Hub : public Device {
public:
										Hub(BusManager *bus, Device *parent,
											usb_device_descriptor &desc,
											int8 deviceAddress, bool lowSpeed);

virtual	bool							IsHub()	{ return true; };

		void							Explore();

private:
		usb_interface_descriptor		fInterruptInterface;
		usb_endpoint_descriptor			fInterruptEndpoint;
		usb_hub_descriptor				fHubDescriptor;

		usb_port_status					fPortStatus[8];
		Device							*fChildren[8];
};


/*
 * This class manages a bus. It is created by the Stack object
 * after a host controller gives positive feedback on whether the hardware
 * is found. 
 */
class BusManager {
public:
										BusManager();
virtual									~BusManager();

virtual	status_t						InitCheck();

		Device							*AllocateNewDevice(Device *parent,
											bool lowSpeed);

		int8							AllocateAddress();

virtual	status_t						Start();
virtual	status_t						Stop();

virtual	status_t						SubmitTransfer(Transfer *transfer,
											bigtime_t timeout = 0);

protected:
		void							SetRootHub(Hub *hub) { fRootHub = hub; };
		Device							*GetRootHub() { return fRootHub; };

		bool							fInitOK;

private:
friend	class Device;
friend	class ControlPipe;

static	int32							ExploreThread(void *data);

		sem_id 							fLock;
		bool							fDeviceMap[128];
		ControlPipe						*fDefaultPipe;
		ControlPipe						*fDefaultPipeLowSpeed;
		Device							*fRootHub;
		thread_id						fExploreThread;
};


/*
 * The Pipe class is the communication management between the hardware and\
 * the stack. It creates packets, manages these and performs callbacks.
 */
class Pipe {
public:
enum	pipeDirection 	{ In, Out, Default };
enum	pipeSpeed		{ LowSpeed, NormalSpeed };
enum	pipeType		{ Control, Bulk, Isochronous, Interrupt, Invalid };

										Pipe(Device *device,
											pipeDirection &direction,
											uint8 &endpointAddress);
virtual									~Pipe();

virtual	int8							DeviceAddress();
virtual	pipeType						Type() { return Invalid; };
virtual	pipeSpeed						Speed() { return LowSpeed; };
											//Provide a default: should never be called
virtual	int8							EndpointAddress() { return fEndpoint; };

protected:
friend	class Transfer;

		Device							*fDevice;
		BusManager						*fBus;
		pipeDirection					fDirection;
		uint8							fEndpoint;
};


class ControlPipe : public Pipe {
public:
									ControlPipe(Device *device,
										pipeDirection direction,
										pipeSpeed speed,
										uint8 endpointAddress);

									// Constructor for default control pipe
									ControlPipe(BusManager *bus,
										int8 deviceAddress,
										pipeDirection direction,
										pipeSpeed speed,
										uint8 endpointAddress);

virtual	int8						DeviceAddress();
virtual	pipeType					Type() { return Control; };
virtual	pipeSpeed					Speed() { return fSpeed; };

		status_t					SendRequest(uint8 requestType,
										uint8 request, uint16 value,
										uint16 index, uint16 length,
										void *data, size_t dataLength,
										size_t *actualLength);

		status_t					SendControlMessage(usb_request_data *command,
										void *data, size_t dataLength,
										size_t *actualLength,
										bigtime_t timeout);

private:
		int8						fDeviceAddress;
		pipeSpeed					fSpeed;
};


/*
 * This is a forward definition of a struct that is defined in the individual
 * host controller modules
 */
struct hostcontroller_priv;


/*
 * This class is more like an utility class that performs all functions on
 * packets. The class is only used in the bus_manager: the host controllers
 * receive the internal data structures.
 */
class Transfer {
public:
									Transfer(Pipe *pipe, bool synchronous);
									~Transfer();

		Pipe						*TransferPipe() { return fPipe; };

		void						SetRequestData(usb_request_data *data);
		usb_request_data			*RequestData() { return fRequest; };

		void						SetBuffer(uint8 *buffer);
		uint8						*Buffer() { return fBuffer; };

		void						SetBufferLength(size_t length);
		size_t						BufferLength() { return fBufferLength; };

		void						SetActualLength(size_t *actualLength);
		size_t						*ActualLength() { return fActualLength; };

		void						SetHostPrivate(hostcontroller_priv *priv);
		hostcontroller_priv			*HostPrivate() { return fHostPrivate; };

		void						SetCallbackFunction(usb_callback_func callback);
	
		void						WaitForFinish();
		void						TransferDone();
		void						Finish();

private:
		// Data that is related to the transfer
		Pipe						*fPipe;
		uint8						*fBuffer;
		size_t						fBufferLength;
		size_t						*fActualLength;
		bigtime_t					fTimeout;
		status_t					fStatus;

		usb_callback_func			fCallback;
		sem_id						fSem;
		hostcontroller_priv			*fHostPrivate;

		// For control transfers
		usb_request_data			*fRequest;
};

#endif
