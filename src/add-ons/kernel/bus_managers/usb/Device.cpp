/*
 * Copyright 2003-2014, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */


#include "usb_private.h"


Device::Device(Object* parent, int8 hubAddress, uint8 hubPort,
	usb_device_descriptor& desc, int8 deviceAddress, usb_speed speed,
	bool isRootHub, void* controllerCookie)
	:
	Object(parent),
	fDeviceDescriptor(desc),
	fInitOK(false),
	fAvailable(true),
	fIsRootHub(isRootHub),
	fConfigurations(NULL),
	fCurrentConfiguration(NULL),
	fSpeed(speed),
	fDeviceAddress(deviceAddress),
	fHubAddress(hubAddress),
	fHubPort(hubPort),
	fControllerCookie(controllerCookie)
{
	TRACE("creating device\n");

	fDefaultPipe = new(std::nothrow) ControlPipe(this);
	if (fDefaultPipe == NULL) {
		TRACE_ERROR("could not allocate default pipe\n");
		return;
	}

	fDefaultPipe->InitCommon(fDeviceAddress, 0, fSpeed, Pipe::Default,
		fDeviceDescriptor.max_packet_size_0, 0, fHubAddress, fHubPort);

	// Get the device descriptor
	// We already have a part of it, but we want it all
	size_t actualLength;
	status_t status = GetDescriptor(USB_DESCRIPTOR_DEVICE, 0, 0,
		(void*)&fDeviceDescriptor, sizeof(fDeviceDescriptor), &actualLength);

	if (status < B_OK || actualLength != sizeof(fDeviceDescriptor)) {
		TRACE_ERROR("error while getting the device descriptor\n");
		return;
	}

	TRACE("full device descriptor for device %d:\n", fDeviceAddress);
	TRACE("\tlength:..............%d\n", fDeviceDescriptor.length);
	TRACE("\tdescriptor_type:.....0x%04x\n", fDeviceDescriptor.descriptor_type);
	TRACE("\tusb_version:.........0x%04x\n", fDeviceDescriptor.usb_version);
	TRACE("\tdevice_class:........0x%02x\n", fDeviceDescriptor.device_class);
	TRACE("\tdevice_subclass:.....0x%02x\n", fDeviceDescriptor.device_subclass);
	TRACE("\tdevice_protocol:.....0x%02x\n", fDeviceDescriptor.device_protocol);
	TRACE("\tmax_packet_size_0:...%d\n", fDeviceDescriptor.max_packet_size_0);
	TRACE("\tvendor_id:...........0x%04x\n", fDeviceDescriptor.vendor_id);
	TRACE("\tproduct_id:..........0x%04x\n", fDeviceDescriptor.product_id);
	TRACE("\tdevice_version:......0x%04x\n", fDeviceDescriptor.device_version);
	TRACE("\tmanufacturer:........0x%02x\n", fDeviceDescriptor.manufacturer);
	TRACE("\tproduct:.............0x%02x\n", fDeviceDescriptor.product);
	TRACE("\tserial_number:.......0x%02x\n", fDeviceDescriptor.serial_number);
	TRACE("\tnum_configurations:..%d\n", fDeviceDescriptor.num_configurations);

	// Get the configurations
	fConfigurations = (usb_configuration_info*)malloc(
		fDeviceDescriptor.num_configurations * sizeof(usb_configuration_info));
	if (fConfigurations == NULL) {
		TRACE_ERROR("out of memory during config creations!\n");
		return;
	}

	memset(fConfigurations, 0, fDeviceDescriptor.num_configurations
		* sizeof(usb_configuration_info));
	for (int32 i = 0; i < fDeviceDescriptor.num_configurations; i++) {
		usb_configuration_descriptor configDescriptor;
		status = GetDescriptor(USB_DESCRIPTOR_CONFIGURATION, i, 0,
			(void*)&configDescriptor, sizeof(usb_configuration_descriptor),
			&actualLength);

		if (status < B_OK
			|| actualLength != sizeof(usb_configuration_descriptor)) {
			TRACE_ERROR("error fetching configuration %" B_PRId32 "\n", i);
			return;
		}

		TRACE("configuration %" B_PRId32 "\n", i);
		TRACE("\tlength:..............%d\n", configDescriptor.length);
		TRACE("\tdescriptor_type:.....0x%02x\n",
			configDescriptor.descriptor_type);
		TRACE("\ttotal_length:........%d\n", configDescriptor.total_length);
		TRACE("\tnumber_interfaces:...%d\n",
			configDescriptor.number_interfaces);
		TRACE("\tconfiguration_value:.0x%02x\n",
			configDescriptor.configuration_value);
		TRACE("\tconfiguration:.......0x%02x\n",
			configDescriptor.configuration);
		TRACE("\tattributes:..........0x%02x\n", configDescriptor.attributes);
		TRACE("\tmax_power:...........%d\n", configDescriptor.max_power);

		uint8* configData = (uint8*)malloc(configDescriptor.total_length);
		if (configData == NULL) {
			TRACE_ERROR("out of memory when reading config\n");
			return;
		}

		status = GetDescriptor(USB_DESCRIPTOR_CONFIGURATION, i, 0,
			(void*)configData, configDescriptor.total_length, &actualLength);

		if (status < B_OK || actualLength != configDescriptor.total_length) {
			TRACE_ERROR("error fetching full configuration"
				" descriptor %" B_PRId32 " got %" B_PRIuSIZE " expected %"
				B_PRIu16 "\n", i, actualLength, configDescriptor.total_length);
			free(configData);
			return;
		}

		usb_configuration_descriptor* configuration
			= (usb_configuration_descriptor*)configData;
		fConfigurations[i].descr = configuration;
		fConfigurations[i].interface_count = configuration->number_interfaces;
		fConfigurations[i].interface = (usb_interface_list*)malloc(
			configuration->number_interfaces * sizeof(usb_interface_list));
		if (fConfigurations[i].interface == NULL) {
			TRACE_ERROR("out of memory when creating interfaces\n");
			return;
		}

		memset(fConfigurations[i].interface, 0,
			configuration->number_interfaces * sizeof(usb_interface_list));

		usb_interface_info* currentInterface = NULL;
		uint32 descriptorStart = sizeof(usb_configuration_descriptor);
		while (descriptorStart < actualLength) {
			switch (configData[descriptorStart + 1]) {
				case USB_DESCRIPTOR_INTERFACE:
				{
					TRACE("got interface descriptor\n");
					usb_interface_descriptor* interfaceDescriptor
						= (usb_interface_descriptor*)&configData[
							descriptorStart];
					TRACE("\tlength:.............%d\n",
						interfaceDescriptor->length);
					TRACE("\tdescriptor_type:....0x%02x\n",
						interfaceDescriptor->descriptor_type);
					TRACE("\tinterface_number:...%d\n",
						interfaceDescriptor->interface_number);
					TRACE("\talternate_setting:..%d\n",
						interfaceDescriptor->alternate_setting);
					TRACE("\tnum_endpoints:......%d\n",
						interfaceDescriptor->num_endpoints);
					TRACE("\tinterface_class:....0x%02x\n",
						interfaceDescriptor->interface_class);
					TRACE("\tinterface_subclass:.0x%02x\n",
						interfaceDescriptor->interface_subclass);
					TRACE("\tinterface_protocol:.0x%02x\n",
						interfaceDescriptor->interface_protocol);
					TRACE("\tinterface:..........%d\n",
						interfaceDescriptor->interface);

					if (interfaceDescriptor->interface_number
							>= fConfigurations[i].interface_count) {
						interfaceDescriptor->interface_number
							= fConfigurations[i].interface_count - 1;
						TRACE_ERROR("Corrected invalid interface_number!\n");
					}

					usb_interface_list* interfaceList = &fConfigurations[i]
						.interface[interfaceDescriptor->interface_number];

					// Allocate this alternate
					interfaceList->alt_count++;
					usb_interface_info* newAlternates
						= (usb_interface_info*)realloc(interfaceList->alt,
						interfaceList->alt_count * sizeof(usb_interface_info));
					if (newAlternates == NULL) {
						TRACE_ERROR("out of memory allocating"
							" alternate interface\n");
						interfaceList->alt_count--;
						return;
					}

					interfaceList->alt = newAlternates;

					// Set active interface always to the first one
					interfaceList->active = interfaceList->alt;

					// Setup this alternate
					usb_interface_info* interfaceInfo
						= &interfaceList->alt[interfaceList->alt_count - 1];
					interfaceInfo->descr = interfaceDescriptor;
					interfaceInfo->endpoint_count = 0;
					interfaceInfo->endpoint = NULL;
					interfaceInfo->generic_count = 0;
					interfaceInfo->generic = NULL;

					Interface* interface = new(std::nothrow) Interface(this,
						interfaceDescriptor->interface_number);
					if (interface == NULL) {
						TRACE_ERROR("failed to allocate"
							" interface object\n");
						return;
					}

					interfaceInfo->handle = interface->USBID();
					currentInterface = interfaceInfo;
					break;
				}

				case USB_DESCRIPTOR_ENDPOINT:
				{
					TRACE("got endpoint descriptor\n");
					usb_endpoint_descriptor* endpointDescriptor
						= (usb_endpoint_descriptor*)&configData[descriptorStart];
					TRACE("\tlength:.............%d\n",
						endpointDescriptor->length);
					TRACE("\tdescriptor_type:....0x%02x\n",
						endpointDescriptor->descriptor_type);
					TRACE("\tendpoint_address:...0x%02x\n",
						endpointDescriptor->endpoint_address);
					TRACE("\tattributes:.........0x%02x\n",
						endpointDescriptor->attributes);
					TRACE("\tmax_packet_size:....%d\n",
						endpointDescriptor->max_packet_size);
					TRACE("\tinterval:...........%d\n",
						endpointDescriptor->interval);

					if (currentInterface == NULL)
						break;

					// Allocate this endpoint
					currentInterface->endpoint_count++;
					usb_endpoint_info* newEndpoints = (usb_endpoint_info*)
						realloc(currentInterface->endpoint,
							currentInterface->endpoint_count
								* sizeof(usb_endpoint_info));
					if (newEndpoints == NULL) {
						TRACE_ERROR("out of memory allocating new endpoint\n");
						currentInterface->endpoint_count--;
						return;
					}

					currentInterface->endpoint = newEndpoints;

					// Setup this endpoint
					usb_endpoint_info* endpointInfo = &currentInterface
						->endpoint[currentInterface->endpoint_count - 1];
					endpointInfo->descr = endpointDescriptor;
					endpointInfo->handle = 0;
					break;
				}

				case USB_DESCRIPTOR_ENDPOINT_SS_COMPANION: {
					if (currentInterface != NULL) {
						usb_endpoint_descriptor* desc
							= currentInterface->endpoint[
								currentInterface->endpoint_count - 1].descr;
						if ((uint8*)desc != (&configData[descriptorStart
								- desc->length])) {
							TRACE_ERROR("found endpoint companion descriptor "
								"not immediately following endpoint "
								"descriptor, ignoring!\n");
							break;
						}
						// TODO: It'd be nicer if we could store the endpoint
						// companion descriptor along with the endpoint
						// descriptor, but as the interface struct is public
						// API, that would be an ABI break.
					}

					// fall through
				}

				default:
					TRACE("got generic descriptor\n");
					usb_generic_descriptor* genericDescriptor
						= (usb_generic_descriptor*)&configData[descriptorStart];
					TRACE("\tlength:.............%d\n",
						genericDescriptor->length);
					TRACE("\tdescriptor_type:....0x%02x\n",
						genericDescriptor->descriptor_type);

					if (currentInterface == NULL)
						break;

					// Allocate this descriptor
					currentInterface->generic_count++;
					usb_descriptor** newGenerics = (usb_descriptor**)realloc(
						currentInterface->generic,
						currentInterface->generic_count
							* sizeof(usb_descriptor*));
					if (newGenerics == NULL) {
						TRACE_ERROR("out of memory allocating"
							" generic descriptor\n");
						currentInterface->generic_count--;
						return;
					}

					currentInterface->generic = newGenerics;

					// Add this descriptor
					currentInterface->generic[
						currentInterface->generic_count - 1]
							= (usb_descriptor*)genericDescriptor;
					break;
			}

			descriptorStart += configData[descriptorStart];
		}
	}

	// Set default configuration
	TRACE("setting default configuration\n");
	if (SetConfigurationAt(0) != B_OK) {
		TRACE_ERROR("failed to set default configuration\n");
		return;
	}

	fInitOK = true;
}


