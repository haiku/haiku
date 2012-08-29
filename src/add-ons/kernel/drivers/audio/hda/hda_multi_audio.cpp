/*
 * Copyright 2007-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "hmulti_audio.h"
#include "driver.h"


#ifdef TRACE
#	undef TRACE
#endif

//#define TRACE_MULTI_AUDIO
#ifdef TRACE_MULTI_AUDIO
#	define TRACE(a...) dprintf("\33[34mhda:\33[0m " a)
#else
#	define TRACE(a...) ;
#endif
#define ERROR(a...)	dprintf("\33[34mhda:\33[0m " a)

typedef enum {
	B_MIX_GAIN = 1 << 0,
	B_MIX_MUTE = 1 << 1,
	B_MIX_MUX_MIXER = 1 << 2,
	B_MIX_MUX_SELECTOR = 1 << 3
} mixer_type;


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
get_description(hda_audio_group* audioGroup, multi_description* data)
{
	data->interface_version = B_CURRENT_INTERFACE_VERSION;
	data->interface_minimum = B_CURRENT_INTERFACE_VERSION;

	strcpy(data->friendly_name, "HD Audio");
	strcpy(data->vendor_info, "Haiku");

	int32 inChannels = 0;
	if (audioGroup->record_stream != NULL)
		inChannels = 2;

	int32 outChannels = 0;
	if (audioGroup->playback_stream != NULL)
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

	if (audioGroup->playback_stream != NULL) {
		data->output_rates = audioGroup->playback_stream->sample_rate;
		data->output_formats = audioGroup->playback_stream->sample_format;
	} else {
		data->output_rates = 0;
		data->output_formats = 0;
	}

	if (audioGroup->record_stream != NULL) {
		data->input_rates = audioGroup->record_stream->sample_rate;
		data->input_formats = audioGroup->record_stream->sample_format;
	} else {
		data->input_rates = 0;
		data->input_formats = 0;
	}

	// force existance of 48kHz if variable rates are not supported
	if (data->output_rates == 0)
		data->output_rates = B_SR_48000;
	if (data->input_rates == 0)
		data->input_rates = B_SR_48000;

	data->max_cvsr_rate = 0;
	data->min_cvsr_rate = 0;

	data->lock_sources = B_MULTI_LOCK_INTERNAL;
	data->timecode_sources = 0;
	data->interface_flags
		= B_MULTI_INTERFACE_PLAYBACK | B_MULTI_INTERFACE_RECORD;
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

	if (audioGroup->playback_stream != NULL) {
		data->output.format = audioGroup->playback_stream->sample_format;
		data->output.rate = audioGroup->playback_stream->sample_rate;
	} else {
		data->output.format = 0;
		data->output.rate = 0;
	}

	if (audioGroup->record_stream != NULL) {
		data->input.format = audioGroup->record_stream->sample_format;
		data->input.rate = audioGroup->record_stream->sample_rate;
	} else {
		data->input.format = 0;
		data->input.rate = 0;
	}

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

	if (audioGroup->playback_stream != NULL) {
		audioGroup->playback_stream->sample_format = data->output.format;
		audioGroup->playback_stream->sample_rate = data->output.rate;
		audioGroup->playback_stream->sample_size = format2size(
			audioGroup->playback_stream->sample_format);
	}

	if (audioGroup->record_stream != NULL) {
		audioGroup->record_stream->sample_rate = data->input.rate;
		audioGroup->record_stream->sample_format = data->input.format;
		audioGroup->record_stream->sample_size = format2size(
			audioGroup->record_stream->sample_format);
	}

	return B_OK;
}


static enum strind_id
hda_find_multi_string(hda_widget& widget)
{
	switch (CONF_DEFAULT_DEVICE(widget.d.pin.config)) {
		case PIN_DEV_CD:
			return S_CD;
		case PIN_DEV_LINE_IN:
		case PIN_DEV_LINE_OUT:
			return S_LINE;
		case PIN_DEV_MIC_IN:
			return S_MIC;
		case PIN_DEV_AUX:
			return S_AUX;
		case PIN_DEV_SPDIF_IN:
		case PIN_DEV_SPDIF_OUT:
			return S_SPDIF;
		case PIN_DEV_HEAD_PHONE_OUT:
			return S_HEADPHONE;
	}
	ERROR("couln't find a string for widget %ld in hda_find_multi_string()\n",
		widget.node_id);
	return S_null;
}


static void
hda_find_multi_custom_string(hda_widget& widget, char* custom, uint32 size)
{
	const char* device = NULL;
	switch (CONF_DEFAULT_DEVICE(widget.d.pin.config)) {
		case PIN_DEV_LINE_IN:
			device = "Line in";
		case PIN_DEV_LINE_OUT:
			if (device == NULL)
				device = "Line out";
		case PIN_DEV_MIC_IN:
			if (device == NULL)
				device =  "Mic in";
			switch (CONF_DEFAULT_COLOR(widget.d.pin.config)) {
				case 1:
					device = "Rear";
					break;
				case 2:
					device = "Side";
					break;
				case 3:
					device = "Line in";
					break;
				case 4:
					device = "Front";
					break;
				case 6:
					device = "Center/Sub";
					break;
				case 9:
					device = "Mic in";
					break;
			}
			break;
		case PIN_DEV_SPDIF_IN:
			device = "SPDIF in";
			break;
		case PIN_DEV_SPDIF_OUT:
			device = "SPDIF out";
			break;
		case PIN_DEV_CD:
			device = "CD";
			break;
		case PIN_DEV_HEAD_PHONE_OUT:
			device = "Headphones";
			break;
		case PIN_DEV_SPEAKER:
			device = "Speaker";
			break;
	}
	if (device == NULL) {
		ERROR("couldn't find a string for widget %ld in "
			"hda_find_multi_custom_string()\n", widget.node_id);
	}

	const char* location
		= get_widget_location(CONF_DEFAULT_LOCATION(widget.d.pin.config));
	snprintf(custom, size, "%s%s%s", location ? location : "",
		location ? " " : "", device);
}


static int32
hda_create_group_control(hda_multi *multi, uint32 *index, int32 parent,
	enum strind_id string, const char* name)
{
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
hda_create_channel_control(hda_multi* multi, uint32* index, int32 parent,
	int32 string, hda_widget& widget, bool input, uint32 capabilities,
	int32 inputIndex, bool& gain, bool& mute)
{
	uint32 i = *index, id;
	hda_multi_mixer_control control;

	control.nid = widget.node_id;
	control.input = input;
	control.mute = 0;
	control.gain = 0;
	control.capabilities = capabilities;
	control.index = inputIndex;
	control.mix_control.master = MULTI_CONTROL_MASTERID;
	control.mix_control.parent = parent;

	if (mute && (capabilities & AMP_CAP_MUTE) != 0) {
		control.mix_control.id = MULTI_CONTROL_FIRSTID + i;
		control.mix_control.flags = B_MULTI_MIX_ENABLE;
		control.mix_control.string = S_MUTE;
		control.type = B_MIX_MUTE;
		multi->controls[i++] = control;
		TRACE("control nid %ld mute\n", control.nid);
		mute = false;
	}

	if (gain && AMP_CAP_NUM_STEPS(capabilities) >= 1) {
		control.mix_control.gain.granularity = AMP_CAP_STEP_SIZE(capabilities);
		control.mix_control.gain.min_gain = (0.0 - AMP_CAP_OFFSET(capabilities))
			* control.mix_control.gain.granularity;
		control.mix_control.gain.max_gain = (AMP_CAP_NUM_STEPS(capabilities)
				- AMP_CAP_OFFSET(capabilities))
			* control.mix_control.gain.granularity;

		control.mix_control.id = MULTI_CONTROL_FIRSTID + i;
		control.mix_control.flags = B_MULTI_MIX_GAIN;
		control.mix_control.string = S_null;
		control.type = B_MIX_GAIN;
		strcpy(control.mix_control.name, "Gain");
		multi->controls[i++] = control;
		id = control.mix_control.id;

		// second channel
		control.mix_control.id = MULTI_CONTROL_FIRSTID + i;
		control.mix_control.master = id;
		multi->controls[i++] = control;
		TRACE("control nid %ld %f min %f max %f\n", control.nid,
			control.mix_control.gain.granularity,
			control.mix_control.gain.min_gain,
			control.mix_control.gain.max_gain);
		gain = false;
	}

	*index = i;
}


static void
hda_create_mux_control(hda_multi *multi, uint32 *index, int32 parent,
	hda_widget& widget)
{
	uint32 i = *index, parent2;
	hda_multi_mixer_control control;
	hda_audio_group *audioGroup = multi->group;

	control.nid = widget.node_id;
	control.input = true;
	control.mute = 0;
	control.gain = 0;
	control.mix_control.master = MULTI_CONTROL_MASTERID;
	control.mix_control.parent = parent;
	control.mix_control.id = MULTI_CONTROL_FIRSTID + i;
	control.mix_control.flags = B_MULTI_MIX_MUX;
	control.mix_control.string = S_null;
	control.type = widget.type == WT_AUDIO_MIXER
		? B_MIX_MUX_MIXER : B_MIX_MUX_SELECTOR;
	multi->controls[i] = control;
	strcpy(multi->controls[i].mix_control.name, "");
	i++;
	parent2 = control.mix_control.id;

	for (uint32 j = 0; j < widget.num_inputs; j++) {
		hda_widget *input =
			hda_audio_group_get_widget(audioGroup, widget.inputs[j]);
		if (input->type != WT_PIN_COMPLEX)
			continue;
		control.nid = widget.node_id;
		control.input = true;
		control.mix_control.id = MULTI_CONTROL_FIRSTID + i;
		control.mix_control.flags = B_MULTI_MIX_MUX_VALUE;
		control.mix_control.parent = parent2;
		control.mix_control.string = S_null;
		multi->controls[i] = control;
		hda_find_multi_custom_string(*input,
			multi->controls[i].mix_control.name,
			sizeof(multi->controls[i].mix_control.name));
		i++;
	}

	*index = i;
}


static void
hda_create_control_for_complex(hda_multi* multi, uint32* index, uint32 parent,
	hda_widget& widget, bool& gain, bool& mute)
{
	hda_audio_group* audioGroup = multi->group;

	switch (widget.type) {
		case WT_AUDIO_OUTPUT:
		case WT_AUDIO_MIXER:
		case WT_AUDIO_SELECTOR:
		case WT_PIN_COMPLEX:
			break;
		default:
			return;
	}

	if ((widget.flags & WIDGET_FLAG_WIDGET_PATH) != 0)
		return;

	TRACE("  create widget nid %lu\n", widget.node_id);
	hda_create_channel_control(multi, index, parent, 0,
		widget, false, widget.capabilities.output_amplifier, 0, gain, mute);

	if (!gain && !mute) {
		widget.flags |= WIDGET_FLAG_WIDGET_PATH;
		return;
	}

	if (widget.type == WT_AUDIO_MIXER) {
		hda_create_channel_control(multi, index, parent, 0,
			widget, true, widget.capabilities.input_amplifier, 0, gain, mute);
		if (!gain && !mute) {
			widget.flags |= WIDGET_FLAG_WIDGET_PATH;
			return;
		}
	}

	if (widget.type != WT_AUDIO_OUTPUT && widget.num_inputs > 0) {
		hda_widget& child = *hda_audio_group_get_widget(audioGroup,
			widget.inputs[widget.active_input]);
		hda_create_control_for_complex(multi, index, parent, child, gain, mute);
	}

	widget.flags |= WIDGET_FLAG_WIDGET_PATH;
}


static status_t
hda_create_controls_list(hda_multi* multi)
{
	uint32 index = 0;
	hda_audio_group* audioGroup = multi->group;

	uint32 parent = hda_create_group_control(multi, &index, 0, S_OUTPUT, NULL);
	uint32 parent2;

	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& complex = audioGroup->widgets[i];
		char name[48];

		if (complex.type != WT_PIN_COMPLEX)
			continue;
		if (!PIN_CAP_IS_OUTPUT(complex.d.pin.capabilities))
			continue;
		if ((complex.flags & WIDGET_FLAG_OUTPUT_PATH) == 0)
			continue;

		TRACE("create complex nid %lu\n", complex.node_id);
		hda_find_multi_custom_string(complex, name, sizeof(name));
		parent2 = hda_create_group_control(multi, &index, parent, S_null, name);
		bool gain = true, mute = true;

		hda_create_control_for_complex(multi, &index, parent2, complex, gain,
			mute);
	}

	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];

		if (widget.type != WT_AUDIO_MIXER)
			continue;
		if ((widget.flags & WIDGET_FLAG_WIDGET_PATH) != 0)
			continue;

		TRACE("create widget nid %lu\n", widget.node_id);

		if (AMP_CAP_NUM_STEPS(widget.capabilities.input_amplifier) >= 1) {
			for (uint32 j = 0; j < widget.num_inputs; j++) {
				hda_widget* complex = hda_audio_group_get_widget(audioGroup,
					widget.inputs[j]);
				char name[48];
				if (complex->type != WT_PIN_COMPLEX)
					continue;
				if (!PIN_CAP_IS_INPUT(complex->d.pin.capabilities))
					continue;
				if ((complex->flags & WIDGET_FLAG_OUTPUT_PATH) != 0)
					continue;
				TRACE("  create widget input nid %lu\n", widget.inputs[j]);
				hda_find_multi_custom_string(*complex, name, sizeof(name));
				parent2 = hda_create_group_control(multi, &index,
					parent, S_null, name);
				bool gain = true, mute = true;
				hda_create_channel_control(multi, &index, parent2, 0, widget,
					true, widget.capabilities.input_amplifier, j, gain, mute);
			}
		}

		widget.flags |= WIDGET_FLAG_WIDGET_PATH;
	}

	parent = hda_create_group_control(multi, &index, 0, S_INPUT, NULL);

	for (uint32 i = 0; i < audioGroup->widget_count; i++) {
		hda_widget& widget = audioGroup->widgets[i];

		if (widget.type != WT_AUDIO_INPUT)
			continue;

		uint32 capabilities = widget.capabilities.input_amplifier;
		if (AMP_CAP_NUM_STEPS(capabilities) < 1)
			continue;

		parent2 = hda_create_group_control(multi, &index,
			parent, hda_find_multi_string(widget), "Input");
		bool gain = true, mute = true;
		hda_create_channel_control(multi, &index, parent2, 0,
			widget, true, capabilities, 0, gain, mute);

		if (widget.num_inputs > 1) {
			TRACE("  create mux for nid %lu\n", widget.node_id);
			hda_create_mux_control(multi, &index, parent2, widget);
			continue;
		}

		hda_widget *mixer = hda_audio_group_get_widget(audioGroup,
			widget.inputs[0]);
		if (mixer->type != WT_AUDIO_MIXER && mixer->type != WT_AUDIO_SELECTOR)
			continue;
		TRACE("  create mixer nid %lu\n", mixer->node_id);
		hda_create_mux_control(multi, &index, parent2, *mixer);
	}

	multi->control_count = index;
	TRACE("multi->control_count %lu\n", multi->control_count);
	return B_OK;
}


static status_t
list_mix_controls(hda_audio_group* audioGroup, multi_mix_control_info* mmci)
{
	multi_mix_control *mmc = mmci->controls;
	if (mmci->control_count < 24)
		return B_ERROR;

	if (hda_create_controls_list(audioGroup->multi) < B_OK)
		return B_ERROR;
	for (uint32 i = 0; i < audioGroup->multi->control_count; i++) {
		mmc[i] = audioGroup->multi->controls[i].mix_control;
	}

	mmci->control_count = audioGroup->multi->control_count;
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


static void
get_control_gain_mute(hda_audio_group* audioGroup,
	hda_multi_mixer_control *control, uint32 *resp)
{
	uint32 verb[2];
	verb[0] = MAKE_VERB(audioGroup->codec->addr,
		control->nid,
		VID_GET_AMPLIFIER_GAIN_MUTE,
		(control->input ? AMP_GET_INPUT : AMP_GET_OUTPUT)
		| AMP_GET_LEFT_CHANNEL | AMP_GET_INPUT_INDEX(control->index));
	verb[1] = MAKE_VERB(audioGroup->codec->addr,
		control->nid,
		VID_GET_AMPLIFIER_GAIN_MUTE,
		(control->input ? AMP_GET_INPUT : AMP_GET_OUTPUT)
		| AMP_GET_RIGHT_CHANNEL | AMP_GET_INPUT_INDEX(control->index));
	hda_send_verbs(audioGroup->codec, verb, resp, 2);
}


static status_t
get_mix(hda_audio_group* audioGroup, multi_mix_value_info * mmvi)
{
	uint32 id;
	hda_multi_mixer_control *control = NULL;
	for (int32 i = 0; i < mmvi->item_count; i++) {
		id = mmvi->values[i].id - MULTI_CONTROL_FIRSTID;
		if (id < 0 || id >= audioGroup->multi->control_count) {
			dprintf("hda: get_mix : invalid control id requested : %li\n", id);
			continue;
		}
		control = &audioGroup->multi->controls[id];

		if ((control->mix_control.flags
				& (B_MULTI_MIX_GAIN | B_MULTI_MIX_ENABLE)) != 0) {
			uint32 resp[2];
			get_control_gain_mute(audioGroup, control, resp);
			if ((control->mix_control.flags & B_MULTI_MIX_ENABLE) != 0) {
				mmvi->values[i].enable = (resp[0] & AMP_MUTE) != 0;
				TRACE("get_mix: %ld mute: %d\n", control->nid,
					mmvi->values[i].enable);
			} else if ((control->mix_control.flags & B_MULTI_MIX_GAIN) != 0) {
				uint32 value;
				if (control->mix_control.master == MULTI_CONTROL_MASTERID)
					value = resp[0] & AMP_GAIN_MASK;
				else
					value = resp[1] & AMP_GAIN_MASK;
				mmvi->values[i].gain = (0.0 + value - AMP_CAP_OFFSET(control->capabilities))
						* AMP_CAP_STEP_SIZE(control->capabilities);
				TRACE("get_mix: %ld gain: %f (%ld)\n", control->nid, mmvi->values[i].gain, value);
			}

		} else if ((control->mix_control.flags & B_MIX_MUX_MIXER) != 0) {
			hda_widget* mixer = hda_audio_group_get_widget(audioGroup,
				control->nid);
			mmvi->values[i].mux = 0;
			for (uint32 j = 0; j < mixer->num_inputs; j++) {
				uint32 verb = MAKE_VERB(audioGroup->codec->addr,
					control->nid, VID_GET_AMPLIFIER_GAIN_MUTE, AMP_GET_INPUT
					| AMP_GET_LEFT_CHANNEL | AMP_GET_INPUT_INDEX(j));
				uint32 resp;
				if (hda_send_verbs(audioGroup->codec, &verb, &resp, 1) == B_OK) {
					TRACE("get_mix: %ld mixer %ld is %smute\n", control->nid,
						j, (resp & AMP_MUTE) != 0 ? "" : "un");
					if ((resp & AMP_MUTE) == 0) {
						mmvi->values[i].mux = j;
#ifndef TRACE_MULTI_AUDIO
						break;
#endif
					}
				}
			}
			TRACE("get_mix: %ld mixer: %ld\n", control->nid,
				mmvi->values[i].mux);
		} else if ((control->mix_control.flags & B_MIX_MUX_SELECTOR) != 0) {
			uint32 verb = MAKE_VERB(audioGroup->codec->addr, control->nid,
				VID_GET_CONNECTION_SELECT, 0);
			uint32 resp;
			if (hda_send_verbs(audioGroup->codec, &verb, &resp, 1) == B_OK)
				mmvi->values[i].mux = resp & 0xff;
			TRACE("get_mix: %ld selector: %ld\n", control->nid,
				mmvi->values[i].mux);
		}
	}
	return B_OK;
}


static status_t
set_mix(hda_audio_group* audioGroup, multi_mix_value_info * mmvi)
{
	uint32 id;
	hda_multi_mixer_control *control = NULL;
	for (int32 i = 0; i < mmvi->item_count; i++) {
		id = mmvi->values[i].id - MULTI_CONTROL_FIRSTID;
		if (id < 0 || id >= audioGroup->multi->control_count) {
			dprintf("set_mix : invalid control id requested : %li\n", id);
			continue;
		}
		control = &audioGroup->multi->controls[id];

		if ((control->mix_control.flags & B_MULTI_MIX_ENABLE) != 0) {
			control->mute = (mmvi->values[i].enable ? AMP_MUTE : 0);
			TRACE("set_mix: %ld mute: %lx\n", control->nid, control->mute);
			uint32 resp[2];
			get_control_gain_mute(audioGroup, control, resp);

			uint32 verb[2];
			verb[0] = MAKE_VERB(audioGroup->codec->addr,
				control->nid,
				VID_SET_AMPLIFIER_GAIN_MUTE,
				(control->input ? AMP_SET_INPUT : AMP_SET_OUTPUT)
				| AMP_SET_LEFT_CHANNEL
				| AMP_SET_INPUT_INDEX(control->index)
				| control->mute
				| (resp[0] & AMP_GAIN_MASK));
			TRACE("set_mix: sending verb to %ld: %lx %lx %x %lx\n", control->nid,
				control->mute, resp[0] & AMP_GAIN_MASK, control->input,
				(control->input ? AMP_SET_INPUT : AMP_SET_OUTPUT)
				| AMP_SET_LEFT_CHANNEL
				| AMP_SET_INPUT_INDEX(control->index)
				| control->mute
				| (resp[0] & AMP_GAIN_MASK));
			verb[1] = MAKE_VERB(audioGroup->codec->addr,
				control->nid,
				VID_SET_AMPLIFIER_GAIN_MUTE,
				(control->input ? AMP_SET_INPUT : AMP_SET_OUTPUT)
				| AMP_SET_RIGHT_CHANNEL
				| AMP_SET_INPUT_INDEX(control->index)
				| control->mute
				| (resp[1] & AMP_GAIN_MASK));
			TRACE("set_mix: ctrl2 sending verb to %ld: %lx %lx %x\n", control->nid,
				control->mute, resp[1] & AMP_GAIN_MASK, control->input);
			hda_send_verbs(audioGroup->codec, verb, NULL, 2);
		} else if ((control->mix_control.flags & B_MULTI_MIX_GAIN) != 0) {
			hda_multi_mixer_control *control2 = NULL;
			if (i+1<mmvi->item_count) {
				id = mmvi->values[i + 1].id - MULTI_CONTROL_FIRSTID;
				if (id < 0 || id >= audioGroup->multi->control_count) {
					dprintf("set_mix : invalid control id requested : %li\n", id);
				} else {
					control2 = &audioGroup->multi->controls[id];
					if (control2->mix_control.master != control->mix_control.id)
						control2 = NULL;
				}
			}

			if (control->mix_control.master == MULTI_CONTROL_MASTERID) {
				control->gain = (uint32)(mmvi->values[i].gain
					/ AMP_CAP_STEP_SIZE(control->capabilities)
					+ AMP_CAP_OFFSET(control->capabilities));
			}

			if (control2
				&& control2->mix_control.master != MULTI_CONTROL_MASTERID) {
				control2->gain = (uint32)(mmvi->values[i+1].gain
					/ AMP_CAP_STEP_SIZE(control2->capabilities)
					+ AMP_CAP_OFFSET(control2->capabilities));
			}
			TRACE("set_mix: %ld gain: %lx and %ld gain: %lx\n",
				control->nid, control->gain, control2->nid, control2->gain);
			uint32 resp[2];
			get_control_gain_mute(audioGroup, control, resp);
			control->mute = resp[0] & AMP_MUTE;
			if (control2)
				control2->mute = resp[1] & AMP_MUTE;

			uint32 verb[2];
			verb[0] = MAKE_VERB(audioGroup->codec->addr,
				control->nid,
				VID_SET_AMPLIFIER_GAIN_MUTE,
				(control->input ? AMP_SET_INPUT : AMP_SET_OUTPUT)
				| AMP_SET_LEFT_CHANNEL
				| AMP_SET_INPUT_INDEX(control->index)
				| (control->mute & AMP_MUTE)
				| (control->gain & AMP_GAIN_MASK));
			TRACE("set_mix: sending verb to %ld: %lx %lx %x %lx\n", control->nid,
				control->mute, control->gain, control->input,
				(control->input ? AMP_SET_INPUT : AMP_SET_OUTPUT)
				| AMP_SET_LEFT_CHANNEL
				| AMP_SET_INPUT_INDEX(control->index)
				| (control->mute & AMP_MUTE)
				| (control->gain & AMP_GAIN_MASK));
			if (control2) {
				verb[1] = MAKE_VERB(audioGroup->codec->addr,
					control2->nid,
					VID_SET_AMPLIFIER_GAIN_MUTE,
					(control->input ? AMP_SET_INPUT : AMP_SET_OUTPUT)
					| AMP_SET_RIGHT_CHANNEL
					| AMP_SET_INPUT_INDEX(control->index)
					| (control2->mute & AMP_MUTE)
					| (control2->gain & AMP_GAIN_MASK));
				TRACE("set_mix: ctrl2 sending verb to %ld: %lx %lx %x\n",
					control2->nid, control2->mute, control2->gain,
					control2->input);
			}
			hda_send_verbs(audioGroup->codec, verb, NULL, control2 ? 2 : 1);

			if (control2)
				i++;
		} else if ((control->mix_control.flags & B_MIX_MUX_MIXER) != 0) {
			TRACE("set_mix: %ld mixer: %ld\n", control->nid, mmvi->values[i].mux);
			hda_widget *mixer = hda_audio_group_get_widget(audioGroup,
				control->nid);
			uint32 verb[mixer->num_inputs];
			for (uint32 j = 0; j < mixer->num_inputs; j++) {
				verb[j] = MAKE_VERB(audioGroup->codec->addr,
					control->nid, VID_SET_AMPLIFIER_GAIN_MUTE, AMP_SET_INPUT
					| AMP_SET_LEFT_CHANNEL | AMP_SET_RIGHT_CHANNEL
					| AMP_SET_INPUT_INDEX(j)
					| ((mmvi->values[i].mux == j) ? 0 : AMP_MUTE));
				TRACE("set_mix: %ld mixer %smuting %ld (%lx)\n", control->nid,
					(mmvi->values[i].mux == j) ? "un" : "", j, verb[j]);
			}
			if (hda_send_verbs(audioGroup->codec, verb, NULL, mixer->num_inputs)
				!= B_OK)
				dprintf("hda: Setting mixer %ld failed on widget %ld!\n",
					mmvi->values[i].mux, control->nid);
		} else if ((control->mix_control.flags & B_MIX_MUX_SELECTOR) != 0) {
			uint32 verb = MAKE_VERB(audioGroup->codec->addr, control->nid,
				VID_SET_CONNECTION_SELECT, mmvi->values[i].mux);
			if (hda_send_verbs(audioGroup->codec, &verb, NULL, 1) != B_OK) {
				dprintf("hda: Setting output selector %ld failed on widget "
					"%ld!\n", mmvi->values[i].mux, control->nid);
			}
			TRACE("set_mix: %ld selector: %ld\n", control->nid,
				mmvi->values[i].mux);
		}
	}
	return B_OK;
}


static uint32
default_buffer_length_for_rate(uint32 rate)
{
	// keep the latency about the same as 2048 frames per buffer at 44100 Hz
	switch (rate) {
	case B_SR_8000:
		return 512;
	case B_SR_11025:
		return 512;
	case B_SR_16000:
		return 1024;
	case B_SR_22050:
		return 1024;
	case B_SR_32000:
		return 2048;
	case B_SR_44100:
		return 2048;
	case B_SR_48000:
		return 2048;
	case B_SR_88200:
		return 4096;
	case B_SR_96000:
		return 6144;
	case B_SR_176400:
		return 8192;
	case B_SR_192000:
		return 10240;
	case B_SR_384000:
		return 16384;
	}
	return 2048;
};


static status_t
get_buffers(hda_audio_group* audioGroup, multi_buffer_list* data)
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

	if (data->return_playback_buffer_size == 0) {
		data->return_playback_buffer_size = default_buffer_length_for_rate(
			audioGroup->playback_stream->sample_rate);
	}

	if (data->return_record_buffer_size == 0) {
		data->return_record_buffer_size = default_buffer_length_for_rate(
				audioGroup->record_stream->sample_rate);
	}

	/* ... from here on, we can assume again that a reasonable request is
	   being made */

	data->flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD;

	/* Copy the settings into the streams */

	if (audioGroup->playback_stream != NULL) {
		audioGroup->playback_stream->num_buffers = data->return_playback_buffers;
		audioGroup->playback_stream->num_channels = data->return_playback_channels;
		audioGroup->playback_stream->buffer_length
			= data->return_playback_buffer_size;

		status_t status = hda_stream_setup_buffers(audioGroup,
			audioGroup->playback_stream, "Playback");
		if (status != B_OK) {
			dprintf("hda: Error setting up playback buffers: %s\n",
				strerror(status));
			return status;
		}
	}

	if (audioGroup->record_stream != NULL) {
		audioGroup->record_stream->num_buffers = data->return_record_buffers;
		audioGroup->record_stream->num_channels = data->return_record_channels;
		audioGroup->record_stream->buffer_length
			= data->return_record_buffer_size;

		status_t status = hda_stream_setup_buffers(audioGroup,
			audioGroup->record_stream, "Recording");
		if (status != B_OK) {
			dprintf("hda: Error setting up recording buffers: %s\n",
				strerror(status));
			return status;
		}
	}

	/* Setup data structure for multi_audio API... */

	if (audioGroup->playback_stream != NULL) {
		uint32 playbackSampleSize = audioGroup->playback_stream->sample_size;

		for (int32 i = 0; i < data->return_playback_buffers; i++) {
			for (int32 channelIndex = 0;
					channelIndex < data->return_playback_channels; channelIndex++) {
				data->playback_buffers[i][channelIndex].base
					= (char*)audioGroup->playback_stream->buffers[i]
						+ playbackSampleSize * channelIndex;
				data->playback_buffers[i][channelIndex].stride
					= playbackSampleSize * data->return_playback_channels;
			}
		}
	}

	if (audioGroup->record_stream != NULL) {
		uint32 recordSampleSize = audioGroup->record_stream->sample_size;

		for (int32 i = 0; i < data->return_record_buffers; i++) {
			for (int32 channelIndex = 0;
					channelIndex < data->return_record_channels; channelIndex++) {
				data->record_buffers[i][channelIndex].base
					= (char*)audioGroup->record_stream->buffers[i]
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
buffer_exchange(hda_audio_group* audioGroup, multi_buffer_info* data)
{
	cpu_status status;
	status_t err;
	multi_buffer_info buffer_info;

	if (audioGroup->playback_stream == NULL)
		return B_ERROR;

	if (!audioGroup->playback_stream->running) {
		hda_stream_start(audioGroup->codec->controller,
			audioGroup->playback_stream);
	}
	if (audioGroup->record_stream && !audioGroup->record_stream->running) {
		hda_stream_start(audioGroup->codec->controller,
			audioGroup->record_stream);
	}

#ifdef __HAIKU__
	if (user_memcpy(&buffer_info, data, sizeof(buffer_info)) < B_OK)
		return B_BAD_ADDRESS;
#else
	memcpy(&buffer_info, data, sizeof(buffer_info));
#endif

	/* do playback */
	err = acquire_sem_etc(audioGroup->codec->controller->buffer_ready_sem,
		1, B_CAN_INTERRUPT, 0);
	if (err != B_OK) {
		dprintf("%s: Error waiting for playback buffer to finish (%s)!\n", __func__,
			strerror(err));
		return err;
	}

	status = disable_interrupts();
	acquire_spinlock(&audioGroup->playback_stream->lock);

	buffer_info.playback_buffer_cycle
		= (audioGroup->playback_stream->buffer_cycle)
			% audioGroup->playback_stream->num_buffers;
	buffer_info.played_real_time = audioGroup->playback_stream->real_time;
	buffer_info.played_frames_count = audioGroup->playback_stream->frames_count;

	release_spinlock(&audioGroup->playback_stream->lock);

	if (audioGroup->record_stream) {
		acquire_spinlock(&audioGroup->record_stream->lock);
		buffer_info.record_buffer_cycle
			= (audioGroup->record_stream->buffer_cycle - 1)
				% audioGroup->record_stream->num_buffers;
		buffer_info.recorded_real_time = audioGroup->record_stream->real_time;
		buffer_info.recorded_frames_count
			= audioGroup->record_stream->frames_count;
		release_spinlock(&audioGroup->record_stream->lock);
	}

	restore_interrupts(status);

#ifdef __HAIKU__
	if (user_memcpy(data, &buffer_info, sizeof(buffer_info)) < B_OK)
		return B_BAD_ADDRESS;
#else
	memcpy(data, &buffer_info, sizeof(buffer_info));
#endif

#if 0
	static int debugBuffersExchanged = 0;

	debugBuffersExchanged++;
	if ((debugBuffersExchanged % 100) == 1 && debugBuffersExchanged < 1111)
		dprintf("%s: %d buffers processed\n", __func__, debugBuffersExchanged);
#endif
	return B_OK;
}


static status_t
buffer_force_stop(hda_audio_group* audioGroup)
{
	if (audioGroup->playback_stream != NULL) {
		hda_stream_stop(audioGroup->codec->controller,
			audioGroup->playback_stream);
	}
	if (audioGroup->record_stream != NULL) {
		hda_stream_stop(audioGroup->codec->controller,
			audioGroup->record_stream);
	}

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

			status_t status = get_description(audioGroup, &description);
			if (status != B_OK)
				return status;

			description.channels = originalChannels;
			if (user_memcpy(arg, &description, sizeof(multi_description))
					!= B_OK)
				return B_BAD_ADDRESS;
			return user_memcpy(originalChannels, channels,
				sizeof(multi_channel_info) * description.request_channel_count);
#else
			return get_description(audioGroup, (multi_description*)arg);
#endif
		}

		case B_MULTI_GET_ENABLED_CHANNELS:
			return get_enabled_channels(audioGroup, (multi_channel_enable*)arg);
		case B_MULTI_SET_ENABLED_CHANNELS:
			return B_OK;

		case B_MULTI_GET_GLOBAL_FORMAT:
			return get_global_format(audioGroup, (multi_format_info*)arg);
		case B_MULTI_SET_GLOBAL_FORMAT:
			return set_global_format(audioGroup, (multi_format_info*)arg);

		case B_MULTI_LIST_MIX_CHANNELS:
			return list_mix_channels(audioGroup, (multi_mix_channel_info*)arg);
		case B_MULTI_LIST_MIX_CONTROLS:
			return list_mix_controls(audioGroup, (multi_mix_control_info*)arg);
		case B_MULTI_LIST_MIX_CONNECTIONS:
			return list_mix_connections(audioGroup,
				(multi_mix_connection_info*)arg);
		case B_MULTI_GET_MIX:
			return get_mix(audioGroup, (multi_mix_value_info *)arg);
		case B_MULTI_SET_MIX:
			return set_mix(audioGroup, (multi_mix_value_info *)arg);

		case B_MULTI_GET_BUFFERS:
			return get_buffers(audioGroup, (multi_buffer_list*)arg);

		case B_MULTI_BUFFER_EXCHANGE:
			return buffer_exchange(audioGroup, (multi_buffer_info*)arg);
		case B_MULTI_BUFFER_FORCE_STOP:
			return buffer_force_stop(audioGroup);

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
