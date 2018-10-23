/*
 * BeOS Driver for Intel ICH AC'97 Link interface
 *
 * Copyright (c) 2002, Marcus Overhagen <marcus@overhagen.de>
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <OS.h>
#include <MediaDefs.h>
#include "hmulti_audio.h"
#include "ac97_multi.h"

//#define DEBUG 1

#include "debug.h"
#include "ich.h"
#include "util.h"
#include "config.h"

/*
 *
 * XXX!!! ALL "MIX" ioctls are not implemented by the BeOS R5, get never called :-(
 *        The concept described by B_MULTI_SET_BUFFERS is impossible to implement
 *        B_MULTI_GET_BUFFERS can not be handled as efficient as envisioned by the API creators
 *
 */

static status_t list_mix_controls(multi_mix_control_info *data);
static status_t list_mix_connections(multi_mix_connection_info *data);
static status_t list_mix_channels(multi_mix_channel_info *data);
static status_t get_description(multi_description *data);
static status_t get_enabled_channels(multi_channel_enable *data);
static status_t get_global_format(multi_format_info *data);
static status_t get_buffers(multi_buffer_list *data);
static status_t buffer_exchange(multi_buffer_info *data);
static status_t buffer_force_stop();

static status_t list_mix_controls(multi_mix_control_info * data)
{
	return B_OK;
}

static status_t list_mix_connections(multi_mix_connection_info * data)
{
	data->actual_count = 0;
	return B_OK;
}

static status_t list_mix_channels(multi_mix_channel_info *data)
{
	return B_OK;
}

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

static status_t get_description(multi_description *data)
{
	data->interface_version = B_CURRENT_INTERFACE_VERSION;
	data->interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strcpy(data->friendly_name,"AC97 (ICH)");
	strcpy(data->vendor_info,"Marcus Overhagen");

	data->output_channel_count = 2;
	data->input_channel_count = 2;
	data->output_bus_channel_count = 2;
	data->input_bus_channel_count = 2;
	data->aux_bus_channel_count = 0;

	// for each channel, starting with the first output channel, 
	// then the second, third..., followed by the first input 
	// channel, second, third, ..., followed by output bus
	// channels and input bus channels and finally auxillary channels, 

	LOG(("request_channel_count = %d\n",data->request_channel_count));
	if (data->request_channel_count >= (int)(sizeof(chans) / sizeof(chans[0]))) {
		LOG(("copying data\n"));
		memcpy(data->channels,&chans,sizeof(chans));
	}

	/* determine output rates */	
	data->output_rates = 0;
	if (ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 8000))
		data->output_rates |= B_SR_8000;
	if (ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 11025))
		data->output_rates |= B_SR_11025;
	if (ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 12000))
		data->output_rates |= B_SR_12000;
	if (ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 16000))
		data->output_rates |= B_SR_16000;
	if (ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 22050))
		data->output_rates |= B_SR_22050;
	if (ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 24000))
		data->output_rates |= B_SR_24000;
	if (ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 32000))
		data->output_rates |= B_SR_32000;
	if (ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 44100))
		data->output_rates |= B_SR_44100;
	if (ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 48000))
		data->output_rates |= B_SR_48000;

	/* determine input rates */	
	data->input_rates = 0;
	if (ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 8000))
		data->input_rates |= B_SR_8000;
	if (ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 11025))
		data->input_rates |= B_SR_11025;
	if (ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 12000))
		data->input_rates |= B_SR_12000;
	if (ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 16000))
		data->input_rates |= B_SR_16000;
	if (ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 22050))
		data->input_rates |= B_SR_22050;
	if (ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 24000))
		data->input_rates |= B_SR_24000;
	if (ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 32000))
		data->input_rates |= B_SR_32000;
	if (ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 44100))
		data->input_rates |= B_SR_44100;
	if (ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 48000))
		data->input_rates |= B_SR_48000;

	/* try to set 44100 rate */	
	if (ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 44100)
		&& ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 44100)) {
		config->input_rate = B_SR_44100;
		config->output_rate = B_SR_44100;
	} else {
		/* if that didn't work, continue with 48000 */	
		ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 48000);
		ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 48000);
		config->input_rate = B_SR_48000;
		config->output_rate = B_SR_48000;
	}

	/* force existance of 48kHz if variable rates are not supported */
	if (data->output_rates == 0)
		data->output_rates = B_SR_48000;
	if (data->input_rates == 0)
		data->input_rates = B_SR_48000;

	data->max_cvsr_rate = 0;
	data->min_cvsr_rate = 0;

	data->output_formats = B_FMT_16BIT;
	data->input_formats = B_FMT_16BIT;
	data->lock_sources = B_MULTI_LOCK_INTERNAL;
	data->timecode_sources = 0;
	data->interface_flags = B_MULTI_INTERFACE_PLAYBACK | B_MULTI_INTERFACE_RECORD;
	data->start_latency = 30000;

	strcpy(data->control_panel,"");

	return B_OK;
}

