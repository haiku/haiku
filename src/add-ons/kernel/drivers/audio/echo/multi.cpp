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

#include <driver_settings.h>
#include <OS.h>
#include <MediaDefs.h>
#include "debug.h"
#include "hmulti_audio.h"
#include "multi.h"

//#define DEBUG 1

#include "echo.h"
#include "util.h"

typedef enum {
	B_MIX_GAIN = 1 << 0,
	B_MIX_MUTE = 1 << 1,
	B_MIX_NOMINAL = 1 << 2
} mixer_type;

typedef struct {
	uint8	channels;
	uint8	bitsPerSample;
	uint32	sample_rate;
	uint32	buffer_frames;
	int32	buffer_count;
} echo_settings;

echo_settings current_settings = {
	2,	// channels
	16,	// bits per sample
	48000,	// sample rate
	512,	// buffer frames
	2	// buffer count
};


static void	
echo_channel_get_mix(void *card, MIXER_AUDIO_CHANNEL channel, int32 type, float *values) {
	echo_dev *dev = (echo_dev*) card;
	MIXER_MULTI_FUNCTION multi_function[2];
	PMIXER_FUNCTION function = multi_function[0].MixerFunction;
	INT32 size = ComputeMixerMultiFunctionSize(2);
	function[0].Channel = channel;
	function[1].Channel = channel;
	function[1].Channel.wChannel++;
	switch (type) {
		case B_MIX_GAIN:
			function[0].iFunction = function[1].iFunction = MXF_GET_LEVEL;
			break;
		case B_MIX_MUTE:
			function[0].iFunction = function[1].iFunction = MXF_GET_MUTE;
			break;
		case B_MIX_NOMINAL:
			function[0].iFunction = function[1].iFunction = MXF_GET_NOMINAL;
			break;
	}

	multi_function[0].iCount = 2;
	dev->pEG->ProcessMixerMultiFunction(multi_function, size);

	if (function[0].RtnStatus == ECHOSTATUS_OK) {
		if (type == B_MIX_GAIN) {
			values[0] = (float)function[0].Data.iLevel / 256;
			values[1] = (float)function[1].Data.iLevel / 256;
		} else if (type == B_MIX_MUTE) {
			values[0] = function[0].Data.bMuteOn ? 1.0 : 0.0;
		} else {
			values[0] = function[0].Data.iNominal == 4 ? 1.0 : 0.0;
		}
		PRINT(("echo_channel_get_mix iLevel: %ld, %d, %ld\n", function[0].Data.iLevel, 
			channel.wChannel, channel.dwType));
	}

}


static void	
echo_channel_set_mix(void *card, MIXER_AUDIO_CHANNEL channel, int32 type, float *values) {
	echo_dev *dev = (echo_dev*) card;
	MIXER_MULTI_FUNCTION multi_function[2];
	PMIXER_FUNCTION function = multi_function[0].MixerFunction;
	INT32 size = ComputeMixerMultiFunctionSize(2);
	function[0].Channel = channel;
	function[1].Channel = channel;
	function[1].Channel.wChannel++;
	if (type == B_MIX_GAIN) {
		function[0].Data.iLevel = (int)(values[0] * 256);
		function[0].iFunction = MXF_SET_LEVEL;
		function[1].Data.iLevel = (int)(values[1] * 256);
		function[1].iFunction = MXF_SET_LEVEL;
	} else if (type == B_MIX_MUTE) {
		function[0].Data.bMuteOn = values[0] == 1.0;
		function[0].iFunction = MXF_SET_MUTE;
		function[1].Data.bMuteOn = values[0] == 1.0;
		function[1].iFunction = MXF_SET_MUTE;
	} else {
		function[0].Data.iNominal = values[0] == 1.0 ? 4 : -10;
		function[0].iFunction = MXF_SET_NOMINAL;
		function[1].Data.iNominal = values[0] == 1.0 ? 4 : -10;
		function[1].iFunction = MXF_SET_NOMINAL;
	}

	multi_function[0].iCount = 2;
	dev->pEG->ProcessMixerMultiFunction(multi_function, size);
	
	if (function[0].RtnStatus == ECHOSTATUS_OK) {
		PRINT(("echo_channel_set_mix OK: %ld, %d, %ld\n", function[0].Data.iLevel, 
			channel.wChannel, channel.dwType));
	}

}


