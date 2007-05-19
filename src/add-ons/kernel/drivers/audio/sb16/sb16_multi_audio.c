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

		default:
			return -1;
	}
}

static status_t
get_description(sb16_dev_t* dev, multi_description* data)
{
	data->interface_version = B_CURRENT_INTERFACE_VERSION;
	data->interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strcpy(data->friendly_name,"SoundBlaster 16");
	strcpy(data->vendor_info,"Haiku");

	data->output_channel_count = 2;
	data->input_channel_count = 2;
	data->output_bus_channel_count = 2;
	data->input_bus_channel_count = 2;
	data->aux_bus_channel_count = 0;

	if (data->request_channel_count >= (int)(sizeof(chans) / sizeof(chans[0]))) {
		memcpy(data->channels,&chans,sizeof(chans));
	}

	/* determine output/input rates */	
	data->output_rates =
	data->input_rates = B_SR_44100 | B_SR_22050 | B_SR_11025;

	data->max_cvsr_rate = 0;
	data->min_cvsr_rate = 0;

	data->output_formats = 
	data->input_formats = B_FMT_8BIT_S | B_FMT_16BIT;
	data->lock_sources = B_MULTI_LOCK_INTERNAL;
	data->timecode_sources = 0;
	data->interface_flags = B_MULTI_INTERFACE_PLAYBACK | B_MULTI_INTERFACE_RECORD;
	data->start_latency = 30000;

	strcpy(data->control_panel,"");

	return B_OK;
}

static status_t
get_enabled_channels(sb16_dev_t* dev, multi_channel_enable* data)
{
	B_SET_CHANNEL(data->enable_bits, 0, true);
	B_SET_CHANNEL(data->enable_bits, 1, true);
	B_SET_CHANNEL(data->enable_bits, 2, true);
	B_SET_CHANNEL(data->enable_bits, 3, true);
	data->lock_source = B_MULTI_LOCK_INTERNAL;

	return B_OK;
}

static status_t
get_global_format(sb16_dev_t* dev, multi_format_info* data)
{
	data->output_latency = 0;
	data->input_latency = 0;
	data->timecode_kind = 0;

	data->output.format = dev->playback_stream.sampleformat;
	data->output.rate = dev->playback_stream.samplerate;

	data->input.format = dev->record_stream.sampleformat;
	data->input.rate = dev->record_stream.samplerate;

	return B_OK;
}

static status_t
set_global_format(sb16_dev_t* dev, multi_format_info* data)
{
	dev->playback_stream.sampleformat = data->output.format;
	dev->playback_stream.samplerate = data->output.rate;
	dev->playback_stream.sample_size = format2size(dev->playback_stream.sampleformat);

	dev->record_stream.sampleformat = data->input.format;
	dev->record_stream.samplerate = data->input.rate;
	dev->record_stream.sample_size = format2size(dev->record_stream.sampleformat);

	return B_OK;
}

static int32
create_group_control(multi_mix_control* multi, int32 idx, int32 parent, int32 string, const char* name) 
{
        multi->id = SB16_MULTI_CONTROL_FIRSTID + idx;
        multi->parent = parent;
        multi->flags = B_MULTI_MIX_GROUP;
        multi->master = SB16_MULTI_CONTROL_MASTERID;
        multi->string = string;
        if(name)
                strcpy(multi->name, name);
 
       return multi->id;
}

static status_t
list_mix_controls(sb16_dev_t* dev, multi_mix_control_info * data)
{
	int32 parent;

	parent = create_group_control(data->controls +0, 0, 0, 0, "Record");
        parent = create_group_control(data->controls +1, 1, 0, 0, "AC97 Mixer");
        parent = create_group_control(data->controls +2, 2, 0, S_SETUP, NULL);
        data->control_count = 3;

	return B_OK;
}

static status_t
list_mix_connections(sb16_dev_t* dev, multi_mix_connection_info * data)
{
	return B_ERROR;
}

static status_t
list_mix_channels(sb16_dev_t* dev, multi_mix_channel_info *data)
{
	return B_ERROR;
}