static status_t get_enabled_channels(multi_channel_enable *data)
{
	B_SET_CHANNEL(data->enable_bits, 0, true);
	B_SET_CHANNEL(data->enable_bits, 1, true);
	B_SET_CHANNEL(data->enable_bits, 2, true);
	B_SET_CHANNEL(data->enable_bits, 3, true);
	data->lock_source = B_MULTI_LOCK_INTERNAL;
/*
	uint32			lock_source;
	int32			lock_data;
	uint32			timecode_source;
	uint32 *		connectors;
*/
	return B_OK;
}

static status_t get_global_format(multi_format_info *data)
{
	data->output_latency = 0;
	data->input_latency = 0;
	data->timecode_kind = 0;
	data->input.format = B_FMT_16BIT;
	data->output.format = B_FMT_16BIT;
	data->input.rate = config->input_rate;
	data->output.rate = config->output_rate;
	return B_OK;
}

static status_t set_global_format(multi_format_info *data)
{
	bool in, out;
	LOG(("set_global_format: input.rate  = 0x%x\n", data->input.rate));
	LOG(("set_global_format: input.cvsr  = %d\n", data->input.cvsr));
	LOG(("set_global_format: output.rate = 0x%x\n", data->output.rate));
	LOG(("set_global_format: output.cvsr = %d\n", data->output.cvsr));
	switch (data->input.rate) {
		case B_SR_8000:
			in = ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 8000);
			break;
		case B_SR_11025:
			in = ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 11025);
			break;
		case B_SR_12000:
			in = ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 12000);
			break;
		case B_SR_16000:
			in = ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 16000);
			break;
		case B_SR_22050:
			in = ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 22050);
			break;
		case B_SR_24000:
			in = ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 24000);
			break;
		case B_SR_32000:
			in = ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 32000);
			break;
		case B_SR_44100:
			in = ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 44100);
			break;
		case B_SR_48000:
			in = ac97_set_rate(config->ac97, AC97_PCM_L_R_ADC_RATE, 48000);
			break;
		default:
			in = false;
	}

	switch (data->output.rate) {
		case B_SR_8000:
			out = ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 8000);
			break;
		case B_SR_11025:
			out = ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 11025);
			break;
		case B_SR_12000:
			out = ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 12000);
			break;
		case B_SR_16000:
			out = ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 16000);
			break;
		case B_SR_22050:
			out = ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 22050);
			break;
		case B_SR_24000:
			out = ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 24000);
			break;
		case B_SR_32000:
			out = ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 32000);
			break;
		case B_SR_44100:
			out = ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 44100);
			break;
		case B_SR_48000:
			out = ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 48000);
			break;
		default:
			out = false;
	}

	if (!in || !out)
		return B_ERROR;
		
	config->input_rate = data->input.rate;
	config->output_rate = data->output.rate;
	return B_OK;
}


