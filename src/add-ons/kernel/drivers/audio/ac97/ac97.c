/*
 * AC97 interface
 *
 * Copyright (c) 2002, Marcus Overhagen <marcus@overhagen.de>
 * Copyright (c) 2008-2013, Jérôme Duval
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


#include <KernelExport.h>
#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MediaDefs.h>
#include "ac97.h"

#define LOG(x)	dprintf x

#define B_UTF8_REGISTERED	"\xC2\xAE"

bool ac97_reg_is_valid(ac97_dev *dev, uint8 reg);
void ac97_amp_enable(ac97_dev *dev, bool onoff);
void ac97_dump_capabilities(ac97_dev *dev);
void ac97_detect_capabilities(ac97_dev *dev);
void ac97_detect_rates(ac97_dev *dev);
void ac97_update_register_cache(ac97_dev *dev);


const char * stereo_enhancement_technique[] =
{
	"No 3D Stereo Enhancement",
	"Analog Devices",
	"Creative Technology",
	"National Semiconductor",
	"Yamaha",
	"BBE Sound",
	"Crystal Semiconductor",
	"Qsound Labs",
	"Spatializer Audio Laboratories",
	"SRS Labs",
	"Platform Tech",
	"AKM Semiconductor",
	"Aureal",
	"Aztech Labs",
	"Binaura",
	"ESS Technology",
	"Harman International",
	"Nvidea",
	"Philips",
	"Texas Instruments",
	"VLSI Technology",
	"TriTech",
	"Realtek",
	"Samsung",
	"Wolfson Microelectronics",
	"Delta Integration",
	"SigmaTel",
	"KS Waves",
	"Rockwell",
	"Unknown (29)",
	"Unknown (30)",
	"Unknown (31)"
};


static void default_init(ac97_dev *dev);
static void ad1819_init(ac97_dev *dev);
static void ad1881_init(ac97_dev *dev);
static void ad1885_init(ac97_dev *dev);
static void ad1886_init(ac97_dev *dev);
static void ad1980_init(ac97_dev *dev);
static void ad1981b_init(ac97_dev *dev);
static void alc203_init(ac97_dev *dev);
static void alc650_init(ac97_dev *dev);
static void alc655_init(ac97_dev *dev);
static void alc850_init(ac97_dev *dev);
static void stac9708_init(ac97_dev *dev);
static void stac9721_init(ac97_dev *dev);
static void stac9744_init(ac97_dev *dev);
static void stac9756_init(ac97_dev *dev);
static void stac9758_init(ac97_dev *dev);
static void tr28028_init(ac97_dev *dev);
static void wm9701_init(ac97_dev *dev);
static void wm9703_init(ac97_dev *dev);
static void wm9704_init(ac97_dev *dev);

bool ad1819_set_rate(ac97_dev *dev, uint8 reg, uint32 rate);
bool ad1819_get_rate(ac97_dev *dev, uint8 reg, uint32 *rate);


typedef struct
{
	uint32 id;
	uint32 mask;
	codec_init init;
	const char *info;
} codec_table;


codec_table codecs[] =
{
	{ CODEC_ID_AD1819,	0xffffffff, ad1819_init,	"Analog Devices AD1819A, AD1819B SoundPort" B_UTF8_REGISTERED },
	{ CODEC_ID_AD1881,	0xffffffff, ad1881_init,	"Analog Devices AD1881 SoundMAX" B_UTF8_REGISTERED },
	{ CODEC_ID_AD1881A,	0xffffffff, ad1881_init,	"Analog Devices AD1881A SoundMAX" B_UTF8_REGISTERED },
	{ CODEC_ID_AD1885,	0xffffffff, ad1885_init,	"Analog Devices AD1885 SoundMAX" B_UTF8_REGISTERED },
	{ CODEC_ID_AD1886,	0xffffffff, ad1886_init,	"Analog Devices AD1886 SoundMAX" B_UTF8_REGISTERED },
	{ CODEC_ID_AD1886A,	0xffffffff, ad1881_init,	"Analog Devices AD1886A SoundMAX" B_UTF8_REGISTERED },
	{ CODEC_ID_AD1887,	0xffffffff, ad1881_init,	"Analog Devices AD1887 SoundMAX" B_UTF8_REGISTERED },
	{ CODEC_ID_AD1888,	0xffffffff, ad1881_init,	"Analog Devices AD1888 SoundMAX" B_UTF8_REGISTERED },
	{ CODEC_ID_AD1980,	0xffffffff, ad1980_init,	"Analog Devices AD1980 SoundMAX" B_UTF8_REGISTERED },
	{ 0x41445371,		0xffffffff, default_init,	"Analog Devices 0x41445371 (???)" },
	{ 0x41445372,		0xffffffff, default_init,	"Analog Devices AD1981A SoundMAX" B_UTF8_REGISTERED },
	{ CODEC_ID_AD1981B,	0xffffffff, ad1981b_init,	"Analog Devices AD1981B SoundMAX" B_UTF8_REGISTERED },
	{ CODEC_ID_AD1985,	0xffffffff, default_init,	"Analog Devices AD1985 SoundMAX" B_UTF8_REGISTERED },
	{ CODEC_ID_AD1986,	0xffffffff, default_init,	"Analog Devices AD1986 SoundMAX" B_UTF8_REGISTERED },
	{ CODEC_ID_AK4540,	0xffffffff, default_init,	"Asahi Kasei AK4540" },
	{ CODEC_ID_AK4542,	0xffffffff, default_init,	"Asahi Kasei AK4542" },
	{ CODEC_ID_AK4543,	0xffffffff, default_init,	"Asahi Kasei AK4543" },
	{ 0x414b4d06,		0xffffffff, default_init,	"Asahi Kasei AK4544A" },
	{ 0x414b4d07,		0xffffffff, default_init,	"Asahi Kasei AK4545" },
	{ 0x414c4300, 		0xffffff00, default_init,	"Avance Logic (Realtek) ALC100" }, /* 0x4300 = ALC100 */
	{ 0x414c4320,		0xfffffff0, default_init,	"Avance Logic (Realtek) ALC100/ALC100P, RL5383/RL5522" },
	{ CODEC_ID_ALC201A, 0xfffffff0, default_init,	"Avance Logic (Realtek) ALC200/ALC200A, ALC201/ALC201A" }, /* 0x4710 = ALC201A */
	{ 0x414c4720,		0xffffffff, alc650_init,	"Avance Logic (Realtek) ALC650" }, /* 0x4720 = ALC650 */
	{ 0x414c4730, 		0xffffffff, default_init,	"Avance Logic (Realtek) ALC101" },
	{ 0x414c4740,		0xfffffff0, default_init,	"Avance Logic (Realtek) ALC202/ALC202A" },
	{ 0x414c4750,		0xfffffff0, default_init,	"Avance Logic (Realtek) ALC250" },
	{ 0x414c4760,		0xfffffff0, alc655_init,	"Avance Logic (Realtek) ALC655" }, /* 0x4760 = ALC655 */
	{ 0x414c4770,		0xfffffff0, alc203_init,	"Avance Logic (Realtek) ALC203" },
	{ 0x414c4780,		0xffffffff, alc655_init,	"Avance Logic (Realtek) ALC658" },
	{ 0x414c4790,		0xfffffff0, alc850_init,	"Avance Logic (Realtek) ALC850" },
	{ 0x434d4941,		0xffffffff, default_init,	"C-Media CMI9738" },
	{ 0x434d4961,		0xffffffff, default_init,	"C-Media CMI9739" },
	{ 0x43525900,		0xffffffff, default_init,	"Cirrus Logic CS4297" },
	{ 0x43525903,		0xffffffff, default_init,	"Cirrus Logic CS4297" },
	{ 0x43525913,		0xffffffff, default_init,	"Cirrus Logic CS4297A" },
	{ 0x43525914,		0xffffffff, default_init,	"Cirrus Logic CS4297B" },
	{ 0x43525923,		0xffffffff, default_init,	"Cirrus Logic CS4294C" },
	{ 0x4352592b,		0xffffffff, default_init,	"Cirrus Logic CS4298C" },
	{ CODEC_ID_CS4299A,	0xffffffff, default_init,	"Cirrus Logic CS4299A" },
	{ CODEC_ID_CS4299C,	0xffffffff, default_init,	"Cirrus Logic CS4299C" },
	{ CODEC_ID_CS4299D,	0xffffffff, default_init,	"Cirrus Logic CS4299D" },
	{ 0x43525941,		0xffffffff, default_init,	"Cirrus Logic CS4201A" },
	{ 0x43525951,		0xffffffff, default_init,	"Cirrus Logic CS4205A" },
	{ 0x43525961,		0xffffffff, default_init,	"Cirrus Logic CS4291A" },
	{ 0x43585421,		0xffffffff, default_init,	"HSD11246" },
	{ 0x44543031,		0xffffffff, default_init,	"DT0398" },
	{ 0x454d4328,		0xffffffff, default_init,	"EM28028" },
	{ 0x45838308,		0xffffffff, default_init,	"ESS Technology ES1921" },
	{ 0x49434501,		0xffffffff, default_init,	"ICEnsemble ICE1230" },
	{ 0x49434511,		0xffffffff, default_init,	"ICEnsemble ICE1232" },
	{ 0x49434514,		0xffffffff, default_init,	"ICEnsemble ICE1232A" },
	{ 0x49434551,		0xffffffff, default_init,	"Via Technologies VT1616" }, /* rebranded from ICEnsemble */
	{ 0x49544520,		0xffffffff, default_init,	"Integrated Technology Express ITE2226E" },
	{ 0x49544560,		0xffffffff, default_init,	"Integrated Technology Express ITE2646E" },
	{ 0x4e534331,		0xffffffff, default_init,	"National Semiconductor LM4549" },
	{ CODEC_ID_STAC9700,0xffffffff, default_init,	"SigmaTel STAC9700/9783/9784" },
	{ CODEC_ID_STAC9704,0xffffffff, default_init,	"SigmaTel STAC9701/03, STAC9704/07, STAC9705 (???)" },
	{ CODEC_ID_STAC9705,0xffffffff, default_init,	"SigmaTel STAC9704 (???)" },
	{ CODEC_ID_STAC9708,0xffffffff, stac9708_init,	"SigmaTel STAC9708/9711" },
	{ CODEC_ID_STAC9721,0xffffffff, stac9721_init,	"SigmaTel STAC9721/9723" },
	{ CODEC_ID_STAC9744,0xffffffff, stac9744_init,	"SigmaTel STAC9744" },
	{ CODEC_ID_STAC9750,0xffffffff, default_init,	"SigmaTel STAC9750/51" },
	{ CODEC_ID_STAC9752,0xffffffff, default_init,	"SigmaTel STAC9752/53" },
	{ CODEC_ID_STAC9756,0xffffffff, stac9756_init,	"SigmaTel STAC9756/9757" },
	{ CODEC_ID_STAC9758,0xffffffff, stac9758_init,	"SigmaTel STAC9758/59" },
	{ CODEC_ID_STAC9766,0xffffffff, default_init,	"SigmaTel STAC9766/67" },
	{ 0x53494c22,		0xffffffff, default_init,	"Silicon Laboratory Si3036" },
	{ 0x53494c23,		0xffffffff, default_init,	"Silicon Laboratory Si3038" },
	{ 0x53544d02,		0xffffffff, default_init,	"ST7597" },
	{ 0x54524103,		0xffffffff, default_init,	"TriTech TR28023" },
	{ 0x54524106,		0xffffffff, default_init,	"TriTech TR28026" },
	{ 0x54524108,		0xffffffff, tr28028_init,	"TriTech TR28028" },
	{ 0x54524123,		0xffffffff, default_init,	"TriTech TR28602" },
	{ 0x56494161,		0xffffffff, default_init,	"Via Technologies VIA1612A" },
	{ 0x56494170,		0xffffffff, default_init,	"Via Technologies VIA1617A" },
	{ 0x574d4c00,		0xffffffff, wm9701_init,	"Wolfson WM9701A" },
	{ 0x574d4c03,		0xffffffff, wm9703_init,	"Wolfson WM9703/9704" },
	{ 0x574d4c04,		0xffffffff, wm9704_init,	"Wolfson WM9704 (quad)" },
	{ 0x574d4c05,		0xffffffff, wm9703_init,	"Wolfson WM9705/WM9710" },
	{ 0x574d4d09,		0xffffffff, default_init,	"Wolfson WM9709" },
	{ 0x574d4c12,		0xffffffff, default_init,	"Wolfson WM9711/12" },
	{ 0x574d4c13,		0xffffffff, default_init,	"Wolfson WM9713/14" },
	{ 0x57454301,		0xffffffff, default_init,	"Wolfson W83971D" },
	/* Vendors only: */
	{ 0x41445300,		0xffffff00, default_init,	"Analog Devices" },
	{ 0x414b4d00,		0xffffff00, default_init,	"Asahi Kasei" },
	{ 0x414c4700,		0xffffff00, default_init,	"Avance Logic (Realtek)" },
	{ 0x434d4900,		0xffffff00, default_init,	"C-Media" },
	{ 0x43525900,		0xffffff00, default_init,	"Cirrus Logic" },
	{ 0x45838300,		0xffffff00, default_init,	"ESS Technology" },
	{ 0x49434500,		0xffffff00, default_init,	"ICEnsemble" },
	{ 0x49544500,		0xffffff00, default_init,	"ITE, Inc." },
	{ 0x4e534300,		0xffffff00, default_init,	"National Semiconductor" },
	{ 0x83847600,		0xffffff00, default_init,	"SigmaTel" },
	{ 0x53494c00,		0xffffff00, default_init,	"Silicon Laboratory" },
	{ 0x54524100,		0xffffff00, default_init,	"TriTech" },
	{ 0x54584e00,		0xffffff00, default_init,	"Texas Instruments" },
	{ 0x56494100,		0xffffff00, default_init,	"VIA Technologies" },
	{ 0x574d4c00,		0xffffff00, default_init,	"Wolfson" },
	{ 0x594d4800,		0xffffff00, default_init,	"Yamaha" },
	{ 0x00000000,		0x00000000, default_init,	"Unknown" } /* must be last one, matches every codec */
};


