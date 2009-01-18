/*
 * AC97 interface
 *
 * Copyright (c) 2002, Marcus Overhagen <marcus@overhagen.de>
 * Copyright (c) 2008, Jérôme Duval
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
	/* Baseline audio register set */
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
	
	/* Extended audio register set */
	AC97_EXTENDED_ID		= 0x28,
	AC97_EXTENDED_STAT_CTRL = 0x2A,
	AC97_PCM_FRONT_DAC_RATE = 0x2C,
	AC97_PCM_SURR_DAC_RATE	= 0x2E,
	AC97_PCM_LFE_DAC_RATE	= 0x30,
	AC97_PCM_L_R_ADC_RATE	= 0x32,
	AC97_MIC_ADC_RATE		= 0x34,
	AC97_CENTER_LFE_VOLUME	= 0x36,
	AC97_SURR_VOLUME		= 0x38,
	AC97_SPDIF_CONTROL		= 0x3A,

	/* Vendor ID */
	AC97_VENDOR_ID1			= 0x7C,
	AC97_VENDOR_ID2			= 0x7E,
	
	/* Analog Devices */
	AC97_AD_JACK_SENSE		= 0x72,
	AC97_AD_SERIAL_CONFIG	= 0x74,
	AC97_AD_MISC_CONTROL	= 0x76,
	AC97_AD_SAMPLE_RATE_0	= 0x78,
	AC97_AD_SAMPLE_RATE_1	= 0x7a,
	
	/* Realtek ALC650 */
	AC97_ALC650_SPDIF_INPUT_CHAN_STATUS_LO = 0x60, /* only ALC650 Rev. E and later */
	AC97_ALC650_SPDIF_INPUT_CHAN_STATUS_HI = 0x62, /* only ALC650 Rev. E and later */
	AC97_ALC650_SURR_VOLUME		= 0x64,
	AC97_ALC650_CEN_LFE_VOLUME	= 0x66,
	AC97_ALC650_MULTI_CHAN_CTRL	= 0x6A,
	AC97_ALC650_MISC_CONTROL	= 0x74,
	AC97_ALC650_GPIO_SETUP		= 0x76,
	AC97_ALC650_GPIO_STATUS		= 0x78,
	AC97_ALC650_CLOCK_SOURCE	= 0x7A
};

// AC97_EXTENDED_ID bits
enum {
	EXID_VRA 	= 0x0001,
	EXID_DRA 	= 0x0002,
	EXID_SPDIF 	= 0x0004,
	EXID_VRM 	= 0x0008,
	EXID_DSA0 	= 0x0010,
	EXID_DSA1 	= 0x0020,
	EXID_CDAC 	= 0x0040,
	EXID_SDAC 	= 0x0080,
	EXID_LDAC 	= 0x0100,
	EXID_AMAP 	= 0x0200,
	EXID_REV0 	= 0x0400,
	EXID_REV1 	= 0x0800,
	EXID_bit12 	= 0x1000,
	EXID_bit13 	= 0x2000,
	EXID_ID0 	= 0x4000,
	EXID_ID1 	= 0x8000
};

// some codec_ids
enum {
	CODEC_ID_ALC201A	= 0x414c4710,
	CODEC_ID_AK4540		= 0x414b4d00,
	CODEC_ID_AK4542		= 0x414b4d01,
	CODEC_ID_AK4543		= 0x414b4d02,
	CODEC_ID_AD1819 	= 0x41445303, // ok, AD1819A, AD1819B
	CODEC_ID_AD1881		= 0x41445340, // ok, AD1881
	CODEC_ID_AD1881A	= 0x41445348, // ok, AD1881A
	CODEC_ID_AD1885		= 0x41445360, // ok, AD1885
	CODEC_ID_AD1886		= 0x41445361, // ok, AD1886
	CODEC_ID_AD1886A 	= 0x41445363, // ok, AD1886A
	CODEC_ID_AD1887		= 0x41445362, // ok, AD1887
	CODEC_ID_AD1888		= 0x41445368, // ok, AD1888
	CODEC_ID_AD1980		= 0x41445370, // ok, AD1980
	CODEC_ID_AD1981B	= 0x41445374, // ok, AD1981B
	CODEC_ID_AD1985		= 0x41445375, // ok, AD1985
	CODEC_ID_AD1986		= 0x41445378, // ok, AD1986
	CODEC_ID_CS4299A	= 0x43525931,
	CODEC_ID_CS4299C	= 0x43525933,
	CODEC_ID_CS4299D	= 0x43525934,
	CODEC_ID_STAC9700	= 0x83847600, // ok, STAC9700
	CODEC_ID_STAC9704	= 0x83847604, // STAC9701/03, STAC9704/07, STAC9705 (???)
	CODEC_ID_STAC9705	= 0x83847605, // ???
	CODEC_ID_STAC9708	= 0x83847608, // ok, STAC9708/11
	CODEC_ID_STAC9721	= 0x83847609, // ok, STAC9721/23
	CODEC_ID_STAC9744	= 0x83847644, // ok, STAC9744
	CODEC_ID_STAC9752	= 0x83847652, // ok, STAC9752/53
	CODEC_ID_STAC9756	= 0x83847656, // ok, STAC9756/57
	CODEC_ID_STAC9766	= 0x83847666, // ok, STAC9766/67
};

