
// USBKit.h
//
// Copyright 1999, Be Incorporated.
//
// Prototype USB Kit / Prerelease Sample Code
// Contents may change massively before official release.

#ifndef _USBKIT_H
#define _USBKIT_H

// Provides definitions for usb_xxx_descriptor structures.
#include <USB.h>

#include <List.h>

class BUSBDevice;
class BUSBEndpoint;
class BUSBInterface;
class BUSBConfiguration;
class BUSBRosterLooper;

class BUSBEndpoint
{
public:
// Connectivity functions (this endpoint belongs to an Interface that
// belongs to a Configuration that belongs to a Device)
	const BUSBInterface *Interface(void) const;
	const BUSBConfiguration *Configuration(void) const;
	const BUSBDevice *Device(void) const;

// Raw descriptor data	
	const usb_endpoint_descriptor *Descriptor(void) const;
	
// Descriptor Accessor Functions
	bool IsBulk(void) const;
	bool IsInterrupt(void) const;
	bool IsIsochronous(void) const;
	bool IsInput(void) const;
	bool IsOutput(void) const;
	uint16 MaxPacketSize(void) const;
	uint8 Interval(void) const;

// Initiate a USB Transaction	
	status_t InterruptTransfer(void *data, size_t length) const;
	status_t BulkTransfer(void *data, size_t length) const;
	
private:
	friend BUSBDevice;          // Only created by a USBDevice
	friend BUSBInterface;       // Only destroyed by a containing USBInterface
	BUSBEndpoint(uint32 n, BUSBInterface *ifc);
	~BUSBEndpoint();
	
	BUSBInterface *interface;
	usb_endpoint_descriptor descriptor;
	uint32 endpoint_num;
};

class BUSBInterface
{
public:
// Connectivity functions (this Interface belongs to a Configuration which
// belongs to a Device)
	const BUSBConfiguration *Configuration(void) const;
	const BUSBDevice *Device(void) const;

// Iterate over the Endpoints defined by this Interface
	uint32 CountEndpoints(void) const;
	const BUSBEndpoint *EndpointAt(uint32 n) const;

// Access the raw descriptor
	const usb_interface_descriptor *Descriptor(void) const;

// Accessors for useful bits in the descriptor	
	uint8 Class(void) const;
	uint8 Subclass(void) const;
	uint8 Protocol(void) const;
	const char *InterfaceString(void) const;

private:
	friend BUSBDevice;           // only created by USBDevice
	friend BUSBConfiguration;    // only destroyed by the containing Configuration
	BUSBInterface(uint32 n, BUSBConfiguration *cfc);
	~BUSBInterface();
	
	BUSBConfiguration *configuration;
	usb_interface_descriptor descriptor;
	uint32 interface_num;
	BList endpoints;
};

class BUSBConfiguration
{
public:	
// Connectivity functions (this Configuration belongs to a Device)
	const BUSBDevice *Device(void) const;
	
// Iterators for the Interfaces provided by this Configuration
	uint32 CountInterfaces(void) const;
	const BUSBInterface *InterfaceAt(uint32 n) const;
	
// Access the raw descriptor
	const usb_configuration_descriptor *Descriptor(void) const;
		
private:
	friend BUSBDevice;
	BUSBConfiguration(uint n, BUSBDevice *dev);
	~BUSBConfiguration();
	
	BUSBDevice *device;
	uint configuration_num;
	usb_configuration_descriptor descriptor;
	BList interfaces;
};

class BUSBDevice 
{
public:
	BUSBDevice(const char *path = NULL);
	virtual ~BUSBDevice();
	virtual status_t InitCheck(void);
	status_t SetTo(const char *path);

// Bus Topology information methods:

// Return the logical path (eg 0/0/1 of the device), or NULL if unconfigured
	const char *Location(void) const;
// Return true if this device is a usb hub	
	bool IsHub(void) const;
	
// Accessors for useful bits in the descriptor
	uint16 USBVersion(void) const;
	uint8 Class(void) const;
	uint8 Subclass(void) const; 
	uint8 Protocol(void) const;
	uint8 MaxEndpoint0PacketSize(void) const;
	uint16 VendorID(void) const;
	uint16 ProductID(void) const;
	uint16 Version(void) const;
	const char *ManufacturerString(void) const;
	const char *ProductString(void) const;
	const char *SerialNumberString(void) const;
	
// Access the raw descriptor
	const usb_device_descriptor *Descriptor(void) const;
		
// Read a string descriptor from the Device into a buffer
	size_t GetStringDescriptor(uint32 n, usb_string_descriptor *desc, size_t length) const;
	
// Read a string descriptor, convert it from Unicode to UTF8, return a new char[]
// containing the string (must be delete'd by the caller)	
	char *DecodeStringDescriptor(uint32 n) const;
	
// Iterate over the possible configurations
	uint32 CountConfigurations(void) const;
	const BUSBConfiguration *ConfigurationAt(uint32 n) const;

// View and select the active configuration
	const BUSBConfiguration *ActiveConfiguration(void) const;
	status_t SetConfiguration(const BUSBConfiguration *conf);

// Initiate a Control (endpoint 0) transaction
	status_t ControlTransfer(uint8 request_type, uint8 request, uint16 value,
							 uint16 index, uint16 length, void *data) const;
		
private:
	status_t InterruptTransfer(const BUSBEndpoint *e, void *data, size_t length) const;
	status_t BulkTransfer(const BUSBEndpoint *e, void *data, size_t length) const;
	
	void Release(void);
	void PopulateInterface(BUSBInterface *ifc, usb_interface_descriptor *descr);
	void PopulateConfig(BUSBConfiguration *conf, usb_configuration_descriptor *descr);
	void PopulateDevice(void);
	
	mutable char *serial_string;          // Cache the string descriptors
	mutable char *manufacturer_string;
	mutable char *product_string;
	char *location;                       // Bus Topology info
	bool ishub;
	int fd;	                              // Connection to /dev/bus/usb/...
	usb_device_descriptor descriptor;     // Cache the descriptor
	BUSBConfiguration *active;             // Current active configuration
	BList configurations;                 // All possible configurations
	
friend BUSBEndpoint; // USBEndpoint uses the XxxTransfer() methods
};


class BUSBRoster
{
public:
	BUSBRoster(void);
	virtual ~BUSBRoster();
	
// DeviceAdded() is called when a new device appears on the Bus.
// If the result is not B_OK, the USBDevice instance is deleted and
// there will be no DeviceRemoved notification.
	virtual status_t DeviceAdded(BUSBDevice *dev) = 0;
	
// DeviceRemoved() is called when a device is disconnected from the Bus.
// The USBDevice WILL BE deleted after this method returns.
	virtual void DeviceRemoved(BUSBDevice *dev) = 0;
		
	void Start(void);
	void Stop(void);
private:
	USBRosterLooper *rlooper;

};

#endif
