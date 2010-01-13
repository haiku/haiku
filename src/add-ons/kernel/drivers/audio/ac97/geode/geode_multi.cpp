/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	Jérôme Duval (korli@users.berlios.de)
 */


#include "hmulti_audio.h"
#include "driver.h"


#ifdef TRACE
#	undef TRACE
#endif

#define TRACE_MULTI_AUDIO
#ifdef TRACE_MULTI_AUDIO
#	define TRACE(a...) dprintf("\33[34mgeode:\33[0m " a)
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
		case B_FMT_20BIT:
		case B_FMT_24BIT:
		case B_FMT_32BIT:
		case B_FMT_FLOAT:
			return 4;

		default:
			return -1;
	}
}


static status_t
get_description(geode_controller* controller, multi_description* data)
{
	data->interface_version = B_CURRENT_INTERFACE_VERSION;
	data->interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strcpy(data->friendly_name, "Geode");
	strcpy(data->vendor_info, "Haiku");

	int32 inChannels = 0;
	if (controller->record_stream != NULL)
		inChannels = 2;

	int32 outChannels = 0;
	if (controller->playback_stream != NULL)
		outChannels = 2;

	data->output_channel_count = outChannels;
	data->output_bus_channel_count = outChannels;
	data->input_channel_count = inChannels;
	data->input_bus_channel_count = inChannels;
	data->aux_bus_channel_count = 0;

	dprintf("%s: request_channel_count: %ld\n", __func__,
		data->request_channel_count);

	if (data->request_channel_count >= (int)(sizeof(sChannels)
			/ sizeof(sChannels[0]))) {
		memcpy(data->channels, &sChannels, sizeof(sChannels));
	}

	/* determine output/input rates */
	data->output_rates = B_SR_48000;
	data->input_rates = B_SR_48000;

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

	strcpy(data->control_panel, "");

	return B_OK;
}


static status_t
get_enabled_channels(geode_controller* controller, multi_channel_enable* data)
{
	B_SET_CHANNEL(data->enable_bits, 0, true);
	B_SET_CHANNEL(data->enable_bits, 1, true);
	B_SET_CHANNEL(data->enable_bits, 2, true);
	B_SET_CHANNEL(data->enable_bits, 3, true);
	data->lock_source = B_MULTI_LOCK_INTERNAL;

	return B_OK;
}


static status_t
get_global_format(geode_controller* controller, multi_format_info* data)
{
	data->output_latency = 0;
	data->input_latency = 0;
	data->timecode_kind = 0;

	if (controller->playback_stream != NULL) {
		data->output.format = controller->playback_stream->sample_format;
		data->output.rate = controller->playback_stream->sample_rate;
	} else {
		data->output.format = 0;
		data->output.rate = 0;
	}

	if (controller->record_stream != NULL) {
		data->input.format = controller->record_stream->sample_format;
		data->input.rate = controller->record_stream->sample_format;
	} else {
		data->input.format = 0;
		data->input.rate = 0;
	}

	return B_OK;
}


static status_t
set_global_format(geode_controller* controller, multi_format_info* data)
{
	// TODO: it looks like we're not supposed to fail; fix this!
#if 0
	if ((data->output.format & audioGroup->supported_formats) == 0)
		|| (data->output.rate & audioGroup->supported_rates) == 0)
		return B_BAD_VALUE;
#endif

	if (controller->playback_stream != NULL) {
		controller->playback_stream->sample_format = data->output.format;
		controller->playback_stream->sample_rate = data->output.rate;
		controller->playback_stream->sample_size = format2size(
			controller->playback_stream->sample_format);
	}

	if (controller->record_stream != NULL) {
		controller->record_stream->sample_rate = data->input.rate;
		controller->record_stream->sample_format = data->input.format;
		controller->record_stream->sample_size = format2size(
			controller->record_stream->sample_format);
	}

	return B_OK;
}


