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
#ifndef _AC97_H_
#define _AC97_H_

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
	AC97_PCM_FRONT_DAC_RATE = 0x2C,
	AC97_VENDOR_ID1			= 0x7C,
	AC97_VENDOR_ID2			= 0x7E
};

struct ac97_dev;
typedef struct ac97_dev ac97_dev;

typedef void	(* codec_init)(ac97_dev * /*dev*/);
typedef void	(* codec_amp_enable)(ac97_dev */*dev*/, bool /*onoff*/);
typedef bool 	(* codec_reg_is_valid)(ac97_dev */*dev*/, uint8 /*reg*/);
typedef	uint16	(* codec_reg_read)(void * /*cookie*/, uint8 /*reg*/);
typedef	void	(* codec_reg_write)(void * /*cookie*/, uint8 /*reg*/, uint16 /*value*/);

typedef struct
{
	codec_init			init;
	codec_amp_enable 	amp_enable;
	codec_reg_is_valid	reg_is_valid;
} codec_ops;

struct ac97_dev {
	uint32				capabilities;
	uint16				reg_cache[0x7f];

	void *				cookie;
	
	uint32				codec_id;
	const char *		codec_info;
	const char *		codec_3d_stereo_enhancement;
	
	codec_init			init;
	codec_amp_enable	amp_enable;
	codec_reg_is_valid	reg_is_valid;
	codec_reg_read		reg_read;
	codec_reg_write		reg_write;
	
	uint32 				clock;
};


void	ac97_attach(ac97_dev **dev, codec_reg_read reg_read, codec_reg_write reg_write, void *cookie);
void	ac97_detach(ac97_dev *dev);
void	ac97_suspend(ac97_dev *dev);
void	ac97_resume(ac97_dev *dev);

void	ac97_reg_cached_write(ac97_dev *dev, uint8 reg, uint16 value);
uint16	ac97_reg_cached_read(ac97_dev *dev, uint8 reg);
void	ac97_reg_uncached_write(ac97_dev *dev, uint8 reg, uint16 value);
uint16	ac97_reg_uncached_read(ac97_dev *dev, uint8 reg);

bool	ac97_reg_update(ac97_dev *dev, uint8 reg, uint16 value);
bool	ac97_reg_update_bits(ac97_dev *dev, uint8 reg, uint16 mask, uint16 value);

bool	ac97_set_rate(ac97_dev *dev, uint8 reg, uint32 rate);

void	ac97_set_clock(ac97_dev *dev, uint32 clock);


#endif