static int32
echo_create_group_control(multi_dev *multi, uint32 *index, int32 parent,
	enum strind_id string, const char* name) {
	uint32 i = *index;
	(*index)++;
	multi->controls[i].mix_control.id = MULTI_CONTROL_FIRSTID + i;
	multi->controls[i].mix_control.parent = parent;
	multi->controls[i].mix_control.flags = B_MULTI_MIX_GROUP;
	multi->controls[i].mix_control.master = MULTI_CONTROL_MASTERID;
	multi->controls[i].mix_control.string = string;
	if (name)
		strcpy(multi->controls[i].mix_control.name, name);
	
	return multi->controls[i].mix_control.id;
}

static void
echo_create_channel_control(multi_dev *multi, uint32 *index, int32 parent, int32 string,
	MIXER_AUDIO_CHANNEL channel, bool nominal) {
	uint32 i = *index, id;
	multi_mixer_control control;
	
	control.mix_control.master = MULTI_CONTROL_MASTERID;
	control.mix_control.parent = parent;
	control.channel = channel;
	control.get = &echo_channel_get_mix;
	control.set = &echo_channel_set_mix;
	control.mix_control.gain.min_gain = -128;
	control.mix_control.gain.max_gain = 6;
	control.mix_control.gain.granularity = 0.5;
	
	control.mix_control.id = MULTI_CONTROL_FIRSTID + i;
	control.mix_control.flags = B_MULTI_MIX_ENABLE;
	control.mix_control.string = S_MUTE;
	control.type = B_MIX_MUTE;
	multi->controls[i] = control;
	i++;
	
	control.mix_control.id = MULTI_CONTROL_FIRSTID + i;
	control.mix_control.flags = B_MULTI_MIX_GAIN;
	control.mix_control.string = S_null;
	control.type = B_MIX_GAIN;
	strcpy(control.mix_control.name, "Gain");
	multi->controls[i] = control;
	id = control.mix_control.id;
	i++;
			
	// second channel	
	control.mix_control.id = MULTI_CONTROL_FIRSTID + i;
	control.mix_control.master = id;
	multi->controls[i] = control;
	i++;
	
	// nominal level (+4/-10)
	if (nominal) {
		control.mix_control.id = MULTI_CONTROL_FIRSTID + i;
		control.mix_control.master = MULTI_CONTROL_MASTERID;
		control.mix_control.flags = B_MULTI_MIX_ENABLE;
		control.mix_control.string = S_null;
		control.type = B_MIX_NOMINAL;
		strcpy(control.mix_control.name, "+4dB");
		multi->controls[i] = control;
		i++;
	}

	*index = i;
}


static status_t
echo_create_controls_list(multi_dev *multi)
{				
	uint32 	i = 0, index = 0, parent, parent2;
	echo_dev *card = (echo_dev*)multi->card;
			
	parent = echo_create_group_control(multi, &index, 0, S_OUTPUT, NULL);

	MIXER_AUDIO_CHANNEL channel;
	channel.wCardId = 0;
	channel.dwType = ECHO_BUS_OUT;
	for (i = 0; i < card->caps.wNumBussesOut / 2; i++) {
		channel.wChannel = i * 2;
		parent2 = echo_create_group_control(multi, &index, parent, S_null, "Output");

		echo_create_channel_control(multi, &index, parent2, 0, channel, 
			card->caps.dwBusOutCaps[i * 2] & ECHOCAPS_NOMINAL_LEVEL);
	}
	
	parent = echo_create_group_control(multi, &index, 0, S_INPUT, NULL);

	channel.dwType = ECHO_BUS_IN;
	for (i = 0; i < card->caps.wNumBussesIn / 2; i++) {
		channel.wChannel = i * 2;
		
		parent2 = echo_create_group_control(multi, &index, parent, S_null, "Input");
		
		echo_create_channel_control(multi, &index, parent2, 0, channel, 
			card->caps.dwBusInCaps[i * 2] & ECHOCAPS_NOMINAL_LEVEL);
	}

	multi->control_count = index;
	PRINT(("multi->control_count %lu\n", multi->control_count));
	return B_OK;
}


