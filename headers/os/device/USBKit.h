/*
 * Copyright 2007-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _USBKIT_H
#define _USBKIT_H

#include <SupportDefs.h>
#include <USB3.h>
#include <USB_spec.h>


class BUSBRoster;
class BUSBDevice;
class BUSBConfiguration;
class BUSBInterface;
class BUSBEndpoint;


/*	The BUSBRoster class can be used to watch for devices that get attached or
	removed from the USB bus.
	You subclass the roster and implement the pure virtual DeviceAdded() and
	DeviceRemoved() hooks. */
class BUSBRoster {
public:
									BUSBRoster();
virtual								~BUSBRoster();

		// The DeviceAdded() hook will be called when a new device gets
		// attached to the USB bus. The hook is called with an initialized
		// BUSBDevice object. If you return B_OK from your hook the object
		// will stay valid and the DeviceRemoved() hook will be called for
		// it. Otherwise the object is deleted and DeviceRemoved() is not
		// called.
virtual	status_t					DeviceAdded(BUSBDevice *device) = 0;

		// When a device gets detached from the bus that you hold a
		// BUSBDevice object for (gotten through DeviceAdded()) the
		// DeviceRemoved() hook will be called. The device object gets
		// invalid and will be deleted as soon as you return from the hook
		// so be sure to remove all references to it.
virtual	void						DeviceRemoved(BUSBDevice *device) = 0;

		void						Start();
		void						Stop();

private:
virtual	void						_ReservedUSBRoster1();
virtual	void						_ReservedUSBRoster2();
virtual	void						_ReservedUSBRoster3();
virtual	void						_ReservedUSBRoster4();
virtual	void						_ReservedUSBRoster5();

		void *						fLooper;
		uint32						fReserved[10];
};


/*	The BUSBDevice presents an interface to a USB device. You can either get
	it through the BUSBRoster or by creating one yourself and setting it to
	a valid raw usb device (with a path of "/dev/bus/usb/x").
	The device class provides direct accessors for descriptor fields as well
	as convenience functions to directly get string representations of fields.
	The BUSBDevice also provides access for the BUSBConfiguration objects of
	a device. These objects and all of their child objects depend on the
	parent device and will be deleted as soon as the device object is
	destroyed. */
class BUSBDevice {
public:
									BUSBDevice(const char *path = NULL);
virtual								~BUSBDevice();

virtual	status_t					InitCheck();

		status_t					SetTo(const char *path);
		void						Unset();

		// Returns the location on the bus represented as hub/device sequence.
		const char *				Location() const;
		bool						IsHub() const;

		// These are direct accessors to descriptor fields.
		uint16						USBVersion() const;
		uint8						Class() const;
		uint8						Subclass() const;
		uint8						Protocol() const;
		uint8						MaxEndpoint0PacketSize() const;
		uint16						VendorID() const;
		uint16						ProductID() const;
		uint16						Version() const;

		// The string functions return the string representation of the
		// descriptor data. The strings are decoded to normal 0 terminated
		// C strings and are cached and owned by the object.
		// If a string is not available an empty string is returned.
		const char *				ManufacturerString() const;
		const char *				ProductString() const;
		const char *				SerialNumberString() const;

		const usb_device_descriptor *
									Descriptor() const;

		// GetStringDescriptor() can be used to retrieve the raw
		// usb_string_descriptor with a given index. The strings contained
		// in these descriptors are usually two-byte unicode encoded.
		size_t						GetStringDescriptor(uint32 index,
										usb_string_descriptor *descriptor,
										size_t length) const;

		// Use the DecodeStringDescriptor() convenience function to get a
		// 0-terminated c string for a given string index. Note that this
		// will allocate the string as "new char[];" and needs to be deleted
		// like "delete[] string;" by the caller.
		char *						DecodeStringDescriptor(uint32 index) const;

		size_t						GetDescriptor(uint8 type, uint8 index,
										uint16 languageID, void *data,
										size_t length) const;

		// With ConfigurationAt() or ActiveConfiguration() you can get an
		// object that represents the configuration at a certain index or at
		// the index that is currently configured. Note that the index does not
		// necessarily correspond to the configuration_value present in the
		// configuration descriptor.
		// Use the returned object as an argument to SetConfiguration() to
		// change the active configuration of a device.
		uint32						CountConfigurations() const;
		const BUSBConfiguration *	ConfigurationAt(uint32 index) const;