static codec_table *
find_codec_table(uint32 codecid)
{
	codec_table *codec;
	for (codec = codecs; codec->id; codec++)
		if ((codec->id & codec->mask) == (codecid & codec->mask))
			break;
	return codec;
}


void
ac97_attach(ac97_dev **_dev, codec_reg_read reg_read, codec_reg_write reg_write, void *cookie,
	ushort subvendor_id, ushort subsystem_id)
{
	ac97_dev *dev;
	codec_table *codec;
	int i;

	*_dev = dev = (ac97_dev *) malloc(sizeof(ac97_dev));
	memset(dev->reg_cache, 0, sizeof(dev->reg_cache));
	dev->cookie = cookie;
	dev->reg_read = reg_read;
	dev->reg_write = reg_write;
	dev->set_rate = 0;
	dev->get_rate = 0;
	dev->clock = 48000; /* default clock on non-broken motherboards */
	dev->min_vsr = 0x0001;
	dev->max_vsr = 0xffff;
	dev->reversed_eamp_polarity = false;
	dev->capabilities = 0;

	dev->subsystem = (subvendor_id << 16) | subsystem_id;

	if (dev->subsystem == 0x161f202f
		|| dev->subsystem == 0x161f203a
		|| dev->subsystem == 0x161f203e
		|| dev->subsystem == 0x161f204c
		|| dev->subsystem == 0x104d8144
		|| dev->subsystem == 0x104d8197
		|| dev->subsystem == 0x104d81c0
		|| dev->subsystem == 0x104d81c5
		|| dev->subsystem == 0x103c3089
		|| dev->subsystem == 0x103c309a
		|| dev->subsystem == 0x10338213
		|| dev->subsystem == 0x103382be) {
		dev->reversed_eamp_polarity = true;
	}

	/* reset the codec */
	LOG(("codec reset\n"));
	ac97_reg_uncached_write(dev, AC97_RESET, 0x0000);
	for (i = 0; i < 500; i++) {
		if ((ac97_reg_uncached_read(dev, AC97_POWERDOWN) & 0xf) == 0xf)
			break;
		snooze(1000);
	}

	dev->codec_id = ((uint32)reg_read(cookie, AC97_VENDOR_ID1) << 16) | reg_read(cookie, AC97_VENDOR_ID2);
	codec = find_codec_table(dev->codec_id);
	dev->codec_info = codec->info;
	dev->init = codec->init;

	dev->codec_3d_stereo_enhancement = stereo_enhancement_technique[(ac97_reg_cached_read(dev, AC97_RESET) >> 10) & 31];

	/* setup register cache */
	ac97_update_register_cache(dev);

	ac97_reg_update_bits(dev, AC97_EXTENDED_STAT_CTRL, 1, 1); // enable variable rate audio

	ac97_detect_capabilities(dev);

	dev->init(dev);
	ac97_amp_enable(dev, true);

	/* set mixer defaults, enabled Line-out sources are PCM-out, CD-in, Line-in */
	ac97_reg_update(dev, AC97_CENTER_LFE_VOLUME, 0x0000);	/* set LFE & center volume 0dB */
	ac97_reg_update(dev, AC97_SURR_VOLUME, 0x0000);			/* set surround volume 0dB */
	ac97_reg_update(dev, AC97_MASTER_VOLUME, 0x0000);		/* set master output 0dB */
	ac97_reg_update(dev, AC97_AUX_OUT_VOLUME, 0x0000);		/* set aux output 0dB */
	ac97_reg_update(dev, AC97_MONO_VOLUME, 0x0000);			/* set mono output 0dB */
	ac97_reg_update(dev, AC97_PCM_OUT_VOLUME, 0x0808);		/* enable pcm-out */
	ac97_reg_update(dev, AC97_CD_VOLUME, 0x0808);			/* enable cd-in */
	ac97_reg_update(dev, AC97_LINE_IN_VOLUME, 0x0808);		/* enable line-in */

	/* set record line in */
	ac97_reg_update(dev, AC97_RECORD_SELECT, 0x0404);

	LOG(("codec vendor id      = %#08" B_PRIx32 "\n", dev->codec_id));
	LOG(("codec description     = %s\n", dev->codec_info));
	LOG(("codec 3d enhancement = %s\n", dev->codec_3d_stereo_enhancement));

	ac97_dump_capabilities(dev);
}