static status_t 
echo_get_mix(echo_dev *card, multi_mix_value_info * mmvi)
{
	int32 i;
	uint32 id;
	multi_mixer_control *control = NULL;
	for (i = 0; i < mmvi->item_count; i++) {
		id = mmvi->values[i].id - MULTI_CONTROL_FIRSTID;
		if (id < 0 || id >= card->multi.control_count) {
			PRINT(("echo_get_mix : invalid control id requested : %li\n", id));
			continue;
		}
		control = &card->multi.controls[id];
	
		if (control->mix_control.flags & B_MULTI_MIX_GAIN) {
			if (control->get) {
				float values[2];
				control->get(card, control->channel, control->type, values);
				if (control->mix_control.master == MULTI_CONTROL_MASTERID)
					mmvi->values[i].gain = values[0];
				else
					mmvi->values[i].gain = values[1];
			}			
		}
		
		if (control->mix_control.flags & B_MULTI_MIX_ENABLE && control->get) {
			float values[1];
			control->get(card, control->channel, control->type, values);
			mmvi->values[i].enable = (values[0] == 1.0);
		}
		
		if (control->mix_control.flags & B_MULTI_MIX_MUX && control->get) {
			float values[1];
			control->get(card, control->channel, control->type, values);
			mmvi->values[i].mux = (int32)values[0];
		}
	}
	return B_OK;
}


static status_t 
echo_set_mix(echo_dev *card, multi_mix_value_info * mmvi)
{
	int32 i;
	uint32 id;
	multi_mixer_control *control = NULL;
	for (i = 0; i < mmvi->item_count; i++) {
		id = mmvi->values[i].id - MULTI_CONTROL_FIRSTID;
		if (id < 0 || id >= card->multi.control_count) {
			PRINT(("echo_set_mix : invalid control id requested : %li\n", id));
			continue;
		}
		control = &card->multi.controls[id];
					
		if (control->mix_control.flags & B_MULTI_MIX_GAIN) {
			multi_mixer_control *control2 = NULL;
			if (i + 1 < mmvi->item_count) {
				id = mmvi->values[i + 1].id - MULTI_CONTROL_FIRSTID;
				if (id < 0 || id >= card->multi.control_count) {
					PRINT(("echo_set_mix : invalid control id requested : %li\n", id));
				} else {
					control2 = &card->multi.controls[id];
					if (control2->mix_control.master != control->mix_control.id)
						control2 = NULL;
				}
			}

			if (control->set) {
				float values[2];
				values[0] = 0.0;
				values[1] = 0.0;

				if (control->mix_control.master == MULTI_CONTROL_MASTERID)
					values[0] = mmvi->values[i].gain;
				else
					values[1] = mmvi->values[i].gain;
					
				if (control2 && control2->mix_control.master != MULTI_CONTROL_MASTERID)
					values[1] = mmvi->values[i + 1].gain;
					
				control->set(card, control->channel, control->type, values);
			}
			
			if (control2)
				i++;
		}
	
		if (control->mix_control.flags & B_MULTI_MIX_ENABLE && control->set) {
			float values[1];
			
			values[0] = mmvi->values[i].enable ? 1.0 : 0.0;
			control->set(card, control->channel, control->type, values);
		}
		
		if (control->mix_control.flags & B_MULTI_MIX_MUX && control->set) {
			float values[1];
			
			values[0] = (float)mmvi->values[i].mux;
			control->set(card, control->channel, control->type, values);
		}
	}
	return B_OK;
}


static status_t 
echo_list_mix_controls(echo_dev *card, multi_mix_control_info * mmci)
{
	multi_mix_control	*mmc;
	uint32 i;
	
	mmc = mmci->controls;
	if (mmci->control_count < 24)
		return B_ERROR;
			
	if (echo_create_controls_list(&card->multi) < B_OK)
		return B_ERROR;
	for (i = 0; i < card->multi.control_count; i++) {
		mmc[i] = card->multi.controls[i].mix_control;
	}
	
	mmci->control_count = card->multi.control_count;	
	return B_OK;
}