		const BUSBConfiguration *	ActiveConfiguration() const;
		status_t					SetConfiguration(
										const BUSBConfiguration *configuration);

		// ControlTransfer() sends requests using the default pipe.
		ssize_t						ControlTransfer(uint8 requestType,
										uint8 request, uint16 value,
										uint16 index, uint16 length,
										void *data) const;

private:
virtual	void						_ReservedUSBDevice1();
virtual	void						_ReservedUSBDevice2();
virtual	void						_ReservedUSBDevice3();
virtual	void						_ReservedUSBDevice4();
virtual	void						_ReservedUSBDevice5();

		char *						fPath;
		int							fRawFD;

		usb_device_descriptor		fDescriptor;
		BUSBConfiguration **		fConfigurations;
		uint32						fActiveConfiguration;

mutable	char *						fManufacturerString;
mutable	char *						fProductString;
mutable	char *						fSerialNumberString;

		uint32						fReserved[10];
};


/*	A BUSBConfiguration object represents one of possibly multiple
	configurations a device might have. A valid object can only be gotten
	through the ConfigurationAt() or ActiveConfiguration() methods of a
	BUSBDevice.
	The BUSBConfiguration provides further access into the configuration by
	providing CountInterfaces() and InterfaceAt() to retrieve BUSBInterface
	objects. */
class BUSBConfiguration {
public:
		// Device() returns the parent device of this configuration. This
		// configuration is located at the index returned by Index() within
		// that parent device.
		uint32						Index() const;
		const BUSBDevice *			Device() const;

		// Gets a descriptive string for this configuration, if available.
		// Otherwise an empty string is returned.
		const char *				ConfigurationString() const;

		const usb_configuration_descriptor *
									Descriptor() const;

		// With CountInterfaces() and InterfaceAt() you can iterate through
		// the child interfaces of this configuration. It is the only valid
		// way to get a BUSBInterface object.
		// Note that the interface objects retrieved using InterfaceAt() will
		// be invalid and deleted as soon as this configuration gets deleted.
		uint32						CountInterfaces() const;
		const BUSBInterface *		InterfaceAt(uint32 index) const;

private:
friend	class BUSBDevice;
									BUSBConfiguration(BUSBDevice *device,
										uint32 index, int rawFD);
									~BUSBConfiguration();

		BUSBDevice *				fDevice;
		uint32						fIndex;
		int							fRawFD;

		usb_configuration_descriptor fDescriptor;
		BUSBInterface **			fInterfaces;

mutable	char *						fConfigurationString;

		uint32						fReserved[10];
};


/*	The BUSBInterface class can be used to access the descriptor fields of
	an underleying USB interface. Most importantly though it can be used to
	iterate over and retrieve BUSBEndpoint objects that can be used to
	transfer data over the bus. */
class BUSBInterface {
public:
		// Configuration() returns the parent configuration of this interface.
		// This interface is located at the index returned by Index() in that
		// parent configuration and represents the alternate interface returned
		// by AlternateIndex().
		// Device() is a convenience function to directly reach the parent
		// device of this interface instead of going through the configuration.
		uint32						Index() const;
		uint32						AlternateIndex() const;
		const BUSBConfiguration *	Configuration() const;
		const BUSBDevice *			Device() const;

		// These are accessors to descriptor fields. InterfaceString() tries
		// to return a descriptive string for the interface. If no string is
		// available an empty string is returned.
		uint8						Class() const;
		uint8						Subclass() const;
		uint8						Protocol() const;
		const char *				InterfaceString() const;

		const usb_interface_descriptor *
									Descriptor() const;

		// Use OtherDescriptorAt() to get generic descriptors of an interface.
		// These are usually device/interface class specific or they may
		// represent vendor specific extensions.
		status_t					OtherDescriptorAt(uint32 index,
										usb_descriptor *descriptor,
										size_t length) const;

		// CountEndpoints() and EndpointAt() can be used to iterate over the
		// available endpoints within an interface. EndpointAt() is the only
		// valid way to get BUSBEndpoint object. Note that these objects will
		// get invalid and deleted as soon as the parent interface is deleted.
		uint32						CountEndpoints() const;
		const BUSBEndpoint *		EndpointAt(uint32 index) const;