void
ac97_detach(ac97_dev *dev)
{
	/* Mute everything */
	ac97_reg_update_bits(dev, AC97_CENTER_LFE_VOLUME, 0x8000, 0x8000);
	ac97_reg_update_bits(dev, AC97_SURR_VOLUME, 0x8000, 0x8000);
	ac97_reg_update_bits(dev, AC97_MASTER_VOLUME, 0x8000, 0x8000);
	ac97_reg_update_bits(dev, AC97_AUX_OUT_VOLUME, 0x8000, 0x8000);
	ac97_reg_update_bits(dev, AC97_MONO_VOLUME, 0x8000, 0x8000);
	ac97_reg_update_bits(dev, AC97_PCM_OUT_VOLUME, 0x8000, 0x8000);
	ac97_reg_update_bits(dev, AC97_CD_VOLUME, 0x8000, 0x8000);
	ac97_reg_update_bits(dev, AC97_LINE_IN_VOLUME, 0x8000, 0x8000);

	ac97_amp_enable(dev, false);

	free(dev);
}


void
ac97_suspend(ac97_dev *dev)
{
	ac97_amp_enable(dev, false);
}


void
ac97_resume(ac97_dev *dev)
{
	ac97_amp_enable(dev, true);
}


void
ac97_reg_cached_write(ac97_dev *dev, uint8 reg, uint16 value)
{
	if (!ac97_reg_is_valid(dev, reg))
		return;
	dev->reg_write(dev->cookie, reg, value);
	dev->reg_cache[reg] = value;
}


