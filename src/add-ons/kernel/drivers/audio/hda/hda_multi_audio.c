/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 */


#include "multi_audio.h"
#include "driver.h"


#ifdef TRACE
#	undef TRACE
#endif

#ifdef TRACE_MULTI_AUDIO
#	define TRACE(a...) dprintf("\33[34mhda:\33[0m " a)
#else
#	define TRACE(a...) ;
#endif


static multi_channel_info sChannels[] = {
	{  0, B_MULTI_OUTPUT_CHANNEL,	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
	{  1, B_MULTI_OUTPUT_CHANNEL,	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
	{  2, B_MULTI_INPUT_CHANNEL,	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
	{  3, B_MULTI_INPUT_CHANNEL,	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
	{  4, B_MULTI_OUTPUT_BUS,		B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS,
			B_CHANNEL_MINI_JACK_STEREO },
	{  5, B_MULTI_OUTPUT_BUS,		B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS,
			B_CHANNEL_MINI_JACK_STEREO },
	{  6, B_MULTI_INPUT_BUS,		B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS,
			B_CHANNEL_MINI_JACK_STEREO },
	{  7, B_MULTI_INPUT_BUS,		B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS,
			B_CHANNEL_MINI_JACK_STEREO },
};


static int32
format2size(uint32 format)
{
	switch (format) {
		case B_FMT_8BIT_S:
		case B_FMT_16BIT:
			return 2;

		case B_FMT_18BIT:
		case B_FMT_24BIT:
		case B_FMT_32BIT:
		case B_FMT_FLOAT:
			return 4;

		default:
			return -1;
	}
}


static status_t
get_description(hda_audio_group* audioGroup, multi_description* data)
{
	data->interface_version = B_CURRENT_INTERFACE_VERSION;
	data->interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strcpy(data->friendly_name, "HD Audio");
	strcpy(data->vendor_info, "Haiku");

	data->output_channel_count = 2;
	data->input_channel_count = 2;
	data->output_bus_channel_count = 2;
	data->input_bus_channel_count = 2;
	data->aux_bus_channel_count = 0;

	dprintf("%s: request_channel_count: %ld\n", __func__,
		data->request_channel_count);

	if (data->request_channel_count >= (int)(sizeof(sChannels)
			/ sizeof(sChannels[0]))) {
		memcpy(data->channels, &sChannels, sizeof(sChannels));
	}

	/* determine output/input rates */	
	data->output_rates = audioGroup->supported_rates;
	data->input_rates = audioGroup->supported_rates;

	/* force existance of 48kHz if variable rates are not supported */
	if (data->output_rates == 0)
		data->output_rates = B_SR_48000;
	if (data->input_rates == 0)
		data->input_rates = B_SR_48000;

	data->max_cvsr_rate = 0;
	data->min_cvsr_rate = 0;

	data->output_formats = audioGroup->supported_formats;
	data->input_formats = audioGroup->supported_formats;
	data->lock_sources = B_MULTI_LOCK_INTERNAL;
	data->timecode_sources = 0;
	data->interface_flags = B_MULTI_INTERFACE_PLAYBACK /* | B_MULTI_INTERFACE_RECORD */;
	data->start_latency = 30000;

	strcpy(data->control_panel, "");

	return B_OK;
}


static status_t
get_enabled_channels(hda_audio_group* audioGroup, multi_channel_enable* data)
{
	B_SET_CHANNEL(data->enable_bits, 0, true);
	B_SET_CHANNEL(data->enable_bits, 1, true);
	B_SET_CHANNEL(data->enable_bits, 2, true);
	B_SET_CHANNEL(data->enable_bits, 3, true);
	data->lock_source = B_MULTI_LOCK_INTERNAL;

	return B_OK;
}


static status_t
get_global_format(hda_audio_group* audioGroup, multi_format_info* data)
{
	data->output_latency = 0;
	data->input_latency = 0;
	data->timecode_kind = 0;

	data->output.format = audioGroup->playback_stream->sample_format;
	data->output.rate = audioGroup->playback_stream->sample_rate;

	data->input.format = audioGroup->record_stream->sample_format;
	data->input.rate = audioGroup->record_stream->sample_format;

	return B_OK;
}


static status_t
set_global_format(hda_audio_group* audioGroup, multi_format_info* data)
{
	// TODO: it looks like we're not supposed to fail; fix this!
#if 0
	if ((data->output.format & audioGroup->supported_formats) == 0)
		|| (data->output.rate & audioGroup->supported_rates) == 0)
		return B_BAD_VALUE;
#endif

	audioGroup->playback_stream->sample_format = data->output.format;
	audioGroup->playback_stream->sample_rate = data->output.rate;
	audioGroup->playback_stream->sample_size = format2size(
		audioGroup->playback_stream->sample_format);

	audioGroup->record_stream->sample_rate = data->input.rate;
	audioGroup->record_stream->sample_format = data->input.format;
	audioGroup->record_stream->sample_size = format2size(
		audioGroup->record_stream->sample_format);

	return B_OK;
}


static status_t
list_mix_controls(hda_audio_group* audioGroup, multi_mix_control_info* data)
{
	data->control_count = 0;
	return B_OK;
}


static status_t
list_mix_connections(hda_audio_group* audioGroup,
	multi_mix_connection_info* data)
{
	data->actual_count = 0;
	return B_OK;
}


static status_t
list_mix_channels(hda_audio_group* audioGroup, multi_mix_channel_info *data)
{
	return B_OK;
}


static status_t
get_buffers(hda_audio_group* audioGroup, multi_buffer_list* data)
{
	uint32 playbackSampleSize = audioGroup->playback_stream->sample_size;
	uint32 recordSampleSize = audioGroup->record_stream->sample_size;
	uint32 cidx, bidx;
	status_t status;

	TRACE("playback: %ld buffers, %ld channels, %ld samples\n",
		data->request_playback_buffers, data->request_playback_channels,
		data->request_playback_buffer_size);
	TRACE("record: %ld buffers, %ld channels, %ld samples\n",
		data->request_record_buffers, data->request_record_channels,
		data->request_record_buffer_size);

	/* Determine what buffers we return given the request */

	data->return_playback_buffers = data->request_playback_buffers;
	data->return_playback_channels = data->request_playback_channels;
	data->return_playback_buffer_size = data->request_playback_buffer_size;	
	data->return_record_buffers = data->request_record_buffers;
	data->return_record_channels = data->request_record_channels;
	data->return_record_buffer_size = data->request_record_buffer_size;

	/* Workaround for Haiku multi_audio API, since it prefers to let the
	   driver pick values, while the BeOS multi_audio actually gives the
	   user's defaults. */
	if (data->return_playback_buffers > STREAM_MAX_BUFFERS
		|| data->return_playback_buffers < STREAM_MIN_BUFFERS)
		data->return_playback_buffers = STREAM_MIN_BUFFERS;

	if (data->return_record_buffers > STREAM_MAX_BUFFERS
		|| data->return_record_buffers < STREAM_MIN_BUFFERS)
		data->return_record_buffers = STREAM_MIN_BUFFERS;

	if (data->return_playback_buffer_size == 0)
		data->return_playback_buffer_size = DEFAULT_FRAMES_PER_BUFFER;

	if (data->return_record_buffer_size == 0)
		data->return_record_buffer_size = DEFAULT_FRAMES_PER_BUFFER;

	/* ... from here on, we can assume again that a reasonable request is
	   being made */

	data->flags = B_MULTI_BUFFER_PLAYBACK;

	/* Copy the settings into the streams */
	audioGroup->playback_stream->num_buffers = data->return_playback_buffers;
	audioGroup->playback_stream->num_channels = data->return_playback_channels;
	audioGroup->playback_stream->buffer_length
		= data->return_playback_buffer_size;

	status = hda_stream_setup_buffers(audioGroup, audioGroup->playback_stream,
		"Playback");
	if (status != B_OK) {
		dprintf("hda: Error setting up playback buffers: %s\n",
			strerror(status));
		return status;
	}

	audioGroup->record_stream->num_buffers = data->return_record_buffers;
	audioGroup->record_stream->num_channels = data->return_record_channels;
	audioGroup->record_stream->buffer_length
		= data->return_record_buffer_size;

	status = hda_stream_setup_buffers(audioGroup, audioGroup->record_stream,
		"Recording");
	if (status != B_OK) {
		dprintf("hda: Error setting up recording buffers: %s\n",
			strerror(status));
		return status;
	}

	/* Setup data structure for multi_audio API... */

	for (bidx = 0; bidx < data->return_playback_buffers; bidx++) {
		for (cidx = 0; cidx < data->return_playback_channels; cidx++) {
			data->playback_buffers[bidx][cidx].base
				= audioGroup->playback_stream->buffers[bidx]
					+ playbackSampleSize * cidx;
			data->playback_buffers[bidx][cidx].stride
				= playbackSampleSize * data->return_playback_channels;
		}
	}

	for (bidx = 0; bidx < data->return_record_buffers; bidx++) {
		for (cidx = 0; cidx < data->return_record_channels; cidx++) {
			data->record_buffers[bidx][cidx].base
				= audioGroup->record_stream->buffers[bidx]
					+ recordSampleSize * cidx;
			data->record_buffers[bidx][cidx].stride
				= recordSampleSize * data->return_record_channels;
		}
	}

	return B_OK;
}


/*! playback_buffer_cycle is the buffer we want to have played */
static status_t
buffer_exchange(hda_audio_group* audioGroup, multi_buffer_info* data)
{
	static int debug_buffers_exchanged = 0;
	cpu_status status;
	status_t rc;

	if (!audioGroup->playback_stream->running) {
		hda_stream_start(audioGroup->codec->controller,
			audioGroup->playback_stream);
	}

	/* do playback */
	rc = acquire_sem_etc(audioGroup->playback_stream->buffer_ready_sem,
		1, B_CAN_INTERRUPT, 0);
	if (rc != B_OK) {
		dprintf("%s: Error waiting for playback buffer to finish (%s)!\n", __func__,
			strerror(rc));
		return rc;
	}

	status = disable_interrupts();
	acquire_spinlock(&audioGroup->playback_stream->lock);

	data->playback_buffer_cycle = audioGroup->playback_stream->buffer_cycle;
	data->played_real_time = audioGroup->playback_stream->real_time;
	data->played_frames_count = audioGroup->playback_stream->frames_count;

	release_spinlock(&audioGroup->playback_stream->lock);
	restore_interrupts(status);

	debug_buffers_exchanged++;
	if (((debug_buffers_exchanged % 100) == 1) && (debug_buffers_exchanged < 1111)) {
		dprintf("%s: %d buffers processed\n", __func__, debug_buffers_exchanged);
	}

	return B_OK;
}


static status_t
buffer_force_stop(hda_audio_group* audioGroup)
{
	hda_stream_stop(audioGroup->codec->controller, audioGroup->playback_stream);
	//hda_stream_stop(audioGroup->codec->controller, audioGroup->record_stream);

	return B_OK;
}


status_t
multi_audio_control(void* cookie, uint32 op, void* arg, size_t len)
{
	hda_codec* codec = (hda_codec*)cookie;
	hda_audio_group* audioGroup;

	/* FIXME: We should simply pass the audioGroup into here... */
	if (!codec || codec->num_audio_groups == 0)
		return ENODEV;

	audioGroup = codec->audio_groups[0];

	switch (op) {
		case B_MULTI_GET_DESCRIPTION:
			return get_description(audioGroup, arg);

		case B_MULTI_GET_ENABLED_CHANNELS:
			return get_enabled_channels(audioGroup, arg);
		case B_MULTI_SET_ENABLED_CHANNELS:
			return B_OK;

		case B_MULTI_GET_GLOBAL_FORMAT:
			return get_global_format(audioGroup, arg);
		case B_MULTI_SET_GLOBAL_FORMAT:
			return set_global_format(audioGroup, arg);

		case B_MULTI_LIST_MIX_CHANNELS:
			return list_mix_channels(audioGroup, arg);
		case B_MULTI_LIST_MIX_CONTROLS:
			return list_mix_controls(audioGroup, arg);
		case B_MULTI_LIST_MIX_CONNECTIONS:
			return list_mix_connections(audioGroup, arg);
		case B_MULTI_GET_BUFFERS:
			return get_buffers(audioGroup, arg);

		case B_MULTI_BUFFER_EXCHANGE:
			return buffer_exchange(audioGroup, arg);
		case B_MULTI_BUFFER_FORCE_STOP:
			return buffer_force_stop(audioGroup);

		case B_MULTI_GET_EVENT_INFO:
		case B_MULTI_SET_EVENT_INFO:
		case B_MULTI_GET_EVENT:
		case B_MULTI_GET_CHANNEL_FORMATS:
		case B_MULTI_SET_CHANNEL_FORMATS:
		case B_MULTI_GET_MIX:
		case B_MULTI_SET_MIX:
		case B_MULTI_SET_BUFFERS:
		case B_MULTI_SET_START_TIME:
			return B_ERROR;
	}

	return B_BAD_VALUE;
}