		// Using CountAlternates() you can retrieve the number of alternate
		// interfaces for this interface. Note that this interface itself
		// counts as an alternate so an alternate count of one really means
		// that you are currently using the sole interface present.
		// AlternateAt() returns the interface descriptor of the alternate
		// interface with the specified index. Using that you can peek at the
		// attributes of that alternate (including endpoints) without having to
		// switch to this alternate interface.
		// Note that you cannot use any endpoint you retrieve through an
		// interface you get through AlternateAt(). Even if you switch to that
		// alternate later on, you cannot use an interface returned by
		// AlternateAt(). Instead switch to that alternate using the interface
		// you got from the configuration and then use this switched interface
		// to enumerate the endpoints.
		// ActiveAlternateIndex() returns the index of the currently active
		// alternate interface.
		// With SetAlternate() you can switch this BUSBInterface object to the
		// alternate interface at the specified index. Note that all endpoints
		// retrieved through EndpointAt() will become invalid and will be
		// deleted as soon as you set an alternate interface (even if the
		// resulting interface is the same you were using before).
		uint32						CountAlternates() const;
		uint32						ActiveAlternateIndex() const;
		const BUSBInterface *		AlternateAt(uint32 alternateIndex) const;
		status_t					SetAlternate(uint32 alternateIndex);

private:
friend	class BUSBConfiguration;
									BUSBInterface(BUSBConfiguration *config,
										uint32 index, uint32 alternate,
										int rawFD);
									~BUSBInterface();

		void						_UpdateDescriptorAndEndpoints();

		BUSBConfiguration *			fConfiguration;
		uint32						fIndex;
		uint32						fAlternate;
		int							fRawFD;

		usb_interface_descriptor	fDescriptor;
		BUSBEndpoint **				fEndpoints;

mutable	uint32						fAlternateCount;
mutable	BUSBInterface **			fAlternates;

mutable	char *						fInterfaceString;

		uint32						fReserved[10];
};


/*	The BUSBEndpoint represent a device endpoint that can be used to send or
	receive data. It also allows to query endpoint characteristics like
	endpoint type or direction. */
class BUSBEndpoint {
public:
		// Interface() returns the parent interface of this endpoint.
		// This endpoint is located at the index returned by Index() in the
		// parent interface.
		// Configuration() and Device() are convenience functions to directly
		// reach the parent configuration or device of this endpoint instead
		// of going through the parent objects.
		uint32						Index() const;
		const BUSBInterface *		Interface() const;
		const BUSBConfiguration *	Configuration() const;
		const BUSBDevice *			Device() const;

		// These methods can be used to check for endpoint characteristics.
		bool						IsBulk() const;
		bool						IsInterrupt() const;
		bool						IsIsochronous() const;
		bool						IsControl() const;

		bool						IsInput() const;
		bool						IsOutput() const;

		uint16						MaxPacketSize() const;
		uint8						Interval() const;

		const usb_endpoint_descriptor *
									Descriptor() const;

		// These methods initiate transfers to or from the endpoint. All
		// transfers are synchronous and the actually transfered amount of
		// data is returned as a result. A negative value indicates an error.
		// Which transfer type to use depends on the endpoint type.
		ssize_t						ControlTransfer(uint8 requestType,
										uint8 request, uint16 value,
										uint16 index, uint16 length,
										void *data) const;
		ssize_t						InterruptTransfer(void *data,
										size_t length) const;
		ssize_t						BulkTransfer(void *data,
										size_t length) const;
		ssize_t						IsochronousTransfer(void *data,
										size_t length,
										usb_iso_packet_descriptor *packetDescriptors,
										uint32 packetCount)	const;

		// These are convenience methods for getting and clearing the halt
		// state of an endpoint. They use the control pipe of the device to
		// send the corresponding requests.
		bool						IsStalled() const;
		status_t					ClearStall() const;

private:
friend	class BUSBInterface;
									BUSBEndpoint(BUSBInterface *interface,
										uint32 index, int rawFD);
									~BUSBEndpoint();

		BUSBInterface *				fInterface;
		uint32						fIndex;
		int							fRawFD;

		usb_endpoint_descriptor		fDescriptor;

		uint32						fReserved[10];
};

#endif // _USB_KIT_H