uint16
ac97_reg_cached_read(ac97_dev *dev, uint8 reg)
{
	if (!ac97_reg_is_valid(dev, reg))
		return 0;
	return dev->reg_cache[reg];
}

void
ac97_reg_uncached_write(ac97_dev *dev, uint8 reg, uint16 value)
{
	if (!ac97_reg_is_valid(dev, reg))
		return;
	dev->reg_write(dev->cookie, reg, value);
}


uint16
ac97_reg_uncached_read(ac97_dev *dev, uint8 reg)
{
	if (!ac97_reg_is_valid(dev, reg))
		return 0;
	return dev->reg_read(dev->cookie, reg);
}


bool
ac97_reg_update(ac97_dev *dev, uint8 reg, uint16 value)
{
	if (!ac97_reg_is_valid(dev, reg))
		return false;
	if (ac97_reg_cached_read(dev, reg) == value)
		return false;
	ac97_reg_cached_write(dev, reg, value);
	return true;
}


bool
ac97_reg_update_bits(ac97_dev *dev, uint8 reg, uint16 mask, uint16 value)
{
	uint16 old;
	if (!ac97_reg_is_valid(dev, reg))
		return false;
	old = ac97_reg_cached_read(dev, reg);
	value &= mask;
	value |= (old & ~mask);
	if (old == value)
		return false;
	ac97_reg_cached_write(dev, reg, value);
	return true;
}


void
ac97_update_register_cache(ac97_dev *dev)
{
	int reg;
	for (reg = 0; reg <= 0x7e; reg += 2)
		dev->reg_cache[reg] = ac97_reg_uncached_read(dev, reg);
}


bool
ac97_set_rate(ac97_dev *dev, uint8 reg, uint32 rate)
{
	uint32 value;
	uint32 old;

	if (dev->set_rate)
		return dev->set_rate(dev, reg, rate);

	value = (uint32)((rate * 48000ULL) / dev->clock); /* need 64 bit calculation for rates 96000 or higher */

	LOG(("ac97_set_rate: clock = %" B_PRIu32 ", "
		"rate = %" B_PRIu32 ", "
		"value = %" B_PRIu32 "\n",
		dev->clock, rate, value));

	/* if double rate audio is currently enabled, divide value by 2 */
	if (ac97_reg_cached_read(dev, AC97_EXTENDED_STAT_CTRL) & 0x0002)
		value /= 2;

	if (value < dev->min_vsr || value > dev->max_vsr)
		return false;

	old = ac97_reg_cached_read(dev, reg);
	ac97_reg_cached_write(dev, reg, value);
	if (value != ac97_reg_uncached_read(dev, reg)) {
		LOG(("ac97_set_rate failed, new rate %d\n", ac97_reg_uncached_read(dev, reg)));
		ac97_reg_cached_write(dev, reg, old);
		return false;
	}
	LOG(("ac97_set_rate done\n"));
	return true;
}


bool
ac97_get_rate(ac97_dev *dev, uint8 reg, uint32 *rate)
{
	uint32 value;

	if (dev->get_rate)
		return dev->get_rate(dev, reg, rate);

	value = ac97_reg_cached_read(dev, reg);
	if (value == 0)
		return false;

	/* if double rate audio is currently enabled, multiply value by 2 */
	if (ac97_reg_cached_read(dev, AC97_EXTENDED_STAT_CTRL) & 0x0002)
		value *= 2;

	*rate = (uint32)((value * (uint64)dev->clock) / 48000); /* need 64 bit calculation to avoid overflow*/
	return true;
}


void
ac97_set_clock(ac97_dev *dev, uint32 clock)
{
	LOG(("ac97_set_clock: clock = %" B_PRIu32 "\n", clock));
	dev->clock = clock;
	ac97_detect_rates(dev);
	ac97_dump_capabilities(dev);
}


void
ac97_detect_capabilities(ac97_dev *dev)
{
	uint16 val;

	val = ac97_reg_cached_read(dev, AC97_RESET);
	if (val & 0x0001)
		dev->capabilities |= CAP_PCM_MIC;
	if (val & 0x0004)
		dev->capabilities |= CAP_BASS_TREBLE_CTRL;
	if (val & 0x0008)
		dev->capabilities |= CAP_SIMULATED_STEREO;
	if (val & 0x0010)
		dev->capabilities |= CAP_HEADPHONE_OUT;
	if (val & 0x0020)
		dev->capabilities |= CAP_LAUDNESS;
	if (val & 0x0040)
		dev->capabilities |= CAP_DAC_18BIT;
	if (val & 0x0080)
		dev->capabilities |= CAP_DAC_20BIT;
	if (val & 0x0100)
		dev->capabilities |= CAP_ADC_18BIT;
	if (val & 0x0200)
		dev->capabilities |= CAP_ADC_20BIT;
	if (val & 0x7C00)
		dev->capabilities |= CAP_3D_ENHANCEMENT;

	val = ac97_reg_cached_read(dev, AC97_EXTENDED_ID);
	if (val & EXID_VRA)
		dev->capabilities |= CAP_VARIABLE_PCM;
	if (val & EXID_DRA)
		dev->capabilities |= CAP_DOUBLE_PCM;
	if (val & EXID_SPDIF)
		dev->capabilities |= CAP_SPDIF;
	if (val & EXID_VRM)
		dev->capabilities |= CAP_VARIABLE_MIC;
	if (val & EXID_CDAC)
		dev->capabilities |= CAP_CENTER_DAC;
	if (val & EXID_SDAC)
		dev->capabilities |= CAP_SURR_DAC;
	if (val & EXID_LDAC)
		dev->capabilities |= CAP_LFE_DAC;
	if (val & EXID_AMAP)
		dev->capabilities |= CAP_AMAP;
	if ((val & (EXID_REV0 | EXID_REV1)) == 0)
		dev->capabilities |= CAP_REV21;
	if ((val & (EXID_REV0 | EXID_REV1)) == EXID_REV0)
		dev->capabilities |= CAP_REV22;
	if ((val & (EXID_REV0 | EXID_REV1)) == EXID_REV1)
		dev->capabilities |= CAP_REV23;

	ac97_detect_rates(dev);
}

