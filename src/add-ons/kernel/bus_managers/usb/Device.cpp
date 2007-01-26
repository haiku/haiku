/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#include "usb_p.h"


Device::Device(Object *parent, usb_device_descriptor &desc, int8 deviceAddress,
	usb_speed speed)
	:	Object(parent),
		fDeviceDescriptor(desc),
		fInitOK(false),
		fConfigurations(NULL),
		fCurrentConfiguration(NULL),
		fSpeed(speed),
		fDeviceAddress(deviceAddress)
{
	TRACE(("USB Device %d: creating device\n", fDeviceAddress));

	fDefaultPipe = new(std::nothrow) ControlPipe(this, deviceAddress, 0,
		fSpeed, fDeviceDescriptor.max_packet_size_0);
	if (!fDefaultPipe) {
		TRACE_ERROR(("USB Device %d: could not allocate default pipe\n", fDeviceAddress));
		return;
	}

	// Get the device descriptor
	// We already have a part of it, but we want it all
	size_t actualLength;
	status_t status = GetDescriptor(USB_DESCRIPTOR_DEVICE, 0, 0,
		(void *)&fDeviceDescriptor, sizeof(fDeviceDescriptor), &actualLength);

	if (status < B_OK || actualLength != sizeof(fDeviceDescriptor)) {
		TRACE_ERROR(("USB Device %d: error while getting the device descriptor\n", fDeviceAddress));
		return;
	}

	TRACE(("full device descriptor for device %d:\n", fDeviceAddress));
	TRACE(("\tlength:..............%d\n", fDeviceDescriptor.length));
	TRACE(("\tdescriptor_type:.....0x%04x\n", fDeviceDescriptor.descriptor_type));
	TRACE(("\tusb_version:.........0x%04x\n", fDeviceDescriptor.usb_version));
	TRACE(("\tdevice_class:........0x%02x\n", fDeviceDescriptor.device_class));
	TRACE(("\tdevice_subclass:.....0x%02x\n", fDeviceDescriptor.device_subclass));
	TRACE(("\tdevice_protocol:.....0x%02x\n", fDeviceDescriptor.device_protocol));
	TRACE(("\tmax_packet_size_0:...%d\n", fDeviceDescriptor.max_packet_size_0));
	TRACE(("\tvendor_id:...........0x%04x\n", fDeviceDescriptor.vendor_id));
	TRACE(("\tproduct_id:..........0x%04x\n", fDeviceDescriptor.product_id));
	TRACE(("\tdevice_version:......0x%04x\n", fDeviceDescriptor.device_version));
	TRACE(("\tmanufacturer:........0x%02x\n", fDeviceDescriptor.manufacturer));
	TRACE(("\tproduct:.............0x%02x\n", fDeviceDescriptor.product));
	TRACE(("\tserial_number:.......0x%02x\n", fDeviceDescriptor.serial_number));
	TRACE(("\tnum_configurations:..%d\n", fDeviceDescriptor.num_configurations));

	// Get the configurations
	fConfigurations = (usb_configuration_info *)malloc(
		fDeviceDescriptor.num_configurations * sizeof(usb_configuration_info));
	if (fConfigurations == NULL) {
		TRACE_ERROR(("USB Device %d: out of memory during config creations!\n", fDeviceAddress));
		return;
	}

	for (int32 i = 0; i < fDeviceDescriptor.num_configurations; i++) {
		usb_configuration_descriptor configDescriptor;
		status = GetDescriptor(USB_DESCRIPTOR_CONFIGURATION, i, 0,
			(void *)&configDescriptor, sizeof(usb_configuration_descriptor),
			&actualLength);

		if (status < B_OK || actualLength != sizeof(usb_configuration_descriptor)) {
			TRACE_ERROR(("USB Device %d: error fetching configuration %ld\n", fDeviceAddress, i));
			return;
		}

		TRACE(("USB Device %d: configuration %ld\n", fDeviceAddress, i));
		TRACE(("\tlength:..............%d\n", configDescriptor.length));
		TRACE(("\tdescriptor_type:.....0x%02x\n", configDescriptor.descriptor_type));
		TRACE(("\ttotal_length:........%d\n", configDescriptor.total_length));
		TRACE(("\tnumber_interfaces:...%d\n", configDescriptor.number_interfaces));
		TRACE(("\tconfiguration_value:.0x%02x\n", configDescriptor.configuration_value));
		TRACE(("\tconfiguration:.......0x%02x\n", configDescriptor.configuration));
		TRACE(("\tattributes:..........0x%02x\n", configDescriptor.attributes));
		TRACE(("\tmax_power:...........%d\n", configDescriptor.max_power));

		uint8 *configData = (uint8 *)malloc(configDescriptor.total_length);
		status = GetDescriptor(USB_DESCRIPTOR_CONFIGURATION, i, 0,
			(void *)configData, configDescriptor.total_length, &actualLength);

		if (status < B_OK || actualLength != configDescriptor.total_length) {
			TRACE_ERROR(("USB Device %d: error fetching full configuration descriptor %ld\n", fDeviceAddress, i));
			return;
		}

		usb_configuration_descriptor *configuration = (usb_configuration_descriptor *)configData;
		fConfigurations[i].descr = configuration;
		fConfigurations[i].interface_count = configuration->number_interfaces;
		fConfigurations[i].interface = (usb_interface_list *)malloc(
			configuration->number_interfaces * sizeof(usb_interface_list));
		memset(fConfigurations[i].interface, 0,
			configuration->number_interfaces * sizeof(usb_interface_list));

		usb_interface_info *currentInterface = NULL;
		uint32 descriptorStart = sizeof(usb_configuration_descriptor);
		while (descriptorStart < actualLength) {
			switch (configData[descriptorStart + 1]) {
				case USB_DESCRIPTOR_INTERFACE: {
					TRACE(("USB Device %d: got interface descriptor\n", fDeviceAddress));
					usb_interface_descriptor *interfaceDescriptor = (usb_interface_descriptor *)&configData[descriptorStart];
					TRACE(("\tlength:.............%d\n", interfaceDescriptor->length));
					TRACE(("\tdescriptor_type:....0x%02x\n", interfaceDescriptor->descriptor_type));
					TRACE(("\tinterface_number:...%d\n", interfaceDescriptor->interface_number));
					TRACE(("\talternate_setting:..%d\n", interfaceDescriptor->alternate_setting));
					TRACE(("\tnum_endpoints:......%d\n", interfaceDescriptor->num_endpoints));
					TRACE(("\tinterface_class:....0x%02x\n", interfaceDescriptor->interface_class));
					TRACE(("\tinterface_subclass:.0x%02x\n", interfaceDescriptor->interface_subclass));
					TRACE(("\tinterface_protocol:.0x%02x\n", interfaceDescriptor->interface_protocol));
					TRACE(("\tinterface:..........%d\n", interfaceDescriptor->interface));

					usb_interface_list *interfaceList =
						&fConfigurations[i].interface[interfaceDescriptor->interface_number];

					/* allocate this alternate */
					interfaceList->alt_count++;
					interfaceList->alt = (usb_interface_info *)realloc(
						interfaceList->alt, interfaceList->alt_count
						* sizeof(usb_interface_info));
					interfaceList->active = interfaceList->alt;

					/* setup this alternate */
					usb_interface_info *interfaceInfo =
						&interfaceList->alt[interfaceList->alt_count - 1];
					interfaceInfo->descr = interfaceDescriptor;
					interfaceInfo->endpoint_count = 0;
					interfaceInfo->endpoint = NULL;
					interfaceInfo->generic_count = 0;
					interfaceInfo->generic = NULL;

					Interface *interface = new(std::nothrow) Interface(this,
						interfaceDescriptor->interface_number);
					interfaceInfo->handle = interface->USBID();

					currentInterface = interfaceInfo;
					break;
				}

				case USB_DESCRIPTOR_ENDPOINT: {
					TRACE(("USB Device %d: got endpoint descriptor\n", fDeviceAddress));
					usb_endpoint_descriptor *endpointDescriptor = (usb_endpoint_descriptor *)&configData[descriptorStart];
					TRACE(("\tlength:.............%d\n", endpointDescriptor->length));
					TRACE(("\tdescriptor_type:....0x%02x\n", endpointDescriptor->descriptor_type));
					TRACE(("\tendpoint_address:...0x%02x\n", endpointDescriptor->endpoint_address));
					TRACE(("\tattributes:.........0x%02x\n", endpointDescriptor->attributes));
					TRACE(("\tmax_packet_size:....%d\n", endpointDescriptor->max_packet_size));
					TRACE(("\tinterval:...........%d\n", endpointDescriptor->interval));

					if (!currentInterface)
						break;

					/* allocate this endpoint */
					currentInterface->endpoint_count++;
					currentInterface->endpoint = (usb_endpoint_info *)realloc(
						currentInterface->endpoint,
						currentInterface->endpoint_count
						* sizeof(usb_endpoint_info));

					/* setup this endpoint */
					usb_endpoint_info *endpointInfo =
						&currentInterface->endpoint[currentInterface->endpoint_count - 1];
					endpointInfo->descr = endpointDescriptor;
					endpointInfo->handle = 0;
					break;
				}

				default:
					TRACE(("USB Device %d: got generic descriptor\n", fDeviceAddress));
					usb_generic_descriptor *genericDescriptor = (usb_generic_descriptor *)&configData[descriptorStart];
					TRACE(("\tlength:.............%d\n", genericDescriptor->length));
					TRACE(("\tdescriptor_type:....0x%02x\n", genericDescriptor->descriptor_type));

					if (!currentInterface)
						break;

					/* allocate this descriptor */
					currentInterface->generic_count++;
					currentInterface->generic = (usb_descriptor **)realloc(
						currentInterface->generic,
						currentInterface->generic_count
						* sizeof(usb_descriptor *));

					/* add this descriptor */
					currentInterface->generic[currentInterface->generic_count - 1] =
						(usb_descriptor *)genericDescriptor;
					break;
			}

			descriptorStart += configData[descriptorStart];
		}
	}

	// Set default configuration
	TRACE(("USB Device %d: setting default configuration\n", fDeviceAddress));
	if (SetConfigurationAt(0) < B_OK) {
		TRACE_ERROR(("USB Device %d: failed to set default configuration\n", fDeviceAddress));
		return;
	}

	fInitOK = true;
}