static void
geode_ac97_get_mix(geode_controller *controller, const void *cookie, int32 type, float *values) {
	ac97_source_info *info = (ac97_source_info *)cookie;
	uint16 value, mask;
	float gain;

	switch(type) {
		case B_MIX_GAIN:
			value = ac97_reg_cached_read(controller->ac97, info->reg);
			//TRACE("B_MIX_GAIN value : %u\n", value);
			if (info->type & B_MIX_STEREO) {
				mask = ((1 << (info->bits + 1)) - 1) << 8;
				gain = ((value & mask) >> 8) * info->granularity;
				if (info->polarity == 1)
					values[0] = info->max_gain - gain;
				else
					values[0] = gain - info->min_gain;

				mask = ((1 << (info->bits + 1)) - 1);
				gain = (value & mask) * info->granularity;
				if (info->polarity == 1)
					values[1] = info->max_gain - gain;
				else
					values[1] = gain - info->min_gain;
			} else {
				mask = ((1 << (info->bits + 1)) - 1);
				gain = (value & mask) * info->granularity;
				if (info->polarity == 1)
					values[0] = info->max_gain - gain;
				else
					values[0] = gain - info->min_gain;
			}
			break;
		case B_MIX_MUTE:
			mask = ((1 << 1) - 1) << 15;
			value = ac97_reg_cached_read(controller->ac97, info->reg);
			//TRACE("B_MIX_MUTE value : %u\n", value);
			value &= mask;
			values[0] = ((value >> 15) == 1) ? 1.0 : 0.0;
			break;
		case B_MIX_MICBOOST:
			mask = ((1 << 1) - 1) << 6;
			value = ac97_reg_cached_read(controller->ac97, info->reg);
			//TRACE("B_MIX_MICBOOST value : %u\n", value);
			value &= mask;
			values[0] = ((value >> 6) == 1) ? 1.0 : 0.0;
			break;
		case B_MIX_MUX:
			mask = ((1 << 3) - 1);
			value = ac97_reg_cached_read(controller->ac97, AC97_RECORD_SELECT);
			value &= mask;
			//TRACE("B_MIX_MUX value : %u\n", value);
			values[0] = (float)value;
			break;
	}
}


static void
geode_ac97_set_mix(geode_controller *controller, const void *cookie, int32 type, float *values) {
	ac97_source_info *info = (ac97_source_info *)cookie;
	uint16 value, mask;
	float gain;

	switch(type) {
		case B_MIX_GAIN:
			value = ac97_reg_cached_read(controller->ac97, info->reg);
			if (info->type & B_MIX_STEREO) {
				mask = ((1 << (info->bits + 1)) - 1) << 8;
				value &= ~mask;

				if (info->polarity == 1)
					gain = info->max_gain - values[0];
				else
					gain =  values[0] - info->min_gain;
				value |= ((uint16)(gain	/ info->granularity) << 8) & mask;

				mask = ((1 << (info->bits + 1)) - 1);
				value &= ~mask;
				if (info->polarity == 1)
					gain = info->max_gain - values[1];
				else
					gain =  values[1] - info->min_gain;
				value |= ((uint16)(gain / info->granularity)) & mask;
			} else {
				mask = ((1 << (info->bits + 1)) - 1);
				value &= ~mask;
				if (info->polarity == 1)
					gain = info->max_gain - values[0];
				else
					gain =  values[0] - info->min_gain;
				value |= ((uint16)(gain / info->granularity)) & mask;
			}
			//TRACE("B_MIX_GAIN value : %u\n", value);
			ac97_reg_cached_write(controller->ac97, info->reg, value);
			break;
		case B_MIX_MUTE:
			mask = ((1 << 1) - 1) << 15;
			value = ac97_reg_cached_read(controller->ac97, info->reg);
			value &= ~mask;
			value |= ((values[0] == 1.0 ? 1 : 0 ) << 15 & mask);
			if (info->reg == AC97_SURR_VOLUME) {
				// there is a independent mute for each channel
				mask = ((1 << 1) - 1) << 7;
				value &= ~mask;
				value |= ((values[0] == 1.0 ? 1 : 0 ) << 7 & mask);
			}
			//TRACE("B_MIX_MUTE value : %u\n", value);
			ac97_reg_cached_write(controller->ac97, info->reg, value);
			break;
		case B_MIX_MICBOOST:
			mask = ((1 << 1) - 1) << 6;
			value = ac97_reg_cached_read(controller->ac97, info->reg);
			value &= ~mask;
			value |= ((values[0] == 1.0 ? 1 : 0 ) << 6 & mask);
			//TRACE("B_MIX_MICBOOST value : %u\n", value);
			ac97_reg_cached_write(controller->ac97, info->reg, value);
			break;
		case B_MIX_MUX:
			mask = ((1 << 3) - 1);
			value = ((int32)values[0]) & mask;
			value = value | (value << 8);
			//TRACE("B_MIX_MUX value : %u\n", value);
			ac97_reg_cached_write(controller->ac97, AC97_RECORD_SELECT, value);
			break;
	}

}