void
ac97_detect_rates(ac97_dev *dev)
{
	uint32 oldrate;

	dev->capabilities &= ~CAP_PCM_RATE_MASK;

	if (!ac97_get_rate(dev, AC97_PCM_FRONT_DAC_RATE, &oldrate))
		oldrate = 48000;

	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 20000))
		dev->capabilities |= CAP_PCM_RATE_CONTINUOUS;
	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 8000))
		dev->capabilities |= CAP_PCM_RATE_8000;
	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 11025))
		dev->capabilities |= CAP_PCM_RATE_11025;
	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 12000))
		dev->capabilities |= CAP_PCM_RATE_12000;
	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 16000))
		dev->capabilities |= CAP_PCM_RATE_16000;
	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 22050))
		dev->capabilities |= CAP_PCM_RATE_22050;
	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 24000))
		dev->capabilities |= CAP_PCM_RATE_24000;
	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 32000))
		dev->capabilities |= CAP_PCM_RATE_32000;
	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 44100))
		dev->capabilities |= CAP_PCM_RATE_44100;
	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 48000))
		dev->capabilities |= CAP_PCM_RATE_48000;

	if (dev->capabilities & CAP_DOUBLE_PCM) {
		// enable double rate mode
		if (ac97_reg_update_bits(dev, AC97_EXTENDED_STAT_CTRL, 0x0002, 0x0002)) {
			if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 88200))
				dev->capabilities |= CAP_PCM_RATE_88200;
			if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 96000))
				dev->capabilities |= CAP_PCM_RATE_96000;
			// disable double rate mode
			ac97_reg_update_bits(dev, AC97_EXTENDED_STAT_CTRL, 0x0002, 0x0000);
		}
	}

	ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, oldrate);
}


void
ac97_dump_capabilities(ac97_dev *dev)
{
	LOG(("AC97 capabilities:\n"));
	if (ac97_has_capability(dev, CAP_PCM_MIC))
		LOG(("CAP_PCM_MIC\n"));
	if (ac97_has_capability(dev, CAP_BASS_TREBLE_CTRL))
		LOG(("CAP_BASS_TREBLE_CTRL\n"));
	if (ac97_has_capability(dev, CAP_SIMULATED_STEREO))
		LOG(("CAP_SIMULATED_STEREO\n"));
	if (ac97_has_capability(dev, CAP_HEADPHONE_OUT))
		LOG(("CAP_HEADPHONE_OUT\n"));
	if (ac97_has_capability(dev, CAP_LAUDNESS))
		LOG(("CAP_LAUDNESS\n"));
	if (ac97_has_capability(dev, CAP_DAC_18BIT))
		LOG(("CAP_DAC_18BIT\n"));
	if (ac97_has_capability(dev, CAP_DAC_20BIT))
		LOG(("CAP_DAC_20BIT\n"));
	if (ac97_has_capability(dev, CAP_ADC_18BIT))
		LOG(("CAP_ADC_18BIT\n"));
	if (ac97_has_capability(dev, CAP_ADC_20BIT))
		LOG(("CAP_ADC_20BIT\n"));
	if (ac97_has_capability(dev, CAP_3D_ENHANCEMENT))
		LOG(("CAP_3D_ENHANCEMENT\n"));
	if (ac97_has_capability(dev, CAP_VARIABLE_PCM))
		LOG(("CAP_VARIABLE_PCM\n"));
	if (ac97_has_capability(dev, CAP_DOUBLE_PCM))
		LOG(("CAP_DOUBLE_PCM\n"));
	if (ac97_has_capability(dev, CAP_VARIABLE_MIC))
		LOG(("CAP_VARIABLE_MIC\n"));
	if (ac97_has_capability(dev, CAP_CENTER_DAC))
		LOG(("CAP_CENTER_DAC\n"));
	if (ac97_has_capability(dev, CAP_SURR_DAC))
		LOG(("CAP_SURR_DAC\n"));
	if (ac97_has_capability(dev, CAP_LFE_DAC))
		LOG(("CAP_LFE_DAC\n"));
	if (ac97_has_capability(dev, CAP_AMAP))
		LOG(("CAP_AMAP\n"));
	if (ac97_has_capability(dev, CAP_REV21))
		LOG(("CAP_REV21\n"));
	if (ac97_has_capability(dev, CAP_REV22))
		LOG(("CAP_REV22\n"));
	if (ac97_has_capability(dev, CAP_REV23))
		LOG(("CAP_REV23\n"));
	if (ac97_has_capability(dev, CAP_PCM_RATE_CONTINUOUS))
		LOG(("CAP_PCM_RATE_CONTINUOUS\n"));
	if (ac97_has_capability(dev, CAP_PCM_RATE_8000))
		LOG(("CAP_PCM_RATE_8000\n"));
	if (ac97_has_capability(dev, CAP_PCM_RATE_11025))
		LOG(("CAP_PCM_RATE_11025\n"));
	if (ac97_has_capability(dev, CAP_PCM_RATE_12000))
		LOG(("CAP_PCM_RATE_12000\n"));
	if (ac97_has_capability(dev, CAP_PCM_RATE_16000))
		LOG(("CAP_PCM_RATE_16000\n"));
	if (ac97_has_capability(dev, CAP_PCM_RATE_22050))
		LOG(("CAP_PCM_RATE_22050\n"));
	if (ac97_has_capability(dev, CAP_PCM_RATE_24000))
		LOG(("CAP_PCM_RATE_24000\n"));
	if (ac97_has_capability(dev, CAP_PCM_RATE_32000))
		LOG(("CAP_PCM_RATE_32000\n"));
	if (ac97_has_capability(dev, CAP_PCM_RATE_44100))
		LOG(("CAP_PCM_RATE_44100\n"));
	if (ac97_has_capability(dev, CAP_PCM_RATE_48000))
		LOG(("CAP_PCM_RATE_48000\n"));
	if (ac97_has_capability(dev, CAP_PCM_RATE_88200))
		LOG(("CAP_PCM_RATE_88200\n"));
	if (ac97_has_capability(dev, CAP_PCM_RATE_96000))
		LOG(("CAP_PCM_RATE_96000\n"));
}


