/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009-13 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */


#include "Device.h"

#include <kernel.h>
#include <usb/USB_audio.h>

#include "Driver.h"
#include "Settings.h"


Device::Device(usb_device device)
	:
	fStatus(B_ERROR),
	fOpen(false),
	fRemoved(false),
	fDevice(device),
	fNonBlocking(false),
	fAudioControl(this),
	fBuffersReadySem(-1)
{
	const usb_device_descriptor* deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	if (deviceDescriptor == NULL) {
		TRACE(ERR, "Error of getting USB device descriptor.\n");
		return;
	}

	fVendorID = deviceDescriptor->vendor_id;
	fProductID = deviceDescriptor->product_id;
	fUSBVersion = deviceDescriptor->usb_version;

	fBuffersReadySem = create_sem(0, DRIVER_NAME "_buffers_ready");
	if (fBuffersReadySem < B_OK) {
		TRACE(ERR, "Error of creating ready "
			"buffers semaphore:%#010x\n", fBuffersReadySem);
		return;
	}

	if (_SetupEndpoints() != B_OK)
		return;

	// must be set in derived class constructor
	fStatus = B_OK;
}


Device::~Device()
{
	for (Vector<Stream*>::Iterator I = fStreams.Begin();
			I != fStreams.End(); I++)
		delete *I;

	fStreams.MakeEmpty();

	if (fBuffersReadySem > B_OK)
		delete_sem(fBuffersReadySem);
}


status_t
Device::Open(uint32 flags)
{
	if (fOpen)
		return B_BUSY;
	if (fRemoved)
		return B_ERROR;

	status_t result = StartDevice();
	if (result != B_OK)
		return result;

	// TODO: are we need this???
	fNonBlocking = (flags & O_NONBLOCK) == O_NONBLOCK;
	fOpen = true;
	return result;
}


status_t
Device::Close()
{
	if (fRemoved) {
		fOpen = false;
		return B_OK;
	}

	for (int i = 0; i < fStreams.Count(); i++)
		fStreams[i]->Stop();

	fOpen = false;

	return StopDevice();
}


status_t
Device::Free()
{
	return B_OK;
}


status_t
Device::Read(uint8* buffer, size_t* numBytes)
{
	*numBytes = 0;
	return B_IO_ERROR;
}


status_t
Device::Write(const uint8* buffer, size_t* numBytes)
{
	*numBytes = 0;
	return B_IO_ERROR;
}


