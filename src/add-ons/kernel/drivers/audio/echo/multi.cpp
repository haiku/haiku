/*
 * EchoGals/Echo24 BeOS Driver for Echo audio cards
 *
 * Copyright (c) 2003, Jerome Duval (jerome.duval@free.fr)
 *
 * Original code : BeOS Driver for Intel ICH AC'97 Link interface
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
#include "debug.h"
#include "multi_audio.h"
#include "multi.h"

//#define DEBUG 1

#include "echo.h"
#include "util.h"

static status_t
echo_create_controls_list(multi_dev *multi)
{				
	multi->control_count = 0;
	PRINT(("multi->control_count %u\n", multi->control_count));
	return B_OK;
}

static status_t 
echo_get_mix(echo_dev *card, multi_mix_value_info * MMVI)
{
	return B_OK;
}

static status_t 
echo_set_mix(echo_dev *card, multi_mix_value_info * MMVI)
{
	return B_OK;
}

static status_t 
echo_list_mix_controls(echo_dev *card, multi_mix_control_info * MMCI)
{
	multi_mix_control	*MMC;
	uint32 i;
	
	MMC = MMCI->controls;
	if(MMCI->control_count < 24)
		return B_ERROR;
			
	if(echo_create_controls_list(&card->multi) < B_OK)
		return B_ERROR;
	for(i=0; i<card->multi.control_count; i++) {
		MMC[i] = card->multi.controls[i].mix_control;
	}
	
	MMCI->control_count = card->multi.control_count;	
	return B_OK;
}

static status_t 
echo_list_mix_connections(echo_dev *card, multi_mix_connection_info * data)
{
	return B_ERROR;
}

static status_t 
echo_list_mix_channels(echo_dev *card, multi_mix_channel_info *data)
{
	return B_ERROR;
}

/*multi_channel_info chans[] = {
{  0, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
{  1, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
{  2, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
{  3, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
{  4, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
{  5, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
{  6, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
{  7, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
{  8, B_MULTI_OUTPUT_BUS, 		B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 	B_CHANNEL_MINI_JACK_STEREO },
{  9, B_MULTI_OUTPUT_BUS, 		B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, B_CHANNEL_MINI_JACK_STEREO },
{  10, B_MULTI_INPUT_BUS, 		B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 	B_CHANNEL_MINI_JACK_STEREO },
{  11, B_MULTI_INPUT_BUS, 		B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, B_CHANNEL_MINI_JACK_STEREO },
};*/

/*multi_channel_info chans[] = {
{  0, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
{  1, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
{  2, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_SURROUND_BUS, 0 },
{  3, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_SURROUND_BUS, 0 },
{  4, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_REARLEFT | B_CHANNEL_SURROUND_BUS, 0 },
{  5, B_MULTI_OUTPUT_CHANNEL, 	B_CHANNEL_REARRIGHT | B_CHANNEL_SURROUND_BUS, 0 },
{  6, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
{  7, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
{  8, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 0 },
{  9, B_MULTI_INPUT_CHANNEL, 	B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, 0 },
{  10, B_MULTI_OUTPUT_BUS, 		B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 	B_CHANNEL_MINI_JACK_STEREO },
{  11, B_MULTI_OUTPUT_BUS, 		B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, B_CHANNEL_MINI_JACK_STEREO },
{  12, B_MULTI_INPUT_BUS, 		B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS, 	B_CHANNEL_MINI_JACK_STEREO },
{  13, B_MULTI_INPUT_BUS, 		B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS, B_CHANNEL_MINI_JACK_STEREO },
};*/


