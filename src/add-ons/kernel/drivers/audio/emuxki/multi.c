/*
 * Emuxki BeOS Driver for Creative Labs SBLive!/Audigy series
 *
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
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
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "hmulti_audio.h"
#include "multi.h"
#include "ac97.h"
#include "debug.h"
#include "emuxki.h"
#include "util.h"
#include "io.h"

static void
emuxki_ac97_get_mix(void *card, const void *cookie, int32 type, float *values) {
	emuxki_dev *dev = (emuxki_dev*)card;
	ac97_source_info *info = (ac97_source_info *)cookie;
	uint16 value, mask;
	float gain;

	switch(type) {
		case B_MIX_GAIN:
			value = emuxki_codec_read(&dev->config, info->reg);
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
			value = emuxki_codec_read(&dev->config, info->reg);
			//PRINT(("B_MIX_MUTE value : %u\n", value));
			value &= mask;
			values[0] = ((value >> 15) == 1) ? 1.0 : 0.0;
			break;
		case B_MIX_MICBOOST:
			mask = ((1 << 1) - 1) << 6;
			value = emuxki_codec_read(&dev->config, info->reg);
			//PRINT(("B_MIX_MICBOOST value : %u\n", value));
			value &= mask;
			values[0] = ((value >> 6) == 1) ? 1.0 : 0.0;
			break;
		case B_MIX_MUX:
			mask = ((1 << 3) - 1);
			value = emuxki_codec_read(&dev->config, AC97_RECORD_SELECT);
			value &= mask;
			//PRINT(("B_MIX_MUX value : %u\n", value));
			values[0] = (float)value;
			break;
	}
}

static void
emuxki_ac97_set_mix(void *card, const void *cookie, int32 type, float *values) {
	emuxki_dev *dev = (emuxki_dev*)card;
	ac97_source_info *info = (ac97_source_info *)cookie;
	uint16 value, mask;
	float gain;

	switch(type) {
		case B_MIX_GAIN:
			value = emuxki_codec_read(&dev->config, info->reg);
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
			emuxki_codec_write(&dev->config, info->reg, value);
			break;
		case B_MIX_MUTE:
			mask = ((1 << 1) - 1) << 15;
			value = emuxki_codec_read(&dev->config, info->reg);
			value &= ~mask;
			value |= ((values[0] == 1.0 ? 1 : 0 ) << 15 & mask);
			if (info->reg == AC97_SURROUND_VOLUME) {
				// there is a independent mute for each channel
				mask = ((1 << 1) - 1) << 7;
				value &= ~mask;
				value |= ((values[0] == 1.0 ? 1 : 0 ) << 7 & mask);
			}
			//PRINT(("B_MIX_MUTE value : %u\n", value));
			emuxki_codec_write(&dev->config, info->reg, value);
			break;
		case B_MIX_MICBOOST:
			mask = ((1 << 1) - 1) << 6;
			value = emuxki_codec_read(&dev->config, info->reg);
			value &= ~mask;
			value |= ((values[0] == 1.0 ? 1 : 0 ) << 6 & mask);
			//PRINT(("B_MIX_MICBOOST value : %u\n", value));
			emuxki_codec_write(&dev->config, info->reg, value);
			break;
		case B_MIX_MUX:
			mask = ((1 << 3) - 1);
			value = ((int32)values[0]) & mask;
			value = value | (value << 8);
			//PRINT(("B_MIX_MUX value : %u\n", value));
			emuxki_codec_write(&dev->config, AC97_RECORD_SELECT, value);
			break;
	}

}

static void
emuxki_gpr_get_mix(void *card, const void *cookie, int32 type, float *values) {
	emuxki_gpr_get((emuxki_dev*)card, (emuxki_gpr *)cookie, type, values);
}

static void
emuxki_gpr_set_mix(void *card, const void *cookie, int32 type, float *values) {
	emuxki_gpr_set((emuxki_dev*)card, (emuxki_gpr *)cookie, type, values);
}

static void
emuxki_parameter_get_mix(void *card, const void *cookie, int32 type, float *values) {
	int32 value;
	emuxki_parameter_get((emuxki_dev*)card, cookie, type, &value);
	values[0] = (float)value;
}

static void
emuxki_parameter_set_mix(void *card, const void *cookie, int32 type, float *values) {
	int32 value;
	value = (int32)values[0];
	emuxki_parameter_set((emuxki_dev*)card, cookie, type, &value);
}

static int32
emuxki_create_group_control(multi_dev *multi, int32 *index, int32 parent,
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

static void
emuxki_create_gpr_control(multi_dev *multi, int32 *index, int32 parent, int32 string,
	const emuxki_gpr *gpr) {
	int32 i = *index, id;
	multi_mixer_control control;

	control.mix_control.master = EMU_MULTI_CONTROL_MASTERID;
	control.mix_control.parent = parent;
	control.cookie = gpr;
	control.get = &emuxki_gpr_get_mix;
	control.set = &emuxki_gpr_set_mix;
	control.mix_control.u.gain.min_gain = gpr->min_gain;
	control.mix_control.u.gain.max_gain = gpr->max_gain;
	control.mix_control.u.gain.granularity = gpr->granularity;

	if (gpr->type & EMU_MIX_GAIN) {
		if (gpr->type & EMU_MIX_MUTE) {
			control.mix_control.id = EMU_MULTI_CONTROL_FIRSTID + i;
			control.mix_control.flags = B_MULTI_MIX_ENABLE;
			control.mix_control.string = S_MUTE;
			control.type = EMU_MIX_MUTE;
			multi->controls[i] = control;
			i++;
		}

		control.mix_control.id = EMU_MULTI_CONTROL_FIRSTID + i;
		control.mix_control.flags = B_MULTI_MIX_GAIN;
		strcpy(control.mix_control.name, gpr->name);
		control.type = EMU_MIX_GAIN;
		multi->controls[i] = control;
		id = control.mix_control.id;
		i++;

		if (gpr->type & EMU_MIX_STEREO) {
			control.mix_control.id = EMU_MULTI_CONTROL_FIRSTID + i;
			control.mix_control.master = id;
			multi->controls[i] = control;
			i++;
		}
	}
	*index = i;
}

static status_t
emuxki_create_controls_list(multi_dev *multi)
{
	uint32 	i = 0, index = 0, count, id, parent, parent2, parent3;
	emuxki_dev *card = (emuxki_dev*)multi->card;
	const ac97_source_info *info;

	parent = emuxki_create_group_control(multi, &index, 0, 0, "Playback");

	for (i = EMU_GPR_FIRST_MIX; i < card->gpr_count; i++) {
		const emuxki_gpr *gpr = &card->gpr[i];
		if ((gpr->type & EMU_MIX_PLAYBACK) == 0)
			continue;

		parent2 = emuxki_create_group_control(multi, &index, parent, 0, gpr->name);

		emuxki_create_gpr_control(multi, &index, parent2, 0, gpr);
		if (gpr->type & EMU_MIX_GAIN && gpr->type & EMU_MIX_STEREO)
			i++;
	}

	parent = emuxki_create_group_control(multi, &index, 0, 0, "Record");

	for (i = EMU_GPR_FIRST_MIX; i < card->gpr_count; i++) {
		const emuxki_gpr *gpr = &card->gpr[i];
		if ((gpr->type & EMU_MIX_RECORD) == 0)
			continue;
		parent2 = emuxki_create_group_control(multi, &index, parent, 0, gpr->name);

		emuxki_create_gpr_control(multi, &index, parent2, 0, gpr);
		if (gpr->type & EMU_MIX_GAIN && gpr->type & EMU_MIX_STEREO)
			i++;
	}

	/* AC97 Record */
	info = &source_info[0];
	PRINT(("name : %s\n", info->name));

	parent2 = emuxki_create_group_control(multi, &index, parent, 0, info->name);

	if (info->type & B_MIX_GAIN) {
		if (info->type & B_MIX_MUTE) {
			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_ENABLE;
			multi->controls[index].mix_control.master = EMU_MULTI_CONTROL_MASTERID;
			multi->controls[index].mix_control.parent = parent2;
			multi->controls[index].mix_control.string = S_MUTE;
			multi->controls[index].cookie = info;
			multi->controls[index].type = B_MIX_MUTE;
			multi->controls[index].get = &emuxki_ac97_get_mix;
			multi->controls[index].set = &emuxki_ac97_set_mix;
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
		multi->controls[index].get = &emuxki_ac97_get_mix;
		multi->controls[index].set = &emuxki_ac97_set_mix;
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
			multi->controls[index].get = &emuxki_ac97_get_mix;
			multi->controls[index].set = &emuxki_ac97_set_mix;
			index++;
		}

		if (info->type & B_MIX_RECORDMUX) {
			multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
			multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX;
			multi->controls[index].mix_control.parent = parent2;
			strcpy(multi->controls[index].mix_control.name, "Record mux");
			multi->controls[index].cookie = info;
			multi->controls[index].type = B_MIX_MUX;
			multi->controls[index].get = &emuxki_ac97_get_mix;
			multi->controls[index].set = &emuxki_ac97_set_mix;
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

	parent = emuxki_create_group_control(multi, &index, 0, 0, "AC97 mixer");

	count = source_info_size;
	if (IS_AUDIGY2(&card->config))
		count = 1;
	if (!IS_LIVE_5_1(&card->config) && !IS_AUDIGY(&card->config))
		count--;

	for (i = 1; i < count ; i++) {
		info = &source_info[i];
		PRINT(("name : %s\n", info->name));

		parent2 = emuxki_create_group_control(multi, &index, parent, 0, info->name);

		if (info->type & B_MIX_GAIN) {
			if (info->type & B_MIX_MUTE) {
				multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
				multi->controls[index].mix_control.flags = B_MULTI_MIX_ENABLE;
				multi->controls[index].mix_control.master = EMU_MULTI_CONTROL_MASTERID;
				multi->controls[index].mix_control.parent = parent2;
				multi->controls[index].mix_control.string = S_MUTE;
				multi->controls[index].cookie = info;
				multi->controls[index].type = B_MIX_MUTE;
				multi->controls[index].get = &emuxki_ac97_get_mix;
				multi->controls[index].set = &emuxki_ac97_set_mix;
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
			multi->controls[index].get = &emuxki_ac97_get_mix;
			multi->controls[index].set = &emuxki_ac97_set_mix;
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
				multi->controls[index].get = &emuxki_ac97_get_mix;
				multi->controls[index].set = &emuxki_ac97_set_mix;
				index++;
			}
		}
	}

	parent = emuxki_create_group_control(multi, &index, 0, S_SETUP, NULL);

	/* AC97 20db Boost Mic */
	info = &source_info[6];

	if (info->type & B_MIX_GAIN && info->type & B_MIX_MICBOOST) {
		multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
		multi->controls[index].mix_control.flags = B_MULTI_MIX_ENABLE;
		multi->controls[index].mix_control.master = EMU_MULTI_CONTROL_MASTERID;
		multi->controls[index].mix_control.parent = parent;
		strcpy(multi->controls[index].mix_control.name, "Mic +20dB");
		multi->controls[index].cookie = info;
		multi->controls[index].type = B_MIX_MICBOOST;
		multi->controls[index].get = &emuxki_ac97_get_mix;
		multi->controls[index].set = &emuxki_ac97_set_mix;
		index++;
	}

	if (true) {
		multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
		multi->controls[index].mix_control.flags = B_MULTI_MIX_ENABLE;
		multi->controls[index].mix_control.master = EMU_MULTI_CONTROL_MASTERID;
		multi->controls[index].mix_control.parent = parent;
		strcpy(multi->controls[index].mix_control.name, "Enable digital");
		multi->controls[index].cookie = NULL;
		multi->controls[index].type = EMU_DIGITAL_MODE;
		multi->controls[index].get = &emuxki_parameter_get_mix;
		multi->controls[index].set = &emuxki_parameter_set_mix;
		index++;
	}

	if (true) {
		multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
		multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX;
		multi->controls[index].mix_control.parent = parent;
		strcpy(multi->controls[index].mix_control.name, "Audio mode");
		multi->controls[index].cookie = NULL;
		multi->controls[index].type = EMU_AUDIO_MODE;
		multi->controls[index].get = &emuxki_parameter_get_mix;
		multi->controls[index].set = &emuxki_parameter_set_mix;
		parent2 = multi->controls[index].mix_control.id;
		index++;

		multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
		multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
		multi->controls[index].mix_control.parent = parent2;
		strcpy(multi->controls[index].mix_control.name, "2.0");
		index++;
		multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
		multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
		multi->controls[index].mix_control.parent = parent2;
		strcpy(multi->controls[index].mix_control.name, "4.0");
		index++;
		multi->controls[index].mix_control.id = EMU_MULTI_CONTROL_FIRSTID + index;
		multi->controls[index].mix_control.flags = B_MULTI_MIX_MUX_VALUE;
		multi->controls[index].mix_control.parent = parent2;
		strcpy(multi->controls[index].mix_control.name, "5.1");
		index++;
	}

	multi->control_count = index;
	PRINT(("multi->control_count %" B_PRIu32 "\n", multi->control_count));
	return B_OK;
}

static status_t
emuxki_get_mix(emuxki_dev *card, multi_mix_value_info * mmvi)
{
	int32 i;
	uint32 id;
	multi_mixer_control *control = NULL;
	for (i = 0; i < mmvi->item_count; i++) {
		id = mmvi->values[i].id - EMU_MULTI_CONTROL_FIRSTID;
		if (id < 0 || id >= card->multi.control_count) {
			PRINT(("emuxki_get_mix : "
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
emuxki_set_mix(emuxki_dev *card, multi_mix_value_info * mmvi)
{
	int32 i;
	uint32 id;
	multi_mixer_control *control = NULL;
	for (i = 0; i < mmvi->item_count; i++) {
		id = mmvi->values[i].id - EMU_MULTI_CONTROL_FIRSTID;
		if (id < 0 || id >= card->multi.control_count) {
			PRINT(("emuxki_set_mix : "
				"invalid control id requested : %" B_PRIi32 "\n", id));
			continue;
		}
		control = &card->multi.controls[id];

		if (control->mix_control.flags & B_MULTI_MIX_GAIN) {
			multi_mixer_control *control2 = NULL;
			if (i+1<mmvi->item_count) {
				id = mmvi->values[i + 1].id - EMU_MULTI_CONTROL_FIRSTID;
				if (id < 0 || id >= card->multi.control_count) {
					PRINT(("emuxki_set_mix : "
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
emuxki_list_mix_controls(emuxki_dev *card, multi_mix_control_info * mmci)
{
	multi_mix_control	*mmc;
	uint32 i;

	mmc = mmci->controls;
	if (mmci->control_count < EMU_MULTICONTROLSNUM)
		return B_ERROR;

	if (emuxki_create_controls_list(&card->multi) < B_OK)
		return B_ERROR;
	for (i = 0; i < card->multi.control_count; i++) {
		mmc[i] = card->multi.controls[i].mix_control;
	}

	mmci->control_count = card->multi.control_count;
	return B_OK;
}

static status_t
emuxki_list_mix_connections(emuxki_dev *card, multi_mix_connection_info * data)
{
	return B_ERROR;
}

static status_t
emuxki_list_mix_channels(emuxki_dev *card, multi_mix_channel_info *data)
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
emuxki_create_channels_list(multi_dev *multi)
{
	emuxki_stream *stream;
	uint32 index, i, designations, nchannels;
	int32 mode;
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

	for (mode=EMU_USE_PLAY; mode!=-1;
		mode = (mode == EMU_USE_PLAY) ? EMU_USE_RECORD : -1) {
		LIST_FOREACH(stream, &((emuxki_dev*)multi->card)->streams, next) {
			if ((stream->use & mode) == 0)
				continue;

			nchannels = stream->nmono + 2 * stream->nstereo;
			if (nchannels == 2)
				designations = B_CHANNEL_STEREO_BUS;
			else
				designations = B_CHANNEL_SURROUND_BUS;

			for (i = 0; i < nchannels; i++) {
				chans[index].channel_id = index;
				chans[index].kind = (mode == EMU_USE_PLAY) ? B_MULTI_OUTPUT_CHANNEL : B_MULTI_INPUT_CHANNEL;
				chans[index].designations = designations | chan_designations[i];
				chans[index].connectors = 0;
				index++;
			}
		}

		if (mode==EMU_USE_PLAY) {
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
emuxki_get_description(emuxki_dev *card, multi_description *data)
{
	int32 size;

	data->interface_version = B_CURRENT_INTERFACE_VERSION;
	data->interface_minimum = B_CURRENT_INTERFACE_VERSION;

	if (IS_AUDIGY2_VALUE(&card->config))
		strncpy(data->friendly_name, FRIENDLY_NAME_AUDIGY2_VALUE, 32);
	else if (IS_AUDIGY2(&card->config))
		strncpy(data->friendly_name, FRIENDLY_NAME_AUDIGY2, 32);
	else if (IS_AUDIGY(&card->config))
		strncpy(data->friendly_name, FRIENDLY_NAME_AUDIGY, 32);
	else if (IS_LIVE_5_1(&card->config))
		strncpy(data->friendly_name, FRIENDLY_NAME_LIVE_5_1, 32);
	else
		strncpy(data->friendly_name, FRIENDLY_NAME_LIVE, 32);
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

	strcpy(data->control_panel,"");

	return B_OK;
}

static status_t
emuxki_get_enabled_channels(emuxki_dev *card, multi_channel_enable *data)
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
emuxki_set_enabled_channels(emuxki_dev *card, multi_channel_enable *data)
{
	PRINT(("set_enabled_channels 0 : %s\n", B_TEST_CHANNEL(data->enable_bits, 0) ? "enabled": "disabled"));
	PRINT(("set_enabled_channels 1 : %s\n", B_TEST_CHANNEL(data->enable_bits, 1) ? "enabled": "disabled"));
	PRINT(("set_enabled_channels 2 : %s\n", B_TEST_CHANNEL(data->enable_bits, 2) ? "enabled": "disabled"));
	PRINT(("set_enabled_channels 3 : %s\n", B_TEST_CHANNEL(data->enable_bits, 3) ? "enabled": "disabled"));
	return B_OK;
}

static status_t
emuxki_get_global_format(emuxki_dev *card, multi_format_info *data)
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
emuxki_get_buffers(emuxki_dev *card, multi_buffer_list *data)
{
	int32 i, j, pchannels, pchannels2, rchannels, rchannels2;

	LOG(("flags = %#x\n",data->flags));
	LOG(("request_playback_buffers = %#x\n",data->request_playback_buffers));
	LOG(("request_playback_channels = %#x\n",data->request_playback_channels));
	LOG(("request_playback_buffer_size = %#x\n",data->request_playback_buffer_size));
	LOG(("request_record_buffers = %#x\n",data->request_record_buffers));
	LOG(("request_record_channels = %#x\n",data->request_record_channels));
	LOG(("request_record_buffer_size = %#x\n",data->request_record_buffer_size));

	pchannels = card->pstream->nmono + card->pstream->nstereo * 2;
	pchannels2 = card->pstream2->nmono + card->pstream2->nstereo * 2;
	rchannels = card->rstream->nmono + card->rstream->nstereo * 2;
	rchannels2 = card->rstream2->nmono + card->rstream2->nstereo * 2;

	if (data->request_playback_buffers < current_settings.buffer_count ||
		data->request_playback_channels < (pchannels + pchannels2) ||
		data->request_record_buffers < current_settings.buffer_count ||
		data->request_record_channels < (rchannels + rchannels2)) {
		LOG(("not enough channels/buffers\n"));
	}

	data->flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD;

	data->return_playback_buffers = current_settings.buffer_count;	/* playback_buffers[b][] */
	data->return_playback_channels = pchannels + pchannels2;		/* playback_buffers[][c] */
	data->return_playback_buffer_size = current_settings.buffer_frames;		/* frames */

	for (i = 0; i < current_settings.buffer_count; i++)
		for (j=0; j<pchannels; j++)
			emuxki_stream_get_nth_buffer(card->pstream, j, i,
				&data->playback_buffers[i][j].base,
				&data->playback_buffers[i][j].stride);

	for (i = 0; i < current_settings.buffer_count; i++)
		for (j=0; j<pchannels2; j++)
			emuxki_stream_get_nth_buffer(card->pstream2, j, i,
				&data->playback_buffers[i][pchannels + j].base,
				&data->playback_buffers[i][pchannels + j].stride);

	data->return_record_buffers = current_settings.buffer_count;
	data->return_record_channels = rchannels + rchannels2;
	data->return_record_buffer_size = current_settings.buffer_frames;	/* frames */

	for (i = 0; i < current_settings.buffer_count; i++)
		for (j=0; j<rchannels; j++)
			emuxki_stream_get_nth_buffer(card->rstream, j, i,
				&data->record_buffers[i][j].base,
				&data->record_buffers[i][j].stride);

	for (i = 0; i < current_settings.buffer_count; i++)
		for (j=0; j<rchannels2; j++)
			emuxki_stream_get_nth_buffer(card->rstream2, j, i,
				&data->record_buffers[i][rchannels + j].base,
				&data->record_buffers[i][rchannels + j].stride);

	return B_OK;
}


static void
emuxki_play_inth(void* inthparams)
{
	emuxki_stream *stream = (emuxki_stream *)inthparams;
	//int32 count;

	acquire_spinlock(&slock);
	stream->real_time = system_time();
	stream->frames_count += current_settings.buffer_frames;
	stream->buffer_cycle = stream->first_voice->trigblk;
	stream->update_needed = true;
	release_spinlock(&slock);

	//get_sem_count(stream->card->buffer_ready_sem, &count);
	//if (count <= 0)
		release_sem_etc(stream->card->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
}

static void
emuxki_record_inth(void* inthparams)
{
	emuxki_stream *stream = (emuxki_stream *)inthparams;
	//int32 count;

	//TRACE(("emuxki_record_inth\n"));

	acquire_spinlock(&slock);
	stream->real_time = system_time();
	stream->frames_count += current_settings.buffer_frames;
	stream->buffer_cycle = (stream->first_voice->trigblk
		+ stream->first_voice->blkmod -1) % stream->first_voice->blkmod;
	stream->update_needed = true;
	release_spinlock(&slock);

	//get_sem_count(stream->card->buffer_ready_sem, &count);
	//if (count <= 0)
		release_sem_etc(stream->card->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
}

static status_t
emuxki_buffer_exchange(emuxki_dev *card, multi_buffer_info *data)
{
	cpu_status status;
	emuxki_stream *pstream, *rstream;
	multi_buffer_info buffer_info;

#ifdef __HAIKU__
	if (user_memcpy(&buffer_info, data, sizeof(buffer_info)) < B_OK)
		return B_BAD_ADDRESS;
#else
	memcpy(&buffer_info, data, sizeof(buffer_info));
#endif

	buffer_info.flags = B_MULTI_BUFFER_PLAYBACK | B_MULTI_BUFFER_RECORD;

	if (!(card->pstream->state & EMU_STATE_STARTED))
		emuxki_stream_start(card->pstream, emuxki_play_inth, card->pstream);

	if (!(card->pstream2->state & EMU_STATE_STARTED))
		emuxki_stream_start(card->pstream2, emuxki_play_inth, card->pstream2);

	if (!(card->rstream->state & EMU_STATE_STARTED))
		emuxki_stream_start(card->rstream, emuxki_record_inth, card->rstream);

	if (!(card->rstream2->state & EMU_STATE_STARTED))
		emuxki_stream_start(card->rstream2, emuxki_record_inth, card->rstream2);


	if (acquire_sem_etc(card->buffer_ready_sem, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, 50000)
		== B_TIMED_OUT) {
		LOG(("buffer_exchange timeout ff\n"));
		LOG(("EMU_IPR = %#08x\n",emuxki_reg_read_32(&card->config, EMU_IPR)));
		LOG(("EMU_INTE = %#08x\n",emuxki_reg_read_32(&card->config, EMU_INTE)));
		LOG(("EMU_HCFG = %#08x\n",emuxki_reg_read_32(&card->config, EMU_HCFG)));
	}

	status = lock();

	LIST_FOREACH(pstream, &card->streams, next) {
		if ((pstream->use & EMU_USE_PLAY) == 0 ||
			(pstream->state & EMU_STATE_STARTED) == 0)
			continue;
		if (pstream->update_needed)
			break;
	}

	LIST_FOREACH(rstream, &card->streams, next) {
		if ((rstream->use & EMU_USE_RECORD) == 0 ||
			(rstream->state & EMU_STATE_STARTED) == 0)
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
emuxki_buffer_force_stop(emuxki_dev *card)
{
	return B_OK;
}

static status_t
emuxki_multi_control(void *cookie, uint32 op, void *data, size_t length)
{
	emuxki_dev *card = (emuxki_dev *)cookie;

    switch (op) {
		case B_MULTI_GET_DESCRIPTION:
			LOG(("B_MULTI_GET_DESCRIPTION\n"));
			return emuxki_get_description(card, (multi_description *)data);
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
			return emuxki_get_enabled_channels(card, (multi_channel_enable *)data);
		case B_MULTI_SET_ENABLED_CHANNELS:
			LOG(("B_MULTI_SET_ENABLED_CHANNELS\n"));
			return emuxki_set_enabled_channels(card, (multi_channel_enable *)data);
		case B_MULTI_GET_GLOBAL_FORMAT:
			LOG(("B_MULTI_GET_GLOBAL_FORMAT\n"));
			return emuxki_get_global_format(card, (multi_format_info *)data);
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
			return emuxki_get_mix(card, (multi_mix_value_info *)data);
		case B_MULTI_SET_MIX:
			LOG(("B_MULTI_SET_MIX\n"));
			return emuxki_set_mix(card, (multi_mix_value_info *)data);
		case B_MULTI_LIST_MIX_CHANNELS:
			LOG(("B_MULTI_LIST_MIX_CHANNELS\n"));
			return emuxki_list_mix_channels(card, (multi_mix_channel_info *)data);
		case B_MULTI_LIST_MIX_CONTROLS:
			LOG(("B_MULTI_LIST_MIX_CONTROLS\n"));
			return emuxki_list_mix_controls(card, (multi_mix_control_info *)data);
		case B_MULTI_LIST_MIX_CONNECTIONS:
			LOG(("B_MULTI_LIST_MIX_CONNECTIONS\n"));
			return emuxki_list_mix_connections(card, (multi_mix_connection_info *)data);
		case B_MULTI_GET_BUFFERS:			/* Fill out the struct for the first time; doesn't start anything. */
			LOG(("B_MULTI_GET_BUFFERS\n"));
			return emuxki_get_buffers(card, data);
		case B_MULTI_SET_BUFFERS:			/* Set what buffers to use, if the driver supports soft buffers. */
			LOG(("B_MULTI_SET_BUFFERS\n"));
			return B_ERROR; /* we do not support soft buffers */
		case B_MULTI_SET_START_TIME:			/* When to actually start */
			LOG(("B_MULTI_SET_START_TIME\n"));
			return B_ERROR;
		case B_MULTI_BUFFER_EXCHANGE:		/* stop and go are derived from this being called */
			//TRACE(("B_MULTI_BUFFER_EXCHANGE\n"));
			return emuxki_buffer_exchange(card, (multi_buffer_info *)data);
		case B_MULTI_BUFFER_FORCE_STOP:		/* force stop of playback, nothing in data */
			LOG(("B_MULTI_BUFFER_FORCE_STOP\n"));
			return emuxki_buffer_force_stop(card);
	}
	LOG(("ERROR: unknown multi_control %#x\n",op));
	return B_ERROR;
}

static status_t emuxki_open(const char *name, uint32 flags, void** cookie);
static status_t emuxki_close(void* cookie);
static status_t emuxki_free(void* cookie);
static status_t emuxki_control(void* cookie, uint32 op, void* arg, size_t len);
static status_t emuxki_read(void* cookie, off_t position, void *buf, size_t* num_bytes);
static status_t emuxki_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes);

device_hooks multi_hooks = {
	emuxki_open, 			/* -> open entry point */
	emuxki_close, 			/* -> close entry point */
	emuxki_free,			/* -> free cookie */
	emuxki_control, 		/* -> control entry point */
	emuxki_read,			/* -> read entry point */
	emuxki_write,			/* -> write entry point */
	NULL,					/* start select */
	NULL,					/* stop select */
	NULL,					/* scatter-gather read from the device */
	NULL					/* scatter-gather write to the device */
};

static status_t
emuxki_open(const char *name, uint32 flags, void** cookie)
{
	emuxki_dev *card = NULL;
	emuxki_recparams recparams;
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
	if (card->pstream2 !=NULL)
		return B_ERROR;
	if (card->rstream !=NULL)
		return B_ERROR;
	if (card->rstream2 !=NULL)
		return B_ERROR;

	*cookie = card;
	card->multi.card = card;

	LOG(("voice_new\n"));

	card->rstream2 = emuxki_stream_new(card, EMU_USE_RECORD, current_settings.buffer_frames, current_settings.buffer_count);
	card->rstream = emuxki_stream_new(card, EMU_USE_RECORD, current_settings.buffer_frames, current_settings.buffer_count);
	card->pstream2 = emuxki_stream_new(card, EMU_USE_PLAY, current_settings.buffer_frames, current_settings.buffer_count);
	card->pstream = emuxki_stream_new(card, EMU_USE_PLAY, current_settings.buffer_frames, current_settings.buffer_count);

	card->buffer_ready_sem = create_sem(0,"pbuffer ready");

	LOG(("voice_setaudio\n"));

	emuxki_stream_set_audioparms(card->pstream, true, current_settings.channels,
		current_settings.bitsPerSample == 16, current_settings.sample_rate);
	emuxki_stream_set_audioparms(card->pstream2, false, 4,
		current_settings.bitsPerSample == 16, current_settings.sample_rate);
	emuxki_stream_set_audioparms(card->rstream, true, current_settings.channels,
		current_settings.bitsPerSample == 16, current_settings.sample_rate);
	emuxki_stream_set_audioparms(card->rstream2, true, current_settings.channels,
		current_settings.bitsPerSample == 16, current_settings.sample_rate);
	recparams.efx_voices[0] = 3; // channels 1,2
	recparams.efx_voices[1] = 0;
	emuxki_stream_set_recparms(card->rstream, EMU_RECSRC_ADC, NULL);
	emuxki_stream_set_recparms(card->rstream2, EMU_RECSRC_FX, &recparams);

	card->pstream->first_channel = 0;
	card->pstream2->first_channel = current_settings.channels;
	card->rstream->first_channel = current_settings.channels + 4;
	card->rstream2->first_channel = 2 * current_settings.channels + 4;

	emuxki_stream_commit_parms(card->pstream);
	emuxki_stream_commit_parms(card->pstream2);
	emuxki_stream_commit_parms(card->rstream);
	emuxki_stream_commit_parms(card->rstream2);

	emuxki_create_channels_list(&card->multi);

	return B_OK;
}

static status_t
emuxki_close(void* cookie)
{
	//emuxki_dev *card = cookie;
	LOG(("close()\n"));

	return B_OK;
}

static status_t
emuxki_free(void* cookie)
{
	emuxki_dev *card = cookie;
	emuxki_stream *stream;
	LOG(("free()\n"));

	if (card->buffer_ready_sem > B_OK)
			delete_sem(card->buffer_ready_sem);

	LIST_FOREACH(stream, &card->streams, next) {
		emuxki_stream_halt(stream);
	}

	while (!LIST_EMPTY(&card->streams)) {
		emuxki_stream_delete(LIST_FIRST(&card->streams));
	}

	card->pstream = NULL;
	card->pstream2 = NULL;
	card->rstream = NULL;
	card->rstream2 = NULL;

	return B_OK;
}

static status_t
emuxki_control(void* cookie, uint32 op, void* arg, size_t len)
{
	return emuxki_multi_control(cookie, op, arg, len);
}

static status_t
emuxki_read(void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was read */
	return B_IO_ERROR;
}

static status_t
emuxki_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was written */
	return B_IO_ERROR;
}