static int32
create_group_control(geode_multi *multi, uint32 *index, uint32 parent,
	strind_id string, const char* name) {
	int32 i = *index;
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


static status_t
create_controls_list(geode_multi *multi)
{
	uint32 	i = 0, index = 0, count, id, parent, parent2, parent3;
	const ac97_source_info *info;

	/* AC97 Mixer */
	parent = create_group_control(multi, &index, 0, S_null, "AC97 mixer");

	count = source_info_size;
	//Note that we ignore first item in source_info
	//It's for recording, but do match this with ac97.c's source_info
	for (i = 1; i < count ; i++) {
		info = &source_info[i];
		TRACE("name : %s\n", info->name);

		parent2 = create_group_control(multi, &index, parent, S_null, info->name);

		if (info->type & B_MIX_GAIN) {
			if (info->type & B_MIX_MUTE) {
				multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
				multi->controls[index].mix_control.flags = B_MULTI_MIX_ENABLE;
				multi->controls[index].mix_control.master = MULTI_CONTROL_MASTERID;
				multi->controls[index].mix_control.parent = parent2;
				multi->controls[index].mix_control.string = S_MUTE;
				multi->controls[index].cookie = info;
				multi->controls[index].type = B_MIX_MUTE;
				multi->controls[index].get = &geode_ac97_get_mix;
				multi->controls[index].set = &geode_ac97_set_mix;
				index++;
			}

			multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_GAIN;
			multi->controls[index].mix_control.master = MULTI_CONTROL_MASTERID;
			multi->controls[index].mix_control.parent = parent2;
			strcpy(multi->controls[index].mix_control.name, info->name);
			multi->controls[index].mix_control.gain.min_gain = info->min_gain;
			multi->controls[index].mix_control.gain.max_gain = info->max_gain;
			multi->controls[index].mix_control.gain.granularity = info->granularity;
			multi->controls[index].cookie = info;
			multi->controls[index].type = B_MIX_GAIN;
			multi->controls[index].get = &geode_ac97_get_mix;
			multi->controls[index].set = &geode_ac97_set_mix;
			id = multi->controls[index].mix_control.id;
			index++;

			if (info->type & B_MIX_STEREO) {
				multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
				multi->controls[index].mix_control.flags = B_MULTI_MIX_GAIN;
				multi->controls[index].mix_control.master = id;
				multi->controls[index].mix_control.parent = parent2;
				strcpy(multi->controls[index].mix_control.name, info->name);
				multi->controls[index].mix_control.gain.min_gain = info->min_gain;
				multi->controls[index].mix_control.gain.max_gain = info->max_gain;
				multi->controls[index].mix_control.gain.granularity = info->granularity;
				multi->controls[index].cookie = info;
				multi->controls[index].type = B_MIX_GAIN;
				multi->controls[index].get = &geode_ac97_get_mix;
				multi->controls[index].set = &geode_ac97_set_mix;
				index++;
			}

			if (info->type & B_MIX_MICBOOST) {
				multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
				multi->controls[index].mix_control.flags = B_MULTI_MIX_ENABLE;
				multi->controls[index].mix_control.master = MULTI_CONTROL_MASTERID;
				multi->controls[index].mix_control.parent = parent2;
				strcpy(multi->controls[index].mix_control.name, "+20 dB");
				multi->controls[index].cookie = info;
				multi->controls[index].type = B_MIX_MICBOOST;
				multi->controls[index].get = &geode_ac97_get_mix;
				multi->controls[index].set = &geode_ac97_set_mix;
				index++;
			}
		}
	}

	/* AC97 Record */
	parent = create_group_control(multi, &index, 0, S_null, "Recording");

	info = &source_info[0];
	TRACE("name : %s\n", info->name);

	parent2 = create_group_control(multi, &index, parent, S_null, info->name);

	if (info->type & B_MIX_GAIN) {
		if (info->type & B_MIX_MUTE) {
			multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_ENABLE;
			multi->controls[index].mix_control.master = MULTI_CONTROL_MASTERID;
			multi->controls[index].mix_control.parent = parent2;
			multi->controls[index].mix_control.string = S_MUTE;
			multi->controls[index].cookie = info;
			multi->controls[index].type = B_MIX_MUTE;
			multi->controls[index].get = &geode_ac97_get_mix;
			multi->controls[index].set = &geode_ac97_set_mix;
			index++;
		}

		multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
		multi->controls[index].mix_control.flags = B_MULTI_MIX_GAIN;
		multi->controls[index].mix_control.master = MULTI_CONTROL_MASTERID;
		multi->controls[index].mix_control.parent = parent2;
		strcpy(multi->controls[index].mix_control.name, info->name);
		multi->controls[index].mix_control.gain.min_gain = info->min_gain;
		multi->controls[index].mix_control.gain.max_gain = info->max_gain;
		multi->controls[index].mix_control.gain.granularity = info->granularity;
		multi->controls[index].cookie = info;
		multi->controls[index].type = B_MIX_GAIN;
		multi->controls[index].get = &geode_ac97_get_mix;
		multi->controls[index].set = &geode_ac97_set_mix;
		id = multi->controls[index].mix_control.id;
		index++;

		if (info->type & B_MIX_STEREO) {
			multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_GAIN;
			multi->controls[index].mix_control.master = id;
			multi->controls[index].mix_control.parent = parent2;
			strcpy(multi->controls[index].mix_control.name, info->name);
			multi->controls[index].mix_control.gain.min_gain = info->min_gain;
			multi->controls[index].mix_control.gain.max_gain = info->max_gain;
			multi->controls[index].mix_control.gain.granularity = info->granularity;
			multi->controls[index].cookie = info;
			multi->controls[index].type = B_MIX_GAIN;
			multi->controls[index].get = &geode_ac97_get_mix;
			multi->controls[index].set = &geode_ac97_set_mix;
			index++;
		}

		if (info->type & B_MIX_RECORDMUX) {
			multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX;
			multi->controls[index].mix_control.parent = parent2;
			strcpy(multi->controls[index].mix_control.name, "Record mux");
			multi->controls[index].cookie = info;
			multi->controls[index].type = B_MIX_MUX;
			multi->controls[index].get = &geode_ac97_get_mix;
			multi->controls[index].set = &geode_ac97_set_mix;
			parent3 = multi->controls[index].mix_control.id;
			index++;

			multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			multi->controls[index].mix_control.string = S_MIC;
			index++;
			multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			strcpy(multi->controls[index].mix_control.name, "CD in");
			index++;
			multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			strcpy(multi->controls[index].mix_control.name, "Video in");
			index++;
			multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			strcpy(multi->controls[index].mix_control.name, "Aux in");
			index++;
			multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			strcpy(multi->controls[index].mix_control.name, "Line in");
			index++;
			multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			multi->controls[index].mix_control.string = S_STEREO_MIX;
			index++;
			multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			multi->controls[index].mix_control.string = S_MONO_MIX;
			index++;
			multi->controls[index].mix_control.id = MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			strcpy(multi->controls[index].mix_control.name, "TAD");
			index++;
		}
	}

	multi->control_count = index;
	TRACE("multi->control_count %lu\n", multi->control_count);
	return B_OK;
}


static status_t
list_mix_controls(geode_controller* controller, multi_mix_control_info* mmci)
{
	multi_mix_control* mmc = mmci->controls;
	if (mmci->control_count < 24)
		return B_ERROR;

	if (create_controls_list(controller->multi) < B_OK)
		return B_ERROR;
	for (uint32 i = 0; i < controller->multi->control_count; i++) {
		mmc[i] = controller->multi->controls[i].mix_control;
	}

	mmci->control_count = controller->multi->control_count;
	return B_OK;
}


static status_t
list_mix_connections(geode_controller* controller,
	multi_mix_connection_info* data)
{
	data->actual_count = 0;
	return B_OK;
}


static status_t
list_mix_channels(geode_controller* controller, multi_mix_channel_info *data)
{
	return B_OK;
}


static status_t
get_mix(geode_controller *controller, multi_mix_value_info * mmvi)
{
	for (int32 i = 0; i < mmvi->item_count; i++) {
		uint32 id = mmvi->values[i].id - MULTI_CONTROL_FIRSTID;
		if (id < 0 || id >= controller->multi->control_count) {
			dprintf("geode_get_mix : invalid control id requested : %li\n", id);
			continue;
		}
		multi_mixer_control *control = &controller->multi->controls[id];

		if (control->mix_control.flags & B_MULTI_MIX_GAIN) {
			if (control->get) {
				float values[2];
				control->get(controller, control->cookie, control->type, values);
				if (control->mix_control.master == MULTI_CONTROL_MASTERID)
					mmvi->values[i].gain = values[0];
				else
					mmvi->values[i].gain = values[1];
			}
		}

		if (control->mix_control.flags & B_MULTI_MIX_ENABLE && control->get) {
			float values[1];
			control->get(controller, control->cookie, control->type, values);
			mmvi->values[i].enable = (values[0] == 1.0);
		}

		if (control->mix_control.flags & B_MULTI_MIX_MUX && control->get) {
			float values[1];
			control->get(controller, control->cookie, control->type, values);
			mmvi->values[i].mux = (int32)values[0];
		}
	}
	return B_OK;
}


static status_t
set_mix(geode_controller *controller, multi_mix_value_info * mmvi)
{
	geode_multi *multi = controller->multi;
	for (int32 i = 0; i < mmvi->item_count; i++) {
		uint32 id = mmvi->values[i].id - MULTI_CONTROL_FIRSTID;
		if (id < 0 || id >= multi->control_count) {
			dprintf("geode_set_mix : invalid control id requested : %li\n", id);
			continue;
		}
		multi_mixer_control *control = &multi->controls[id];

		if (control->mix_control.flags & B_MULTI_MIX_GAIN) {
			multi_mixer_control *control2 = NULL;
			if (i+1<mmvi->item_count) {
				id = mmvi->values[i + 1].id - MULTI_CONTROL_FIRSTID;
				if (id < 0 || id >= multi->control_count) {
					dprintf("geode_set_mix : invalid control id requested : %li\n", id);
				} else {
					control2 = &multi->controls[id];
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
					values[1] = mmvi->values[i+1].gain;

				control->set(controller, control->cookie, control->type, values);
			}

			if (control2)
				i++;
		}

		if (control->mix_control.flags & B_MULTI_MIX_ENABLE && control->set) {
			float values[1];

			values[0] = mmvi->values[i].enable ? 1.0 : 0.0;
			control->set(controller, control->cookie, control->type, values);
		}

		if (control->mix_control.flags & B_MULTI_MIX_MUX && control->set) {
			float values[1];

			values[0] = (float)mmvi->values[i].mux;
			control->set(controller, control->cookie, control->type, values);
		}
	}
	return B_OK;
}


static status_t
get_buffers(geode_controller* controller, multi_buffer_list* data)
{
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

	data->flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD;

	/* Copy the settings into the streams */

	if (controller->playback_stream != NULL) {
		controller->playback_stream->num_buffers = data->return_playback_buffers;
		controller->playback_stream->num_channels = data->return_playback_channels;
		controller->playback_stream->buffer_length
			= data->return_playback_buffer_size;

		status_t status = geode_stream_setup_buffers(
			controller->playback_stream, "Playback");
		if (status != B_OK) {
			dprintf("geode: Error setting up playback buffers: %s\n",
				strerror(status));
			return status;
		}
	}

	if (controller->record_stream != NULL) {
		controller->record_stream->num_buffers = data->return_record_buffers;
		controller->record_stream->num_channels = data->return_record_channels;
		controller->record_stream->buffer_length
			= data->return_record_buffer_size;

		status_t status = geode_stream_setup_buffers(
			controller->record_stream, "Recording");
		if (status != B_OK) {
			dprintf("geode: Error setting up recording buffers: %s\n",
				strerror(status));
			return status;
		}
	}

	/* Setup data structure for multi_audio API... */

	if (controller->playback_stream != NULL) {
		uint32 playbackSampleSize = controller->playback_stream->sample_size;

		for (int32 i = 0; i < data->return_playback_buffers; i++) {
			for (int32 channelIndex = 0;
					channelIndex < data->return_playback_channels; channelIndex++) {
				data->playback_buffers[i][channelIndex].base
					= (char*)controller->playback_stream->buffers[i]
						+ playbackSampleSize * channelIndex;
				data->playback_buffers[i][channelIndex].stride
					= playbackSampleSize * data->return_playback_channels;
			}
		}
	}

	if (controller->record_stream != NULL) {
		uint32 recordSampleSize = controller->record_stream->sample_size;

		for (int32 i = 0; i < data->return_record_buffers; i++) {
			for (int32 channelIndex = 0;
					channelIndex < data->return_record_channels; channelIndex++) {
				data->record_buffers[i][channelIndex].base
					= (char*)controller->record_stream->buffers[i]
						+ recordSampleSize * channelIndex;
				data->record_buffers[i][channelIndex].stride
					= recordSampleSize * data->return_record_channels;
			}
		}
	}

	return B_OK;
}


/*! playback_buffer_cycle is the buffer we want to have played */
static status_t
buffer_exchange(geode_controller* controller, multi_buffer_info* data)
{
	static int debug_buffers_exchanged = 0;
	cpu_status status;
	status_t err;
	multi_buffer_info buffer_info;

	if (controller->playback_stream == NULL)
		return B_ERROR;

	if (!controller->playback_stream->running) {
		geode_stream_start(controller->playback_stream);
	}
	if (controller->record_stream && !controller->record_stream->running) {
		geode_stream_start(controller->record_stream);
	}

#ifdef __HAIKU__
	if (user_memcpy(&buffer_info, data, sizeof(buffer_info)) < B_OK)
		return B_BAD_ADDRESS;
#else
	memcpy(&buffer_info, data, sizeof(buffer_info));
#endif

	/* do playback */
	err = acquire_sem_etc(controller->playback_stream->buffer_ready_sem,
		1, B_CAN_INTERRUPT, 0);
	if (err != B_OK) {
		dprintf("%s: Error waiting for playback buffer to finish (%s)!\n", __func__,
			strerror(err));
		return err;
	}

	status = disable_interrupts();
	acquire_spinlock(&controller->playback_stream->lock);

	buffer_info.playback_buffer_cycle = controller->playback_stream->buffer_cycle;
	buffer_info.played_real_time = controller->playback_stream->real_time;
	buffer_info.played_frames_count = controller->playback_stream->frames_count;

	release_spinlock(&controller->playback_stream->lock);

	if (controller->record_stream) {
		acquire_spinlock(&controller->record_stream->lock);
		buffer_info.record_buffer_cycle = controller->record_stream->buffer_cycle;
		buffer_info.recorded_real_time = controller->record_stream->real_time;
		buffer_info.recorded_frames_count = controller->record_stream->frames_count;
		release_spinlock(&controller->record_stream->lock);
	}

	restore_interrupts(status);

#ifdef __HAIKU__
	if (user_memcpy(data, &buffer_info, sizeof(buffer_info)) < B_OK)
		return B_BAD_ADDRESS;
#else
	memcpy(data, &buffer_info, sizeof(buffer_info));
#endif

	debug_buffers_exchanged++;
	if (((debug_buffers_exchanged % 100) == 1) && (debug_buffers_exchanged < 1111)) {
		dprintf("%s: %d buffers processed\n", __func__, debug_buffers_exchanged);
	}

	return B_OK;
}


static status_t
buffer_force_stop(geode_controller* controller)
{
	if (controller->playback_stream != NULL) {
		geode_stream_stop(controller->playback_stream);
	}
	if (controller->record_stream != NULL) {
		geode_stream_stop(controller->record_stream);
	}

	return B_OK;
}


status_t
multi_audio_control(geode_controller* controller, uint32 op, void* arg, size_t len)
{
	// TODO: make userland-safe when built for Haiku!

	switch (op) {
		case B_MULTI_GET_DESCRIPTION:
		{
#ifdef __HAIKU__
			multi_description description;
			multi_channel_info channels[16];
			multi_channel_info* originalChannels;

			if (user_memcpy(&description, arg, sizeof(multi_description))
					!= B_OK)
				return B_BAD_ADDRESS;

			originalChannels = description.channels;
			description.channels = channels;
			if (description.request_channel_count > 16)
				description.request_channel_count = 16;

			status_t status = get_description(controller, &description);
			if (status != B_OK)
				return status;

			description.channels = originalChannels;
			if (user_memcpy(arg, &description, sizeof(multi_description))
					!= B_OK)
				return B_BAD_ADDRESS;
			return user_memcpy(originalChannels, channels, sizeof(multi_channel_info)
					* description.request_channel_count);
#else
			return get_description(controller, (multi_description*)arg);
#endif
		}

		case B_MULTI_GET_ENABLED_CHANNELS:
			return get_enabled_channels(controller, (multi_channel_enable*)arg);
		case B_MULTI_SET_ENABLED_CHANNELS:
			return B_OK;

		case B_MULTI_GET_GLOBAL_FORMAT:
			return get_global_format(controller, (multi_format_info*)arg);
		case B_MULTI_SET_GLOBAL_FORMAT:
			return set_global_format(controller, (multi_format_info*)arg);

		case B_MULTI_LIST_MIX_CHANNELS:
			return list_mix_channels(controller, (multi_mix_channel_info*)arg);
		case B_MULTI_LIST_MIX_CONTROLS:
			return list_mix_controls(controller, (multi_mix_control_info*)arg);
		case B_MULTI_LIST_MIX_CONNECTIONS:
			return list_mix_connections(controller,
				(multi_mix_connection_info*)arg);
		case B_MULTI_GET_MIX:
			return get_mix(controller, (multi_mix_value_info *)arg);
		case B_MULTI_SET_MIX:
			return set_mix(controller, (multi_mix_value_info *)arg);

		case B_MULTI_GET_BUFFERS:
			return get_buffers(controller, (multi_buffer_list*)arg);

		case B_MULTI_BUFFER_EXCHANGE:
			return buffer_exchange(controller, (multi_buffer_info*)arg);
		case B_MULTI_BUFFER_FORCE_STOP:
			return buffer_force_stop(controller);

		case B_MULTI_GET_EVENT_INFO:
		case B_MULTI_SET_EVENT_INFO:
		case B_MULTI_GET_EVENT:
		case B_MULTI_GET_CHANNEL_FORMATS:
		case B_MULTI_SET_CHANNEL_FORMATS:
		case B_MULTI_SET_BUFFERS:
		case B_MULTI_SET_START_TIME:
			return B_ERROR;
	}

	return B_BAD_VALUE;
}