static status_t
get_buffers(sb16_dev_t* dev, multi_buffer_list* data)
{
	uint32 playback_sample_size = dev->playback_stream.sample_size;
	uint32 record_sample_size = dev->record_stream.sample_size;
	uint32 cidx, bidx;
	status_t rc;

	dprintf("%s: playback: %ld buffers, %ld channels, %ld samples\n", __func__, 
		data->request_playback_buffers, data->request_playback_channels, data->request_playback_buffer_size);
	dprintf("%s: record: %ld buffers, %ld channels, %ld samples\n", __func__, 
		data->request_record_buffers, data->request_record_channels, data->request_record_buffer_size);

	/* Workaround for Haiku multi_audio API, since it prefers to let the driver pick
		values, while the BeOS multi_audio actually gives the user's defaults. */
	if (data->request_playback_buffers > STRMAXBUF ||
		data->request_playback_buffers < STRMINBUF) {
		data->request_playback_buffers = STRMINBUF;
	}
	
	if (data->request_record_buffers > STRMAXBUF ||
		data->request_record_buffers < STRMINBUF) {
		data->request_record_buffers = STRMINBUF;
	}

	if (data->request_playback_buffer_size == 0)
		data->request_playback_buffer_size  = DEFAULT_FRAMESPERBUF;

	if (data->request_record_buffer_size == 0)
		data->request_record_buffer_size  = DEFAULT_FRAMESPERBUF;

	/* ... from here on, we can assume again that a reasonable request is being made */

	data->flags = 0;

	/* Copy the requested settings into the streams */
	dev->playback_stream.num_buffers = data->request_playback_buffers;
	dev->playback_stream.num_channels = data->request_playback_channels;
	dev->playback_stream.buffer_length = data->request_playback_buffer_size;
	if ((rc=sb16_stream_setup_buffers(dev, &dev->playback_stream, "Playback")) != B_OK) {
		dprintf("%s: Error setting up playback buffers (%s)\n", __func__, strerror(rc));
		return rc;
	}

	dev->record_stream.num_buffers = data->request_record_buffers;
	dev->record_stream.num_channels = data->request_record_channels;
	dev->record_stream.buffer_length = data->request_record_buffer_size;
	if ((rc=sb16_stream_setup_buffers(dev, &dev->record_stream, "Recording")) != B_OK) {
		dprintf("%s: Error setting up recording buffers (%s)\n", __func__, strerror(rc));
		return rc;
	}

	/* Setup data structure for multi_audio API... */
	data->return_playback_buffers = data->request_playback_buffers;
	data->return_playback_channels = data->request_playback_channels;
	data->return_playback_buffer_size = data->request_playback_buffer_size;		/* frames */

	for (bidx=0; bidx < data->return_playback_buffers; bidx++) {
		for (cidx=0; cidx < data->return_playback_channels; cidx++) {
			data->playback_buffers[bidx][cidx].base = dev->playback_stream.buffers[bidx] + (playback_sample_size * cidx);
			data->playback_buffers[bidx][cidx].stride = playback_sample_size * data->return_playback_channels;
		}
	}

	data->return_record_buffers = data->request_record_buffers;
	data->return_record_channels = data->request_record_channels;
	data->return_record_buffer_size = data->request_record_buffer_size;			/* frames */

	for (bidx=0; bidx < data->return_record_buffers; bidx++) {
		for (cidx=0; cidx < data->return_record_channels; cidx++) {
			data->record_buffers[bidx][cidx].base = dev->record_stream.buffers[bidx] + (record_sample_size * cidx);
			data->record_buffers[bidx][cidx].stride = record_sample_size * data->return_record_channels;
		}
	}

	return B_OK;
}

static status_t
buffer_exchange(sb16_dev_t* dev, multi_buffer_info* data)
{
	static int debug_buffers_exchanged = 0;
	cpu_status status;
	status_t rc;

	if (!dev->playback_stream.running)
		sb16_stream_start(dev, &dev->playback_stream);

	/* do playback */
	rc=acquire_sem(dev->playback_stream.buffer_ready_sem);
	if (rc != B_OK) {
		dprintf("%s: Error waiting for playback buffer to finish (%s)!\n", __func__,
			strerror(rc));
		return rc;
	}

	status = disable_interrupts();
	acquire_spinlock(&dev->playback_stream.lock);

	data->playback_buffer_cycle = dev->playback_stream.buffer_cycle;
	data->played_real_time = dev->playback_stream.real_time;
	data->played_frames_count = dev->playback_stream.frames_count;

	release_spinlock(&dev->playback_stream.lock);
	restore_interrupts(status);

	debug_buffers_exchanged++;
	if (((debug_buffers_exchanged % 100) == 1) && (debug_buffers_exchanged < 1111)) {
		dprintf("%s: %d buffers processed\n", __func__, debug_buffers_exchanged);
	}

	return B_OK;
}

static status_t
buffer_force_stop(sb16_dev_t* dev)
{
	sb16_stream_stop(dev, &dev->playback_stream);
	sb16_stream_stop(dev, &dev->record_stream);

	delete_sem(dev->playback_stream.buffer_ready_sem);
	delete_sem(dev->record_stream.buffer_ready_sem);

	return B_OK;
}

status_t
multi_audio_control(void* cookie, uint32 op, void* arg, size_t len)
{
	switch(op) {
		case B_MULTI_GET_DESCRIPTION:			return get_description(cookie, arg);
		case B_MULTI_GET_EVENT_INFO:			return B_ERROR;
		case B_MULTI_SET_EVENT_INFO:			return B_ERROR;
		case B_MULTI_GET_EVENT:					return B_ERROR;
		case B_MULTI_GET_ENABLED_CHANNELS:		return get_enabled_channels(cookie, arg);
		case B_MULTI_SET_ENABLED_CHANNELS:		return B_OK;
		case B_MULTI_GET_GLOBAL_FORMAT:			return get_global_format(cookie, arg);
		case B_MULTI_SET_GLOBAL_FORMAT:			return set_global_format(cookie, arg);
		case B_MULTI_GET_CHANNEL_FORMATS:		return B_ERROR;
		case B_MULTI_SET_CHANNEL_FORMATS:		return B_ERROR;
		case B_MULTI_GET_MIX:					return B_ERROR;
		case B_MULTI_SET_MIX:					return B_ERROR;
		case B_MULTI_LIST_MIX_CHANNELS:			return list_mix_channels(cookie, arg);
		case B_MULTI_LIST_MIX_CONTROLS:			return list_mix_controls(cookie, arg);
		case B_MULTI_LIST_MIX_CONNECTIONS:		return list_mix_connections(cookie, arg);
		case B_MULTI_GET_BUFFERS:				return get_buffers(cookie, arg);
		case B_MULTI_SET_BUFFERS:				return B_ERROR;
		case B_MULTI_SET_START_TIME:			return B_ERROR;
		case B_MULTI_BUFFER_EXCHANGE:			return buffer_exchange(cookie, arg);
		case B_MULTI_BUFFER_FORCE_STOP:			return buffer_force_stop(cookie);
	}

	return B_BAD_VALUE;
}
