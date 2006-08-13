/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#ifndef _USB_P_
#define _USB_P_

#include <lock.h>
#include "usbspec_p.h"


class Hub;
class Device;
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


struct usb_driver_info {
	const char						*driver_name;
	usb_support_descriptor			*support_descriptors;
	uint32							support_descriptor_count;
	const char						*republish_driver_name;
	usb_notify_hooks				notify_hooks;
	usb_driver_info					*link;
};


class Stack {
public:
										Stack();
										~Stack();

		status_t						InitCheck();

		bool							Lock();
		void							Unlock();

		void							AddBusManager(BusManager *bus);

		status_t						AllocateChunk(void **logicalAddress,
											void **physicalAddress, uint8 size);
		status_t						FreeChunk(void *logicalAddress,
											void *physicalAddress, uint8 size);

		area_id							AllocateArea(void **logicalAddress,
											void **physicalAddress,
											size_t size, const char *name);

		void							NotifyDeviceChange(Device *device,
											bool added);

		// USB API
		status_t						RegisterDriver(const char *driverName,
											const usb_support_descriptor *descriptors,
											size_t descriptorCount,
											const char *republishDriverName);

		status_t						InstallNotify(const char *driverName,
											const usb_notify_hooks *hooks);
		status_t						UninstallNotify(const char *driverName);

private:
		Vector<BusManager *>			fBusManagers;

		benaphore						fLock;
		area_id							fAreas[USB_MAX_AREAS];
		void							*fLogical[USB_MAX_AREAS];
		void							*fPhysical[USB_MAX_AREAS];
		uint16							fAreaFreeCount[USB_MAX_AREAS];

		addr_t							fListhead8;
		addr_t							fListhead16;
		addr_t							fListhead32;
		addr_t							fListhead64;

		usb_driver_info					*fDriverList;
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

virtual	status_t						SubmitTransfer(Transfer *transfer);
virtual	status_t						SubmitRequest(Transfer *transfer);

		Stack							*GetStack() { return fStack; };
		void							SetStack(Stack *stack) { fStack = stack; };

		Hub								*GetRootHub() { return fRootHub; };
		void							SetRootHub(Hub *hub) { fRootHub = hub; };

protected:
		bool							fInitOK;

private:
static	int32							ExploreThread(void *data);

		sem_id 							fLock;
		bool							fDeviceMap[128];
		ControlPipe						*fDefaultPipe;
		ControlPipe						*fDefaultPipeLowSpeed;
		Hub								*fRootHub;
		Stack							*fStack;
		thread_id						fExploreThread;
};


/*
 * The Pipe class is the communication management between the hardware and
 * the stack. It creates packets, manages these and performs callbacks.
 */
class Pipe {
public:
enum	pipeDirection 	{ In, Out, Default };
enum	pipeSpeed		{ LowSpeed, NormalSpeed };
enum	pipeType		{ Control, Bulk, Isochronous, Interrupt, Invalid };

										Pipe(Device *device,
											pipeDirection direction,
											pipeSpeed speed,
											uint8 endpointAddress,
											uint32 maxPacketSize);
virtual									~Pipe();

virtual	int8							DeviceAddress();
virtual	pipeType						Type() { return Invalid; };
virtual	pipeSpeed						Speed() { return fSpeed; };
virtual	pipeDirection					Direction() { return fDirection; };
virtual	int8							EndpointAddress() { return fEndpoint; };
virtual	uint32							MaxPacketSize() { return fMaxPacketSize; };

virtual	bool							DataToggle() { return fDataToggle; };
virtual	void							SetDataToggle(bool toggle) { fDataToggle = toggle; };

		status_t						SubmitTransfer(Transfer *transfer);
		status_t						CancelQueuedTransfers();

protected:
		Device							*fDevice;
		BusManager						*fBus;
		pipeDirection					fDirection;
		pipeSpeed						fSpeed;
		uint8							fEndpoint;
		uint32							fMaxPacketSize;
		bool							fDataToggle;
};


class InterruptPipe : public Pipe {
public:
										InterruptPipe(Device *device,
											pipeDirection direction,
											pipeSpeed speed,
											uint8 endpointAddress,
											uint32 maxPacketSize);

virtual	pipeType						Type() { return Interrupt; };

		status_t						QueueInterrupt(void *data,
											size_t dataLength,
											usb_callback_func callback,
											void *callbackCookie);
};


class BulkPipe : public Pipe {
public:
										BulkPipe(Device *device,
											pipeDirection direction,
											pipeSpeed speed,
											uint8 endpointAddress,
											uint32 maxPacketSize);

virtual	pipeType						Type() { return Bulk; };

		status_t						QueueBulk(void *data,
											size_t dataLength,
											usb_callback_func callback,
											void *callbackCookie);
};


class IsochronousPipe : public Pipe {
public:
										IsochronousPipe(Device *device,
											pipeDirection direction,
											pipeSpeed speed,
											uint8 endpointAddress,
											uint32 maxPacketSize);

virtual	pipeType						Type() { return Isochronous; };

		status_t						QueueIsochronous(void *data,
											size_t dataLength,
											rlea *rleArray,
											uint16 bufferDurationMS,
											usb_callback_func callback,
											void *callbackCookie);
};


class ControlPipe : public Pipe {
public:
										ControlPipe(Device *device,
											pipeSpeed speed,
											uint8 endpointAddress,
											uint32 maxPacketSize);

