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
#ifndef _AC97_H_
#define _AC97_H_

#include "config.h"

enum AC97_REGISTER {
	AC97_RESET				= 0x00,
	AC97_MASTER_VOLUME		= 0x02,
	AC97_AUX_OUT_VOLUME		= 0x04,
	AC97_MONO_VOLUME		= 0x06,
	AC97_MASTER_TONE		= 0x08,
	AC97_PC_BEEP_VOLUME		= 0x0A,
	AC97_PHONE_VOLUME		= 0x0C,
	AC97_MIC_VOLUME			= 0x0E,
	AC97_LINE_IN_VOLUME		= 0x10,
	AC97_CD_VOLUME			= 0x12,
	AC97_VIDEO_VOLUME		= 0x14,
	AC97_AUX_IN_VOLUME		= 0x16,
	AC97_PCM_OUT_VOLUME		= 0x18,
	AC97_RECORD_SELECT		= 0x1A,
	AC97_RECORD_GAIN		= 0x1C,
	AC97_RECORD_GAIN_MIC	= 0x1E,
	AC97_GENERAL_PURPOSE	= 0x20,
	AC97_3D_CONTROL			= 0x22,
	AC97_PAGING				= 0x24,
	AC97_POWERDOWN			= 0x26,
	AC97_EXTENDED_AUDIO_ID 	= 0x28,
	AC97_EXTENDED_AUDIO_STATUS	= 0x2A,
	AC97_PCM_FRONT_DAC_RATE	= 0x2C,
	AC97_PCM_SURR_DAC_RATE	= 0x2E,
	AC97_PCM_LFE_DAC_RATE	= 0x30,
	AC97_PCM_LR_ADC_RATE	= 0x32,
	AC97_MIC_ADC_RATE		= 0x34,
	AC97_CENTER_LFE_VOLUME	= 0x36,
	AC97_SURROUND_VOLUME	= 0x38,
	AC97_SPDIF_CONTROL		= 0x3A,
	AC97_VENDOR_ID1			= 0x7C,
	AC97_VENDOR_ID2			= 0x7E
};

const char *	ac97_get_3d_stereo_enhancement(device_config *config);
const char *	ac97_get_vendor_id_description(device_config *config);
uint32			ac97_get_vendor_id(device_config *config);
void			ac97_init(device_config *config);

void ac97_amp_enable(device_config *config, bool yesno);

typedef enum {
	B_MIX_GAIN = 1 << 0,
	B_MIX_MUTE = 1 << 1,
	B_MIX_MONO = 1 << 2,
	B_MIX_STEREO = 1 << 3,
	B_MIX_MUX = 1 << 4,
	B_MIX_MICBOOST = 1 << 5,
	B_MIX_RECORDMUX = 1 << 6
} ac97_mixer_type;

typedef struct _ac97_source_info {
	const char *name;
	ac97_mixer_type  type;
	
	int32	id;
	uint8	reg;
	uint16	default_value;
	uint8 	bits:3;
	uint8	ofs:4;
	uint8	mute:1;
	uint8	polarity:1; // max_gain -> 0
	float	min_gain;
	float	max_gain;
	float	granularity;
} ac97_source_info;

extern const ac97_source_info source_info[];
extern const int32 source_info_size;

#endif