bool
ac97_has_capability(ac97_dev *dev, uint64 cap)
{
	// return (dev->capabilities & cap); // does not work because of 64 bit to integer trucation
	return (dev->capabilities & cap) != 0;
}

/*************************************************
 * Codec specific initialization, etc.
 */

bool
ac97_reg_is_valid(ac97_dev *dev, uint8 reg)
{
	if (reg & 1)
		return false;
	if (reg > 0x7e)
		return false;

	switch (dev->codec_id) {
		case CODEC_ID_AK4540:
		case CODEC_ID_AK4542:
			if (reg < 0x1e || reg == 0x20 || reg == 0x26 || reg > 0x7a)
				return true;
			return false;

		case CODEC_ID_AD1819:
		case CODEC_ID_AD1881:
		case CODEC_ID_AD1881A:
			if (reg < 0x3a || reg > 0x6e)
				return true;
			return false;

		case CODEC_ID_AD1885:
		case CODEC_ID_AD1886:
		case CODEC_ID_AD1886A:
		case CODEC_ID_AD1887:
			if (reg < 0x3c || reg == 0x5a || reg > 0x6e)
				return true;
			return false;

		case CODEC_ID_STAC9700:
		case CODEC_ID_STAC9704:
		case CODEC_ID_STAC9705:
		case CODEC_ID_STAC9708:
		case CODEC_ID_STAC9721:
		case CODEC_ID_STAC9744:
		case CODEC_ID_STAC9756:
			if (reg < 0x3c || reg > 0x58)
				return true;
			return false;

		default:
			return true;
	}
}


void
ac97_amp_enable(ac97_dev *dev, bool yesno)
{
	switch (dev->codec_id) {
		case CODEC_ID_CS4299A:
		case CODEC_ID_CS4299C:
		case CODEC_ID_CS4299D:
			LOG(("cs4299_amp_enable\n"));
			if (yesno)
				ac97_reg_cached_write(dev, 0x68, 0x8004);
			else
				ac97_reg_cached_write(dev, 0x68, 0);
			break;

		default:
			LOG(("ac97_amp_enable, reverse eamp = %d\n", dev->reversed_eamp_polarity));
			LOG(("powerdown register was = %#04x\n", ac97_reg_uncached_read(dev, AC97_POWERDOWN)));
			if (dev->reversed_eamp_polarity)
				yesno = !yesno;
			if (yesno)
				ac97_reg_cached_write(dev, AC97_POWERDOWN, ac97_reg_uncached_read(dev, AC97_POWERDOWN) & ~0x8000); /* switch on (low active) */
			else
				ac97_reg_cached_write(dev, AC97_POWERDOWN, ac97_reg_uncached_read(dev, AC97_POWERDOWN) | 0x8000); /* switch off */
			LOG(("powerdown register is = %#04x\n", ac97_reg_uncached_read(dev, AC97_POWERDOWN)));
		break;
	}
}


bool
ad1819_set_rate(ac97_dev *dev, uint8 reg, uint32 rate)
{
	uint32 value;

	value = (uint32)((rate * 48000ULL) / dev->clock); /* need 64 bit calculation for rates 96000 or higher */

	LOG(("ad1819_set_rate: clock = %" B_PRIu32 ", "
		"rate = %" B_PRIu32 ", "
		"value = %" B_PRIu32 "\n",
		dev->clock, rate, value));

	if (value < 0x1B58 || value > 0xBB80)
		return false;

	switch (reg) {
		case AC97_PCM_FRONT_DAC_RATE:
			ac97_reg_cached_write(dev, AC97_AD_SAMPLE_RATE_0, value);
			return true;

		case AC97_PCM_L_R_ADC_RATE:
			ac97_reg_cached_write(dev, AC97_AD_SAMPLE_RATE_1, value);
			return true;

		default:
			return false;
	}
}


bool
ad1819_get_rate(ac97_dev *dev, uint8 reg, uint32 *rate)
{
	uint32 value;

	switch (reg) {
		case AC97_PCM_FRONT_DAC_RATE:
			value = ac97_reg_cached_read(dev, AC97_AD_SAMPLE_RATE_0);
			break;

		case AC97_PCM_L_R_ADC_RATE:
			value = ac97_reg_cached_read(dev, AC97_AD_SAMPLE_RATE_1);
			break;

		default:
			return false;
	}

	*rate = (uint32)((value * (uint64)dev->clock) / 48000); /* need 64 bit calculation to avoid overflow*/
	return true;
}


void
default_init(ac97_dev *dev)
{
	LOG(("default_init\n"));
}


void
ad1819_init(ac97_dev *dev)
{
	LOG(("ad1819_init\n"));

	/* Default config for system with single AD1819 codec */
	ac97_reg_cached_write(dev, AC97_AD_SERIAL_CONFIG, 0x7000);
	ac97_update_register_cache(dev);

	/* The AD1819 chip has proprietary  sample rate controls
	 * Setup sample rate 0 generator for DAC,
	 * Setup sample rate 1 generator for ADC,
	 * ARSR=1, DRSR=0, ALSR=1, DLSR=0
	 */
	ac97_reg_cached_write(dev, AC97_AD_MISC_CONTROL, 0x0101);
	/* connect special rate set/get functions */
	dev->set_rate = ad1819_set_rate;
	dev->get_rate = ad1819_get_rate;
	ac97_detect_rates(dev);
	ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 48000);
	ac97_set_rate(dev, AC97_PCM_L_R_ADC_RATE, 48000);
}


void
ad1881_init(ac97_dev *dev)
{
	LOG(("ad1881_init\n"));

	/* Default config for system with single AD1819 codec,
	 * BROKEN on systems with master & slave codecs */
	ac97_reg_cached_write(dev, AC97_AD_SERIAL_CONFIG, 0x7000);
	ac97_update_register_cache(dev);

	/* Setup DAC and ADC rate generator assignments compatible with AC97 */
	ac97_reg_cached_write(dev, AC97_AD_MISC_CONTROL, 0x0404);

	/* Setup variable frame rate limits */
	dev->min_vsr = 0x1B58;	/*  7kHz */
	dev->max_vsr = 0xBB80;	/* 48kHz */
}