Device::~Device()
{
	// Destroy open endpoints. Do not send a device request to unconfigure
	// though, since we may be deleted because the device was unplugged
	// already.
	Unconfigure(false);

	// Free all allocated resources
	for (int32 i = 0; i < fDeviceDescriptor.num_configurations; i++) {
		usb_configuration_info *configuration = &fConfigurations[i];
		for (size_t j = 0; j < configuration->interface_count; j++) {
			usb_interface_list *interfaceList = configuration->interface;
			for (size_t k = 0; k < interfaceList->alt_count; k++) {
				usb_interface_info *interface = &interfaceList->alt[k];
				delete (Interface *)GetStack()->GetObject(interface->handle);
				free(interface->endpoint);
				free(interface->generic);
			}

			free(interfaceList->alt);
		}

		free(configuration->interface);
		free(configuration->descr);
	}

	free(fConfigurations);
	delete fDefaultPipe;
}


status_t
Device::InitCheck()
{
	if (fInitOK)
		return B_OK;

	return B_ERROR;
}


status_t
Device::GetDescriptor(uint8 descriptorType, uint8 index, uint16 languageID,
	void *data, size_t dataLength, size_t *actualLength)
{
	return fDefaultPipe->SendRequest(
		USB_REQTYPE_DEVICE_IN | USB_REQTYPE_STANDARD,		// type
		USB_REQUEST_GET_DESCRIPTOR,							// request
		(descriptorType << 8) | index,						// value
		languageID,											// index
		dataLength,											// length
		data,												// buffer
		dataLength,											// buffer length
		actualLength);										// actual length
}