Device::~Device()
{
	// Cancel transfers on the default pipe and put its USBID to prevent
	// further transfers from being queued.
	if (fDefaultPipe != NULL) {
		fDefaultPipe->PutUSBID(false);
		fDefaultPipe->CancelQueuedTransfers(true);
		fDefaultPipe->WaitForUnbusy();
	}

	// Destroy open endpoints. Do not send a device request to unconfigure
	// though, since we may be deleted because the device was unplugged already.
	Unconfigure(false);

	// Destroy all Interfaces in the Configurations hierarchy.
	for (int32 i = 0; fConfigurations != NULL
			&& i < fDeviceDescriptor.num_configurations; i++) {
		usb_configuration_info* configuration = &fConfigurations[i];
		if (configuration == NULL || configuration->interface == NULL)
			continue;

		for (size_t j = 0; j < configuration->interface_count; j++) {
			usb_interface_list* interfaceList = &configuration->interface[j];
			if (interfaceList->alt == NULL)
				continue;

			for (size_t k = 0; k < interfaceList->alt_count; k++) {
				usb_interface_info* interface = &interfaceList->alt[k];
				Interface* interfaceObject =
					(Interface*)GetStack()->GetObject(interface->handle);
				if (interfaceObject != NULL)
					interfaceObject->SetBusy(false);
				delete interfaceObject;
				interface->handle = 0;
			}
		}
	}

	// Remove ourselves from the stack before deleting public structures.
	PutUSBID();
	delete fDefaultPipe;

	if (fConfigurations == NULL) {
		// we didn't get far in device setup, so everything below is unneeded
		return;
	}

	// Free the Configurations hierarchy.
	for (int32 i = 0; i < fDeviceDescriptor.num_configurations; i++) {
		usb_configuration_info* configuration = &fConfigurations[i];
		if (configuration == NULL)
			continue;

		free(configuration->descr);
		if (configuration->interface == NULL)
			continue;

		for (size_t j = 0; j < configuration->interface_count; j++) {
			usb_interface_list* interfaceList = &configuration->interface[j];
			if (interfaceList->alt == NULL)
				continue;

			for (size_t k = 0; k < interfaceList->alt_count; k++) {
				usb_interface_info* interface = &interfaceList->alt[k];
				free(interface->endpoint);
				free(interface->generic);
			}

			free(interfaceList->alt);
		}

		free(configuration->interface);
	}

	free(fConfigurations);
}