static void
echo_create_channels_list(multi_dev *multi)
{
	echo_stream *stream;
	int32 mode;
	uint32 index, i, designations;
	multi_channel_info *chans;
	uint32 chan_designations[] = {
		B_CHANNEL_LEFT,
		B_CHANNEL_RIGHT,
		B_CHANNEL_REARLEFT,
		B_CHANNEL_REARRIGHT,
		B_CHANNEL_CENTER,
		B_CHANNEL_SUB
	};
	
	chans = multi->chans;
	index = 0;

	for(mode=ECHO_USE_PLAY; mode!=-1; 
		mode = (mode == ECHO_USE_PLAY) ? ECHO_USE_RECORD : -1) {
		LIST_FOREACH(stream, &((echo_dev*)multi->card)->streams, next) {
			if ((stream->use & mode) == 0)
				continue;
				
			if(stream->channels == 2)
				designations = B_CHANNEL_STEREO_BUS;
			else
				designations = B_CHANNEL_SURROUND_BUS;
			
			for(i=0; i<stream->channels; i++) {
				chans[index].channel_id = index;
				chans[index].kind = (mode == ECHO_USE_PLAY) ? B_MULTI_OUTPUT_CHANNEL : B_MULTI_INPUT_CHANNEL;
				chans[index].designations = designations | chan_designations[i];
				chans[index].connectors = 0;
				index++;
			}
		}
		
		if(mode==ECHO_USE_PLAY) {
			multi->output_channel_count = index;
		} else {
			multi->input_channel_count = index - multi->output_channel_count;
		}
	}
	
	chans[index].channel_id = index;
	chans[index].kind = B_MULTI_OUTPUT_BUS;
	chans[index].designations = B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS;
	chans[index].connectors = B_CHANNEL_MINI_JACK_STEREO;
	index++;
	
	chans[index].channel_id = index;
	chans[index].kind = B_MULTI_OUTPUT_BUS;
	chans[index].designations = B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS;
	chans[index].connectors = B_CHANNEL_MINI_JACK_STEREO;
	index++;
	
	multi->output_bus_channel_count = index - multi->output_channel_count 
		- multi->input_channel_count;
	
	chans[index].channel_id = index;
	chans[index].kind = B_MULTI_INPUT_BUS;
	chans[index].designations = B_CHANNEL_LEFT | B_CHANNEL_STEREO_BUS;
	chans[index].connectors = B_CHANNEL_MINI_JACK_STEREO;
	index++;
	
	chans[index].channel_id = index;
	chans[index].kind = B_MULTI_INPUT_BUS;
	chans[index].designations = B_CHANNEL_RIGHT | B_CHANNEL_STEREO_BUS;
	chans[index].connectors = B_CHANNEL_MINI_JACK_STEREO;
	index++;
	
	multi->input_bus_channel_count = index - multi->output_channel_count 
		- multi->input_channel_count - multi->output_bus_channel_count;
		
	multi->aux_bus_channel_count = 0;
}


static status_t 
echo_get_description(echo_dev *card, multi_description *data)
{
	int32 size;

	data->interface_version = B_CURRENT_INTERFACE_VERSION;
	data->interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strncpy(data->friendly_name, card->name, 32);
	strcpy(data->vendor_info, AUTHOR);

	data->output_channel_count = card->multi.output_channel_count;
	data->input_channel_count = card->multi.input_channel_count;
	data->output_bus_channel_count = card->multi.output_bus_channel_count;
	data->input_bus_channel_count = card->multi.input_bus_channel_count;
	data->aux_bus_channel_count = card->multi.aux_bus_channel_count;
	
	size = card->multi.output_channel_count + card->multi.input_channel_count
			+ card->multi.output_bus_channel_count + card->multi.input_bus_channel_count
			+ card->multi.aux_bus_channel_count;
			
	// for each channel, starting with the first output channel, 
	// then the second, third..., followed by the first input 
	// channel, second, third, ..., followed by output bus
	// channels and input bus channels and finally auxillary channels, 

	LOG(("request_channel_count = %d\n",data->request_channel_count));
	if (data->request_channel_count >= size) {
		LOG(("copying data\n"));
		memcpy(data->channels, card->multi.chans, size * sizeof(card->multi.chans[0]));
	}

	data->output_rates = B_SR_48000;// | B_SR_44100 | B_SR_CVSR;
	data->input_rates = B_SR_48000;// | B_SR_44100 | B_SR_CVSR;
	//data->output_rates = B_SR_44100;
	//data->input_rates = B_SR_44100;
	data->min_cvsr_rate = 0;
	data->max_cvsr_rate = 48000;
	//data->max_cvsr_rate = 44100;

	data->output_formats = B_FMT_16BIT;
	data->input_formats = B_FMT_16BIT;
	data->lock_sources = B_MULTI_LOCK_INTERNAL;
	data->timecode_sources = 0;
	data->interface_flags = B_MULTI_INTERFACE_PLAYBACK | B_MULTI_INTERFACE_RECORD;
	data->start_latency = 3000;

	strcpy(data->control_panel,"");

	return B_OK;
}

static status_t 
echo_get_enabled_channels(echo_dev *card, multi_channel_enable *data)
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

