/*
 * Auich BeOS Driver for Intel Southbridge audio
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
#include <string.h>
#include <strings.h>

#include <kernel.h>

#include "hmulti_audio.h"
#include "multi.h"
#include "ac97.h"

//#define DEBUG 1

#include "debug.h"
#include "auich.h"
#include "util.h"
#include "io.h"


static void
auich_ac97_get_mix(void *card, const void *cookie, int32 type, float *values) {
	auich_dev *dev = (auich_dev*)card;
	ac97_source_info *info = (ac97_source_info *)cookie;
	uint16 value, mask;
	float gain;

	switch(type) {
		case B_MIX_GAIN:
			value = auich_codec_read(&dev->config, info->reg);
			//PRINT(("B_MIX_GAIN value : %u\n", value));
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
			value = auich_codec_read(&dev->config, info->reg);
			//PRINT(("B_MIX_MUTE value : %u\n", value));
			value &= mask;
			values[0] = ((value >> 15) == 1) ? 1.0 : 0.0;
			break;
		case B_MIX_MICBOOST:
			mask = ((1 << 1) - 1) << 6;
			value = auich_codec_read(&dev->config, info->reg);
			//PRINT(("B_MIX_MICBOOST value : %u\n", value));
			value &= mask;
			values[0] = ((value >> 6) == 1) ? 1.0 : 0.0;
			break;
		case B_MIX_MUX:
			mask = ((1 << 3) - 1);
			value = auich_codec_read(&dev->config, AC97_RECORD_SELECT);
			value &= mask;
			//PRINT(("B_MIX_MUX value : %u\n", value));
			values[0] = (float)value;
			break;
	}
}


static void
auich_ac97_set_mix(void *card, const void *cookie, int32 type, float *values) {
	auich_dev *dev = (auich_dev*)card;
	ac97_source_info *info = (ac97_source_info *)cookie;
	uint16 value, mask;
	float gain;

	switch(type) {
		case B_MIX_GAIN:
			value = auich_codec_read(&dev->config, info->reg);
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
			//PRINT(("B_MIX_GAIN value : %u\n", value));
			auich_codec_write(&dev->config, info->reg, value);
			break;
		case B_MIX_MUTE:
			mask = ((1 << 1) - 1) << 15;
			value = auich_codec_read(&dev->config, info->reg);
			value &= ~mask;
			value |= ((values[0] == 1.0 ? 1 : 0 ) << 15 & mask);
			if (info->reg == AC97_SURR_VOLUME) {
				// there is a independent mute for each channel
				mask = ((1 << 1) - 1) << 7;
				value &= ~mask;
				value |= ((values[0] == 1.0 ? 1 : 0 ) << 7 & mask);
			}
			//PRINT(("B_MIX_MUTE value : %u\n", value));
			auich_codec_write(&dev->config, info->reg, value);
			break;
		case B_MIX_MICBOOST:
			mask = ((1 << 1) - 1) << 6;
			value = auich_codec_read(&dev->config, info->reg);
			value &= ~mask;
			value |= ((values[0] == 1.0 ? 1 : 0 ) << 6 & mask);
			//PRINT(("B_MIX_MICBOOST value : %u\n", value));
			auich_codec_write(&dev->config, info->reg, value);
			break;
		case B_MIX_MUX:
			mask = ((1 << 3) - 1);
			value = ((int32)values[0]) & mask;
			value = value | (value << 8);
			//PRINT(("B_MIX_MUX value : %u\n", value));
			auich_codec_write(&dev->config, AC97_RECORD_SELECT, value);
			break;
	}

}


static int32
auich_create_group_control(multi_dev *multi, int32 *index, int32 parent,
	int32 string, const char* name) {
	int32 i = *index;
	(*index)++;
	multi->controls[i].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + i;
	multi->controls[i].mix_control.parent = parent;
	multi->controls[i].mix_control.flags = B_MULTI_MIX_GROUP;
	multi->controls[i].mix_control.master = EMU_MULTI_CONTROL_MASTERID;
	multi->controls[i].mix_control.string = string;
	if (name)
		strcpy(multi->controls[i].mix_control.name, name);

	return multi->controls[i].mix_control.id;
}


static status_t
auich_create_controls_list(multi_dev *multi)
{
	uint32 	i = 0, index = 0, count, id, parent, parent2, parent3;
	const ac97_source_info *info;

	/* AC97 Mixer */
	parent = auich_create_group_control(multi, &index, 0, 0, "AC97 mixer");

	count = source_info_size;
	//Note that we ignore first item in source_info
	//It's for recording, but do match this with ac97.c's source_info
	for (i = 1; i < count ; i++) {
		info = &source_info[i];
		PRINT(("name : %s\n", info->name));

		parent2 = auich_create_group_control(multi, &index, parent, 0, info->name);

		if (info->type & B_MIX_GAIN) {
			if (info->type & B_MIX_MUTE) {
				multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
				multi->controls[index].mix_control.flags = B_MULTI_MIX_ENABLE;
				multi->controls[index].mix_control.master = EMU_MULTI_CONTROL_MASTERID;
				multi->controls[index].mix_control.parent = parent2;
				multi->controls[index].mix_control.string = S_MUTE;
				multi->controls[index].cookie = info;
				multi->controls[index].type = B_MIX_MUTE;
				multi->controls[index].get = &auich_ac97_get_mix;
				multi->controls[index].set = &auich_ac97_set_mix;
				index++;
			}

			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_GAIN;
			multi->controls[index].mix_control.master = EMU_MULTI_CONTROL_MASTERID;
			multi->controls[index].mix_control.parent = parent2;
			strcpy(multi->controls[index].mix_control.name, info->name);
			multi->controls[index].mix_control.u.gain.min_gain = info->min_gain;
			multi->controls[index].mix_control.u.gain.max_gain = info->max_gain;
			multi->controls[index].mix_control.u.gain.granularity = info->granularity;
			multi->controls[index].cookie = info;
			multi->controls[index].type = B_MIX_GAIN;
			multi->controls[index].get = &auich_ac97_get_mix;
			multi->controls[index].set = &auich_ac97_set_mix;
			id = multi->controls[index].mix_control.id;
			index++;

			if (info->type & B_MIX_STEREO) {
				multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
				multi->controls[index].mix_control.flags = B_MULTI_MIX_GAIN;
				multi->controls[index].mix_control.master = id;
				multi->controls[index].mix_control.parent = parent2;
				strcpy(multi->controls[index].mix_control.name, info->name);
				multi->controls[index].mix_control.u.gain.min_gain = info->min_gain;
				multi->controls[index].mix_control.u.gain.max_gain = info->max_gain;
				multi->controls[index].mix_control.u.gain.granularity = info->granularity;
				multi->controls[index].cookie = info;
				multi->controls[index].type = B_MIX_GAIN;
				multi->controls[index].get = &auich_ac97_get_mix;
				multi->controls[index].set = &auich_ac97_set_mix;
				index++;
			}

			if (info->type & B_MIX_MICBOOST) {
				multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
				multi->controls[index].mix_control.flags = B_MULTI_MIX_ENABLE;
				multi->controls[index].mix_control.master = EMU_MULTI_CONTROL_MASTERID;
				multi->controls[index].mix_control.parent = parent2;
				strcpy(multi->controls[index].mix_control.name, "+20 dB");
				multi->controls[index].cookie = info;
				multi->controls[index].type = B_MIX_MICBOOST;
				multi->controls[index].get = &auich_ac97_get_mix;
				multi->controls[index].set = &auich_ac97_set_mix;
				index++;
			}
		}
	}

	/* AC97 Record */
	parent = auich_create_group_control(multi, &index, 0, 0, "Recording");

	info = &source_info[0];
	PRINT(("name : %s\n", info->name));

	parent2 = auich_create_group_control(multi, &index, parent, 0, info->name);

	if (info->type & B_MIX_GAIN) {
		if (info->type & B_MIX_MUTE) {
			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_ENABLE;
			multi->controls[index].mix_control.master = EMU_MULTI_CONTROL_MASTERID;
			multi->controls[index].mix_control.parent = parent2;
			multi->controls[index].mix_control.string = S_MUTE;
			multi->controls[index].cookie = info;
			multi->controls[index].type = B_MIX_MUTE;
			multi->controls[index].get = &auich_ac97_get_mix;
			multi->controls[index].set = &auich_ac97_set_mix;
			index++;
		}

		multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
		multi->controls[index].mix_control.flags = B_MULTI_MIX_GAIN;
		multi->controls[index].mix_control.master = EMU_MULTI_CONTROL_MASTERID;
		multi->controls[index].mix_control.parent = parent2;
		strcpy(multi->controls[index].mix_control.name, info->name);
		multi->controls[index].mix_control.u.gain.min_gain = info->min_gain;
		multi->controls[index].mix_control.u.gain.max_gain = info->max_gain;
		multi->controls[index].mix_control.u.gain.granularity = info->granularity;
		multi->controls[index].cookie = info;
		multi->controls[index].type = B_MIX_GAIN;
		multi->controls[index].get = &auich_ac97_get_mix;
		multi->controls[index].set = &auich_ac97_set_mix;
		id = multi->controls[index].mix_control.id;
		index++;

		if (info->type & B_MIX_STEREO) {
			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_GAIN;
			multi->controls[index].mix_control.master = id;
			multi->controls[index].mix_control.parent = parent2;
			strcpy(multi->controls[index].mix_control.name, info->name);
			multi->controls[index].mix_control.u.gain.min_gain = info->min_gain;
			multi->controls[index].mix_control.u.gain.max_gain = info->max_gain;
			multi->controls[index].mix_control.u.gain.granularity = info->granularity;
			multi->controls[index].cookie = info;
			multi->controls[index].type = B_MIX_GAIN;
			multi->controls[index].get = &auich_ac97_get_mix;
			multi->controls[index].set = &auich_ac97_set_mix;
			index++;
		}

		if (info->type & B_MIX_RECORDMUX) {
			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX;
			multi->controls[index].mix_control.parent = parent2;
			strcpy(multi->controls[index].mix_control.name, "Record mux");
			multi->controls[index].cookie = info;
			multi->controls[index].type = B_MIX_MUX;
			multi->controls[index].get = &auich_ac97_get_mix;
			multi->controls[index].set = &auich_ac97_set_mix;
			parent3 = multi->controls[index].mix_control.id;
			index++;

			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			multi->controls[index].mix_control.string = S_MIC;
			index++;
			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			strcpy(multi->controls[index].mix_control.name, "CD in");
			index++;
			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			strcpy(multi->controls[index].mix_control.name, "Video in");
			index++;
			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			strcpy(multi->controls[index].mix_control.name, "Aux in");
			index++;
			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			strcpy(multi->controls[index].mix_control.name, "Line in");
			index++;
			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			multi->controls[index].mix_control.string = S_STEREO_MIX;
			index++;
			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			multi->controls[index].mix_control.string = S_MONO_MIX;
			index++;
			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
			multi->controls[index].mix_control.parent = parent3;
			strcpy(multi->controls[index].mix_control.name, "TAD");
			index++;
		}
	}

	multi->control_count = index;
	PRINT(("multi->control_count %" B_PRIu32 "\n", multi->control_count));
	return B_OK;
}


