#include "multi_audio.h"
#include "driver.h"

multi_channel_info chans[] = {
	{  0, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
	{  1, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
	{  2, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
	{  3, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
	{  4, B_MULTI_OUTPUT_BUS, 		B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 	B_CHANNEL_MINI_JACK_STEREO },
	{  5, B_MULTI_OUTPUT_BUS, 		B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, B_CHANNEL_MINI_JACK_STEREO },
	{  6, B_MULTI_INPUT_BUS, 		B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 	B_CHANNEL_MINI_JACK_STEREO },
	{  7, B_MULTI_INPUT_BUS, 		B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, B_CHANNEL_MINI_JACK_STEREO },
};

static int32
format2size(uint32 format)
{
	switch(format) {
		case B_FMT_8BIT_S:
		case B_FMT_16BIT:
			return 2;

		case B_FMT_18BIT:
		case B_FMT_24BIT:
		case B_FMT_32BIT:
			return 4;

		case B_FMT_FLOAT:
			return 8;
			
		default:
			return -1;
	}
}

static status_t
get_description(hda_codec* codec, multi_description* data)
{
	data->interface_version = B_CURRENT_INTERFACE_VERSION;
	data->interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strcpy(data->friendly_name,"HD Audio");
	strcpy(data->vendor_info,"Haiku");

	data->output_channel_count = 2;
	data->input_channel_count = 2;
	data->output_bus_channel_count = 2;
	data->input_bus_channel_count = 2;
	data->aux_bus_channel_count = 0;

	dprintf("%s: request_channel_count: %ld\n", __func__, data->request_channel_count);

	if (data->request_channel_count >= (int)(sizeof(chans) / sizeof(chans[0]))) {
		memcpy(data->channels,&chans,sizeof(chans));
	}

	/* determine output/input rates */	
	data->output_rates = codec->afg_defrates;
	data->input_rates = codec->afg_defrates;

	/* force existance of 48kHz if variable rates are not supported */
	if (data->output_rates == 0)
		data->output_rates = B_SR_48000;
	if (data->input_rates == 0)
		data->input_rates = B_SR_48000;

	data->max_cvsr_rate = 0;
	data->min_cvsr_rate = 0;

	data->output_formats = codec->afg_deffmts;
	data->input_formats = codec->afg_deffmts;
	data->lock_sources = B_MULTI_LOCK_INTERNAL;
	data->timecode_sources = 0;
	data->interface_flags = B_MULTI_INTERFACE_PLAYBACK /* | B_MULTI_INTERFACE_RECORD */;
	data->start_latency = 30000;

	strcpy(data->control_panel,"");

	return B_OK;
}

static status_t
get_enabled_channels(hda_codec* codec, multi_channel_enable* data)
{
	B_SET_CHANNEL(data->enable_bits, 0, true);
	B_SET_CHANNEL(data->enable_bits, 1, true);
	B_SET_CHANNEL(data->enable_bits, 2, true);
	B_SET_CHANNEL(data->enable_bits, 3, true);
	data->lock_source = B_MULTI_LOCK_INTERNAL;

	return B_OK;
}

static status_t
get_global_format(hda_codec* codec, multi_format_info* data)
{
	data->output_latency = 0;
	data->input_latency = 0;
	data->timecode_kind = 0;

	data->output.format = codec->playback_stream->sampleformat;
	data->output.rate = codec->playback_stream->samplerate;

	data->input.format = codec->record_stream->sampleformat;
	data->input.rate = codec->record_stream->sampleformat;

	return B_OK;
}

static status_t
set_global_format(hda_codec* codec, multi_format_info* data)
{
	codec->playback_stream->sampleformat = data->output.format;
	codec->playback_stream->samplerate = data->output.rate;
	codec->playback_stream->sample_size = format2size(codec->playback_stream->sampleformat);

	codec->record_stream->samplerate = data->input.rate;
	codec->record_stream->sampleformat = data->input.format;
	codec->record_stream->sample_size = format2size(codec->record_stream->sampleformat);

	return B_OK;
}

static status_t
list_mix_controls(hda_codec* codec, multi_mix_control_info * data)
{
	return B_OK;
}

static status_t
list_mix_connections(hda_codec* codec, multi_mix_connection_info * data)
{
	data->actual_count = 0;
	return B_OK;
}

static status_t
list_mix_channels(hda_codec* codec, multi_mix_channel_info *data)
{
	return B_OK;
}

static status_t
get_buffers(hda_codec* codec, multi_buffer_list* data)
{
	uint32 playback_sample_size = codec->playback_stream->sample_size;
	uint32 record_sample_size = codec->record_stream->sample_size;
	uint32 cidx, bidx;
	status_t rc;

	dprintf("%s: playback: %ld buffers, %ld channels, %ld samples\n", __func__, 
		data->request_playback_buffers, data->request_playback_channels, data->request_playback_buffer_size);
	dprintf("%s: record: %ld buffers, %ld channels, %ld samples\n", __func__, 
		data->request_record_buffers, data->request_record_channels, data->request_record_buffer_size);

	if (data->request_playback_buffers > STRMAXBUF ||
		data->request_playback_buffers < STRMINBUF ||
		data->request_record_buffers > STRMAXBUF ||
		data->request_record_buffers < STRMINBUF) {
		return B_BAD_VALUE;
	}

	data->flags = 0;

	/* Copy the requested settings into the streams */
	codec->playback_stream->num_buffers = data->request_playback_buffers;
	codec->playback_stream->num_channels = data->request_playback_channels;
	codec->playback_stream->buffer_length = data->request_playback_buffer_size;
	if ((rc=hda_stream_setup_buffers(codec, codec->playback_stream, "Playback")) != B_OK) {
		dprintf("%s: Error setting up playback buffers (%s)\n", __func__, strerror(rc));
		return rc;
	}

	codec->record_stream->num_buffers = data->request_record_buffers;
	codec->record_stream->num_channels = data->request_record_channels;
	codec->record_stream->buffer_length = data->request_record_buffer_size;
	if ((rc=hda_stream_setup_buffers(codec, codec->record_stream, "Recording")) != B_OK) {
		dprintf("%s: Error setting up recording buffers (%s)\n", __func__, strerror(rc));
		return rc;
	}

	/* Setup data structure for multi_audio API... */
	data->return_playback_buffers = data->request_playback_buffers;
	data->return_playback_channels = data->request_playback_channels;
	data->return_playback_buffer_size = data->request_playback_buffer_size;		/* frames */

	for (bidx=0; bidx < data->return_playback_buffers; bidx++) {
		for (cidx=0; cidx < data->return_playback_channels; cidx++) {
			data->playback_buffers[bidx][cidx].base = codec->playback_stream->buffers[bidx] + (playback_sample_size * cidx);
			data->playback_buffers[bidx][cidx].stride = playback_sample_size * data->return_playback_channels;
		}
	}

	data->return_record_buffers = data->request_record_buffers;
	data->return_record_channels = data->request_record_channels;
	data->return_record_buffer_size = data->request_record_buffer_size;			/* frames */

	for (bidx=0; bidx < data->return_record_buffers; bidx++) {
		for (cidx=0; cidx < data->return_record_channels; cidx++) {
			data->record_buffers[bidx][cidx].base = codec->record_stream->buffers[bidx] + (record_sample_size * cidx);
			data->record_buffers[bidx][cidx].stride = record_sample_size * data->return_record_channels;
		}
	}

	return B_OK;
}

static status_t
buffer_exchange(hda_codec* codec, multi_buffer_info* data)
{
	static int debug_buffers_exchanged = 0;
	cpu_status status;
	status_t rc;

	if (!codec->playback_stream->running)
		hda_stream_start(codec->ctrlr, codec->playback_stream);

	/* do playback */
	rc=acquire_sem(codec->playback_stream->buffer_ready_sem);
	if (rc != B_OK) {
		dprintf("%s: Error waiting for playback buffer to finish (%s)!\n", __func__,
			strerror(rc));
		return rc;
	}

	status = disable_interrupts();
	acquire_spinlock(&codec->playback_stream->lock);

	data->playback_buffer_cycle = codec->playback_stream->buffer_cycle;
	data->played_real_time = codec->playback_stream->real_time;
	data->played_frames_count = codec->playback_stream->frames_count;

	release_spinlock(&codec->playback_stream->lock);
	restore_interrupts(status);

	debug_buffers_exchanged++;
	if (((debug_buffers_exchanged % 100) == 1) && (debug_buffers_exchanged < 1111)) {
		dprintf("%s: %d buffers processed\n", __func__, debug_buffers_exchanged);
	}

	return B_OK;
}

static status_t
buffer_force_stop(hda_codec* codec)
{
	hda_stream_stop(codec->ctrlr, codec->playback_stream);
	hda_stream_stop(codec->ctrlr, codec->record_stream);

	delete_sem(codec->playback_stream->buffer_ready_sem);
//	delete_sem(codec->record_stream->buffer_ready_sem);

	return B_OK;
}

status_t
multi_audio_control(void* cookie, uint32 op, void* arg, size_t len)
{
	hda_codec* codec = (hda_codec*)cookie;

	switch(op) {
		case B_MULTI_GET_DESCRIPTION:			return get_description(codec, arg);
		case B_MULTI_GET_EVENT_INFO:			return B_ERROR;
		case B_MULTI_SET_EVENT_INFO:			return B_ERROR;
		case B_MULTI_GET_EVENT:					return B_ERROR;
		case B_MULTI_GET_ENABLED_CHANNELS:		return get_enabled_channels(codec, arg);
		case B_MULTI_SET_ENABLED_CHANNELS:		return B_OK;
		case B_MULTI_GET_GLOBAL_FORMAT:			return get_global_format(codec, arg);
		case B_MULTI_SET_GLOBAL_FORMAT:			return set_global_format(codec, arg);
		case B_MULTI_GET_CHANNEL_FORMATS:		return B_ERROR;
		case B_MULTI_SET_CHANNEL_FORMATS:		return B_ERROR;
		case B_MULTI_GET_MIX:					return B_ERROR;
		case B_MULTI_SET_MIX:					return B_ERROR;
		case B_MULTI_LIST_MIX_CHANNELS:			return list_mix_channels(codec, arg);
		case B_MULTI_LIST_MIX_CONTROLS:			return list_mix_controls(codec, arg);
		case B_MULTI_LIST_MIX_CONNECTIONS:		return list_mix_connections(codec, arg);
		case B_MULTI_GET_BUFFERS:				return get_buffers(codec, arg);
		case B_MULTI_SET_BUFFERS:				return B_ERROR;
		case B_MULTI_SET_START_TIME:			return B_ERROR;
		case B_MULTI_BUFFER_EXCHANGE:			return buffer_exchange(codec, arg);
		case B_MULTI_BUFFER_FORCE_STOP:			return buffer_force_stop(codec);
	}

	return B_BAD_VALUE;
}
