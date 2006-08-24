/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#include "usb_p.h"


Device::Device(BusManager *bus, Device *parent, usb_device_descriptor &desc,
	int8 deviceAddress, bool lowSpeed)
	:	ControlPipe(bus, deviceAddress,
			lowSpeed ? Pipe::LowSpeed : Pipe::NormalSpeed,
			desc.max_packet_size_0),
		fDeviceDescriptor(desc),
		fConfigurations(NULL),
		fCurrentConfiguration(NULL),
		fInitOK(false),
		fLowSpeed(lowSpeed),
		fBus(bus),
		fParent(parent),
		fDeviceAddress(deviceAddress),
		fLock(-1),
		fNotifyCookie(NULL)
{
	TRACE(("USB Device: new device\n"));

	fLock = create_sem(1, "USB Device Lock");
	if (fLock < B_OK) {
		TRACE_ERROR(("USB Device: could not create locking semaphore\n"));
		return;
	}

	set_sem_owner(fLock, B_SYSTEM_TEAM);

	fMaxPacketIn[0] = fMaxPacketOut[0] = fDeviceDescriptor.max_packet_size_0; 

	// Get the device descriptor
	// We already have a part of it, but we want it all
	size_t actualLength;
	status_t status = GetDescriptor(USB_DESCRIPTOR_DEVICE, 0, 0,
		(void *)&fDeviceDescriptor, sizeof(fDeviceDescriptor), &actualLength);

	if (status < B_OK || actualLength != sizeof(fDeviceDescriptor)) {
		TRACE_ERROR(("USB Device: error while getting the device descriptor\n"));
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
	TRACE(("\tmanufacturer:........0x%04x\n", fDeviceDescriptor.manufacturer));
	TRACE(("\tproduct:.............0x%02x\n", fDeviceDescriptor.product));
	TRACE(("\tserial_number:.......0x%02x\n", fDeviceDescriptor.serial_number));
	TRACE(("\tnum_configurations:..%d\n", fDeviceDescriptor.num_configurations));

	// Get the configurations
	fConfigurations = (usb_configuration_info *)malloc(
		fDeviceDescriptor.num_configurations * sizeof(usb_configuration_info));
	if (fConfigurations == NULL) {
		TRACE_ERROR(("USB Device: out of memory during config creations!\n"));
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

		TRACE(("USB Device %d: configuration %d\n", fDeviceAddress, i));
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

					Interface *interface = new(std::nothrow) Interface(this);
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

					Pipe *endpoint = NULL;
					switch (endpointDescriptor->attributes & 0x03) {
						case 0x00: /* Control Endpoint */
							endpoint = new(std::nothrow) ControlPipe(this,
								Speed(),
								endpointDescriptor->endpoint_address & 0x0f,
								endpointDescriptor->max_packet_size);
							break;

						case 0x01: /* Isochronous Endpoint */
							endpoint = new(std::nothrow) IsochronousPipe(this,
								endpointDescriptor->endpoint_address & 0x80 > 0 ? Pipe::In : Pipe::Out,
								Speed(),
								endpointDescriptor->endpoint_address & 0x0f,
								endpointDescriptor->max_packet_size);
							break;

						case 0x02: /* Bulk Endpoint */
							endpoint = new(std::nothrow) BulkPipe(this,
								endpointDescriptor->endpoint_address & 0x80 > 0 ? Pipe::In : Pipe::Out,
								Speed(),
								endpointDescriptor->endpoint_address & 0x0f,
								endpointDescriptor->max_packet_size);
							break;

						case 0x03: /* Interrupt Endpoint */
							endpoint = new(std::nothrow) InterruptPipe(this,
								endpointDescriptor->endpoint_address & 0x80 > 0 ? Pipe::In : Pipe::Out,
								Speed(),
								endpointDescriptor->endpoint_address & 0x0f,
								endpointDescriptor->max_packet_size);
							break;
					}

					endpointInfo->handle = endpoint->USBID();
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
					currentInterface->generic[currentInterface->generic_count] =
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
	return SendRequest(
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
	for (uint8 i = 0; i < fDeviceDescriptor.num_configurations; i++) {
		if (configuration == &fConfigurations[i])
			return SetConfigurationAt(i);
	}

	return B_BAD_VALUE;
}


status_t
Device::SetConfigurationAt(uint8 index)
{
	if (index >= fDeviceDescriptor.num_configurations)
		return B_BAD_VALUE;

	status_t result = SendRequest(
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

	// Wait some for the configuration being finished
	snooze(USB_DELAY_SET_CONFIGURATION);
	return B_OK;
}


const usb_device_descriptor *
Device::DeviceDescriptor() const
{
	return &fDeviceDescriptor;
}


void
Device::ReportDevice(usb_support_descriptor *supportDescriptors,
	uint32 supportDescriptorCount, const usb_notify_hooks *hooks, bool added)
{
	TRACE(("USB Device ReportDevice\n"));
	if (supportDescriptorCount == 0 || supportDescriptors == NULL) {
		if (added && hooks->device_added != NULL)
			hooks->device_added(USBID(), &fNotifyCookie);
		else if (!added && hooks->device_removed != NULL)
			hooks->device_removed(fNotifyCookie);
		return;
	}

	for (uint32 i = 0; i < supportDescriptorCount; i++) {
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

			if (added && hooks->device_added != NULL)
				hooks->device_added(USBID(), &fNotifyCookie);
			else if (!added && hooks->device_removed != NULL)
				hooks->device_removed(fNotifyCookie);
			return;
		}

		// we have to check all interfaces for matching class/subclass/protocol
		for (uint32 j = 0; j < fDeviceDescriptor.num_configurations; j++) {
			for (uint32 k = 0; k < fConfigurations[j].interface_count; k++) {
				for (uint32 l = 0; l < fConfigurations[j].interface[k].alt_count; l++) {
					usb_interface_descriptor *descriptor = fConfigurations[j].interface[k].alt[l].descr;
					if ((supportDescriptors[i].dev_class == 0
						|| descriptor->interface_class == supportDescriptors[i].dev_class)
						&& (supportDescriptors[i].dev_subclass == 0
						|| descriptor->interface_subclass == supportDescriptors[i].dev_subclass)
						&& (supportDescriptors[i].dev_protocol == 0
						|| descriptor->interface_protocol == supportDescriptors[i].dev_protocol)) {

						if (added && hooks->device_added != NULL)
							hooks->device_added(USBID(), &fNotifyCookie);
						else if (!added && hooks->device_removed != NULL)
							hooks->device_removed(fNotifyCookie);
						return;
					}
				}
			}
		}
	}
}


status_t
Device::BuildDeviceName(char *string, uint32 *index, size_t bufferSize,
	Device *device)
{
	if (!fParent)
		return B_ERROR;

	fParent->BuildDeviceName(string, index, bufferSize, this);		
	return B_OK;
}


status_t
Device::SetFeature(uint16 selector)
{
	return SendRequest(
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
	return SendRequest(
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
	return SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_DEVICE_IN,
		USB_REQUEST_GET_STATUS,
		0,
		0,
		2,
		(void *)status,
		2,
		NULL);
}