void
ad1885_init(ac97_dev *dev)
{
	LOG(("ad1885_init\n"));
	ad1881_init(dev);

	/* disable jack sense 0 and 1 (JS0, JS1) to turn off automatic mute */
	ac97_reg_cached_write(dev, AC97_AD_JACK_SENSE, ac97_reg_cached_read(dev, AC97_AD_JACK_SENSE) | 0x0300);
}


void
ad1886_init(ac97_dev *dev)
{
	LOG(("ad1886_init\n"));
	ad1881_init(dev);

	/* change jack sense to always activate outputs*/
	ac97_reg_cached_write(dev, AC97_AD_JACK_SENSE, 0x0010);
	/* change SPDIF to a valid value */
	ac97_reg_cached_write(dev, AC97_SPDIF_CONTROL, 0x2a20);
}


void
ad1980_init(ac97_dev *dev)
{
	LOG(("ad1980_init\n"));

	/* Select only master codec,
	 * SPDIF and DAC are linked
	 */
	ac97_reg_cached_write(dev, AC97_AD_SERIAL_CONFIG, 0x1001);
	ac97_update_register_cache(dev);

	/* Select Line-out driven with mixer data (not surround data)
	 * Select Headphone-out driven with mixer data (not surround data),
	 * LOSEL = 0, HPSEL = 1
	 * XXX this one needs to be changed to support surround	out
	 */
	ac97_reg_cached_write(dev, AC97_AD_MISC_CONTROL, 0x0400);
}


void
ad1981b_init(ac97_dev *dev)
{
	LOG(("ad1981b_init\n"));
	if (dev->subsystem == 0x0e11005a
		|| dev->subsystem == 0x103c006d
		|| dev->subsystem == 0x103c088c
		|| dev->subsystem == 0x103c0890
		|| dev->subsystem == 0x103c0934
		|| dev->subsystem == 0x103c0938
		|| dev->subsystem == 0x103c0944
		|| dev->subsystem == 0x103c099c
		|| dev->subsystem == 0x101402d9) {
		ac97_reg_cached_write(dev, AC97_AD_JACK_SENSE,
				ac97_reg_cached_read(dev, AC97_AD_JACK_SENSE) | 0x0800);
	}
}


void
alc203_init(ac97_dev *dev)
{
	LOG(("alc203_init\n"));

	ac97_reg_update_bits(dev, AC97_ALC650_CLOCK_SOURCE, 0x400, 0x400);
}


void
alc650_init(ac97_dev *dev)
{
	LOG(("alc650_init\n"));

	/* Enable Surround, LFE and Center downmix into Line-out,
	 * Set Surround-out as duplicated Line-out.
	 */
	ac97_reg_cached_write(dev, AC97_ALC650_MULTI_CHAN_CTRL, 0x0007);

	/* Set Surround DAC Volume to 0dB
	 * Set Center/LFE DAC Volume to 0dB
	 * (but both should already be set, as these are hardware reset defaults)
	 */
	ac97_reg_cached_write(dev, AC97_ALC650_SURR_VOLUME, 0x0808);
	ac97_reg_cached_write(dev, AC97_ALC650_CEN_LFE_VOLUME, 0x0808);
}


void
alc655_init(ac97_dev *dev)
{
	uint16 val;
	LOG(("alc655_init\n"));

	ac97_reg_update_bits(dev, AC97_PAGING, 0xf, 0);

	val = ac97_reg_cached_read(dev, AC97_ALC650_CLOCK_SOURCE);
	// TODO update bits for specific devices
	val &= ~0x1000;
	ac97_reg_cached_write(dev, AC97_ALC650_CLOCK_SOURCE, val);

	ac97_reg_cached_write(dev, AC97_ALC650_MULTI_CHAN_CTRL, 0x8000);
	ac97_reg_cached_write(dev, AC97_ALC650_SURR_VOLUME, 0x808);
	ac97_reg_cached_write(dev, AC97_ALC650_CEN_LFE_VOLUME, 0x808);

	if (dev->codec_id == 0x414c4781)
		ac97_reg_update_bits(dev, AC97_ALC650_MISC_CONTROL, 0x800, 0x800);
}


void
alc850_init(ac97_dev *dev)
{
	LOG(("alc850_init\n"));

	ac97_reg_update_bits(dev, AC97_PAGING, 0xf, 0);

	ac97_reg_cached_write(dev, AC97_ALC650_MULTI_CHAN_CTRL, 0x8000);
	ac97_reg_cached_write(dev, AC97_ALC650_CLOCK_SOURCE, 0x20d2);
	ac97_reg_cached_write(dev, AC97_ALC650_GPIO_SETUP, 0x8a90);
	ac97_reg_cached_write(dev, AC97_ALC650_SURR_VOLUME, 0x808);
	ac97_reg_cached_write(dev, AC97_ALC650_CEN_LFE_VOLUME, 0x808);
}


void
stac9708_init(ac97_dev *dev)
{
	LOG(("stac9708_init\n"));
	/* ALSA initializes some registers that according to the
	 * documentation for this codec do not exist. If the
	 * following doesn't work, we may need to do that, too.
	 */
	/* The Analog Special reg is at 0x6C, other codecs have it at 0x6E */
	/* Set Analog Special to default (DAC/ADC -6dB disabled) */
	ac97_reg_cached_write(dev, 0x6C, 0x0000);
	/* Set Multi Channel to default */
	ac97_reg_cached_write(dev, 0x74, 0x0000);
}


void
stac9721_init(ac97_dev *dev)
{
	LOG(("stac9721_init\n"));
	/* Set Analog Special to default (DAC/ADC -6dB disabled) */
	ac97_reg_cached_write(dev, 0x6E, 0x0000);
	/* Enable register 0x72 */
	ac97_reg_cached_write(dev, 0x70, 0xabba);
	/* Set Analog Current to -50% */
	ac97_reg_cached_write(dev, 0x72, 0x0002);
	/* Set Multi Channel to default */
	ac97_reg_cached_write(dev, 0x74, 0x0000);
	/* Enable register 0x78 */
	ac97_reg_cached_write(dev, 0x76, 0xabba);
	/* Set Clock Access to default */
	ac97_reg_cached_write(dev, 0x78, 0x0000);
}