status_t
Device::Control(uint32 op, void* buffer, size_t length)
{
	switch (op) {
		case B_MULTI_GET_DESCRIPTION:
		{
			multi_description description;
			multi_channel_info channels[16];
			multi_channel_info* originalChannels;

			if (user_memcpy(&description, buffer, sizeof(multi_description))
					!= B_OK)
				return B_BAD_ADDRESS;

			originalChannels = description.channels;
			description.channels = channels;
			if (description.request_channel_count > 16)
				description.request_channel_count = 16;

			status_t status = _MultiGetDescription(&description);
			if (status != B_OK)
				return status;

			description.channels = originalChannels;
			if (user_memcpy(buffer, &description, sizeof(multi_description))
					!= B_OK)
				return B_BAD_ADDRESS;
			return user_memcpy(originalChannels, channels,
				sizeof(multi_channel_info) * description.request_channel_count);
		}
		case B_MULTI_GET_EVENT_INFO:
			TRACE(ERR, "B_MULTI_GET_EVENT_INFO n/i\n");
			return B_ERROR;

		case B_MULTI_SET_EVENT_INFO:
			TRACE(ERR, "B_MULTI_SET_EVENT_INFO n/i\n");
			return B_ERROR;

		case B_MULTI_GET_EVENT:
			TRACE(ERR, "B_MULTI_GET_EVENT n/i\n");
			return B_ERROR;

		case B_MULTI_GET_ENABLED_CHANNELS:
		{
			multi_channel_enable enable;
			uint32 enable_bits;
			uchar* orig_enable_bits;

			if (user_memcpy(&enable, buffer, sizeof(enable)) != B_OK
					|| !IS_USER_ADDRESS(enable.enable_bits)) {
				return B_BAD_ADDRESS;
			}

			orig_enable_bits = enable.enable_bits;
			enable.enable_bits = (uchar*)&enable_bits;
			status_t status = _MultiGetEnabledChannels(&enable);
			if (status != B_OK)
				return status;

			enable.enable_bits = orig_enable_bits;
			if (user_memcpy(enable.enable_bits, &enable_bits,
					sizeof(enable_bits)) != B_OK
				|| user_memcpy(buffer, &enable,
					sizeof(multi_channel_enable)) != B_OK) {
				return B_BAD_ADDRESS;
			}

			return B_OK;
		}
		case B_MULTI_SET_ENABLED_CHANNELS:
		{
			multi_channel_enable enable;
			uint32 enable_bits;
			uchar* orig_enable_bits;

			if (user_memcpy(&enable, buffer, sizeof(enable)) != B_OK
				|| !IS_USER_ADDRESS(enable.enable_bits)) {
				return B_BAD_ADDRESS;
			}

			orig_enable_bits = enable.enable_bits;
			enable.enable_bits = (uchar*)&enable_bits;
			status_t status = _MultiSetEnabledChannels(&enable);
			if (status != B_OK)
				return status;

			enable.enable_bits = orig_enable_bits;
			if (user_memcpy(enable.enable_bits, &enable_bits,
					sizeof(enable_bits)) < B_OK
				|| user_memcpy(buffer, &enable, sizeof(multi_channel_enable))
					< B_OK) {
				return B_BAD_ADDRESS;
			}

			return B_OK;
		}
		case B_MULTI_GET_GLOBAL_FORMAT:
		{
			multi_format_info info;
			if (user_memcpy(&info, buffer, sizeof(multi_format_info)) != B_OK)
				return B_BAD_ADDRESS;

			status_t status = _MultiGetGlobalFormat(&info);
			if (status != B_OK)
				return status;
			if (user_memcpy(buffer, &info, sizeof(multi_format_info)) != B_OK)
				return B_BAD_ADDRESS;
			return B_OK;
		}
		case B_MULTI_SET_GLOBAL_FORMAT:
		{
			multi_format_info info;
			if (user_memcpy(&info, buffer, sizeof(multi_format_info)) != B_OK)
				return B_BAD_ADDRESS;

			status_t status = _MultiSetGlobalFormat(&info);
			if (status != B_OK)
				return status;
			return user_memcpy(buffer, &info, sizeof(multi_format_info));
		}
		case B_MULTI_GET_CHANNEL_FORMATS:
			TRACE(ERR, "B_MULTI_GET_CHANNEL_FORMATS n/i\n");
			return B_ERROR;

		case B_MULTI_SET_CHANNEL_FORMATS:
			TRACE(ERR, "B_MULTI_SET_CHANNEL_FORMATS n/i\n");
			return B_ERROR;

		case B_MULTI_GET_MIX:
		case B_MULTI_SET_MIX: {
			multi_mix_value_info info;
			if (user_memcpy(&info, buffer, sizeof(multi_mix_value_info)) != B_OK)
				return B_BAD_ADDRESS;

			multi_mix_value* originalValues = info.values;
			size_t mixValueSize = info.item_count * sizeof(multi_mix_value);
			multi_mix_value* values = (multi_mix_value*)alloca(mixValueSize);
			if (user_memcpy(values, info.values, mixValueSize) != B_OK)
				return B_BAD_ADDRESS;
			info.values = values;

			status_t status;
			if (op == B_MULTI_GET_MIX)
				status = _MultiGetMix(&info);
			else
				status = _MultiSetMix(&info);
			if (status != B_OK)
				return status;
			// the multi_mix_value_info is not modified
			return user_memcpy(originalValues, values, mixValueSize);
		}

		case B_MULTI_LIST_MIX_CHANNELS:
			TRACE(ERR, "B_MULTI_LIST_MIX_CHANNELS n/i\n");
			return B_ERROR;

		case B_MULTI_LIST_MIX_CONTROLS:
		{
			multi_mix_control_info info;
			multi_mix_control* original_controls;
			size_t allocSize;
			multi_mix_control *controls;

			if (user_memcpy(&info, buffer, sizeof(multi_mix_control_info))
				!= B_OK) {
				return B_BAD_ADDRESS;
			}

			original_controls = info.controls;
			allocSize = sizeof(multi_mix_control) * info.control_count;
			controls = (multi_mix_control *)malloc(allocSize);
			if (controls == NULL)
				return B_NO_MEMORY;

			if (!IS_USER_ADDRESS(info.controls)
				|| user_memcpy(controls, info.controls, allocSize) < B_OK) {
				free(controls);
				return B_BAD_ADDRESS;
			}
			info.controls = controls;

			status_t status = _MultiListMixControls(&info);
			if (status != B_OK) {
				free(controls);
				return status;
			}

			info.controls = original_controls;
			status = user_memcpy(info.controls, controls, allocSize);
			if (status == B_OK) {
				status = user_memcpy(buffer, &info,
					sizeof(multi_mix_control_info));
			}
			if (status != B_OK)
				status = B_BAD_ADDRESS;
			free(controls);
			return status;
		}
		case B_MULTI_LIST_MIX_CONNECTIONS:
			TRACE(ERR, "B_MULTI_LIST_MIX_CONNECTIONS n/i\n");
			return B_ERROR;

		case B_MULTI_GET_BUFFERS:
			// Fill out the struct for the first time; doesn't start anything.
		{
			multi_buffer_list list;
			if (user_memcpy(&list, buffer, sizeof(multi_buffer_list)) != B_OK)
				return B_BAD_ADDRESS;
			buffer_desc **original_playback_descs = list.playback_buffers;
			buffer_desc **original_record_descs = list.record_buffers;

			buffer_desc *playback_descs[list.request_playback_buffers];
			buffer_desc *record_descs[list.request_record_buffers];

			if (!IS_USER_ADDRESS(list.playback_buffers)
				|| user_memcpy(playback_descs, list.playback_buffers,
					sizeof(buffer_desc*) * list.request_playback_buffers)
					< B_OK
				|| !IS_USER_ADDRESS(list.record_buffers)
				|| user_memcpy(record_descs, list.record_buffers,
					sizeof(buffer_desc*) * list.request_record_buffers)
					< B_OK) {
				return B_BAD_ADDRESS;
			}

			list.playback_buffers = playback_descs;
			list.record_buffers = record_descs;
			status_t status = _MultiGetBuffers(&list);
			if (status != B_OK)
				return status;

			list.playback_buffers = original_playback_descs;
			list.record_buffers = original_record_descs;

			if (user_memcpy(buffer, &list, sizeof(multi_buffer_list)) < B_OK
				|| user_memcpy(original_playback_descs, playback_descs,
					sizeof(buffer_desc*) * list.request_playback_buffers)
					< B_OK
				|| user_memcpy(original_record_descs, record_descs,
					sizeof(buffer_desc*) * list.request_record_buffers)
					< B_OK) {
				status = B_BAD_ADDRESS;
			}

			return status;
		}

		case B_MULTI_SET_BUFFERS:
			// Set what buffers to use, if the driver supports soft buffers.
			TRACE(ERR, "B_MULTI_SET_BUFFERS n/i\n");
			return B_ERROR; /* we do not support soft buffers */

		case B_MULTI_SET_START_TIME:
			// When to actually start
			TRACE(ERR, "B_MULTI_SET_START_TIME n/i\n");
			return B_ERROR;

		case B_MULTI_BUFFER_EXCHANGE:
			// stop and go are derived from this being called
			return _MultiBufferExchange((multi_buffer_info*)buffer);

		case B_MULTI_BUFFER_FORCE_STOP:
			// force stop of playback, nothing in data
			return _MultiBufferForceStop();

		default:
			TRACE(ERR, "Unhandled IOCTL catched: %#010x\n", op);
	}

	return B_DEV_INVALID_IOCTL;
}