status_t
Device::InitCheck()
{
	if (fInitOK)
		return B_OK;

	return B_ERROR;
}


status_t
Device::Changed(change_item** changeList, bool added)
{
	fAvailable = added;
	change_item* changeItem = new(std::nothrow) change_item;
	if (changeItem == NULL)
		return B_NO_MEMORY;

	changeItem->added = added;
	changeItem->device = this;
	changeItem->link = *changeList;
	*changeList = changeItem;
	return B_OK;
}


status_t
Device::GetDescriptor(uint8 descriptorType, uint8 index, uint16 languageID,
	void* data, size_t dataLength, size_t* actualLength)
{
	if (!fAvailable)
		return B_ERROR;

	return fDefaultPipe->SendRequest(
		USB_REQTYPE_DEVICE_IN | USB_REQTYPE_STANDARD,
		USB_REQUEST_GET_DESCRIPTOR, (descriptorType << 8) | index,
		languageID, dataLength, data, dataLength, actualLength);
}


const usb_configuration_info*
Device::Configuration() const
{
	return fCurrentConfiguration;
}


const usb_configuration_info*
Device::ConfigurationAt(uint8 index) const
{
	if (index >= fDeviceDescriptor.num_configurations)
		return NULL;

	return &fConfigurations[index];
}