// capabilities
enum ac97_capability {
	CAP_PCM_MIC				= 0x0000000000000001ULL, /* dedicated mic PCM channel */
	CAP_BASS_TREBLE_CTRL	= 0x0000000000000002ULL,
	CAP_SIMULATED_STEREO	= 0x0000000000000004ULL,
	CAP_HEADPHONE_OUT		= 0x0000000000000008ULL,
	CAP_LAUDNESS			= 0x0000000000000010ULL,
	CAP_DAC_18BIT			= 0x0000000000000020ULL,
	CAP_DAC_20BIT			= 0x0000000000000040ULL,
	CAP_ADC_18BIT			= 0x0000000000000080ULL,
	CAP_ADC_20BIT			= 0x0000000000000100ULL,
	CAP_3D_ENHANCEMENT		= 0x0000000000000200ULL,
	CAP_VARIABLE_PCM		= 0x0000000000000400ULL, /* variable rate PCM */
	CAP_DOUBLE_PCM			= 0x0000000000000800ULL, /* double rate PCM */
	CAP_SPDIF				= 0x0000000000001000ULL,
	CAP_VARIABLE_MIC		= 0x0000000000002000ULL, /* variable rate mic PCM */
	CAP_CENTER_DAC			= 0x0000000000004000ULL,
	CAP_SURR_DAC			= 0x0000000000008000ULL,
	CAP_LFE_DAC				= 0x0000000000010000ULL,
	CAP_AMAP				= 0x0000000000020000ULL,
	CAP_REV21				= 0x0000000000040000ULL,
	CAP_REV22				= 0x0000000000080000ULL,
	CAP_REV23				= 0x0000000000100000ULL,
	CAP_PCM_RATE_CONTINUOUS	= 0x0000000000200000ULL,
	CAP_PCM_RATE_8000		= 0x0000000000400000ULL,
	CAP_PCM_RATE_11025		= 0x0000000000800000ULL,
	CAP_PCM_RATE_12000		= 0x0000000001000000ULL,
	CAP_PCM_RATE_16000		= 0x0000000002000000ULL,
	CAP_PCM_RATE_22050		= 0x0000000004000000ULL,
	CAP_PCM_RATE_24000		= 0x0000000008000000ULL,
	CAP_PCM_RATE_32000		= 0x0000000010000000ULL,
	CAP_PCM_RATE_44100		= 0x0000000020000000ULL,
	CAP_PCM_RATE_48000		= 0x0000000040000000ULL,
	CAP_PCM_RATE_88200		= 0x0000000080000000ULL,
	CAP_PCM_RATE_96000		= 0x0000000100000000ULL,
	CAP_PCM_RATE_MASK		= ( CAP_PCM_RATE_CONTINUOUS | CAP_PCM_RATE_8000 | CAP_PCM_RATE_11025 |
								CAP_PCM_RATE_12000 | CAP_PCM_RATE_16000 | CAP_PCM_RATE_22050 |
								CAP_PCM_RATE_24000 | CAP_PCM_RATE_32000 | CAP_PCM_RATE_44100 |
								CAP_PCM_RATE_48000 | CAP_PCM_RATE_88200 | CAP_PCM_RATE_96000)
};

struct ac97_dev;
typedef struct ac97_dev ac97_dev;

typedef void	(* codec_init)(ac97_dev * dev);
typedef	uint16	(* codec_reg_read)(void * cookie, uint8 reg);
typedef	void	(* codec_reg_write)(void * cookie, uint8 reg, uint16 value);
typedef bool	(* codec_set_rate)(ac97_dev *dev, uint8 reg, uint32 rate);
typedef bool	(* codec_get_rate)(ac97_dev *dev, uint8 reg, uint32 *rate);

struct ac97_dev {
	uint16				reg_cache[0x7f];

	void *				cookie;
	
	uint32				codec_id;
	const char *		codec_info;
	const char *		codec_3d_stereo_enhancement;
	
	codec_init			init;
	codec_reg_read		reg_read;
	codec_reg_write		reg_write;
	codec_set_rate		set_rate;
	codec_get_rate		get_rate;

	uint32				max_vsr;	
	uint32				min_vsr;	
	uint32 				clock;
	uint64				capabilities;
	bool				reversed_eamp_polarity;
	uint32				subsystem;
};

#ifdef __cplusplus
extern "C" {
#endif

void	ac97_attach(ac97_dev **dev, codec_reg_read reg_read, codec_reg_write reg_write, void *cookie, 
	ushort subvendor_id, ushort subsystem_id);
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
bool	ac97_get_rate(ac97_dev *dev, uint8 reg, uint32 *rate);

bool	ac97_has_capability(ac97_dev *dev, uint64 cap);

void	ac97_set_clock(ac97_dev *dev, uint32 clock);

#ifdef __cplusplus
}
#endif

// multi support

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