static status_t get_buffers(multi_buffer_list *data)
{
	LOG(("flags = %#x\n",data->flags));
	LOG(("request_playback_buffers = %#x\n",data->request_playback_buffers));
	LOG(("request_playback_channels = %#x\n",data->request_playback_channels));
	LOG(("request_playback_buffer_size = %#x\n",data->request_playback_buffer_size));
	LOG(("request_record_buffers = %#x\n",data->request_record_buffers));
	LOG(("request_record_channels = %#x\n",data->request_record_channels));
	LOG(("request_record_buffer_size = %#x\n",data->request_record_buffer_size));

	if (data->request_playback_buffers < 2 ||
		data->request_playback_channels < 2 ||
		data->request_record_buffers < 2 ||
		data->request_record_channels < 2) {
		LOG(("not enough channels/buffers\n"));
	}

	ASSERT(BUFFER_COUNT == 2);
	
//	data->flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD; // XXX ???
	data->flags = 0;
	
	data->return_playback_buffers = 2;			/* playback_buffers[b][] */
	data->return_playback_channels = 2;			/* playback_buffers[][c] */
	data->return_playback_buffer_size = BUFFER_SIZE / 4;		/* frames */

	/* buffer 0, left channel */
	data->playback_buffers[0][0].base = chan_po->userbuffer[0]; /* pointer to first sample for channel for buffer */
	data->playback_buffers[0][0].stride = 4; /* offset to next sample */
	/* buffer 0, right channel */
	data->playback_buffers[0][1].base = ((char*)chan_po->userbuffer[0]) + 2; /* pointer to first sample for channel for buffer */
	data->playback_buffers[0][1].stride = 4; /* offset to next sample */
	/* buffer 1, left channel */
	data->playback_buffers[1][0].base = chan_po->userbuffer[1]; /* pointer to first sample for channel for buffer */
	data->playback_buffers[1][0].stride = 4; /* offset to next sample */
	/* buffer 1, right channel */
	data->playback_buffers[1][1].base = ((char*)chan_po->userbuffer[1]) + 2; /* pointer to first sample for channel for buffer */
	data->playback_buffers[1][1].stride = 4; /* offset to next sample */
	
	data->return_record_buffers = 2;
	data->return_record_channels = 2;
	data->return_record_buffer_size = BUFFER_SIZE / 4;			/* frames */

	/* buffer 0, left channel */
	data->record_buffers[0][0].base = chan_pi->userbuffer[0]; /* pointer to first sample for channel for buffer */
	data->record_buffers[0][0].stride = 4; /* offset to next sample */
	/* buffer 0, right channel */
	data->record_buffers[0][1].base = ((char*)chan_pi->userbuffer[0]) + 2; /* pointer to first sample for channel for buffer */
	data->record_buffers[0][1].stride = 4; /* offset to next sample */
	/* buffer 1, left channel */
	data->record_buffers[1][0].base = chan_pi->userbuffer[1]; /* pointer to first sample for channel for buffer */
	data->record_buffers[1][0].stride = 4; /* offset to next sample */
	/* buffer 1, right channel */
	data->record_buffers[1][1].base = ((char*)chan_pi->userbuffer[1]) + 2; /* pointer to first sample for channel for buffer */
	data->record_buffers[1][1].stride = 4; /* offset to next sample */

	return B_OK;
}

static status_t buffer_exchange(multi_buffer_info *data)
{
#if DEBUG
	static int debug_buffers_exchanged = 0;
#endif
	cpu_status status;
	void *backbuffer;
/*	
	static int next_input = 0;
*/	
	/* 
	 * Only playback works for now, recording seems to be broken, but I don't know why
	 */
	
	if (!chan_po->running)
		start_chan(chan_po);

/*
	if (!chan_pi->running)
		start_chan(chan_pi);
*/

	/* do playback */
	acquire_sem(chan_po->buffer_ready_sem);
	status = lock();
	backbuffer = (void *)chan_po->backbuffer;
	data->played_real_time = chan_po->played_real_time;
	data->played_frames_count = chan_po->played_frames_count;
	unlock(status);
	memcpy(backbuffer, chan_po->userbuffer[data->playback_buffer_cycle], BUFFER_SIZE);

	/* do record */
	data->record_buffer_cycle = 0;
	data->recorded_frames_count = 0;
/*
	if (B_OK == acquire_sem_etc(chan_pi->buffer_ready_sem, 1, B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT,200)) {
		status = lock();
		backbuffer = (void *)chan_pi->backbuffer;
		data->recorded_real_time = chan_pi->played_real_time;
		unlock(status);
		data->recorded_frames_count = BUFFER_SIZE / 4;
		data->record_buffer_cycle = next_input;
		next_input = (next_input + 1) % 2;
		memcpy(chan_pi->userbuffer[data->record_buffer_cycle],backbuffer, BUFFER_SIZE);
	} else {
		data->record_buffer_cycle = next_input;
		data->recorded_frames_count = 0;
	}
*/
#if DEBUG
	debug_buffers_exchanged++;
	if (((debug_buffers_exchanged % 100) == 1) && (debug_buffers_exchanged < 1111)) {
		LOG(("buffer_exchange: %d buffers processed\n", debug_buffers_exchanged));
	}
#endif

	return B_OK;

/*		
	if (data->flags & B_MULTI_BUFFER_PLAYBACK) {
		dprintf("B_MULTI_BUFFER_PLAYBACK\n");
	}

	if (data->flags & B_MULTI_BUFFER_RECORD) {
		dprintf("B_MULTI_BUFFER_RECORD\n");
		data->recorded_real_time = 0;
		data->recorded_frames_count = 0;
		data->record_buffer_cycle = 0;
	}
	if (data->flags & B_MULTI_BUFFER_METERING) {
		dprintf("B_MULTI_BUFFER_METERING\n");
		data->meter_channel_count = 0;
		// data->meters_peak
		// data->meters_average
	}
	if (data->flags & B_MULTI_BUFFER_TIMECODE) {
		dprintf("B_MULTI_BUFFER_TIMECODE\n");
		data->hours = 0;
		data->minutes = 0;
		data->seconds = 0;
		data->tc_frames = 0;
		data->at_frame_delta = 0;
	}
	return B_OK;
*/
}