status_t
Device::SetConfiguration(const usb_configuration_info* configuration)
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
	if (!fAvailable)
		return B_ERROR;
	if (index >= fDeviceDescriptor.num_configurations)
		return B_BAD_VALUE;
	if (&fConfigurations[index] == fCurrentConfiguration)
		return B_OK;

	// Destroy our open endpoints
	Unconfigure(false);

	// Tell the device to set the configuration
	status_t result = fDefaultPipe->SendRequest(
		USB_REQTYPE_DEVICE_OUT | USB_REQTYPE_STANDARD,
		USB_REQUEST_SET_CONFIGURATION,
		fConfigurations[index].descr->configuration_value, 0, 0, NULL, 0, NULL);
	if (result < B_OK)
		return result;

	// Set current configuration
	fCurrentConfiguration = &fConfigurations[index];

	// Initialize all the endpoints that are now active
	InitEndpoints(-1);

	// Wait some for the configuration being finished
	if (!fIsRootHub)
		snooze(USB_DELAY_SET_CONFIGURATION);
	return B_OK;
}


void
Device::InitEndpoints(int32 interfaceIndex)
{
	for (size_t j = 0; j < fCurrentConfiguration->interface_count; j++) {
		if (interfaceIndex >= 0 && j != (size_t)interfaceIndex)
			continue;

		usb_interface_info* interfaceInfo
			= fCurrentConfiguration->interface[j].active;
		if (interfaceInfo == NULL)
			continue;

		for (size_t i = 0; i < interfaceInfo->endpoint_count; i++) {
			usb_endpoint_info* endpoint = &interfaceInfo->endpoint[i];
			Pipe* pipe = NULL;

			usb_endpoint_ss_companion_descriptor* comp_descr = NULL;
			if (fSpeed == USB_SPEED_SUPERSPEED) {
				// We should have a companion descriptor for this device.
				// Let's find it: it'll be the "i"th one.
				size_t k = 0;
				for (size_t j = 0; j < interfaceInfo->generic_count; j++) {
					usb_descriptor* desc = interfaceInfo->generic[j];
					if (desc->endpoint.descriptor_type
							!= USB_DESCRIPTOR_ENDPOINT_SS_COMPANION) {
						continue;
					}
					if (k == i) {
						comp_descr =
							(usb_endpoint_ss_companion_descriptor*)desc;
						break;
					}
					k++;
				}
				if (comp_descr == NULL) {
					TRACE_ERROR("SuperSpeed device without an endpoint companion "
						"descriptor!");
				}
			}

			Pipe::pipeDirection direction = Pipe::Out;
			if ((endpoint->descr->endpoint_address & 0x80) != 0)
				direction = Pipe::In;

			switch (endpoint->descr->attributes & 0x03) {
				case USB_ENDPOINT_ATTR_CONTROL:		// Control Endpoint
					pipe = new(std::nothrow) ControlPipe(this);
					direction = Pipe::Default;
					break;

				case USB_ENDPOINT_ATTR_ISOCHRONOUS:	// Isochronous Endpoint
					pipe = new(std::nothrow) IsochronousPipe(this);
					break;

				case USB_ENDPOINT_ATTR_BULK:		// Bulk Endpoint
					pipe = new(std::nothrow) BulkPipe(this);
					break;

				case USB_ENDPOINT_ATTR_INTERRUPT:	// Interrupt Endpoint
					pipe = new(std::nothrow) InterruptPipe(this);
					break;
			}

			if (pipe == NULL) {
				TRACE_ERROR("failed to allocate pipe\n");
				endpoint->handle = 0;
				continue;
			}

			pipe->InitCommon(fDeviceAddress,
				endpoint->descr->endpoint_address & 0x0f,
				fSpeed, direction, endpoint->descr->max_packet_size,
				endpoint->descr->interval, fHubAddress, fHubPort);
			if (comp_descr != NULL) {
				pipe->InitSuperSpeed(comp_descr->max_burst,
					comp_descr->bytes_per_interval);
			}
			endpoint->handle = pipe->USBID();
		}
	}
}