const usb_configuration_info *
Device::Configuration() const
{
	return fCurrentConfiguration;
}


const usb_configuration_info *
Device::ConfigurationAt(uint8 index) const
{
	if (index >= fDeviceDescriptor.num_configurations)
		return NULL;

	return &fConfigurations[index];
}


status_t
Device::SetConfiguration(const usb_configuration_info *configuration)
{
	if (!configuration)
		return Unconfigure(true);

	for (uint8 i = 0; i < fDeviceDescriptor.num_configurations; i++) {
		if (configuration->descr->configuration_value
			== fConfigurations[i].descr->configuration_value)
			return SetConfigurationAt(i);
	}

	return B_BAD_VALUE;
}


status_t
Device::SetConfigurationAt(uint8 index)
{
	if (index >= fDeviceDescriptor.num_configurations)
		return B_BAD_VALUE;
	if (&fConfigurations[index] == fCurrentConfiguration)
		return B_OK;

	// Destroy our open endpoints
	Unconfigure(false);

	// Tell the device to set the configuration
	status_t result = fDefaultPipe->SendRequest(
		USB_REQTYPE_DEVICE_OUT | USB_REQTYPE_STANDARD,		// type
		USB_REQUEST_SET_CONFIGURATION,						// request
		fConfigurations[index].descr->configuration_value,	// value
		0,													// index
		0,													// length
		NULL,												// buffer
		0,													// buffer length
		NULL);												// actual length

	if (result < B_OK)
		return result;

	// Set current configuration
	fCurrentConfiguration = &fConfigurations[index];

	// Initialize all the endpoints that are now active
	usb_interface_info *interfaceInfo = fCurrentConfiguration->interface[0].active;
	for (size_t i = 0; i < interfaceInfo->endpoint_count; i++) {
		usb_endpoint_info *endpoint = &interfaceInfo->endpoint[i];
		Pipe *pipe = NULL;

		switch (endpoint->descr->attributes & 0x03) {
			case 0x00: /* Control Endpoint */
				pipe = new(std::nothrow) ControlPipe(this, fDeviceAddress,
					endpoint->descr->endpoint_address & 0x0f, fSpeed,
					endpoint->descr->max_packet_size);
				break;

			case 0x01: /* Isochronous Endpoint */
				pipe = new(std::nothrow) IsochronousPipe(this, fDeviceAddress,
					endpoint->descr->endpoint_address & 0x0f,
					(endpoint->descr->endpoint_address & 0x80) > 0 ? Pipe::In : Pipe::Out,
					fSpeed, endpoint->descr->max_packet_size);
				break;

			case 0x02: /* Bulk Endpoint */
				pipe = new(std::nothrow) BulkPipe(this, fDeviceAddress,
					endpoint->descr->endpoint_address & 0x0f,
					(endpoint->descr->endpoint_address & 0x80) > 0 ? Pipe::In : Pipe::Out,
					fSpeed, endpoint->descr->max_packet_size);
				break;

			case 0x03: /* Interrupt Endpoint */
				pipe = new(std::nothrow) InterruptPipe(this, fDeviceAddress,
					endpoint->descr->endpoint_address & 0x0f,
					(endpoint->descr->endpoint_address & 0x80) > 0 ? Pipe::In : Pipe::Out,
					fSpeed, endpoint->descr->max_packet_size);
				break;
		}

		endpoint->handle = pipe->USBID();
	}

	// Wait some for the configuration being finished
	snooze(USB_DELAY_SET_CONFIGURATION);
	return B_OK;
}