static status_t 
echo_set_enabled_channels(echo_dev *card, multi_channel_enable *data)
{
	PRINT(("set_enabled_channels 0 : %s\n", B_TEST_CHANNEL(data->enable_bits, 0) ? "enabled": "disabled"));
	PRINT(("set_enabled_channels 1 : %s\n", B_TEST_CHANNEL(data->enable_bits, 1) ? "enabled": "disabled"));
	PRINT(("set_enabled_channels 2 : %s\n", B_TEST_CHANNEL(data->enable_bits, 2) ? "enabled": "disabled"));
	PRINT(("set_enabled_channels 3 : %s\n", B_TEST_CHANNEL(data->enable_bits, 3) ? "enabled": "disabled"));
	return B_OK;
}

static status_t 
echo_get_global_format(echo_dev *card, multi_format_info *data)
{
	data->output_latency = 0;
	data->input_latency = 0;
	data->timecode_kind = 0;
	data->input.rate = B_SR_48000;
	data->input.cvsr = 48000;
	data->input.format = B_FMT_16BIT;
	data->output.rate = B_SR_48000;
	data->output.cvsr = 48000;
	data->output.format = B_FMT_16BIT;
	/*data->input.rate = B_SR_44100;
	data->input.cvsr = 44100;
	data->input.format = B_FMT_16BIT;
	data->output.rate = B_SR_44100;
	data->output.cvsr = 44100;
	data->output.format = B_FMT_16BIT;*/
	return B_OK;
}

static status_t 
echo_get_buffers(echo_dev *card, multi_buffer_list *data)
{
	int32 i, j, pchannels, rchannels;
	
	LOG(("flags = %#x\n",data->flags));
	LOG(("request_playback_buffers = %#x\n",data->request_playback_buffers));
	LOG(("request_playback_channels = %#x\n",data->request_playback_channels));
	LOG(("request_playback_buffer_size = %#x\n",data->request_playback_buffer_size));
	LOG(("request_record_buffers = %#x\n",data->request_record_buffers));
	LOG(("request_record_channels = %#x\n",data->request_record_channels));
	LOG(("request_record_buffer_size = %#x\n",data->request_record_buffer_size));
	
	pchannels = card->pstream->channels;
	rchannels = card->rstream->channels;
	
	if (data->request_playback_buffers < BUFFER_COUNT ||
		data->request_playback_channels < (pchannels) ||
		data->request_record_buffers < BUFFER_COUNT ||
		data->request_record_channels < (rchannels)) {
		LOG(("not enough channels/buffers\n"));
	}

	ASSERT(BUFFER_COUNT == 2);
	
	data->flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD; // XXX ???
//	data->flags = 0;
		
	data->return_playback_buffers = BUFFER_COUNT;	/* playback_buffers[b][] */
	data->return_playback_channels = pchannels;		/* playback_buffers[][c] */
	data->return_playback_buffer_size = BUFFER_FRAMES;		/* frames */

	for(i=0; i<BUFFER_COUNT; i++)
		for(j=0; j<pchannels; j++)
			echo_stream_get_nth_buffer(card->pstream, j, i, 
				&data->playback_buffers[i][j].base,
				&data->playback_buffers[i][j].stride);
					
	data->return_record_buffers = BUFFER_COUNT;
	data->return_record_channels = rchannels;
	data->return_record_buffer_size = BUFFER_FRAMES;	/* frames */

	for(i=0; i<BUFFER_COUNT; i++)
		for(j=0; j<rchannels; j++)
			echo_stream_get_nth_buffer(card->rstream, j, i, 
				&data->record_buffers[i][j].base,
				&data->record_buffers[i][j].stride);
		
	return B_OK;
}