void
Device::Removed()
{
	fRemoved = true;

	for (int i = 0; i < fStreams.Count(); i++)
		fStreams[i]->OnRemove();
}


status_t
Device::SetupDevice(bool deviceReplugged)
{
	return B_OK;
}


status_t
Device::CompareAndReattach(usb_device device)
{
	const usb_device_descriptor* deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	if (deviceDescriptor == NULL) {
		TRACE(ERR, "Error of getting USB device descriptor.\n");
		return B_ERROR;
	}

	if (deviceDescriptor->vendor_id != fVendorID
		&& deviceDescriptor->product_id != fProductID)
		// this certainly isn't the same device
		return B_BAD_VALUE;

	// this is the same device that was replugged - clear the removed state,
	// re- setup the endpoints and transfers and open the device if it was
	// previously opened
	fDevice = device;
	fRemoved = false;
	status_t result = _SetupEndpoints();
	if (result != B_OK) {
		fRemoved = true;
		return result;
	}

	// we need to setup hardware on device replug
	result = SetupDevice(true);
	if (result != B_OK)
		return result;

	if (fOpen) {
		fOpen = false;
		result = Open(fNonBlocking ? O_NONBLOCK : 0);
	}

	return result;
}


status_t
Device::_MultiGetDescription(multi_description* multiDescription)
{
	multi_description Description;
	if (user_memcpy(&Description, multiDescription,
			sizeof(multi_description)) != B_OK)
		return B_BAD_ADDRESS;

	Description.interface_version = B_CURRENT_INTERFACE_VERSION;
	Description.interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strlcpy(Description.friendly_name, "USB Audio",
		sizeof(Description.friendly_name));

	strlcpy(Description.vendor_info, "S.Zharski",
		sizeof(Description.vendor_info));

	Description.output_channel_count = 0;
	Description.input_channel_count = 0;
	Description.output_bus_channel_count = 0;
	Description.input_bus_channel_count = 0;
	Description.aux_bus_channel_count = 0;

	Description.output_rates = 0;
	Description.input_rates = 0;

	Description.min_cvsr_rate = 0;
	Description.max_cvsr_rate = 0;

	Description.output_formats = 0;
	Description.input_formats = 0;
	Description.lock_sources = B_MULTI_LOCK_INTERNAL;
	Description.timecode_sources = 0;
	Description.interface_flags = 0;
	Description.start_latency = 3000;

	Description.control_panel[0] = '\0';

	Vector<_AudioControl*>	USBTerminals;

	// channels (USB I/O  terminals) are already in fStreams
	// in outputs->inputs order, use them.
	for (int i = 0; i < fStreams.Count(); i++) {
		uint8 id = fStreams[i]->TerminalLink();
		_AudioControl* control = fAudioControl.Find(id);
		// if (control->SubType() == USB_AUDIO_AC_OUTPUT_TERMINAL) {
		// if (control->SubType() == USB_AUDIO_AC_INPUT_TERMINAL) {
		//	USBTerminals.PushFront(control);
		//	fStreams[i]->GetFormatsAndRates(Description);
		// } else
		// if (control->SubType() == IDSInputTerminal) {
			USBTerminals.PushBack(control);
			fStreams[i]->GetFormatsAndRates(&Description);
		// }
	}

	Vector<multi_channel_info> Channels;
	fAudioControl.GetChannelsDescription(Channels, &Description, USBTerminals);
	fAudioControl.GetBusChannelsDescription(Channels, &Description );

	// Description.request_channel_count = channels + bus_channels;

	TraceMultiDescription(&Description, Channels);

	if (user_memcpy(multiDescription, &Description,
			sizeof(multi_description)) != B_OK)
		return B_BAD_ADDRESS;

	if (user_memcpy(multiDescription->channels,
			&Channels[0], sizeof(multi_channel_info) * min_c(Channels.Count(),
			Description.request_channel_count)) != B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


void
Device::TraceMultiDescription(multi_description* Description,
		Vector<multi_channel_info>& Channels)
{
	TRACE(API, "interface_version:%d\n", Description->interface_version);
	TRACE(API, "interface_minimum:%d\n", Description->interface_minimum);
	TRACE(API, "friendly_name:%s\n", Description->friendly_name);
	TRACE(API, "vendor_info:%s\n", Description->vendor_info);
	TRACE(API, "output_channel_count:%d\n", Description->output_channel_count);
	TRACE(API, "input_channel_count:%d\n", Description->input_channel_count);
	TRACE(API, "output_bus_channel_count:%d\n",
		Description->output_bus_channel_count);
	TRACE(API, "input_bus_channel_count:%d\n",
		Description->input_bus_channel_count);
	TRACE(API, "aux_bus_channel_count:%d\n", Description->aux_bus_channel_count);
	TRACE(API, "output_rates:%#08x\n", Description->output_rates);
	TRACE(API, "input_rates:%#08x\n", Description->input_rates);
	TRACE(API, "min_cvsr_rate:%f\n", Description->min_cvsr_rate);
	TRACE(API, "max_cvsr_rate:%f\n", Description->max_cvsr_rate);
	TRACE(API, "output_formats:%#08x\n", Description->output_formats);
	TRACE(API, "input_formats:%#08x\n", Description->input_formats);
	TRACE(API, "lock_sources:%d\n", Description->lock_sources);
	TRACE(API, "timecode_sources:%d\n", Description->timecode_sources);
	TRACE(API, "interface_flags:%#08x\n", Description->interface_flags);
	TRACE(API, "start_latency:%d\n", Description->start_latency);
	TRACE(API, "control_panel:%s\n", Description->control_panel);

	// multi_channel_info* Channels = Description->channels;
	// for (int i = 0; i < Description->request_channel_count; i++) {
	for (int i = 0; i < Channels.Count(); i++) {
		TRACE(API, " channel_id:%d\n", Channels[i].channel_id);
		TRACE(API, "  kind:%#02x\n", Channels[i].kind);
		TRACE(API, "  designations:%#08x\n", Channels[i].designations);
		TRACE(API, "  connectors:%#08x\n", Channels[i].connectors);
	}

	TRACE(API, "request_channel_count:%d\n\n",
		Description->request_channel_count);
}


status_t
Device::_MultiGetEnabledChannels(multi_channel_enable* Enable)
{
	status_t status = B_OK;

	Enable->lock_source = B_MULTI_LOCK_INTERNAL;

	uint32 offset = 0;
	for (int i = 0; i < fStreams.Count() && status == B_OK; i++)
		status = fStreams[i]->GetEnabledChannels(offset, Enable);

	return status;
}


status_t
Device::_MultiSetEnabledChannels(multi_channel_enable* Enable)
{
	status_t status = B_OK;
	uint32 offset = 0;
	for (int i = 0; i < fStreams.Count() && status == B_OK; i++)
		status = fStreams[i]->SetEnabledChannels(offset, Enable);

	return status;
}


status_t
Device::_MultiGetGlobalFormat(multi_format_info* Format)
{
	status_t status = B_OK;

	Format->output_latency = 0;
	Format->input_latency = 0;
	Format->timecode_kind = 0;

	// uint32 offset = 0;
	for (int i = 0; i < fStreams.Count() && status == B_OK; i++)
		status = fStreams[i]->GetGlobalFormat(Format);

	return status;
}


status_t
Device::_MultiSetGlobalFormat(multi_format_info* Format)
{
	status_t status = B_OK;

	TRACE(API, "output_latency:%lld\n", Format->output_latency);
	TRACE(API, "input_latency:%lld\n",  Format->input_latency);
	TRACE(API, "timecode_kind:%#08x\n", Format->timecode_kind);

	// uint32 offset = 0;
	for (int i = 0; i < fStreams.Count() && status == B_OK; i++)
		status = fStreams[i]->SetGlobalFormat(Format);

	return status;
}


status_t
Device::_MultiGetBuffers(multi_buffer_list* List)
{
	status_t status = B_OK;

	TRACE(API, "info_size:%d\n"
	"request_playback_buffers:%d\n"
	"request_playback_channels:%d\n"
	"request_playback_buffer_size:%d\n"
	"request_record_buffers:%d\n"
	"request_record_channels:%d\n"
	"request_record_buffer_size:%d\n",
		List->info_size,
		List->request_playback_buffers,
		List->request_playback_channels,
		List->request_playback_buffer_size,
		List->request_record_buffers,
		List->request_record_channels,
		List->request_record_buffer_size);

	List->flags = 0;
	List->return_playback_channels = 0;
	List->return_record_channels = 0;

	for (int i = 0; i < fStreams.Count() && status == B_OK; i++)
		status = fStreams[i]->GetBuffers(List);

	TRACE(API, "flags:%#x\n"
	"return_playback_buffers:%d\n"
	"return_playback_channels:%d\n"
	"return_playback_buffer_size:%d\n"
	"return_record_buffers:%d\n"
	"return_record_channels:%d\n"
	"return_record_buffer_size:%d\n",
		List->flags,
		List->return_playback_buffers,
		List->return_playback_channels,
		List->return_playback_buffer_size,
		List->return_record_buffers,
		List->return_record_channels,
		List->return_record_buffer_size);

#if 0
	TRACE(API, "playback buffers\n");
	for (int32_t b = 0; b <  List->return_playback_buffers; b++)
		for (int32 c = 0; c < List->return_playback_channels; c++)
			TRACE(API, "%d:%d %08x:%d\n", b, c, List->playback_buffers[b][c].base,
				List->playback_buffers[b][c].stride);

	TRACE(API, "record buffers:\n");
	for (int32_t b = 0; b <  List->return_record_buffers; b++)
		for (int32 c = 0; c < List->return_record_channels; c++)
			TRACE(API, "%d:%d %08x:%d\n", b, c, List->record_buffers[b][c].base,
				List->record_buffers[b][c].stride);
#endif
	return B_OK;
}


status_t
Device::_MultiBufferExchange(multi_buffer_info* multiInfo)
{
	multi_buffer_info Info;
	if (!IS_USER_ADDRESS(multiInfo)
		|| user_memcpy(&Info, multiInfo, sizeof(multi_buffer_info)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	for (int i = 0; i < fStreams.Count(); i++)
		if (!fStreams[i]->IsRunning())
			fStreams[i]->Start();

	status_t status = acquire_sem_etc(fBuffersReadySem, 1,
		B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, 50000);
	if (status == B_TIMED_OUT) {
		TRACE(ERR, "Timeout during buffers exchange.\n");
		return status;
	}

	status = B_ERROR;
	for (int i = 0; i < fStreams.Count(); i++)
		if (fStreams[i]->ExchangeBuffer(&Info)) {
			status = B_OK;
			break;
		}

	if (status != B_OK) {
		TRACE(ERR, "Error processing buffers:%08x.\n", status);
		return status;
	}

	if (user_memcpy(multiInfo, &Info, sizeof(multi_buffer_info)) != B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
Device::_MultiBufferForceStop()
{
	for (int i = 0; i < fStreams.Count(); i++)
		fStreams[i]->Stop();
	return B_OK;
}


status_t
Device::_MultiGetMix(multi_mix_value_info* Info)
{
	return fAudioControl.GetMix(Info);
}


status_t
Device::_MultiSetMix(multi_mix_value_info* Info)
{
	return fAudioControl.SetMix(Info);
}


status_t
Device::_MultiListMixControls(multi_mix_control_info* Info)
{
	status_t status = fAudioControl.ListMixControls(Info);
	TraceListMixControls(Info);
	return status;
}


void
Device::TraceListMixControls(multi_mix_control_info* Info)
{
	TRACE(MIX, "control_count:%d\n.", Info->control_count);

	int32 i = 0;
	while (Info->controls[i].id > 0) {
		multi_mix_control &c = Info->controls[i];
		TRACE(MIX, "id:%#08x\n",		c.id);
		TRACE(MIX, "flags:%#08x\n",	c.flags);
		TRACE(MIX, "master:%#08x\n", c.master);
		TRACE(MIX, "parent:%#08x\n", c.parent);
		TRACE(MIX, "string:%d\n",	c.string);
		TRACE(MIX, "name:%s\n",		c.name);
		i++;
	}
}


status_t
Device::_SetupEndpoints()
{
	const usb_configuration_info* config
		= gUSBModule->get_nth_configuration(fDevice, 0);

	if (config == NULL) {
		TRACE(ERR, "Error of getting USB device configuration.\n");
		return B_ERROR;
	}

	if (config->interface_count <= 0) {
		TRACE(ERR, "Error:no interfaces found in USB device configuration\n");
		return B_ERROR;
	}

	for (size_t i = 0; i < config->interface_count; i++) {
		usb_interface_info* Interface = config->interface[i].active;
		if (Interface->descr->interface_class != USB_AUDIO_INTERFACE_AUDIO_CLASS)
			continue;

		switch (Interface->descr->interface_subclass) {
			case USB_AUDIO_INTERFACE_AUDIOCONTROL_SUBCLASS:
				fAudioControl.Init(i, Interface);
				break;
			case USB_AUDIO_INTERFACE_AUDIOSTREAMING_SUBCLASS:
				{
					Stream* stream = new(std::nothrow) Stream(this, i,
						&config->interface[i]);
					if (B_OK == stream->Init()) {
						// put the stream in the correct order:
						// first output that input ones.
						if (stream->IsInput())
							fStreams.PushBack(stream);
						else
							fStreams.PushFront(stream);
					} else
						delete stream;
				}
				break;
			default:
				TRACE(ERR, "Ignore interface of unsupported subclass %#x.\n",
					Interface->descr->interface_subclass);
				break;
		}
	}

	if (fAudioControl.InitCheck() == B_OK && fStreams.Count() > 0) {
		TRACE(INF, "Found device %#06x:%#06x\n", fVendorID, fProductID);

		status_t status = gUSBModule->set_configuration(fDevice, config);
		if (status != B_OK)
			return status;

		for (int i = 0; i < fStreams.Count(); i++)
			fStreams[i]->OnSetConfiguration(fDevice, config);

		return B_OK;
	}

	return B_NO_INIT;
}


status_t
Device::StopDevice()
{
	status_t result = B_OK;

	if (result != B_OK)
		TRACE(ERR, "Error of writing %#04x RX Control:%#010x\n", 0, result);

	//TRACE_RET(result);
	return result;
}