static status_t 
echo_list_mix_connections(echo_dev* card, multi_mix_connection_info* data)
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

	for (mode=ECHO_USE_PLAY; mode!=-1; 
		mode = (mode == ECHO_USE_PLAY) ? ECHO_USE_RECORD : -1) {
		LIST_FOREACH(stream, &((echo_dev*)multi->card)->streams, next) {
			if ((stream->use & mode) == 0)
				continue;
				
			if (stream->channels == 2)
				designations = B_CHANNEL_STEREO_BUS;
			else
				designations = B_CHANNEL_SURROUND_BUS;
			
			for (i = 0; i < stream->channels; i++) {
				chans[index].channel_id = index;
				chans[index].kind = (mode == ECHO_USE_PLAY) ? B_MULTI_OUTPUT_CHANNEL : B_MULTI_INPUT_CHANNEL;
				chans[index].designations = designations | chan_designations[i];
				chans[index].connectors = 0;
				index++;
			}
		}
		
		if (mode==ECHO_USE_PLAY) {
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

	strlcpy(data->friendly_name, card->caps.szName, sizeof(data->friendly_name));
	strlcpy(data->vendor_info, AUTHOR, sizeof(data->vendor_info));

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

	switch (current_settings.sample_rate) {
		case 192000: data->output_rates = data->input_rates = B_SR_192000; break;
		case 96000: data->output_rates = data->input_rates = B_SR_96000; break;
		case 48000: data->output_rates = data->input_rates = B_SR_48000; break;
		case 44100: data->output_rates = data->input_rates = B_SR_44100; break;
	}
	data->min_cvsr_rate = 0;
	data->max_cvsr_rate = 48000;

	switch (current_settings.bitsPerSample) {
		case 8: data->output_formats = data->input_formats = B_FMT_8BIT_U; break;
		case 16: data->output_formats = data->input_formats = B_FMT_16BIT; break;
		case 24: data->output_formats = data->input_formats = B_FMT_24BIT; break;
		case 32: data->output_formats = data->input_formats = B_FMT_32BIT; break;
	}
	data->lock_sources = B_MULTI_LOCK_INTERNAL;
	data->timecode_sources = 0;
	data->interface_flags = B_MULTI_INTERFACE_PLAYBACK | B_MULTI_INTERFACE_RECORD;
	data->start_latency = 3000;

	strcpy(data->control_panel, "");

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
	switch (current_settings.sample_rate) {
		case 192000: data->output.rate = data->input.rate = B_SR_192000; break;
		case 96000: data->output.rate = data->input.rate = B_SR_96000; break;
		case 48000: data->output.rate = data->input.rate = B_SR_48000; break;
		case 44100: data->output.rate = data->input.rate = B_SR_44100; break;
	}
	switch (current_settings.bitsPerSample) {
		case 8: data->input.format = data->output.format = B_FMT_8BIT_U; break;
		case 16: data->input.format = data->output.format = B_FMT_16BIT; break;
		case 24: data->input.format = data->output.format = B_FMT_24BIT; break;
		case 32: data->input.format = data->output.format = B_FMT_32BIT; break;
	}
	data->input.cvsr = data->output.cvsr = current_settings.sample_rate;
	return B_OK;
}


static status_t 
echo_get_buffers(echo_dev *card, multi_buffer_list *data)
{
	int32 i, j, channels;
	echo_stream *stream;
	
	LOG(("flags = %#x\n",data->flags));
	LOG(("request_playback_buffers = %#x\n",data->request_playback_buffers));
	LOG(("request_playback_channels = %#x\n",data->request_playback_channels));
	LOG(("request_playback_buffer_size = %#x\n",data->request_playback_buffer_size));
	LOG(("request_record_buffers = %#x\n",data->request_record_buffers));
	LOG(("request_record_channels = %#x\n",data->request_record_channels));
	LOG(("request_record_buffer_size = %#x\n",data->request_record_buffer_size));
	
	if (data->request_playback_buffers < current_settings.buffer_count ||
		data->request_record_buffers < current_settings.buffer_count) {
		LOG(("not enough channels/buffers\n"));
	}

	ASSERT(current_settings.buffer_count == 2);
	
	data->flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD; // XXX ???
		
	data->return_playback_buffers = current_settings.buffer_count;	/* playback_buffers[b][] */
	data->return_playback_channels = 0;	/* playback_buffers[][c] */
	data->return_playback_buffer_size = current_settings.buffer_frames;		/* frames */

	LIST_FOREACH(stream, &card->streams, next) {
		if ((stream->use & ECHO_USE_PLAY) == 0)
			continue;
		LOG(("get_buffers pipe %d\n", stream->pipe));
		channels = data->return_playback_channels;
		data->return_playback_channels += stream->channels;
		if (data->request_playback_channels < data->return_playback_channels) {
			LOG(("not enough channels\n"));
		}	
		for (i = 0; i < current_settings.buffer_count; i++)
			for (j=0; j<stream->channels; j++)
				echo_stream_get_nth_buffer(stream, j, i, 
					&data->playback_buffers[i][channels+j].base,
					&data->playback_buffers[i][channels+j].stride);
	}
	
	data->return_record_buffers = current_settings.buffer_count;
	data->return_record_channels = 0;
	data->return_record_buffer_size = current_settings.buffer_frames;	/* frames */

	LIST_FOREACH(stream, &card->streams, next) {
		if ((stream->use & ECHO_USE_PLAY) != 0)
			continue;
		LOG(("get_buffers pipe %d\n", stream->pipe));
		channels = data->return_record_channels;
		data->return_record_channels += stream->channels;
		if (data->request_record_channels < data->return_record_channels) {
			LOG(("not enough channels\n"));
		}
		for (i = 0; i < current_settings.buffer_count; i++)
			for (j = 0; j < stream->channels; j++)
				echo_stream_get_nth_buffer(stream, j, i, 
					&data->record_buffers[i][channels + j].base,
					&data->record_buffers[i][channels + j].stride);
	}
		
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
	stream->frames_count += current_settings.buffer_frames;
	stream->buffer_cycle = (stream->trigblk 
		+ stream->blkmod) % stream->blkmod;
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
	stream->frames_count += current_settings.buffer_frames;
	stream->buffer_cycle = (stream->trigblk 
		+ stream->blkmod - 1) % stream->blkmod;
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
	echo_stream *pstream, *rstream, *stream;
	multi_buffer_info buffer_info;
	
#ifdef __HAIKU__
	if (user_memcpy(&buffer_info, data, sizeof(buffer_info)) < B_OK)
		return B_BAD_ADDRESS;
#else
	memcpy(&buffer_info, data, sizeof(buffer_info));
#endif

	buffer_info.flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD;
	
	LIST_FOREACH(stream, &card->streams, next) {
		if ((stream->state & ECHO_STATE_STARTED) != 0)
			continue;
		echo_stream_start(stream, 
			((stream->use & ECHO_USE_PLAY) == 0) ? echo_record_inth : echo_play_inth, stream);
	}
	
	if (acquire_sem_etc(card->buffer_ready_sem, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, 50000)
		== B_TIMED_OUT) {
		LOG(("buffer_exchange timeout ff\n"));
	}
	
	status = lock();
	
	LIST_FOREACH(pstream, &card->streams, next) {
		if ((pstream->use & ECHO_USE_PLAY) == 0 || 
			(pstream->state & ECHO_STATE_STARTED) == 0)
			continue;
		if (pstream->update_needed)	
			break;
	}
	
	LIST_FOREACH(rstream, &card->streams, next) {
		if ((rstream->use & ECHO_USE_RECORD) == 0 ||
			(rstream->state & ECHO_STATE_STARTED) == 0)
			continue;
		if (rstream->update_needed)	
			break;
	}
	
	if (!pstream)
		pstream = card->pstream;
	if (!rstream)
		rstream = card->rstream;
	
	/* do playback */
	buffer_info.playback_buffer_cycle = pstream->buffer_cycle;
	buffer_info.played_real_time = pstream->real_time;
	buffer_info.played_frames_count = pstream->frames_count;
	buffer_info._reserved_0 = pstream->first_channel;
	pstream->update_needed = false;
	
	/* do record */
	buffer_info.record_buffer_cycle = rstream->buffer_cycle;
	buffer_info.recorded_frames_count = rstream->frames_count;
	buffer_info.recorded_real_time = rstream->real_time;
	buffer_info._reserved_1 = rstream->first_channel;
	rstream->update_needed = false;
	unlock(status);

#ifdef __HAIKU__
	if (user_memcpy(data, &buffer_info, sizeof(buffer_info)) < B_OK)
		return B_BAD_ADDRESS;
#else
	memcpy(data, &buffer_info, sizeof(buffer_info));
#endif

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
	
#ifdef CARDBUS
	// Check
	if (card->plugged == false) {
		LOG(("device %s unplugged\n", card->name));
		return B_ERROR;
	}
#endif

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
	int i, first_record_channel;
	echo_stream *stream = NULL;
	
	LOG(("echo_open()\n"));
	
#ifdef CARDBUS
	LIST_FOREACH(card, &devices, next) {
		if (!strcmp(card->name, name)) {
			break;
		}
	}
#else
	for (i = 0; i < num_cards; i++) {
		if (!strcmp(cards[i].name, name)) {
			card = &cards[i];
		}
	}
#endif
	
	if (card == NULL) {
		LOG(("open() card not found %s\n", name));
#ifdef CARDBUS
		LIST_FOREACH(card, &devices, next) {
			LOG(("open() card available %s\n", card->name));
		}
#else
		for (int ix=0; ix<num_cards; ix++) {
			LOG(("open() card available %s\n", cards[ix].name));
		}
#endif
		return B_ERROR;
	}

#ifdef CARDBUS
	if (card->plugged == false) {
		LOG(("device %s unplugged\n", name));
		return B_ERROR;
	}
#endif
		
	LOG(("open() got card\n"));
	
	if (card->pstream != NULL)
		return B_ERROR;
	if (card->rstream != NULL)
		return B_ERROR;
			
	*cookie = card;
	card->multi.card = card;
#ifdef CARDBUS
	card->opened = true;
#endif

	void *settings_handle;
	// get driver settings
	settings_handle = load_driver_settings ("echo.settings");
	if (settings_handle != NULL) {
		const char* item;
		char* end;
		uint32 value;

		item = get_driver_parameter (settings_handle, "channels", NULL, NULL);
		if (item) {
			value = strtoul (item, &end, 0);
			if (*end == '\0') current_settings.channels = value;
		}
		PRINT(("channels %u\n", current_settings.channels));
		
		item = get_driver_parameter (settings_handle, "bitsPerSample", NULL, NULL);
		if (item) {
			value = strtoul (item, &end, 0);
			if (*end == '\0') current_settings.bitsPerSample = value;
		}
		PRINT(("bitsPerSample %u\n", current_settings.bitsPerSample));
		
		item = get_driver_parameter (settings_handle, "sample_rate", NULL, NULL);
		if (item) {
			value = strtoul (item, &end, 0);
			if (*end == '\0') current_settings.sample_rate = value;
		}
		PRINT(("sample_rate %lu\n", current_settings.sample_rate));
		
		item = get_driver_parameter (settings_handle, "buffer_frames", NULL, NULL);
		if (item) {
			value = strtoul (item, &end, 0);
			if (*end == '\0') current_settings.buffer_frames = value;
		}
		PRINT(("buffer_frames %lu\n", current_settings.buffer_frames));

		item = get_driver_parameter (settings_handle, "buffer_count", NULL, NULL);
		if (item) {
			value = strtoul (item, &end, 0);
			if (*end == '\0') current_settings.buffer_count = value;
		}
		PRINT(("buffer_count %lu\n", current_settings.buffer_count));
		
		unload_driver_settings (settings_handle);
	}
		
	LOG(("creating play streams\n"));

	i = card->caps.wNumPipesOut - 2;
	first_record_channel = card->caps.wNumPipesOut;
#ifdef ECHO3G_FAMILY
	if (current_settings.sample_rate > 50000) {
		i = card->caps.wFirstDigitalBusOut;
		first_record_channel = card->caps.wFirstDigitalBusOut + 2;
	}
#endif
	
	for (; i >= 0 ; i -= 2) {
		stream = echo_stream_new(card, ECHO_USE_PLAY, current_settings.buffer_frames, current_settings.buffer_count);
		if (!card->pstream)
			card->pstream = stream;
		echo_stream_set_audioparms(stream, current_settings.channels, 
			current_settings.bitsPerSample, current_settings.sample_rate, i);
		stream->first_channel = i;
	}
	
	LOG(("creating record streams\n"));
	i = card->caps.wNumPipesIn - 2;
#ifdef ECHO3G_FAMILY
	if (current_settings.sample_rate > 50000) {
		i = card->caps.wFirstDigitalBusIn;
	}
#endif
	
	for (; i >= 0; i-=2) {
		stream = echo_stream_new(card, ECHO_USE_RECORD, current_settings.buffer_frames, current_settings.buffer_count);
		if (!card->rstream)
			card->rstream = stream;
		echo_stream_set_audioparms(stream, current_settings.channels, 
			current_settings.bitsPerSample, current_settings.sample_rate, i);
		stream->first_channel = i + first_record_channel;
	}
	
	card->buffer_ready_sem = create_sem(0, "pbuffer ready");
	
	LOG(("creating channels list\n"));
	echo_create_channels_list(&card->multi);

	return B_OK;
}


static status_t
echo_close(void* cookie)
{
	LOG(("close()\n"));
#ifdef CARDBUS
	echo_dev *card = (echo_dev *) cookie;
	card->opened = false;
#endif
		
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
	
	while (!LIST_EMPTY(&card->streams)) {
		echo_stream_delete(LIST_FIRST(&card->streams));
	}

	card->pstream = NULL;
	card->rstream = NULL;
	
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
