/*
 * Copyright 2007-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Bek, host.haiku@gmx.de
 */
#include "driver.h"


// Convenience function to determine the byte count
// of a sample for a given format.
// Note: Currently null_audio only supports 16 bit,
// but that is supposed to change later
int32
format_to_sample_size(uint32 format)
{
	switch(format) {
		case B_FMT_8BIT_S:
			return 1;
		case B_FMT_16BIT:
			return 2;

		case B_FMT_18BIT:
		case B_FMT_24BIT:
		case B_FMT_32BIT:
		case B_FMT_FLOAT:
			return 4;

		default:
			return 0;
	}
}


multi_channel_info channel_descriptions[] = {
	{  0, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
	{  1, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
	{  2, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
	{  3, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
	{  4, B_MULTI_OUTPUT_BUS, 		B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 	B_CHANNEL_MINI_JACK_STEREO },
	{  5, B_MULTI_OUTPUT_BUS, 		B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, B_CHANNEL_MINI_JACK_STEREO },
	{  6, B_MULTI_INPUT_BUS, 		B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 	B_CHANNEL_MINI_JACK_STEREO },
	{  7, B_MULTI_INPUT_BUS, 		B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, B_CHANNEL_MINI_JACK_STEREO },
};


static status_t
get_description(void* cookie, multi_description* data)
{
	multi_description description;

	dprintf("null_audio: %s\n" , __func__ );

	if (user_memcpy(&description, data, sizeof(multi_description)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	description.interface_version = B_CURRENT_INTERFACE_VERSION;
	description.interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strcpy(description.friendly_name,"Virtual audio (null_audio)");
	strcpy(description.vendor_info,"Host/Haiku");

	description.output_channel_count = 2;
	description.input_channel_count = 2;
	description.output_bus_channel_count = 2;
	description.input_bus_channel_count = 2;
	description.aux_bus_channel_count = 0;

	description.output_rates = B_SR_44100;
	description.input_rates = B_SR_44100;

	description.max_cvsr_rate = 0;
	description.min_cvsr_rate = 0;

	description.output_formats = B_FMT_16BIT;
	description.input_formats = B_FMT_16BIT;
	description.lock_sources = B_MULTI_LOCK_INTERNAL;
	description.timecode_sources = 0;
	description.interface_flags = B_MULTI_INTERFACE_PLAYBACK | B_MULTI_INTERFACE_RECORD;
	description.start_latency = 30000;

	strcpy(description.control_panel,"");

	if (user_memcpy(data, &description, sizeof(multi_description)) != B_OK)
		return B_BAD_ADDRESS;

	if (description.request_channel_count
			>= sizeof(channel_descriptions) / sizeof(channel_descriptions[0])) {
		if (user_memcpy(data->channels,
					&channel_descriptions, sizeof(channel_descriptions)) != B_OK)
			return B_BAD_ADDRESS;
	}

	return B_OK;
}


static status_t
get_enabled_channels(void* cookie, multi_channel_enable* data)
{
	dprintf("null_audio: %s\n" , __func__ );
	// By default we say, that all channels are enabled
	// and that this cannot be changed
	B_SET_CHANNEL(data->enable_bits, 0, true);
	B_SET_CHANNEL(data->enable_bits, 1, true);
	B_SET_CHANNEL(data->enable_bits, 2, true);
	B_SET_CHANNEL(data->enable_bits, 3, true);
	return B_OK;
}


static status_t
set_global_format(device_t* device, multi_format_info* data)
{
	// The media kit asks us to set our streams
	// according to its settings
	dprintf("null_audio: %s\n" , __func__ );
	device->playback_stream.format = data->output.format;
	device->playback_stream.rate = data->output.rate;

	device->record_stream.format = data->input.format;
	device->record_stream.rate = data->input.rate;

	return B_OK;
}


static status_t
get_global_format(device_t* device, multi_format_info* data)
{
	dprintf("null_audio: %s\n" , __func__ );
	// Zero latency is unlikely to happen, so we fake some
	// additional latency
	data->output_latency = 30;
	data->input_latency = 30;
	data->timecode_kind = 0;

	data->output.format = device->playback_stream.format;
	data->output.rate = device->playback_stream.rate;
	data->input.format = device->record_stream.format;
	data->input.rate = device->record_stream.rate;

	return B_OK;
}


static int32
create_group_control(multi_mix_control* multi, int32 idx, int32 parent,
					int32 string, const char* name)
{
	multi->id = MULTI_AUDIO_BASE_ID + idx;
	multi->parent = parent;
	multi->flags = B_MULTI_MIX_GROUP;
	multi->master = MULTI_AUDIO_MASTER_ID;
	multi->string = string;
	if (name)
		strcpy(multi->name, name);

	return multi->id;
}


static status_t
list_mix_controls(device_t* device, multi_mix_control_info * data)
{
	int32 parent;
	dprintf("null_audio: %s\n" , __func__ );

	parent = create_group_control(data->controls +0, 0, 0, 0, "Record");
	parent = create_group_control(data->controls +1, 1, 0, 0, "Playback");
	data->control_count = 2;

	return B_OK;
}


static status_t
list_mix_connections(void* cookie, multi_mix_connection_info* connection_info)
{
	dprintf("null_audio: %s\n" , __func__ );
	return B_ERROR;
}


static status_t
list_mix_channels(void* cookie, multi_mix_channel_info* channel_info)
{
	dprintf("null_audio: %s\n" , __func__ );
	return B_ERROR;
}


static status_t
get_buffers(device_t* device, multi_buffer_list* data)
{
 	uint32 playback_sample_size
	 	= format_to_sample_size(device->playback_stream.format);
 	uint32 record_sample_size
		= format_to_sample_size(device->record_stream.format);
 	uint32 cidx, bidx;
 	status_t result;

	dprintf("null_audio: %s\n" , __func__ );

	// Workaround for Haiku multi_audio API, since it prefers
	// to let the driver pick values, while the BeOS multi_audio
	// actually gives the user's defaults.
	if (data->request_playback_buffers > STRMAXBUF
		|| data->request_playback_buffers < STRMINBUF) {
		data->request_playback_buffers = STRMINBUF;
	}

	if (data->request_record_buffers > STRMAXBUF
		|| data->request_record_buffers < STRMINBUF) {
		data->request_record_buffers = STRMINBUF;
	}

	if (data->request_playback_buffer_size == 0)
		data->request_playback_buffer_size = FRAMES_PER_BUFFER;

	if (data->request_record_buffer_size == 0)
		data->request_record_buffer_size = FRAMES_PER_BUFFER;

	// ... from here on, we can assume again that
	// a reasonable request is being made

	data->flags = 0;

	// Copy the requested settings into the streams
	// and initialize the virtual buffers properly
	device->playback_stream.num_buffers = data->request_playback_buffers;
	device->playback_stream.num_channels = data->request_playback_channels;
	device->playback_stream.buffer_length = data->request_playback_buffer_size;
	result = null_hw_create_virtual_buffers(&device->playback_stream,
				"null_audio_playback_sem");
	if (result != B_OK) {
		dprintf("null_audio %s: Error setting up playback buffers (%s)\n",
													__func__, strerror(result));
		return result;
	}

	device->record_stream.num_buffers = data->request_record_buffers;
	device->record_stream.num_channels = data->request_record_channels;
	device->record_stream.buffer_length = data->request_record_buffer_size;
	result = null_hw_create_virtual_buffers(&device->record_stream,
				"null_audio_record_sem");
	if (result != B_OK) {
		dprintf("null_audio %s: Error setting up recording buffers (%s)\n",
													__func__, strerror(result));
		return result;
	}

	/* Setup data structure for multi_audio API... */
	data->return_playback_buffers = data->request_playback_buffers;
	data->return_playback_channels = data->request_playback_channels;
	data->return_playback_buffer_size = data->request_playback_buffer_size;

	for (bidx = 0; bidx < data->return_playback_buffers; bidx++) {
		for (cidx = 0; cidx < data->return_playback_channels; cidx++) {
			data->playback_buffers[bidx][cidx].base
				= device->playback_stream.buffers[bidx] + (playback_sample_size * cidx);
			data->playback_buffers[bidx][cidx].stride
				= playback_sample_size * data->return_playback_channels;
		}
	}

	data->return_record_buffers = data->request_record_buffers;
	data->return_record_channels = data->request_record_channels;
	data->return_record_buffer_size = data->request_record_buffer_size;

	for (bidx = 0; bidx < data->return_record_buffers; bidx++) {
		for (cidx = 0; cidx < data->return_record_channels; cidx++) {
			data->record_buffers[bidx][cidx].base
				= device->record_stream.buffers[bidx] + (record_sample_size * cidx);
			data->record_buffers[bidx][cidx].stride
				= record_sample_size * data->return_record_channels;
		}
	}

	return B_OK;
}


static status_t
buffer_exchange(device_t* device, multi_buffer_info* info)
{
	//dprintf("null_audio: %s\n" , __func__ );
	static int debug_buffers_exchanged = 0;
	cpu_status status;
	status_t result;

	multi_buffer_info buffer_info;
	if (user_memcpy(&buffer_info, info, sizeof(multi_buffer_info)) != B_OK)
		return B_BAD_ADDRESS;

	// On first call, we start our fake hardware.
	// Usually one would jump into his interrupt handler now
	if (!device->running)
		null_start_hardware(device);

	result = acquire_sem(device->playback_stream.buffer_ready_sem);
	if (result != B_OK) {
		dprintf("null_audio: %s, Could not get playback buffer\n", __func__);
		return result;
	}

	result = acquire_sem(device->record_stream.buffer_ready_sem);
	if (result != B_OK) {
		dprintf("null_audio: %s, Could not get record buffer\n", __func__);
		return result;
	}

	status = disable_interrupts();
	acquire_spinlock(&device->playback_stream.lock);

	buffer_info.playback_buffer_cycle = device->playback_stream.buffer_cycle;
	buffer_info.played_real_time = device->playback_stream.real_time;
	buffer_info.played_frames_count = device->playback_stream.frames_count;

	buffer_info.record_buffer_cycle = device->record_stream.buffer_cycle;
	buffer_info.recorded_real_time = device->record_stream.real_time;
	buffer_info.recorded_frames_count = device->record_stream.frames_count;

	release_spinlock(&device->playback_stream.lock);
	restore_interrupts(status);

	debug_buffers_exchanged++;
	if (((debug_buffers_exchanged % 5000) == 0) ) {
		dprintf("null_audio: %s: %d buffers processed\n",
				__func__, debug_buffers_exchanged);
	}

	if (user_memcpy(info, &buffer_info, sizeof(multi_buffer_info)) != B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


static status_t
buffer_force_stop(device_t* device)
{
	dprintf("null_audio: %s\n" , __func__ );

	if (device == NULL)
		return B_ERROR;

	if (device->running)
		null_stop_hardware(device);

	delete_area(device->playback_stream.buffer_area);
	delete_area(device->record_stream.buffer_area);

	delete_sem(device->playback_stream.buffer_ready_sem);
	delete_sem(device->record_stream.buffer_ready_sem);

	return B_OK;
}


status_t
multi_audio_control(void* cookie, uint32 op, void* arg, size_t len)
{
	switch(op) {
		case B_MULTI_GET_DESCRIPTION:		return get_description(cookie, arg);
		case B_MULTI_GET_EVENT_INFO:		return B_ERROR;
		case B_MULTI_SET_EVENT_INFO:		return B_ERROR;
		case B_MULTI_GET_EVENT:				return B_ERROR;
		case B_MULTI_GET_ENABLED_CHANNELS:	return get_enabled_channels(cookie, arg);
		case B_MULTI_SET_ENABLED_CHANNELS:	return B_OK;
		case B_MULTI_GET_GLOBAL_FORMAT:		return get_global_format(cookie, arg);
		case B_MULTI_SET_GLOBAL_FORMAT:		return set_global_format(cookie, arg);
		case B_MULTI_GET_CHANNEL_FORMATS:	return B_ERROR;
		case B_MULTI_SET_CHANNEL_FORMATS:	return B_ERROR;
		case B_MULTI_GET_MIX:				return B_ERROR;
		case B_MULTI_SET_MIX:				return B_ERROR;
		case B_MULTI_LIST_MIX_CHANNELS:		return list_mix_channels(cookie, arg);
		case B_MULTI_LIST_MIX_CONTROLS:		return list_mix_controls(cookie, arg);
		case B_MULTI_LIST_MIX_CONNECTIONS:	return list_mix_connections(cookie, arg);
		case B_MULTI_GET_BUFFERS:			return get_buffers(cookie, arg);
		case B_MULTI_SET_BUFFERS:			return B_ERROR;
		case B_MULTI_SET_START_TIME:		return B_ERROR;
		case B_MULTI_BUFFER_EXCHANGE:		return buffer_exchange(cookie, arg);
		case B_MULTI_BUFFER_FORCE_STOP:		return buffer_force_stop(cookie);
	}

	dprintf("null_audio: %s - unknown op\n" , __func__);
	return B_BAD_VALUE;
}