status_t
Device::Unconfigure(bool atDeviceLevel)
{
	// If we only want to destroy our open pipes before setting
	// another configuration unconfigure will be called with
	// atDevice = false. otherwise we explicitly want to unconfigure
	// the device and have to send it the corresponding request.
	if (atDeviceLevel && fAvailable) {
		status_t result = fDefaultPipe->SendRequest(
			USB_REQTYPE_DEVICE_OUT | USB_REQTYPE_STANDARD,
			USB_REQUEST_SET_CONFIGURATION, 0, 0, 0, NULL, 0, NULL);
		if (result < B_OK)
			return result;

		snooze(USB_DELAY_SET_CONFIGURATION);
	}

	if (!fCurrentConfiguration)
		return B_OK;

	ClearEndpoints(-1);
	fCurrentConfiguration = NULL;
	return B_OK;
}


void
Device::ClearEndpoints(int32 interfaceIndex)
{
	if (fCurrentConfiguration == NULL
		|| fCurrentConfiguration->interface == NULL)
		return;

	for (size_t j = 0; j < fCurrentConfiguration->interface_count; j++) {
		if (interfaceIndex >= 0 && j != (size_t)interfaceIndex)
			continue;

		usb_interface_info* interfaceInfo
			= fCurrentConfiguration->interface[j].active;
		if (interfaceInfo == NULL || interfaceInfo->endpoint == NULL)
			continue;

		for (size_t i = 0; i < interfaceInfo->endpoint_count; i++) {
			usb_endpoint_info* endpoint = &interfaceInfo->endpoint[i];
			Pipe* pipe = (Pipe*)GetStack()->GetObject(endpoint->handle);
			if (pipe != NULL)
				pipe->SetBusy(false);
			delete pipe;
			endpoint->handle = 0;
		}
	}
}


