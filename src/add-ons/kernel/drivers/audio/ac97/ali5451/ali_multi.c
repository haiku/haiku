/*
 * Copyright 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 *
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>
#include <drivers/PCI.h>
#include <hmulti_audio.h>
#include <string.h>

#include "ac97.h"
#include "queue.h"
#include "driver.h"
#include "util.h"
#include "ali_hardware.h"
#include "ali_multi.h"
#include "debug.h"


#define ALI_SUPPORTED_OUTPUT_CHANNELS 2
#define ALI_SUPPORTED_INPUT_CHANNELS 2

#define ALI_VALID_OUTPUT_SAMPLE_RATES (B_SR_8000 | B_SR_11025 | B_SR_12000 | \
	B_SR_16000 | B_SR_22050 | B_SR_24000 | B_SR_32000 | B_SR_44100 | B_SR_48000)
#define ALI_VALID_OUTPUT_FORMATS (B_FMT_16BIT | B_FMT_8BIT_S | B_FMT_8BIT_U)
#define ALI_VALID_INPUT_SAMPLE_RATES (B_SR_48000)
#define ALI_VALID_INPUT_FORMATS ALI_VALID_OUTPUT_FORMATS

#define ALI_MIXER_GRANULARITY 1.5f

#define ALI_MIX_OUTPUT_ID 0x000
#define ALI_MIX_INPUT_ID 0x100

#define ALI_MIX_GROUP_ID 1
#define ALI_MIX_MUTE_ID 2
#define ALI_MIX_LEFT_ID 3
#define ALI_MIX_RIGHT_ID 4
#define ALI_MIX_MONO_ID ALI_MIX_RIGHT_ID
#define ALI_MIX_MIC_BOOST_ID 5
#define ALI_MIX_INPUT_SELECTOR_ID 6
#define ALI_MIX_RECORD_INPUT 15

#define ALI_MIX_FT_STEREO 1
#define ALI_MIX_FT_MUTE 2
#define ALI_MIX_FT_MICBST 4
#define ALI_MIX_FT_GAIN 8
#define ALI_MIX_FT_INSEL 16

typedef struct {
	int32 string_id;
	const char *name;
	uint8 reg;
	float min_gain;
	float max_gain;
	uint8 features;
} ali_ac97_mixer_info;

#define ALI_OUTPUT_MIXERS 7
#define ALI_INPUT_MIXERS 1
#define ALI_ALL_MIXERS (ALI_OUTPUT_MIXERS + ALI_INPUT_MIXERS)

static const ali_ac97_mixer_info ali_ac97_mixers[ALI_ALL_MIXERS] = {
	{S_MASTER, NULL, AC97_MASTER_VOLUME, -46.5f, 0.0f, ALI_MIX_FT_STEREO | ALI_MIX_FT_MUTE},
	{S_WAVE, NULL, AC97_PCM_OUT_VOLUME, -34.5f, 12.0f, ALI_MIX_FT_STEREO | ALI_MIX_FT_MUTE},
	{S_HEADPHONE, NULL, AC97_AUX_OUT_VOLUME, -34.5f, 12.0f, ALI_MIX_FT_STEREO | ALI_MIX_FT_MUTE},
	{S_CD, NULL, AC97_CD_VOLUME, -34.5f, 12.0f, ALI_MIX_FT_STEREO | ALI_MIX_FT_MUTE},
	{S_LINE, NULL, AC97_LINE_IN_VOLUME, -34.5f, 12.0f, ALI_MIX_FT_STEREO | ALI_MIX_FT_MUTE},
	{S_MIC, NULL, AC97_MIC_VOLUME, -34.5f, 12.0f, ALI_MIX_FT_MUTE | ALI_MIX_FT_MICBST},
	{S_AUX, NULL, AC97_AUX_IN_VOLUME, -34.5f, 12.0f, ALI_MIX_FT_STEREO | ALI_MIX_FT_MUTE},
	{S_null, "Record", AC97_RECORD_GAIN, 0.0f, 22.5f,
		ALI_MIX_FT_STEREO | ALI_MIX_FT_MUTE | ALI_MIX_FT_GAIN | ALI_MIX_FT_INSEL},
};

typedef struct {
	int32 string_id;
	const char *name;
} ali_ac97_mux_input_info;

#define ALI_MUX_INPUTS 7

static const ali_ac97_mux_input_info ali_ac97_mux_inputs[ALI_MUX_INPUTS] = {
	{S_MIC, NULL},
	{S_CD, NULL},
	{S_MUTE, NULL},
	{S_AUX, NULL},
	{S_LINE, NULL},
	{S_STEREO_MIX, NULL},
	{S_MONO_MIX, NULL},
};

static const multi_channel_info channel_descriptions[] = {
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
get_description(void *cookie, multi_description *data)
{
	multi_description description;
	if (user_memcpy(&description, data, sizeof(multi_description)) != B_OK)
		return B_BAD_ADDRESS;

	description.interface_version = B_CURRENT_INTERFACE_VERSION;
	description.interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strcpy(description.friendly_name, "ALI M5451");
	strcpy(description.vendor_info, "Krzysztof Ćwiertnia");

	description.output_channel_count = ALI_SUPPORTED_OUTPUT_CHANNELS;
	description.input_channel_count = ALI_SUPPORTED_INPUT_CHANNELS;
	description.output_bus_channel_count = ALI_SUPPORTED_OUTPUT_CHANNELS;
	description.input_bus_channel_count = ALI_SUPPORTED_INPUT_CHANNELS;
	description.aux_bus_channel_count = 0;

	description.output_formats = ALI_VALID_OUTPUT_FORMATS;
	description.output_rates = ALI_VALID_OUTPUT_SAMPLE_RATES;

	description.input_formats = ALI_VALID_INPUT_FORMATS;
	description.input_rates = ALI_VALID_INPUT_SAMPLE_RATES;

	description.lock_sources = B_MULTI_LOCK_INTERNAL;
	description.timecode_sources = 0;
	description.interface_flags = B_MULTI_INTERFACE_PLAYBACK|B_MULTI_INTERFACE_RECORD;
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
get_enabled_channels(ali_dev *card, multi_channel_enable *data)
{
	B_SET_CHANNEL(data->enable_bits, 0, true);
	B_SET_CHANNEL(data->enable_bits, 1, true);
	B_SET_CHANNEL(data->enable_bits, 2, true);
	B_SET_CHANNEL(data->enable_bits, 3, true);

	data->lock_source = B_MULTI_LOCK_INTERNAL;

	return B_OK;
}


static status_t
get_global_format(ali_dev *card, multi_format_info *data)
{
	data->output_latency = 0;
	data->input_latency = 0;
	data->timecode_kind = 0;

	data->output.format = card->global_output_format.format;
	data->output.rate = card->global_output_format.rate;

	data->input.format = card->global_input_format.format;
	data->input.rate = card->global_input_format.rate;

	return B_OK;
}


static status_t
set_global_format(ali_dev *card, multi_format_info *data)
{
	if ((ALI_VALID_OUTPUT_FORMATS & data->output.format) != 0)
		card->global_output_format.format = data->output.format;
	if ((ALI_VALID_OUTPUT_SAMPLE_RATES & data->output.rate) != 0)
		card->global_output_format.rate = data->output.rate;

	if ((ALI_VALID_INPUT_FORMATS & data->input.format) != 0)
		card->global_input_format.format = data->input.format;
	if ((ALI_VALID_INPUT_SAMPLE_RATES & data->input.rate) != 0)
		card->global_input_format.rate = data->input.rate;

	if (!ali_stream_prepare(card->playback_stream, 2,
			card->global_output_format.format,
			util_sample_rate_in_bits(card->global_output_format.rate)))
		return B_ERROR;
	if (!ali_stream_prepare(card->record_stream, 2,
			card->global_input_format.format,
			util_sample_rate_in_bits(card->global_input_format.rate)))
			return B_ERROR;

	return B_OK;
}


static void
get_mix_mixer(ali_dev *card, multi_mix_value *mmv, int32 id)
{
	uint8 mixer_id = (uint8) id >> 4, gadget_id = (uint8) id & 0xf;
	const ali_ac97_mixer_info *mixer_info;

	if (ALI_ALL_MIXERS <= mixer_id)
		return;
	mixer_info = &ali_ac97_mixers[mixer_id];

	switch (gadget_id) {
		case ALI_MIX_MUTE_ID:
			mmv->u.enable = ((ac97_reg_cached_read(card->codec, mixer_info->reg)
					& 0x8000) == 0x8000);
			break;

		case ALI_MIX_LEFT_ID:
		case ALI_MIX_RIGHT_ID:
			{
				float val = (ALI_MIXER_GRANULARITY
						* ((ac97_reg_cached_read(card->codec, mixer_info->reg)
							>> ((gadget_id==ALI_MIX_LEFT_ID)?8:0)) & 0x1f));
				mmv->u.gain = ((mixer_info->features&ALI_MIX_FT_GAIN)
					? mixer_info->min_gain + val : mixer_info->max_gain - val);
			}
			break;

		case ALI_MIX_MIC_BOOST_ID:
			mmv->u.enable = ((ac97_reg_cached_read(card->codec, mixer_info->reg)
					& 0x40) == 0x40);
			break;

		case ALI_MIX_INPUT_SELECTOR_ID:
			mmv->u.mux = (ac97_reg_cached_read(card->codec,
					AC97_RECORD_SELECT) & 0x7);
			if (ALI_MUX_INPUTS <= mmv->u.mux) {
				mmv->u.mux = 0x2;
				ac97_reg_cached_write(card->codec, AC97_RECORD_SELECT, 0x202);
			}
			break;
	}
}


static status_t
get_mix(ali_dev *card, multi_mix_value_info *mmvi)
{
	int32 i;

	for (i = 0; i < mmvi->item_count; i++)
		get_mix_mixer(card, &(mmvi->values[i]),
			mmvi->values[i].id - MULTI_AUDIO_BASE_ID);

	return B_OK;
}


static void
set_mix_mixer(ali_dev *card, multi_mix_value *mmv, int32 id)
{
	uint8 mixer_id = (uint8) id >> 4, gadget_id = (uint8) id & 0xf;
	const ali_ac97_mixer_info *mixer_info;
	uint16 val;

	if (ALI_ALL_MIXERS <= mixer_id)
		return;
	mixer_info = &ali_ac97_mixers[mixer_id];

	switch (gadget_id) {
		case ALI_MIX_MUTE_ID:
			val = ac97_reg_cached_read(card->codec, mixer_info->reg);
			val = mmv->u.enable ? val | 0x8000 : val & 0x7fff;
			ac97_reg_cached_write(card->codec, mixer_info->reg, val);
			break;

		case ALI_MIX_LEFT_ID:
		case ALI_MIX_RIGHT_ID:
			val = ac97_reg_cached_read(card->codec, mixer_info->reg);
			val &= (gadget_id == ALI_MIX_LEFT_ID) ? 0x805f : 0x9f00;
			val |= (uint16) (((mixer_info->features&ALI_MIX_FT_GAIN)
					? mixer_info->min_gain + mmv->u.gain
					: mixer_info->max_gain - mmv->u.gain)
						/ ALI_MIXER_GRANULARITY)
							<< ((gadget_id == ALI_MIX_LEFT_ID) ? 8 : 0);
			ac97_reg_cached_write(card->codec, mixer_info->reg, val);
			break;

		case ALI_MIX_MIC_BOOST_ID:
			val = ac97_reg_cached_read(card->codec, mixer_info->reg);
			val = mmv->u.enable ? val | 0x40 : val & 0xffbf;
			ac97_reg_cached_write(card->codec, mixer_info->reg, val);
			break;

		case ALI_MIX_INPUT_SELECTOR_ID:
			val = (mmv->u.mux << 8) | mmv->u.mux;
			ac97_reg_cached_write(card->codec, AC97_RECORD_SELECT, val);
			break;
	}
}


static status_t
set_mix(ali_dev *card, multi_mix_value_info *mmvi)
{
	int32 i;

	for (i = 0; i < mmvi->item_count; i++)
		set_mix_mixer(card, &(mmvi->values[i]),
			mmvi->values[i].id - MULTI_AUDIO_BASE_ID);

	return B_OK;
}


static int32
create_group_control(multi_mix_control *multi, int32 *idx, int32 id,
	int32 parent, int32 string_id, const char *name)
{
	multi_mix_control *control = multi + *idx;

	control->id = MULTI_AUDIO_BASE_ID + id;
	control->parent = parent;
	control->flags = B_MULTI_MIX_GROUP;
	control->master = MULTI_AUDIO_MASTER_ID;
	control->string = string_id;
	if (string_id == S_null && name)
		strcpy(control->name, name);

	(*idx)++;

	return control->id;
}


static void
create_vol_control(multi_mix_control *multi, int32 *idx, int32 id, int32 parent)
{
	const ali_ac97_mixer_info *mixer_info;
	multi_mix_control *control;

	id &= 0xf;

	if (ALI_ALL_MIXERS <= id)
		return;
	mixer_info = &ali_ac97_mixers[id];

	id <<= 4;

	parent = create_group_control(multi, idx, (parent & 0x100) | id
		| ALI_MIX_GROUP_ID,	parent, mixer_info->string_id, mixer_info->name);

	control = multi + *idx;

	if (mixer_info->features & ALI_MIX_FT_MUTE) {
		control->id = MULTI_AUDIO_BASE_ID | (parent & 0x100) | id
			| ALI_MIX_MUTE_ID;
		control->parent = parent;
		control->flags = B_MULTI_MIX_ENABLE;
		control->master = MULTI_AUDIO_MASTER_ID;
		control->string = S_MUTE;

		control++;
		(*idx)++;
	}

	control->id = MULTI_AUDIO_BASE_ID | (parent & 0x100) | id
		| ((mixer_info->features&ALI_MIX_FT_STEREO)
			?ALI_MIX_LEFT_ID:ALI_MIX_MONO_ID);
	control->parent = parent;
	control->flags = B_MULTI_MIX_GAIN;
	control->master = MULTI_AUDIO_MASTER_ID;
	control->string = S_GAIN;
		// S_null; ? FIXME

	control->u.gain.min_gain = mixer_info->min_gain;
	control->u.gain.max_gain = mixer_info->max_gain;
	control->u.gain.granularity = ALI_MIXER_GRANULARITY;

	if (mixer_info->features & ALI_MIX_FT_STEREO) {
		control[1] = control[0];
		control++;
		(*idx)++;

		control->master = control->id;
		control->id = MULTI_AUDIO_BASE_ID | (parent & 0x100) | id
			| ALI_MIX_RIGHT_ID;
	}

	control++;
	(*idx)++;

	if (mixer_info->features & ALI_MIX_FT_MICBST) {
		control->id = MULTI_AUDIO_BASE_ID | (parent & 0x100) | id
			| ALI_MIX_MIC_BOOST_ID;
		control->parent = parent;
		control->flags = B_MULTI_MIX_ENABLE;
		control->master = MULTI_AUDIO_MASTER_ID;
		control->string = S_null;
		strcpy(control->name, "+20dB");

		control++;
		(*idx)++;
	}

	if (mixer_info->features & ALI_MIX_FT_INSEL) {
		int32 is_parent;
		const ali_ac97_mux_input_info *mux_input;

		is_parent = control->id = MULTI_AUDIO_BASE_ID | (parent & 0x100)
			| id | ALI_MIX_INPUT_SELECTOR_ID;
		control->parent = parent;
		control->flags = B_MULTI_MIX_MUX;
		control->master = MULTI_AUDIO_MASTER_ID;
		control->string = S_INPUT;

		control++;
		(*idx)++;

		for (id = 0; id < ALI_MUX_INPUTS; id++) {
			mux_input = &ali_ac97_mux_inputs[id];

			control->id = MULTI_AUDIO_BASE_ID | (parent & 0x100) | (id<<4)
				| ALI_MIX_RECORD_INPUT;
			control->parent = is_parent;
			control->flags = B_MULTI_MIX_MUX_VALUE;
			control->master = MULTI_AUDIO_MASTER_ID;
			control->string = mux_input->string_id;
			if (control->string == S_null && mux_input->name)
				strcpy(control->name, mux_input->name);

			control++;
			(*idx)++;
		}
	}
}


static status_t
list_mix_controls(ali_dev *card, multi_mix_control_info *mmci)
{
	int32 index = 0, parent, i;

	parent = create_group_control(mmci->controls, &index, ALI_MIX_OUTPUT_ID, 0,
		S_OUTPUT, NULL);
	for (i = 0; i < ALI_OUTPUT_MIXERS; i++)
		create_vol_control(mmci->controls, &index, i, parent);

	parent = create_group_control(mmci->controls, &index, ALI_MIX_INPUT_ID, 0,
		S_INPUT, NULL);
	for (i = ALI_OUTPUT_MIXERS; i < ALI_OUTPUT_MIXERS + ALI_INPUT_MIXERS; i++)
		create_vol_control(mmci->controls, &index, i, parent);

	mmci->control_count = index;

	return B_OK;
}


static status_t
get_buffers(ali_dev *card, multi_buffer_list *data)
{
	uint8 buf_ndx, ch_ndx;

	data->return_playback_buffers = card->playback_stream->buf_count;
	data->return_playback_channels = card->playback_stream->channels;
	data->return_playback_buffer_size = card->playback_stream->buf_frames;

	for (buf_ndx = 0; buf_ndx<card->playback_stream->buf_count; buf_ndx++)
		for (ch_ndx = 0; ch_ndx < card->playback_stream->channels; ch_ndx++)
			ali_stream_get_buffer_part(card->playback_stream, ch_ndx, buf_ndx,
				&data->playback_buffers[buf_ndx][ch_ndx].base,
				&data->playback_buffers[buf_ndx][ch_ndx].stride);

	data->return_record_buffers = card->record_stream->buf_count;
	data->return_record_channels = card->record_stream->channels;
	data->return_record_buffer_size = card->record_stream->buf_frames;

	for (buf_ndx = 0; buf_ndx < card->record_stream->buf_count; buf_ndx++)
		for (ch_ndx = 0; ch_ndx < card->record_stream->channels; ch_ndx++)
			ali_stream_get_buffer_part(card->record_stream, ch_ndx, buf_ndx,
				&data->record_buffers[buf_ndx][ch_ndx].base,
				&data->record_buffers[buf_ndx][ch_ndx].stride);

	return B_OK;
}


static status_t
buffer_exchange(ali_dev *card, multi_buffer_info *info)
{
	status_t res;
	ali_stream *play_s, *rec_s;

	multi_buffer_info buffer_info;
	if (user_memcpy(&buffer_info, info, sizeof(multi_buffer_info)) != B_OK)
		return B_BAD_ADDRESS;

	play_s = card->playback_stream;
	rec_s = card->record_stream;

	if (!play_s || !rec_s)
		return B_ERROR;

	if (!play_s->is_playing)
		if (!ali_stream_start(play_s))
			return B_ERROR;
	if (!rec_s->is_playing)
		if (!ali_stream_start(rec_s))
			return B_ERROR;

	res = acquire_sem_etc(card->sem_buf_ready, 1, B_CAN_INTERRUPT, 0);
	if (res != B_OK) {
		TRACE("buffer_exchange: error acquiring semaphore\n");
		return res;
	}

	LOCK(card->lock_sts);

	buffer_info.played_frames_count = play_s->frames_count;
	buffer_info.played_real_time = play_s->real_time;
	buffer_info.playback_buffer_cycle = play_s->buffer_cycle;

	buffer_info.recorded_frames_count = rec_s->frames_count;
	buffer_info.recorded_real_time = rec_s->real_time;
	buffer_info.record_buffer_cycle = rec_s->buffer_cycle;

	UNLOCK(card->lock_sts);

	if (user_memcpy(info, &buffer_info, sizeof(multi_buffer_info)) != B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


static status_t
force_stop(ali_dev *card)
{
	ali_stream *stream;

	TRACE("force_stop\n");

	LIST_FOREACH(stream, &card->streams, next) {
		ali_stream_stop(stream);
	}

	return B_OK;
}


status_t
multi_audio_control(void *cookie, uint32 op, void *arg, size_t len)
{
	switch (op) {
		case B_MULTI_GET_DESCRIPTION: return get_description(cookie, arg);

		case B_MULTI_GET_EVENT_INFO: return B_ERROR;
		case B_MULTI_SET_EVENT_INFO: return B_ERROR;
		case B_MULTI_GET_EVENT: return B_ERROR;

		case B_MULTI_GET_ENABLED_CHANNELS: return get_enabled_channels(cookie, arg);
		case B_MULTI_SET_ENABLED_CHANNELS: return B_OK;
		case B_MULTI_GET_GLOBAL_FORMAT: return get_global_format(cookie, arg);
		case B_MULTI_SET_GLOBAL_FORMAT: return set_global_format(cookie, arg);

		case B_MULTI_GET_CHANNEL_FORMATS: return B_ERROR;
		case B_MULTI_SET_CHANNEL_FORMATS: return B_ERROR;
		case B_MULTI_GET_MIX: return get_mix(cookie, arg);
		case B_MULTI_SET_MIX: return set_mix(cookie, arg);

		case B_MULTI_LIST_MIX_CHANNELS: return B_ERROR;
		case B_MULTI_LIST_MIX_CONTROLS: return list_mix_controls(cookie, arg);
		case B_MULTI_LIST_MIX_CONNECTIONS: return B_ERROR;

		case B_MULTI_GET_BUFFERS: return get_buffers(cookie, arg);
		case B_MULTI_SET_BUFFERS: return B_ERROR;
			// do not support in-software buffers

		case B_MULTI_SET_START_TIME: return B_ERROR;
		case B_MULTI_BUFFER_EXCHANGE: return buffer_exchange(cookie, arg);
		case B_MULTI_BUFFER_FORCE_STOP:	return force_stop(cookie);
	}

	return B_BAD_VALUE;
}