static status_t
auich_get_mix(auich_dev *card, multi_mix_value_info * mmvi)
{
	int32 i, id;
	multi_mixer_control *control = NULL;
	for (i = 0; i < mmvi->item_count; i++) {
		id = mmvi->values[i].id - EMU_MULTI_CONTROL_FIRSTID;
		if (id < 0 || id >= card->multi.control_count) {
			PRINT(("auich_get_mix : "
				"invalid control id requested : %" B_PRIi32 "\n", id));
			continue;
		}
		control = &card->multi.controls[id];

		if (control->mix_control.flags & B_MULTI_MIX_GAIN) {
			if (control->get) {
				float values[2];
				control->get(card, control->cookie, control->type, values);
				if (control->mix_control.master == EMU_MULTI_CONTROL_MASTERID)
					mmvi->values[i].u.gain = values[0];
				else
					mmvi->values[i].u.gain = values[1];
			}
		}

		if (control->mix_control.flags & B_MULTI_MIX_ENABLE && control->get) {
			float values[1];
			control->get(card, control->cookie, control->type, values);
			mmvi->values[i].u.enable = (values[0] == 1.0);
		}

		if (control->mix_control.flags & B_MULTI_MIX_MUX && control->get) {
			float values[1];
			control->get(card, control->cookie, control->type, values);
			mmvi->values[i].u.mux = (int32)values[0];
		}
	}
	return B_OK;
}