status_t
Device::Unconfigure(bool atDeviceLevel)
{
	// if we only want to destroy our open pipes before setting
	// another configuration unconfigure will be called with
	// atDevice = false. otherwise we explicitly want to unconfigure
	// the device and have to send it the corresponding request.
	if (atDeviceLevel) {
		status_t result = fDefaultPipe->SendRequest(
			USB_REQTYPE_DEVICE_OUT | USB_REQTYPE_STANDARD,	// type
			USB_REQUEST_SET_CONFIGURATION,					// request
			0,												// value
			0,												// index
			0,												// length
			NULL,											// buffer
			0,												// buffer length
			NULL);											// actual length

		if (result < B_OK)
			return result;

		snooze(USB_DELAY_SET_CONFIGURATION);
	}

	if (!fCurrentConfiguration)
		return B_OK;

	usb_interface_info *interfaceInfo = fCurrentConfiguration->interface[0].active;
	for (size_t i = 0; i < interfaceInfo->endpoint_count; i++) {
		usb_endpoint_info *endpoint = &interfaceInfo->endpoint[i];
		delete (Pipe *)GetStack()->GetObject(endpoint->handle);
		endpoint->handle = 0;
	}

	fCurrentConfiguration = NULL;
	return B_OK;
}


const usb_device_descriptor *
Device::DeviceDescriptor() const
{
	return &fDeviceDescriptor;
}