void
echo_play_inth(void* inthparams)
{
	echo_stream *stream = (echo_stream *)inthparams;
	//int32 count;
	
	//TRACE(("echo_play_inth\n"));
	
	acquire_spinlock(&slock);
	stream->real_time = system_time();
	stream->frames_count += BUFFER_FRAMES;
	stream->buffer_cycle = (stream->trigblk 
		+ stream->blkmod -1) % stream->blkmod;
	stream->update_needed = true;
	release_spinlock(&slock);
			
	//get_sem_count(stream->card->buffer_ready_sem, &count);
	//if (count <= 0)
		release_sem_etc(stream->card->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
}

void
echo_record_inth(void* inthparams)
{
	echo_stream *stream = (echo_stream *)inthparams;
	//int32 count;
	
	//TRACE(("echo_record_inth\n"));
	
	acquire_spinlock(&slock);
	stream->real_time = system_time();
	stream->frames_count += BUFFER_FRAMES;
	stream->buffer_cycle = (stream->trigblk 
		+ stream->blkmod -1) % stream->blkmod;
	stream->update_needed = true;
	release_spinlock(&slock);
			
	//get_sem_count(stream->card->buffer_ready_sem, &count);
	//if (count <= 0)
		release_sem_etc(stream->card->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
}

static status_t 
echo_buffer_exchange(echo_dev *card, multi_buffer_info *data)
{
	cpu_status status;
	echo_stream *pstream, *rstream;

	data->flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD;
	
	if (!(card->pstream->state & ECHO_STATE_STARTED))
		echo_stream_start(card->pstream, echo_play_inth, card->pstream);
	
	if (!(card->rstream->state & ECHO_STATE_STARTED))
		echo_stream_start(card->rstream, echo_record_inth, card->rstream);

	if(acquire_sem_etc(card->buffer_ready_sem, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, 50000)
		== B_TIMED_OUT) {
		LOG(("buffer_exchange timeout ff\n"));
	}
	
	status = lock();
	
	LIST_FOREACH(pstream, &card->streams, next) {
		if ((pstream->use & ECHO_USE_PLAY) == 0 || 
			(pstream->state & ECHO_STATE_STARTED) == 0)
			continue;
		if(pstream->update_needed)	
			break;
	}
	
	LIST_FOREACH(rstream, &card->streams, next) {
		if ((rstream->use & ECHO_USE_RECORD) == 0 ||
			(rstream->state & ECHO_STATE_STARTED) == 0)
			continue;
		if(rstream->update_needed)	
			break;
	}
	
	if(!pstream)
		pstream = card->pstream;
	if(!rstream)
		rstream = card->rstream;
	
	/* do playback */
	data->playback_buffer_cycle = pstream->buffer_cycle;
	data->played_real_time = pstream->real_time;
	data->played_frames_count = pstream->frames_count;
	data->_reserved_0 = pstream->first_channel;
	pstream->update_needed = false;
	
	/* do record */
	data->record_buffer_cycle = rstream->buffer_cycle;
	data->recorded_frames_count = rstream->frames_count;
	data->recorded_real_time = rstream->real_time;
	data->_reserved_1 = rstream->first_channel;
	rstream->update_needed = false;
	unlock(status);
	
	//TRACE(("buffer_exchange ended\n"));
	return B_OK;
}

static status_t 
echo_buffer_force_stop(echo_dev *card)
{
	//echo_voice_halt(card->pvoice);
	return B_OK;
}

static status_t 
echo_multi_control(void *cookie, uint32 op, void *data, size_t length)
{
	echo_dev *card = (echo_dev *)cookie;

    switch (op) {
		case B_MULTI_GET_DESCRIPTION: 
			LOG(("B_MULTI_GET_DESCRIPTION\n"));
			return echo_get_description(card, (multi_description *)data);
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
			return echo_get_enabled_channels(card, (multi_channel_enable *)data);
		case B_MULTI_SET_ENABLED_CHANNELS:
			LOG(("B_MULTI_SET_ENABLED_CHANNELS\n"));
			return echo_set_enabled_channels(card, (multi_channel_enable *)data);
		case B_MULTI_GET_GLOBAL_FORMAT:
			LOG(("B_MULTI_GET_GLOBAL_FORMAT\n"));
			return echo_get_global_format(card, (multi_format_info *)data);
		case B_MULTI_SET_GLOBAL_FORMAT:
			LOG(("B_MULTI_SET_GLOBAL_FORMAT\n"));
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
			return echo_get_mix(card, (multi_mix_value_info *)data);
		case B_MULTI_SET_MIX:
			LOG(("B_MULTI_SET_MIX\n"));
			return echo_set_mix(card, (multi_mix_value_info *)data);
		case B_MULTI_LIST_MIX_CHANNELS:
			LOG(("B_MULTI_LIST_MIX_CHANNELS\n"));
			return echo_list_mix_channels(card, (multi_mix_channel_info *)data);
		case B_MULTI_LIST_MIX_CONTROLS:
			LOG(("B_MULTI_LIST_MIX_CONTROLS\n"));
			return echo_list_mix_controls(card, (multi_mix_control_info *)data);
		case B_MULTI_LIST_MIX_CONNECTIONS:
			LOG(("B_MULTI_LIST_MIX_CONNECTIONS\n"));
			return echo_list_mix_connections(card, (multi_mix_connection_info *)data);
		case B_MULTI_GET_BUFFERS:			/* Fill out the struct for the first time; doesn't start anything. */
			LOG(("B_MULTI_GET_BUFFERS\n"));
			return echo_get_buffers(card, (multi_buffer_list*)data);
		case B_MULTI_SET_BUFFERS:			/* Set what buffers to use, if the driver supports soft buffers. */
			LOG(("B_MULTI_SET_BUFFERS\n"));
			return B_ERROR; /* we do not support soft buffers */
		case B_MULTI_SET_START_TIME:			/* When to actually start */
			LOG(("B_MULTI_SET_START_TIME\n"));
			return B_ERROR;
		case B_MULTI_BUFFER_EXCHANGE:		/* stop and go are derived from this being called */
			//TRACE(("B_MULTI_BUFFER_EXCHANGE\n"));
			return echo_buffer_exchange(card, (multi_buffer_info *)data);
		case B_MULTI_BUFFER_FORCE_STOP:		/* force stop of playback, nothing in data */
			LOG(("B_MULTI_BUFFER_FORCE_STOP\n"));
			return echo_buffer_force_stop(card);
	}
	LOG(("ERROR: unknown multi_control %#x\n",op));
	return B_ERROR;
}

static status_t echo_open(const char *name, uint32 flags, void** cookie);
static status_t echo_close(void* cookie);
static status_t echo_free(void* cookie);
static status_t echo_control(void* cookie, uint32 op, void* arg, size_t len);
static status_t echo_read(void* cookie, off_t position, void *buf, size_t* num_bytes);
static status_t echo_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes);

device_hooks multi_hooks = {
	echo_open, 			/* -> open entry point */
	echo_close, 			/* -> close entry point */
	echo_free,			/* -> free cookie */
	echo_control, 		/* -> control entry point */
	echo_read,			/* -> read entry point */
	echo_write,			/* -> write entry point */
	NULL,					/* start select */
	NULL,					/* stop select */
	NULL,					/* scatter-gather read from the device */
	NULL					/* scatter-gather write to the device */
};

static status_t
echo_open(const char *name, uint32 flags, void** cookie)
{
	echo_dev *card = NULL;
	int ix;
	
	LOG(("echo_open()\n"));
	
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(cards[ix].name, name)) {
			card = &cards[ix];
		}
	}
	
	if(card == NULL) {
		LOG(("open() card not found %s\n", name));
		for (ix=0; ix<num_cards; ix++) {
			LOG(("open() card available %s\n", cards[ix].name)); 
		}
		return B_ERROR;
	}
		
	LOG(("open() got card\n"));
	
	if(card->pstream !=NULL)
		return B_ERROR;
	if(card->rstream !=NULL)
		return B_ERROR;
			
	*cookie = card;
	card->multi.card = card;
		
	LOG(("stream_new\n"));
		
	card->rstream = echo_stream_new(card, ECHO_USE_RECORD, BUFFER_FRAMES, BUFFER_COUNT);
	card->pstream = echo_stream_new(card, ECHO_USE_PLAY, BUFFER_FRAMES, BUFFER_COUNT);
	
	card->buffer_ready_sem = create_sem(0, "pbuffer ready");
		
	LOG(("stream_setaudio\n"));
	
	echo_stream_set_audioparms(card->pstream, 2, 16, 48000);
	echo_stream_set_audioparms(card->rstream, 2, 16, 48000);
		
	card->pstream->first_channel = 0;
	card->rstream->first_channel = 2;
	
	echo_create_channels_list(&card->multi);
		
	return B_OK;
}

static status_t
echo_close(void* cookie)
{
	//echo_dev *card = cookie;
	LOG(("close()\n"));
		
	return B_OK;
}

static status_t
echo_free(void* cookie)
{
	echo_dev *card = (echo_dev *) cookie;
	echo_stream *stream;
	LOG(("echo_free()\n"));
		
	if (card->buffer_ready_sem > B_OK)
			delete_sem(card->buffer_ready_sem);
			
	LIST_FOREACH(stream, &card->streams, next) {
		echo_stream_halt(stream);
	}
	
	while(!LIST_EMPTY(&card->streams)) {
		echo_stream_delete(LIST_FIRST(&card->streams));
	}
	
	return B_OK;
}

static status_t
echo_control(void* cookie, uint32 op, void* arg, size_t len)
{
	return echo_multi_control(cookie, op, arg, len);
}

static status_t
echo_read(void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was read */
	return B_IO_ERROR;
}

static status_t
echo_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was written */
	return B_IO_ERROR;
}