static status_t buffer_force_stop()
{
	return B_OK;
}

status_t multi_control(void *cookie, uint32 op, void *data, size_t length)
{
    switch (op) {
		case B_MULTI_GET_DESCRIPTION: 
			LOG(("B_MULTI_GET_DESCRIPTION\n"));
			return get_description((multi_description *)data);
		case B_MULTI_GET_EVENT_INFO:
			LOG(("B_MULTI_GET_EVENT_INFO\n"));
			return B_ERROR;
		case B_MULTI_SET_EVENT_INFO:
			LOG(("B_MULTI_SET_EVENT_INFO\n"));
			return B_ERROR;
		case B_MULTI_GET_EVENT:
			LOG(("B_MULTI_GET_EVENT\n"));
			return B_ERROR;
		case B_MULTI_GET_ENABLED_CHANNELS:
			LOG(("B_MULTI_GET_ENABLED_CHANNELS\n"));
			return get_enabled_channels((multi_channel_enable *)data);
		case B_MULTI_SET_ENABLED_CHANNELS:
			LOG(("B_MULTI_SET_ENABLED_CHANNELS\n"));
			return B_OK; break;
		case B_MULTI_GET_GLOBAL_FORMAT:
			LOG(("B_MULTI_GET_GLOBAL_FORMAT\n"));
			return get_global_format((multi_format_info *)data);
		case B_MULTI_SET_GLOBAL_FORMAT:
			LOG(("B_MULTI_SET_GLOBAL_FORMAT\n"));
			set_global_format((multi_format_info *)data);
			return B_OK; /* XXX BUG! we *MUST* return B_OK, returning B_ERROR will prevent 
						  * BeOS to accept the format returned in B_MULTI_GET_GLOBAL_FORMAT
						  */
		case B_MULTI_GET_CHANNEL_FORMATS:
			LOG(("B_MULTI_GET_CHANNEL_FORMATS\n"));
			return B_ERROR;
		case B_MULTI_SET_CHANNEL_FORMATS:	/* only implemented if possible */
			LOG(("B_MULTI_SET_CHANNEL_FORMATS\n"));
			return B_ERROR;
		case B_MULTI_GET_MIX:
			LOG(("B_MULTI_GET_MIX\n"));
			return B_ERROR;
		case B_MULTI_SET_MIX:
			LOG(("B_MULTI_SET_MIX\n"));
			return B_ERROR;
		case B_MULTI_LIST_MIX_CHANNELS:
			LOG(("B_MULTI_LIST_MIX_CHANNELS\n"));
			return list_mix_channels((multi_mix_channel_info *)data);
		case B_MULTI_LIST_MIX_CONTROLS:
			LOG(("B_MULTI_LIST_MIX_CONTROLS\n"));
			return list_mix_controls((multi_mix_control_info *)data);
		case B_MULTI_LIST_MIX_CONNECTIONS:
			LOG(("B_MULTI_LIST_MIX_CONNECTIONS\n"));
			return list_mix_connections((multi_mix_connection_info *)data);
		case B_MULTI_GET_BUFFERS:			/* Fill out the struct for the first time; doesn't start anything. */
			LOG(("B_MULTI_GET_BUFFERS\n"));
			return get_buffers(data);
		case B_MULTI_SET_BUFFERS:			/* Set what buffers to use, if the driver supports soft buffers. */
			LOG(("B_MULTI_SET_BUFFERS\n"));
			return B_ERROR; /* we do not support soft buffers */
		case B_MULTI_SET_START_TIME:			/* When to actually start */
			LOG(("B_MULTI_SET_START_TIME\n"));
			return B_ERROR;
		case B_MULTI_BUFFER_EXCHANGE:		/* stop and go are derived from this being called */
//			dprintf("B_MULTI_BUFFER_EXCHANGE\n");
			return buffer_exchange((multi_buffer_info *)data);
		case B_MULTI_BUFFER_FORCE_STOP:		/* force stop of playback */
			LOG(("B_MULTI_BUFFER_FORCE_STOP\n"));
			return buffer_force_stop();
	}
	LOG(("ERROR: unknown multi_control %#x\n",op));
	return B_ERROR;
}