status_t
Device::SetAltInterface(const usb_interface_info* interface)
{
	uint8 interfaceNumber = interface->descr->interface_number;
	// Tell the device to set the alternate settings
	status_t result = fDefaultPipe->SendRequest(
		USB_REQTYPE_INTERFACE_OUT | USB_REQTYPE_STANDARD,
		USB_REQUEST_SET_INTERFACE,
		interface->descr->alternate_setting, interfaceNumber, 0, NULL, 0, NULL);
	if (result < B_OK)
		return result;

	// Clear the no longer active endpoints
	ClearEndpoints(interfaceNumber);

	// Update the active pointer of the interface list
	usb_interface_list* interfaceList
		= &fCurrentConfiguration->interface[interfaceNumber];
	interfaceList->active
		= &interfaceList->alt[interface->descr->alternate_setting];

	// Initialize the new endpoints
	InitEndpoints(interfaceNumber);
	return result;
}


const usb_device_descriptor*
Device::DeviceDescriptor() const
{
	return &fDeviceDescriptor;
}


status_t
Device::ReportDevice(usb_support_descriptor* supportDescriptors,
	uint32 supportDescriptorCount, const usb_notify_hooks* hooks,
	usb_driver_cookie** cookies, bool added, bool recursive)
{
	TRACE("reporting device\n");
	bool supported = false;
	if (supportDescriptorCount == 0 || supportDescriptors == NULL)
		supported = true;

	for (uint32 i = 0; !supported && i < supportDescriptorCount; i++) {
		if ((supportDescriptors[i].vendor != 0
				&& fDeviceDescriptor.vendor_id != supportDescriptors[i].vendor)
			|| (supportDescriptors[i].product != 0
				&& fDeviceDescriptor.product_id
					!= supportDescriptors[i].product))
			continue;

		if ((supportDescriptors[i].dev_class == 0
				|| fDeviceDescriptor.device_class
					== supportDescriptors[i].dev_class)
			&& (supportDescriptors[i].dev_subclass == 0
				|| fDeviceDescriptor.device_subclass
					== supportDescriptors[i].dev_subclass)
			&& (supportDescriptors[i].dev_protocol == 0
				|| fDeviceDescriptor.device_protocol
					== supportDescriptors[i].dev_protocol)) {
			supported = true;
		}

		// we have to check all interfaces for matching class/subclass/protocol
		for (uint32 j = 0;
				!supported && j < fDeviceDescriptor.num_configurations; j++) {
			for (uint32 k = 0;
					!supported && k < fConfigurations[j].interface_count; k++) {
				for (uint32 l = 0; !supported
					&& l < fConfigurations[j].interface[k].alt_count; l++) {
					usb_interface_descriptor* descriptor
						= fConfigurations[j].interface[k].alt[l].descr;
					if ((supportDescriptors[i].dev_class == 0
							|| descriptor->interface_class
								== supportDescriptors[i].dev_class)
						&& (supportDescriptors[i].dev_subclass == 0
							|| descriptor->interface_subclass
								== supportDescriptors[i].dev_subclass)
						&& (supportDescriptors[i].dev_protocol == 0
							|| descriptor->interface_protocol
								== supportDescriptors[i].dev_protocol)) {
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
		usb_driver_cookie* cookie = new(std::nothrow) usb_driver_cookie;
		if (hooks->device_added(id, &cookie->cookie) >= B_OK) {
			cookie->device = id;
			cookie->link = *cookies;
			*cookies = cookie;
		} else
			delete cookie;
	} else {
		usb_driver_cookie** pointer = cookies;
		usb_driver_cookie* cookie = *cookies;
		while (cookie != NULL) {
			if (cookie->device == id)
				break;
			pointer = &cookie->link;
			cookie = cookie->link;
		}

		if (cookie == NULL) {
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
Device::BuildDeviceName(char* string, uint32* index, size_t bufferSize,
	Device* device)
{
	if (!Parent() || (Parent()->Type() & USB_OBJECT_HUB) == 0)
		return B_ERROR;

	((Hub*)Parent())->BuildDeviceName(string, index, bufferSize, this);
	return B_OK;
}


status_t
Device::SetFeature(uint16 selector)
{
	if (!fAvailable)
		return B_ERROR;

	TRACE("set feature %u\n", selector);
	return fDefaultPipe->SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_DEVICE_OUT,
		USB_REQUEST_SET_FEATURE, selector, 0, 0, NULL, 0, NULL);
}


status_t
Device::ClearFeature(uint16 selector)
{
	if (!fAvailable)
		return B_ERROR;

	TRACE("clear feature %u\n", selector);
	return fDefaultPipe->SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_DEVICE_OUT,
		USB_REQUEST_CLEAR_FEATURE, selector, 0, 0, NULL, 0, NULL);
}


status_t
Device::GetStatus(uint16* status)
{
	if (!fAvailable)
		return B_ERROR;

	TRACE("get status\n");
	return fDefaultPipe->SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_DEVICE_IN,
		USB_REQUEST_GET_STATUS, 0, 0, 2, (void*)status, 2, NULL);
}
