
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

class USBDevice;
class USBEndpoint;
class USBInterface;
class USBConfiguration;
class USBRosterLooper;

class USBEndpoint
{
public:
// Connectivity functions (this endpoint belongs to an Interface that
// belongs to a Configuration that belongs to a Device)
	const USBInterface *Interface(void) const;
	const USBConfiguration *Configuration(void) const;
	const USBDevice *Device(void) const;

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
	friend USBDevice;          // Only created by a USBDevice
	friend USBInterface;       // Only destroyed by a containing USBInterface
	USBEndpoint(uint32 n, USBInterface *ifc);
	~USBEndpoint();
	
	USBInterface *interface;
	usb_endpoint_descriptor descriptor;
	uint32 endpoint_num;
};

class USBInterface
{
public:
// Connectivity functions (this Interface belongs to a Configuration which
// belongs to a Device)
	const USBConfiguration *Configuration(void) const;
	const USBDevice *Device(void) const;

// Iterate over the Endpoints defined by this Interface
	uint32 CountEndpoints(void) const;
	const USBEndpoint *EndpointAt(uint32 n) const;

// Access the raw descriptor
	const usb_interface_descriptor *Descriptor(void) const;

// Accessors for useful bits in the descriptor	
	uint8 Class(void) const;
	uint8 Subclass(void) const;
	uint8 Protocol(void) const;
	const char *InterfaceString(void) const;

private:
	friend USBDevice;           // only created by USBDevice
	friend USBConfiguration;    // only destroyed by the containing Configuration
	USBInterface(uint32 n, USBConfiguration *cfc);
	~USBInterface();
	
	USBConfiguration *configuration;
	usb_interface_descriptor descriptor;
	uint32 interface_num;
	BList endpoints;
};

class USBConfiguration
{
public:	
// Connectivity functions (this Configuration belongs to a Device)
	const USBDevice *Device(void) const;
	
// Iterators for the Interfaces provided by this Configuration
	uint32 CountInterfaces(void) const;
	const USBInterface *InterfaceAt(uint32 n) const;
	
// Access the raw descriptor
	const usb_configuration_descriptor *Descriptor(void) const;
		
private:
	friend USBDevice;
	USBConfiguration(uint n, USBDevice *dev);
	~USBConfiguration();
	
	USBDevice *device;
	uint configuration_num;
	usb_configuration_descriptor descriptor;
	BList interfaces;
};

class USBDevice 
{
public:
	USBDevice(const char *path = NULL);
	virtual ~USBDevice();
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
	const USBConfiguration *ConfigurationAt(uint32 n) const;

// View and select the active configuration
	const USBConfiguration *ActiveConfiguration(void) const;
	status_t SetConfiguration(const USBConfiguration *conf);

// Initiate a Control (endpoint 0) transaction
	status_t ControlTransfer(uint8 request_type, uint8 request, uint16 value,
							 uint16 index, uint16 length, void *data) const;
		
private:
	status_t InterruptTransfer(const USBEndpoint *e, void *data, size_t length) const;
	status_t BulkTransfer(const USBEndpoint *e, void *data, size_t length) const;
	
	void Release(void);
	void PopulateInterface(USBInterface *ifc, usb_interface_descriptor *descr);
	void PopulateConfig(USBConfiguration *conf, usb_configuration_descriptor *descr);
	void PopulateDevice(void);
	
	mutable char *serial_string;          // Cache the string descriptors
	mutable char *manufacturer_string;
	mutable char *product_string;
	char *location;                       // Bus Topology info
	bool ishub;
	int fd;	                              // Connection to /dev/bus/usb/...
	usb_device_descriptor descriptor;     // Cache the descriptor
	USBConfiguration *active;             // Current active configuration
	BList configurations;                 // All possible configurations
	
friend USBEndpoint; // USBEndpoint uses the XxxTransfer() methods
};


class USBRoster
{
public:
	USBRoster(void);
	virtual ~USBRoster();
	
// DeviceAdded() is called when a new device appears on the Bus.
// If the result is not B_OK, the USBDevice instance is deleted and
// there will be no DeviceRemoved notification.
	virtual status_t DeviceAdded(USBDevice *dev) = 0;
	
// DeviceRemoved() is called when a device is disconnected from the Bus.
// The USBDevice WILL BE deleted after this method returns.
	virtual void DeviceRemoved(USBDevice *dev) = 0;
		
	void Start(void);
	void Stop(void);
private:
	USBRosterLooper *rlooper;

};

#endif 