										// Constructor for default control pipe
										ControlPipe(BusManager *bus,
											int8 deviceAddress,
											pipeSpeed speed,
											uint8 endpointAddress,
											uint32 maxPacketSize);

virtual	int8							DeviceAddress();
virtual	pipeType						Type() { return Control; };

										// The data toggle is not relevant
										// for control transfers, as they are
										// always enclosed by a setup and
										// status packet. The toggle always
										// starts at 1.
virtual	bool							DataToggle() { return true; };
virtual	void							SetDataToggle(bool toggle) {};

		status_t						SendRequest(uint8 requestType,
											uint8 request, uint16 value,
											uint16 index, uint16 length,
											void *data, size_t dataLength,
											size_t *actualLength);

		status_t						QueueRequest(uint8 requestType,
											uint8 request, uint16 value,
											uint16 index, uint16 length,
											void *data, size_t dataLength,
											usb_callback_func callback,
											void *callbackCookie);

		// Convenience functions for standard requests
virtual	status_t						SetFeature(uint16 selector);
virtual	status_t						ClearFeature(uint16 selector);
virtual	status_t						GetStatus(uint16 *status);

private:
		int8							fDeviceAddress;
};


class Interface : public ControlPipe {
public:
										Interface(Device *device);

		// Convenience functions for standard requests
virtual	status_t						SetFeature(uint16 selector);
virtual	status_t						ClearFeature(uint16 selector);
virtual	status_t						GetStatus(uint16 *status);

private:
		Device							*fDevice;
};


class Device : public ControlPipe {
public:
										Device(BusManager *bus, Device *parent,
											usb_device_descriptor &desc,
											int8 deviceAddress, bool lowSpeed);

		status_t						InitCheck();

virtual	bool							IsHub()	{ return false; };
		int8							Address() { return fDeviceAddress; };
		BusManager						*Manager() { return fBus; };
		bool							LowSpeed() { return fLowSpeed; };

virtual	status_t						GetDescriptor(uint8 descriptorType,
											uint8 index, uint16 languageID,
											void *data, size_t dataLength,
											size_t *actualLength);

		const usb_device_descriptor		*DeviceDescriptor() const;

		const usb_configuration_info	*Configuration() const;
		const usb_configuration_info	*ConfigurationAt(uint8 index) const;
		status_t						SetConfiguration(const usb_configuration_info *configuration);
		status_t						SetConfigurationAt(uint8 index);

virtual	void							ReportDevice(
											usb_support_descriptor *supportDescriptors,
											uint32 supportDescriptorCount,
											const usb_notify_hooks *hooks,
											bool added);

		// Convenience functions for standard requests
virtual	status_t						SetFeature(uint16 selector);
virtual	status_t						ClearFeature(uint16 selector);
virtual	status_t						GetStatus(uint16 *status);

protected:
		usb_device_descriptor			fDeviceDescriptor;
		usb_configuration_info			*fConfigurations;
		usb_configuration_info			*fCurrentConfiguration;
		bool							fInitOK;
		bool							fLowSpeed;
		BusManager						*fBus;
		Device							*fParent;
		int8							fDeviceAddress;
		uint32							fMaxPacketIn[16];
		uint32							fMaxPacketOut[16];
		sem_id							fLock;
		void							*fNotifyCookie;
};


class Hub : public Device {
public:
										Hub(BusManager *bus, Device *parent,
											usb_device_descriptor &desc,
											int8 deviceAddress, bool lowSpeed);

virtual	bool							IsHub()	{ return true; };

virtual	status_t						GetDescriptor(uint8 descriptorType,
											uint8 index, uint16 languageID,
											void *data, size_t dataLength,
											size_t *actualLength);

		status_t						UpdatePortStatus(uint8 index);
		status_t						ResetPort(uint8 index);
		void							Explore();
static	void							InterruptCallback(void *cookie,
											uint32 status, void *data,
											uint32 actualLength);

virtual	void							ReportDevice(
											usb_support_descriptor *supportDescriptors,
											uint32 supportDescriptorCount,
											const usb_notify_hooks *hooks,
											bool added);

private:
		InterruptPipe					*fInterruptPipe;
		usb_hub_descriptor				fHubDescriptor;

		usb_port_status					fInterruptStatus[8];
		usb_port_status					fPortStatus[8];
		Device							*fChildren[8];
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
		usb_request_data			*RequestData() { return fRequestData; };

		void						SetData(uint8 *buffer, size_t length);
		uint8						*Data() { return fData; };
		size_t						DataLength() { return fDataLength; };

		void						SetActualLength(size_t *actualLength);
		size_t						*ActualLength() { return fActualLengthPointer; };

		void						SetHostPrivate(hostcontroller_priv *priv);
		hostcontroller_priv			*HostPrivate() { return fHostPrivate; };

		void						SetCallback(usb_callback_func callback,
										void *cookie);

		status_t					WaitForFinish();
		void						Finished(uint32 status, size_t actualLength);

private:
		// Data that is related to the transfer
		Pipe						*fPipe;
		uint8						*fData;
		size_t						fDataLength;
		size_t						*fActualLengthPointer;
		size_t						fActualLength;
		uint32						fStatus;

		usb_callback_func			fCallback;
		void						*fCallbackCookie;

		sem_id						fSem;
		hostcontroller_priv			*fHostPrivate;

		// For control transfers
		usb_request_data			*fRequestData;
};

#endif