status_t
Device::ReportDevice(usb_support_descriptor *supportDescriptors,
	uint32 supportDescriptorCount, const usb_notify_hooks *hooks,
	usb_driver_cookie **cookies, bool added)
{
	TRACE(("USB Device %d: reporting device\n", fDeviceAddress));
	bool supported = false;
	if (supportDescriptorCount == 0 || supportDescriptors == NULL)
		supported = true;

	for (uint32 i = 0; !supported && i < supportDescriptorCount; i++) {
		if ((supportDescriptors[i].vendor != 0
			&& fDeviceDescriptor.vendor_id != supportDescriptors[i].vendor)
			|| (supportDescriptors[i].product != 0
			&& fDeviceDescriptor.product_id != supportDescriptors[i].product))
			continue;

		if ((supportDescriptors[i].dev_class == 0
			|| fDeviceDescriptor.device_class == supportDescriptors[i].dev_class)
			&& (supportDescriptors[i].dev_subclass == 0
			|| fDeviceDescriptor.device_subclass == supportDescriptors[i].dev_subclass)
			&& (supportDescriptors[i].dev_protocol == 0
			|| fDeviceDescriptor.device_protocol == supportDescriptors[i].dev_protocol)) {
			supported = true;
		}

		// we have to check all interfaces for matching class/subclass/protocol
		for (uint32 j = 0; !supported && j < fDeviceDescriptor.num_configurations; j++) {
			for (uint32 k = 0; !supported && k < fConfigurations[j].interface_count; k++) {
				for (uint32 l = 0; !supported && l < fConfigurations[j].interface[k].alt_count; l++) {
					usb_interface_descriptor *descriptor = fConfigurations[j].interface[k].alt[l].descr;
					if ((supportDescriptors[i].dev_class == 0
						|| descriptor->interface_class == supportDescriptors[i].dev_class)
						&& (supportDescriptors[i].dev_subclass == 0
						|| descriptor->interface_subclass == supportDescriptors[i].dev_subclass)
						&& (supportDescriptors[i].dev_protocol == 0
						|| descriptor->interface_protocol == supportDescriptors[i].dev_protocol)) {
						supported = true;
					}
				}
			}
		}
	}

	if (!supported)
		return B_UNSUPPORTED;

	if ((added && hooks->device_added == NULL)
		|| (!added && hooks->device_removed == NULL)) {
		// hooks are not installed, but report success to indicate that
		// the driver supports the device
		return B_OK;
	}

	usb_id id = USBID();
	if (added) {
		usb_driver_cookie *cookie = new(std::nothrow) usb_driver_cookie;
		if (hooks->device_added(id, &cookie->cookie) >= B_OK) {
			cookie->device = id;
			cookie->link = *cookies;
			*cookies = cookie;
		} else
			delete cookie;
	} else {
		usb_driver_cookie **pointer = cookies;
		usb_driver_cookie *cookie = *cookies;
		while (cookie) {
			if (cookie->device == id)
				break;
			pointer = &cookie->link;
			cookie = cookie->link;
		}

		if (!cookie) {
			// the device is supported, but there is no cookie. this most
			// probably means that the device_added hook above failed.
			return B_OK;
		}

		hooks->device_removed(cookie->cookie);
		*pointer = cookie->link;
		delete cookie;
	}

	return B_OK;
}


status_t
Device::BuildDeviceName(char *string, uint32 *index, size_t bufferSize,
	Device *device)
{
	if (!Parent() || (Parent()->Type() & USB_OBJECT_HUB) == 0)
		return B_ERROR;

	((Hub *)Parent())->BuildDeviceName(string, index, bufferSize, this);
	return B_OK;
}


status_t
Device::SetFeature(uint16 selector)
{
	return fDefaultPipe->SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_DEVICE_OUT,
		USB_REQUEST_SET_FEATURE,
		selector,
		0,
		0,
		NULL,
		0,
		NULL);
}


status_t
Device::ClearFeature(uint16 selector)
{
	return fDefaultPipe->SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_DEVICE_OUT,
		USB_REQUEST_CLEAR_FEATURE,
		selector,
		0,
		0,
		NULL,
		0,
		NULL);
}


status_t
Device::GetStatus(uint16 *status)
{
	return fDefaultPipe->SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_DEVICE_IN,
		USB_REQUEST_GET_STATUS,
		0,
		0,
		2,
		(void *)status,
		2,
		NULL);
}