void
stac9744_init(ac97_dev *dev)
{
	LOG(("stac9744_init\n"));
	/* Set Analog Special to default (DAC/ADC -6dB disabled) */
	ac97_reg_cached_write(dev, 0x6E, 0x0000);
	/* Enable register 0x72 */
	ac97_reg_cached_write(dev, 0x70, 0xabba);
	/* Set Analog Current to -50% */
	ac97_reg_cached_write(dev, 0x72, 0x0002);
	/* Set Multi Channel to default */
	ac97_reg_cached_write(dev, 0x74, 0x0000);
	/* Enable register 0x78 */
	ac97_reg_cached_write(dev, 0x76, 0xabba);
	/* Set Clock Access to default */
	ac97_reg_cached_write(dev, 0x78, 0x0000);
}


void
stac9756_init(ac97_dev *dev)
{
	LOG(("stac9756_init\n"));
	/* Set Analog Special to default (AC97 all-mix, DAC/ADC -6dB disabled) */
	ac97_reg_cached_write(dev, 0x6E, 0x1000);
	/* Enable register 0x72 */
	ac97_reg_cached_write(dev, 0x70, 0xabba);
	/* Set Analog Current to -50% */
	ac97_reg_cached_write(dev, 0x72, 0x0002);
	/* Set Multi Channel to default */
	ac97_reg_cached_write(dev, 0x74, 0x0000);
	/* Enable register 0x78 */
	ac97_reg_cached_write(dev, 0x76, 0xabba);
	/* Set Clock Access to default */
	ac97_reg_cached_write(dev, 0x78, 0x0000);
}



void
stac9758_init(ac97_dev *dev)
{
	LOG(("stac9758_init\n"));

	ac97_reg_update_bits(dev, AC97_PAGING, 0xf, 0);

	if (dev->subsystem == 0x107b0601) {
		ac97_reg_cached_write(dev, 0x64, 0xfc70);
		ac97_reg_cached_write(dev, 0x68, 0x2102);
		ac97_reg_cached_write(dev, 0x66, 0x0203);
		ac97_reg_cached_write(dev, 0x72, 0x0041);
	} else {
		ac97_reg_cached_write(dev, 0x64, 0xd794);
		ac97_reg_cached_write(dev, 0x68, 0x2001);
		ac97_reg_cached_write(dev, 0x66, 0x0201);
		ac97_reg_cached_write(dev, 0x72, 0x0040);
	}
}


void
tr28028_init(ac97_dev *dev)
{
	LOG(("tr28028_init\n"));
	ac97_reg_cached_write(dev, AC97_POWERDOWN, 0x0300);
	ac97_reg_cached_write(dev, AC97_POWERDOWN, 0x0000);
	ac97_reg_cached_write(dev, AC97_SURR_VOLUME, 0x0000);
	ac97_reg_cached_write(dev, AC97_SPDIF_CONTROL, 0x0000);
}


void
wm9701_init(ac97_dev *dev)
{
	LOG(("wm9701_init\n"));
	/* ALSA writes some of these registers, but the codec
	 * documentation states explicitly that 0x38 and 0x70 to 0x74
	 * are not used in the WM9701A
	 */

	/* DVD noise patch (?) */
	ac97_reg_cached_write(dev, 0x5a, 0x0200);
}


void
wm9703_init(ac97_dev *dev)
{
	LOG(("wm9703_init\n"));
	/* Set front mixer value to unmuted */
	ac97_reg_cached_write(dev, 0x72, 0x0808);
	/* Disable loopback, etc */
	ac97_reg_cached_write(dev, AC97_GENERAL_PURPOSE, 0x8000);
}


void
wm9704_init(ac97_dev *dev)
{
	LOG(("wm9704_init\n"));
	/* Set read DAC value to unmuted */
	ac97_reg_cached_write(dev, 0x70, 0x0808);
	/* Set front mixer value to unmuted */
	ac97_reg_cached_write(dev, 0x72, 0x0808);
	/* Set rear mixer value to unmuted */
	ac97_reg_cached_write(dev, 0x74, 0x0808);
	/* DVD noise patch (?) */
	ac97_reg_cached_write(dev, 0x5a, 0x0200);
}


const ac97_source_info source_info[] = {
	{ "Recording", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO|B_MIX_RECORDMUX, 100, AC97_RECORD_GAIN, 0x8000, 4, 0, 1, 0, 0.0, 22.5, 1.5 },
	{ "Master", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 101, AC97_MASTER_VOLUME, 0x8000, 5, 0, 1, 1,-46.5, 0.0, 1.5 },
	//{ "Bass/Treble", B_MIX_GAIN|B_MIX_STEREO, 102, AC97_MASTER_TONE, 0x0f0f, 4, 0, 1, 1,-12.0, 10.5, 1.5 },
	//{ "Aux out", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 103, AC97_AUX_OUT_VOLUME, 0x8000, 5, 0, 1, 1,-46.5, 0.0, 1.5 },
	{ "PCM out", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 104, AC97_PCM_OUT_VOLUME, 0x8808, 5, 0, 1, 1,-34.5, 12.0, 1.5 },
	{ "CD", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 105, AC97_CD_VOLUME, 0x8808, 5, 0, 1, 1,-34.5, 12.0, 1.5 },
	{ "Aux In", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 106, AC97_AUX_IN_VOLUME, 0x8808, 5, 0, 1, 1,-34.5, 12.0, 1.5 },
	{ "TAD", B_MIX_GAIN|B_MIX_MUTE|B_MIX_MONO, 107, AC97_PHONE_VOLUME, 0x8008, 5, 0, 1, 1,-34.5, 12.0, 1.5 },
	{ "Mic", B_MIX_GAIN|B_MIX_MUTE|B_MIX_MONO|B_MIX_MICBOOST, 108, AC97_MIC_VOLUME, 0x8008, 5, 0, 1, 1,-34.5, 12.0, 1.5 },
	{ "Line in", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 109, AC97_LINE_IN_VOLUME, 0x8808, 5, 0, 1, 1,-34.5, 12.0, 1.5 },
	//{ "Center/Lfe", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 111, AC97_CENTER_LFE_VOLUME, 0x8080, 5, 0, 1, 1,-46.5, 0.0, 1.5 },
	{ "Center/Lfe" /* should be "Surround" but no */, B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 110, AC97_SURR_VOLUME, 0x8080, 5, 0, 1, 1,-46.5, 0.0, 1.5 }
};

const int32 source_info_size = (sizeof(source_info)/sizeof(source_info[0]));