static status_t
auich_set_mix(auich_dev *card, multi_mix_value_info * mmvi)
{
	int32 i, id;
	multi_mixer_control *control = NULL;
	for (i = 0; i < mmvi->item_count; i++) {
		id = mmvi->values[i].id - EMU_MULTI_CONTROL_FIRSTID;
		if (id < 0 || id >= card->multi.control_count) {
			PRINT(("auich_set_mix : "
				"invalid control id requested : %" B_PRIi32 "\n", id));
			continue;
		}
		control = &card->multi.controls[id];

		if (control->mix_control.flags & B_MULTI_MIX_GAIN) {
			multi_mixer_control *control2 = NULL;
			if (i+1<mmvi->item_count) {
				id = mmvi->values[i + 1].id - EMU_MULTI_CONTROL_FIRSTID;
				if (id < 0 || id >= card->multi.control_count) {
					PRINT(("auich_set_mix : "
						"invalid control id requested : %" B_PRIi32 "\n", id));
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

				if (control->mix_control.master == EMU_MULTI_CONTROL_MASTERID)
					values[0] = mmvi->values[i].u.gain;
				else
					values[1] = mmvi->values[i].u.gain;

				if (control2 && control2->mix_control.master != EMU_MULTI_CONTROL_MASTERID)
					values[1] = mmvi->values[i+1].u.gain;

				control->set(card, control->cookie, control->type, values);
			}

			if (control2)
				i++;
		}

		if (control->mix_control.flags & B_MULTI_MIX_ENABLE && control->set) {
			float values[1];

			values[0] = mmvi->values[i].u.enable ? 1.0 : 0.0;
			control->set(card, control->cookie, control->type, values);
		}

		if (control->mix_control.flags & B_MULTI_MIX_MUX && control->set) {
			float values[1];

			values[0] = (float)mmvi->values[i].u.mux;
			control->set(card, control->cookie, control->type, values);
		}
	}
	return B_OK;
}


static status_t
auich_list_mix_controls(auich_dev *card, multi_mix_control_info * mmci)
{
	multi_mix_control	*mmc;
	int32 i;

	mmc = mmci->controls;
	if (mmci->control_count < 24)
		return B_ERROR;

	if (auich_create_controls_list(&card->multi) < B_OK)
		return B_ERROR;
	for (i = 0; i < card->multi.control_count; i++) {
		mmc[i] = card->multi.controls[i].mix_control;
	}

	mmci->control_count = card->multi.control_count;
	return B_OK;
}


static status_t
auich_list_mix_connections(auich_dev *card, multi_mix_connection_info * data)
{
	return B_ERROR;
}


static status_t
auich_list_mix_channels(auich_dev *card, multi_mix_channel_info *data)
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
auich_create_channels_list(multi_dev *multi)
{
	auich_stream *stream;
	uint32 index, i, mode, designations;
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

	for (mode=AUICH_USE_PLAY; mode!=-1;
		mode = (mode == AUICH_USE_PLAY) ? AUICH_USE_RECORD : -1) {
		LIST_FOREACH(stream, &((auich_dev*)multi->card)->streams, next) {
			if ((stream->use & mode) == 0)
				continue;

			if (stream->channels == 2)
				designations = B_CHANNEL_STEREO_BUS;
			else
				designations = B_CHANNEL_SURROUND_BUS;

			for (i = 0; i < stream->channels; i++) {
				chans[index].channel_id = index;
				chans[index].kind = (mode == AUICH_USE_PLAY) ? B_MULTI_OUTPUT_CHANNEL : B_MULTI_INPUT_CHANNEL;
				chans[index].designations = designations | chan_designations[i];
				chans[index].connectors = 0;
				index++;
			}
		}

		if (mode==AUICH_USE_PLAY) {
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
auich_get_description(auich_dev *card, multi_description *data)
{
	uint32 size;

	data->interface_version = B_CURRENT_INTERFACE_VERSION;
	data->interface_minimum = B_CURRENT_INTERFACE_VERSION;

	switch(card->info.vendor_id) {
		case INTEL_VENDOR_ID:
			strncpy(data->friendly_name, FRIENDLY_NAME_ICH, 32);
		break;
		case SIS_VENDOR_ID:
			strncpy(data->friendly_name, FRIENDLY_NAME_SIS, 32);
		break;
		case NVIDIA_VENDOR_ID:
			strncpy(data->friendly_name, FRIENDLY_NAME_NVIDIA, 32);
		break;
		case AMD_VENDOR_ID:
			strncpy(data->friendly_name, FRIENDLY_NAME_AMD, 32);
		break;
	}

	strcpy(data->vendor_info, AUTHOR);

	/*data->output_channel_count = 6;
	data->input_channel_count = 4;
	data->output_bus_channel_count = 2;
	data->input_bus_channel_count = 2;
	data->aux_bus_channel_count = 0;*/

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
		case 48000: data->output_rates = data->input_rates = B_SR_48000; break;
		case 44100: data->output_rates = data->input_rates = B_SR_44100; break;
	}
	data->min_cvsr_rate = 0;
	data->max_cvsr_rate = 48000;

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
auich_get_enabled_channels(auich_dev *card, multi_channel_enable *data)
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
auich_get_global_format(auich_dev *card, multi_format_info *data)
{
	data->output_latency = 0;
	data->input_latency = 0;
	data->timecode_kind = 0;
	switch (current_settings.sample_rate) {
		case 48000:
			data->input.rate = data->output.rate = B_SR_48000;
			data->input.cvsr = data->output.cvsr = 48000;
			break;
		case 44100:
			data->input.rate = data->output.rate = B_SR_44100;
			data->input.cvsr = data->output.cvsr = 44100;
			break;
	}
	data->input.format = data->output.format = B_FMT_16BIT;
	return B_OK;
}


static status_t
auich_set_global_format(auich_dev *card, multi_format_info* data)
{
	// TODO: it looks like we're not supposed to fail; fix this!
	return B_OK;
}


static status_t
auich_get_buffers(auich_dev *card, multi_buffer_list *data)
{
	uint8 i, j, pchannels, rchannels, bufcount;

	LOG(("flags = %#x\n",data->flags));
	LOG(("request_playback_buffers = %#x\n",data->request_playback_buffers));
	LOG(("request_playback_channels = %#x\n",data->request_playback_channels));
	LOG(("request_playback_buffer_size = %#x\n",data->request_playback_buffer_size));
	LOG(("request_record_buffers = %#x\n",data->request_record_buffers));
	LOG(("request_record_channels = %#x\n",data->request_record_channels));
	LOG(("request_record_buffer_size = %#x\n",data->request_record_buffer_size));

	pchannels = card->pstream->channels;
	rchannels = card->rstream->channels;

	if (data->request_playback_buffers < current_settings.buffer_count ||
		data->request_playback_channels < (pchannels) ||
		data->request_record_buffers < current_settings.buffer_count ||
		data->request_record_channels < (rchannels)) {
		LOG(("not enough channels/buffers\n"));
	}

	ASSERT(current_settings.buffer_count == 2);

	data->flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD; // XXX ???
//	data->flags = 0;

	data->return_playback_buffers = current_settings.buffer_count;	/* playback_buffers[b][] */
	data->return_playback_channels = pchannels;		/* playback_buffers[][c] */
	data->return_playback_buffer_size = current_settings.buffer_frames;		/* frames */

	bufcount = current_settings.buffer_count;
	if (bufcount > data->request_playback_buffers)
		bufcount = data->request_playback_buffers;

	for (i = 0; i < bufcount; i++) {
		struct buffer_desc descs[data->return_playback_channels];
		for (j=0; j<pchannels; j++)
			auich_stream_get_nth_buffer(card->pstream, j, i,
				&descs[j].base,
				&descs[j].stride);
		if (!IS_USER_ADDRESS(data->playback_buffers[i])
			|| user_memcpy(data->playback_buffers[i], descs, sizeof(descs))
			< B_OK) {
			return B_BAD_ADDRESS;
		}
	}
	data->return_record_buffers = current_settings.buffer_count;
	data->return_record_channels = rchannels;
	data->return_record_buffer_size = current_settings.buffer_frames;	/* frames */

	bufcount = current_settings.buffer_count;
	if (bufcount > data->request_record_buffers)
		bufcount = data->request_record_buffers;

	for (i = 0; i < bufcount; i++) {
		struct buffer_desc descs[data->return_record_channels];
		for (j=0; j<rchannels; j++)
			auich_stream_get_nth_buffer(card->rstream, j, i,
				&descs[j].base,
				&descs[j].stride);
		if (!IS_USER_ADDRESS(data->record_buffers[i])
			|| user_memcpy(data->record_buffers[i], descs, sizeof(descs))
			< B_OK) {
			return B_BAD_ADDRESS;
		}
	}
	return B_OK;
}


static void
auich_play_inth(void* inthparams)
{
	auich_stream *stream = (auich_stream *)inthparams;
	//int32 count;

	acquire_spinlock(&slock);
	stream->real_time = system_time();
	stream->frames_count += current_settings.buffer_frames;
	stream->buffer_cycle = (stream->trigblk
		+ stream->blkmod - 1) % stream->blkmod;
	stream->update_needed = true;
	release_spinlock(&slock);

	TRACE(("auich_play_inth : cycle : %d\n", stream->buffer_cycle));

	//get_sem_count(stream->card->buffer_ready_sem, &count);
	//if (count <= 0)
		release_sem_etc(stream->card->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
}


static void
auich_record_inth(void* inthparams)
{
	auich_stream *stream = (auich_stream *)inthparams;
	//int32 count;

	acquire_spinlock(&slock);
	stream->real_time = system_time();
	stream->frames_count += current_settings.buffer_frames;
	stream->buffer_cycle = (stream->trigblk
		+ stream->blkmod - 1) % stream->blkmod;
	stream->update_needed = true;
	release_spinlock(&slock);

	TRACE(("auich_record_inth : cycle : %d\n", stream->buffer_cycle));

	//get_sem_count(stream->card->buffer_ready_sem, &count);
	//if (count <= 0)
		release_sem_etc(stream->card->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
}


static status_t
auich_buffer_exchange(auich_dev *card, multi_buffer_info *data)
{
	cpu_status status;
	auich_stream *pstream, *rstream;
	multi_buffer_info buffer_info;

#ifdef __HAIKU__
	if (user_memcpy(&buffer_info, data, sizeof(buffer_info)) < B_OK)
		return B_BAD_ADDRESS;
#else
	memcpy(&buffer_info, data, sizeof(buffer_info));
#endif

	buffer_info.flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD;

	if (!(card->pstream->state & AUICH_STATE_STARTED))
		auich_stream_start(card->pstream, auich_play_inth, card->pstream);

	if (!(card->rstream->state & AUICH_STATE_STARTED))
		auich_stream_start(card->rstream, auich_record_inth, card->rstream);

	if (acquire_sem_etc(card->buffer_ready_sem, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, 50000)
		== B_TIMED_OUT) {
		LOG(("buffer_exchange timeout ff\n"));
	}

	status = lock();

	LIST_FOREACH(pstream, &card->streams, next) {
		if ((pstream->use & AUICH_USE_PLAY) == 0 ||
			(pstream->state & AUICH_STATE_STARTED) == 0)
			continue;
		if (pstream->update_needed)
			break;
	}

	LIST_FOREACH(rstream, &card->streams, next) {
		if ((rstream->use & AUICH_USE_RECORD) == 0 ||
			(rstream->state & AUICH_STATE_STARTED) == 0)
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
auich_buffer_force_stop(auich_dev *card)
{
	//auich_voice_halt(card->pvoice);
	return B_OK;
}


#define cookie_type auich_dev
#define get_description auich_get_description
#define get_enabled_channels auich_get_enabled_channels
#define get_global_format auich_get_global_format
#define set_global_format auich_set_global_format
#define list_mix_channels auich_list_mix_channels
#define list_mix_controls auich_list_mix_controls
#define list_mix_connections auich_list_mix_connections
#define get_mix auich_get_mix
#define set_mix auich_set_mix
#define get_buffers auich_get_buffers
#define buffer_exchange auich_buffer_exchange
#define buffer_force_stop auich_buffer_force_stop
#include "../generic/multi.c"


static status_t
auich_multi_control(void *cookie, uint32 op, void *arg, size_t length)
{
	auich_dev *card = (auich_dev *)cookie;

	return multi_audio_control_generic(card, op, arg, length);
}

static status_t auich_open(const char *name, uint32 flags, void** cookie);
static status_t auich_close(void* cookie);
static status_t auich_free(void* cookie);
static status_t auich_control(void* cookie, uint32 op, void* arg, size_t len);
static status_t auich_read(void* cookie, off_t position, void *buf, size_t* num_bytes);
static status_t auich_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes);

device_hooks multi_hooks = {
	auich_open, 			/* -> open entry point */
	auich_close, 			/* -> close entry point */
	auich_free,			/* -> free cookie */
	auich_control, 		/* -> control entry point */
	auich_read,			/* -> read entry point */
	auich_write,			/* -> write entry point */
	NULL,					/* start select */
	NULL,					/* stop select */
	NULL,					/* scatter-gather read from the device */
	NULL					/* scatter-gather write to the device */
};


static status_t
auich_open(const char *name, uint32 flags, void** cookie)
{
	auich_dev *card = NULL;
	void *settings_handle;
	int ix;

	LOG(("open()\n"));

	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(cards[ix].name, name)) {
			card = &cards[ix];
		}
	}

	if (card == NULL) {
		LOG(("open() card not found %s\n", name));
		for (ix=0; ix<num_cards; ix++) {
			LOG(("open() card available %s\n", cards[ix].name));
		}
		return B_ERROR;
	}

	LOG(("open() got card\n"));

	if (card->pstream !=NULL)
		return B_ERROR;
	if (card->rstream !=NULL)
		return B_ERROR;

	*cookie = card;
	card->multi.card = card;

	// get driver settings
	settings_handle = load_driver_settings(AUICH_SETTINGS);
	if (settings_handle != NULL) {
		const char *item;
		char       *end;
		uint32      value;

		item = get_driver_parameter (settings_handle, "sample_rate", "48000", "48000");
		value = strtoul (item, &end, 0);
		if (*end == '\0')
			current_settings.sample_rate = value;

		item = get_driver_parameter (settings_handle, "buffer_frames", "256", "256");
		value = strtoul (item, &end, 0);
		if (*end == '\0')
			current_settings.buffer_frames = value;

		item = get_driver_parameter (settings_handle, "buffer_count", "4", "4");
		value = strtoul (item, &end, 0);
		if (*end == '\0')
			current_settings.buffer_count = value;

		unload_driver_settings(settings_handle);
	}

	LOG(("stream_new\n"));

	card->rstream = auich_stream_new(card, AUICH_USE_RECORD, current_settings.buffer_frames, current_settings.buffer_count);
	card->pstream = auich_stream_new(card, AUICH_USE_PLAY, current_settings.buffer_frames, current_settings.buffer_count);

	card->buffer_ready_sem = create_sem(0, "pbuffer ready");

	LOG(("stream_setaudio\n"));

	auich_stream_set_audioparms(card->pstream, 2, true, current_settings.sample_rate);
	auich_stream_set_audioparms(card->rstream, 2, true, current_settings.sample_rate);

	card->pstream->first_channel = 0;
	card->rstream->first_channel = 2;

	auich_stream_commit_parms(card->pstream);
	auich_stream_commit_parms(card->rstream);

	auich_create_channels_list(&card->multi);

	return B_OK;
}


static status_t
auich_close(void* cookie)
{
	//auich_dev *card = cookie;
	LOG(("close()\n"));

	return B_OK;
}


static status_t
auich_free(void* cookie)
{
	auich_dev *card = cookie;
	auich_stream *stream;
	LOG(("free()\n"));

	if (card->buffer_ready_sem > B_OK)
			delete_sem(card->buffer_ready_sem);

	LIST_FOREACH(stream, &card->streams, next) {
		auich_stream_halt(stream);
	}

	while (!LIST_EMPTY(&card->streams)) {
		auich_stream_delete(LIST_FIRST(&card->streams));
	}

	card->pstream = NULL;
	card->rstream = NULL;

	return B_OK;
}


static status_t
auich_control(void* cookie, uint32 op, void* arg, size_t len)
{
	return auich_multi_control(cookie, op, arg, len);
}


static status_t
auich_read(void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was read */
	return B_IO_ERROR;
}


static status_t
auich_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was written */
	return B_IO_ERROR;
}
