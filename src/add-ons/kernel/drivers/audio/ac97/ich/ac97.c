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
#include <OS.h>
#include <stdio.h>
#include <MediaDefs.h>
#include "ac97.h"

//#define DEBUG 1

#define REVERSE_EAMP_POLARITY 0

#include "debug.h"
#include "io.h"

#define B_UTF8_REGISTERED	"\xC2\xAE"

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
	"Philips"
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

typedef struct
{
	uint32 id;
	uint32 mask;
	codec_ops *ops;
	const char *info;
} codec_table;

void default_init(ac97_dev *dev);
void ad1886_init(ac97_dev *dev);
void alc650_init(ac97_dev *dev);

void default_amp_enable(ac97_dev *dev, bool onoff);
void cs4299_amp_enable(ac97_dev *dev, bool onoff);

bool default_reg_is_valid(ac97_dev *dev, uint8 reg);

codec_ops default_ops = { default_init, default_amp_enable, default_reg_is_valid };
codec_ops ad1886_ops = { ad1886_init, default_amp_enable, default_reg_is_valid };
codec_ops cs4299_ops = { default_init, cs4299_amp_enable, default_reg_is_valid };
codec_ops alc650_ops = { alc650_init, default_amp_enable, default_reg_is_valid };

codec_table codecs[] = 
{
	/* Vendor ID and description imported from FreeBSD src/sys/dev/sound/pcm/ac97.c */
	{ 0x414b4d00, 0xffffffff, &default_ops, "Asahi Kasei AK4540" },
	{ 0x414b4d01, 0xffffffff, &default_ops, "Asahi Kasei AK4542" },
	{ 0x414b4d02, 0xffffffff, &default_ops, "Asahi Kasei AK4543" },
	{ 0x43525900, 0xffffffff, &default_ops, "Cirrus Logic CS4297" },
	{ 0x43525903, 0xffffffff, &default_ops, "Cirrus Logic CS4297" },
	{ 0x43525913, 0xffffffff, &default_ops, "Cirrus Logic CS4297A" },
	{ 0x43525914, 0xffffffff, &default_ops, "Cirrus Logic CS4297B" },
	{ 0x43525923, 0xffffffff, &default_ops, "Cirrus Logic CS4294C" },
	{ 0x4352592b, 0xffffffff, &default_ops, "Cirrus Logic CS4298C" },
	{ 0x43525931, 0xffffffff, &cs4299_ops,  "Cirrus Logic CS4299A" },
	{ 0x43525933, 0xffffffff, &cs4299_ops,  "Cirrus Logic CS4299C" },
	{ 0x43525934, 0xffffffff, &cs4299_ops,  "Cirrus Logic CS4299D" },
	{ 0x43525941, 0xffffffff, &default_ops, "Cirrus Logic CS4201A" },
	{ 0x43525951, 0xffffffff, &default_ops, "Cirrus Logic CS4205A" },
	{ 0x43525961, 0xffffffff, &default_ops, "Cirrus Logic CS4291A" },
	{ 0x45838308, 0xffffffff, &default_ops, "ESS Technology ES1921" },
	{ 0x49434511, 0xffffffff, &default_ops, "ICEnsemble ICE1232" },
	{ 0x4e534331, 0xffffffff, &default_ops, "National Semiconductor LM4549" },
	{ 0x83847600, 0xffffffff, &default_ops, "SigmaTel STAC9700/9783/9784" },
	{ 0x83847604, 0xffffffff, &default_ops, "SigmaTel STAC9701/9703/9704/9705" },
	{ 0x83847605, 0xffffffff, &default_ops, "SigmaTel STAC9704" },
	{ 0x83847608, 0xffffffff, &default_ops, "SigmaTel STAC9708/9711" },
	{ 0x83847609, 0xffffffff, &default_ops, "SigmaTel STAC9721/9723" },
	{ 0x83847644, 0xffffffff, &default_ops, "SigmaTel STAC9744" },
	{ 0x83847656, 0xffffffff, &default_ops, "SigmaTel STAC9756/9757" },
	{ 0x53494c22, 0xffffffff, &default_ops, "Silicon Laboratory Si3036" },
	{ 0x53494c23, 0xffffffff, &default_ops, "Silicon Laboratory Si3038" },
	{ 0x54524103, 0xffffffff, &default_ops, "TriTech TR?????" },
	{ 0x54524106, 0xffffffff, &default_ops, "TriTech TR28026" },
	{ 0x54524108, 0xffffffff, &default_ops, "TriTech TR28028" },
	{ 0x54524123, 0xffffffff, &default_ops, "TriTech TR28602" },
	{ 0x574d4c00, 0xffffffff, &default_ops, "Wolfson WM9701A" },
	{ 0x574d4c03, 0xffffffff, &default_ops, "Wolfson WM9703/9704" },
	{ 0x574d4c04, 0xffffffff, &default_ops, "Wolfson WM9704 (quad)" },
	/* Assembled from datasheets: */
	{ 0x41445303, 0xffffffff, &default_ops, "Analog Devices AD1819B SoundPort"B_UTF8_REGISTERED },
	{ 0x41445340, 0xffffffff, &default_ops, "Analog Devices AD1881 SoundMAX"B_UTF8_REGISTERED },
	{ 0x41445348, 0xffffffff, &default_ops, "Analog Devices AD1881A SoundMAX"B_UTF8_REGISTERED },
	{ 0x41445360, 0xffffffff, &default_ops, "Analog Devices AD1885 SoundMAX"B_UTF8_REGISTERED },
	{ 0x41445361, 0xffffffff, &ad1886_ops,  "Analog Devices AD1886 SoundMAX"B_UTF8_REGISTERED },
	{ 0x41445362, 0xffffffff, &default_ops, "Analog Devices AD1887 SoundMAX"B_UTF8_REGISTERED },
	{ 0x41445363, 0xffffffff, &ad1886_ops,  "Analog Devices AD1886A SoundMAX"B_UTF8_REGISTERED },
	{ 0x41445371, 0xffffffff, &default_ops, "Analog Devices AD1981A SoundMAX"B_UTF8_REGISTERED },
	{ 0x41445372, 0xffffffff, &default_ops, "Analog Devices AD1981A SoundMAX"B_UTF8_REGISTERED },
	{ 0x414c4320, 0xfffffff0, &default_ops, "Avance Logic (Realtek) ALC100/ALC100P, RL5383/RL5522" },
	{ 0x414c4730, 0xffffffff, &default_ops, "Avance Logic (Realtek) ALC101" },
	{ 0x414c4710, 0xffffffff, &default_ops, "Avance Logic (Realtek) ALC200/ALC200A, ALC201/ALC201A" }, /* 0x4710 = ALC201 */
	{ 0x414c4720, 0xffffffff, &alc650_ops,	"Avance Logic (Realtek) ALC650" }, /* 0x4720 = ALC650 */
	{ 0x414c4740, 0xffffffff, &default_ops, "Avance Logic (Realtek) ALC202/ALC202A" },
	/* Vendors only: */
	{ 0x41445300, 0xffffff00, &default_ops, "Analog Devices" },
	{ 0x414b4d00, 0xffffff00, &default_ops, "Asahi Kasei" },
	{ 0x414c4700, 0xffffff00, &default_ops, "Avance Logic (Realtek)" },
	{ 0x43525900, 0xffffff00, &default_ops, "Cirrus Logic" },
	{ 0x45838300, 0xffffff00, &default_ops, "ESS Technology" },
	{ 0x49434500, 0xffffff00, &default_ops, "ICEnsemble" },
	{ 0x4e534300, 0xffffff00, &default_ops, "National Semiconductor" },
	{ 0x83847600, 0xffffff00, &default_ops, "SigmaTel" },
	{ 0x53494c00, 0xffffff00, &default_ops, "Silicon Laboratory" },
	{ 0x54524100, 0xffffff00, &default_ops, "TriTech" },
	{ 0x574d4c00, 0xffffff00, &default_ops, "Wolfson" },
	{ 0x00000000, 0x00000000, &default_ops, "Unknown" } /* must be last one, matches every codec */
};

codec_table *
find_codec_table(uint32 codecid)
{
	codec_table *codec;
	for (codec = codecs; codec->id; codec++)
		if ((codec->id & codec->mask) == (codecid & codec->mask))
			break;
	return codec;
}

void
ac97_attach(ac97_dev **_dev, codec_reg_read reg_read, codec_reg_write reg_write, void *cookie)
{
	int reg;
	ac97_dev *dev;
	codec_table *codec;
	
	*_dev = dev = (ac97_dev *) malloc(sizeof(ac97_dev));
	dev->cookie = cookie;
	dev->reg_read = reg_read;
	dev->reg_write = reg_write;
	dev->codec_id = (reg_read(cookie, AC97_VENDOR_ID1) << 16) | reg_read(cookie, AC97_VENDOR_ID2);
	codec = find_codec_table(dev->codec_id);
	dev->codec_info = codec->info;
	dev->init = codec->ops->init;
	dev->amp_enable = codec->ops->amp_enable;
	dev->reg_is_valid = codec->ops->reg_is_valid;
	dev->clock = 48000; /* default clock on non-broken motherboards */

	/* setup register cache */
	for (reg = 0; reg <= 0x7e; reg += 2)
		dev->reg_cache[reg] = ac97_reg_uncached_read(dev, reg);

	/* reset the codec */	
	LOG(("codec reset\n"));
	ac97_reg_uncached_write(dev, AC97_RESET, 0x0000);
	snooze(50000); // 50 ms
	

	dev->codec_3d_stereo_enhancement = stereo_enhancement_technique[(ac97_reg_cached_read(dev, AC97_RESET) >> 10) & 31];
	dev->capabilities = 0;

	ac97_reg_update_bits(dev, AC97_EXTENDED_STAT_CTRL, 1, 1); // enable variable rate audio

	ac97_detect_capabilities(dev);

	dev->init(dev);
	dev->amp_enable(dev, true);
	
	/* set defaults, PCM-out and CD-out enabled */
	ac97_reg_update(dev, AC97_CENTER_LFE_VOLUME, 0x0000); 	/* set LFE & center volume 0dB */
	ac97_reg_update(dev, AC97_SURR_VOLUME, 0x0000); 		/* set surround volume 0dB */
	ac97_reg_update(dev, AC97_MASTER_VOLUME, 0x0000); 		/* set master output 0dB */
	ac97_reg_update(dev, AC97_AUX_OUT_VOLUME, 0x0000); 		/* set aux output 0dB */
	ac97_reg_update(dev, AC97_MONO_VOLUME, 0x0000); 		/* set mono output 0dB */
	ac97_reg_update(dev, AC97_CD_VOLUME, 0x0808); 			/* enable cd */
	ac97_reg_update(dev, AC97_PCM_OUT_VOLUME, 0x0808); 		/* enable pcm */
	
	ac97_dump_capabilities(dev);
}

void
ac97_detach(ac97_dev *dev)
{
	free(dev);
}

void
ac97_suspend(ac97_dev *dev)
{
	dev->amp_enable(dev, false);
}

void
ac97_resume(ac97_dev *dev)
{
	dev->amp_enable(dev, true);
}

void
ac97_reg_cached_write(ac97_dev *dev, uint8 reg, uint16 value)
{
	if (!dev->reg_is_valid(dev, reg))
		return;
	dev->reg_write(dev->cookie, reg, value);
	dev->reg_cache[reg] = value;
}

uint16
ac97_reg_cached_read(ac97_dev *dev, uint8 reg)
{
	if (!dev->reg_is_valid(dev, reg))
		return 0;
	return dev->reg_cache[reg];
}

void
ac97_reg_uncached_write(ac97_dev *dev, uint8 reg, uint16 value)
{
	if (!dev->reg_is_valid(dev, reg))
		return;
	dev->reg_write(dev->cookie, reg, value);
}

uint16
ac97_reg_uncached_read(ac97_dev *dev, uint8 reg)
{
	if (!dev->reg_is_valid(dev, reg))
		return 0;
	return dev->reg_read(dev->cookie, reg);
}

bool
ac97_reg_update(ac97_dev *dev, uint8 reg, uint16 value)
{
	if (!dev->reg_is_valid(dev, reg))
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
	if (!dev->reg_is_valid(dev, reg))
		return false;
	old = ac97_reg_cached_read(dev, reg);
	value &= mask;
	value |= (old & ~mask);
	if (old == value)
		return false;
	ac97_reg_cached_write(dev, reg, value);
	return true;
}

bool
ac97_set_rate(ac97_dev *dev, uint8 reg, uint32 rate)
{
	uint32 value;
	uint32 old;

	value = (uint32)((rate * (uint64)dev->clock) / 48000); /* use 64 bit calculation for rates 96000 or higher */

	LOG(("ac97_set_rate: clock = %d, rate = %d, value = %d\n", dev->clock, rate, value));
	
	/* if double rate audio is currently enabled, divide value by 2 */
	if (ac97_reg_cached_read(dev, AC97_EXTENDED_STAT_CTRL) & 0x0002)
		value /= 2;
		
	if (value > 0xffff)
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
	value = ac97_reg_cached_read(dev, reg);
	if (value == 0)
		return false;
	*rate = (value * 48000) / dev->clock;
	return true;
}

void
ac97_set_clock(ac97_dev *dev, uint32 clock)
{
	LOG(("ac97_set_clock: clock = %d\n", clock));
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
	if (val & (EXID_REV0 | EXID_REV1) == 0)
		dev->capabilities |= CAP_REV21;
	if (val & (EXID_REV0 | EXID_REV1) == EXID_REV0)
		dev->capabilities |= CAP_REV22;
	if (val & (EXID_REV0 | EXID_REV1) == EXID_REV1)
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
	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 48000))
		dev->capabilities |= CAP_PCM_RATE_48000;
	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 88200))
		dev->capabilities |= CAP_PCM_RATE_88200;
	if (ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, 96000))
		dev->capabilities |= CAP_PCM_RATE_96000;
		
	ac97_set_rate(dev, AC97_PCM_FRONT_DAC_RATE, oldrate);
}

void
ac97_dump_capabilities(ac97_dev *dev)
{
	LOG(("AC97 capabilities:\n"));
	if (dev->capabilities & CAP_PCM_MIC)
		LOG(("CAP_PCM_MIC\n"));
	if (dev->capabilities & CAP_BASS_TREBLE_CTRL)
		LOG(("CAP_BASS_TREBLE_CTRL\n"));
	if (dev->capabilities & CAP_SIMULATED_STEREO)
		LOG(("CAP_SIMULATED_STEREO\n"));
	if (dev->capabilities & CAP_HEADPHONE_OUT)
		LOG(("CAP_HEADPHONE_OUT\n"));
	if (dev->capabilities & CAP_LAUDNESS)
		LOG(("CAP_LAUDNESS\n"));
	if (dev->capabilities & CAP_DAC_18BIT)
		LOG(("CAP_DAC_18BIT\n"));
	if (dev->capabilities & CAP_DAC_20BIT)
		LOG(("CAP_DAC_20BIT\n"));
	if (dev->capabilities & CAP_ADC_18BIT)
		LOG(("CAP_ADC_18BIT\n"));
	if (dev->capabilities & CAP_ADC_20BIT)
		LOG(("CAP_ADC_20BIT\n"));
	if (dev->capabilities & CAP_3D_ENHANCEMENT)
		LOG(("CAP_3D_ENHANCEMENT\n"));
	if (dev->capabilities & CAP_VARIABLE_PCM)
		LOG(("CAP_VARIABLE_PCM\n"));
	if (dev->capabilities & CAP_DOUBLE_PCM)
		LOG(("CAP_DOUBLE_PCM\n"));
	if (dev->capabilities & CAP_VARIABLE_MIC)
		LOG(("CAP_VARIABLE_MIC\n"));
	if (dev->capabilities & CAP_CENTER_DAC)
		LOG(("CAP_CENTER_DAC\n"));
	if (dev->capabilities & CAP_SURR_DAC)
		LOG(("CAP_SURR_DAC\n"));
	if (dev->capabilities & CAP_LFE_DAC)
		LOG(("CAP_LFE_DAC\n"));
	if (dev->capabilities & CAP_AMAP)
		LOG(("CAP_AMAP\n"));
	if (dev->capabilities & CAP_REV21)
		LOG(("CAP_REV21\n"));
	if (dev->capabilities & CAP_REV22)
		LOG(("CAP_REV22\n"));
	if (dev->capabilities & CAP_REV23)
		LOG(("CAP_REV23\n"));
	if (dev->capabilities & CAP_PCM_RATE_CONTINUOUS)
		LOG(("CAP_PCM_RATE_CONTINUOUS\n"));
	if (dev->capabilities & CAP_PCM_RATE_8000)
		LOG(("CAP_PCM_RATE_8000\n"));
	if (dev->capabilities & CAP_PCM_RATE_11025)
		LOG(("CAP_PCM_RATE_11025\n"));
	if (dev->capabilities & CAP_PCM_RATE_12000)
		LOG(("CAP_PCM_RATE_12000\n"));
	if (dev->capabilities & CAP_PCM_RATE_16000)
		LOG(("CAP_PCM_RATE_16000\n"));
	if (dev->capabilities & CAP_PCM_RATE_22050)
		LOG(("CAP_PCM_RATE_22050\n"));
	if (dev->capabilities & CAP_PCM_RATE_24000)
		LOG(("CAP_PCM_RATE_24000\n"));
	if (dev->capabilities & CAP_PCM_RATE_32000)
		LOG(("CAP_PCM_RATE_32000\n"));
	if (dev->capabilities & CAP_PCM_RATE_44100)
		LOG(("CAP_PCM_RATE_44100\n"));
	if (dev->capabilities & CAP_PCM_RATE_48000)
		LOG(("CAP_PCM_RATE_48000\n"));
	if (dev->capabilities & CAP_PCM_RATE_88200)
		LOG(("CAP_PCM_RATE_88200\n"));
	if (dev->capabilities & CAP_PCM_RATE_96000)
		LOG(("CAP_PCM_RATE_96000\n"));
}

/*************************************************
 */

bool
default_reg_is_valid(ac97_dev *dev, uint8 reg)
{
	if (reg & 1)	return false;
	if (reg > 0x7e)	return false;
	return true;
}

void default_init(ac97_dev *dev)
{
	LOG(("default_init\n"));
}

void default_amp_enable(ac97_dev *dev, bool yesno)
{
	LOG(("default_amp_enable\n"));
	LOG(("powerdown register was = %#04x\n", ac97_reg_uncached_read(dev, AC97_POWERDOWN)));
	#if REVERSE_EAMP_POLARITY
		yesno = !yesno;
		LOG(("using reverse eamp polarity\n"));
	#endif
	if (yesno)
		ac97_reg_cached_write(dev, AC97_POWERDOWN, ac97_reg_uncached_read(dev, AC97_POWERDOWN) & ~0x8000); /* switch on (low active) */
	else
		ac97_reg_cached_write(dev, AC97_POWERDOWN, ac97_reg_uncached_read(dev, AC97_POWERDOWN) | 0x8000); /* switch off */
	LOG(("powerdown register is = %#04x\n", ac97_reg_uncached_read(dev, AC97_POWERDOWN)));
}



void ad1886_init(ac97_dev *dev)
{
	LOG(("ad1886_init\n"));

	LOG(("===\n"));
	LOG(("0x26 = %#04x\n", ac97_reg_uncached_read(dev, 0x26)));
	LOG(("0x2A = %#04x\n", ac97_reg_uncached_read(dev, 0x2A)));
	LOG(("0x3A = %#04x\n", ac97_reg_uncached_read(dev, 0x3A)));
	LOG(("0x72 = %#04x\n", ac97_reg_uncached_read(dev, 0x72)));
	LOG(("0x74 = %#04x\n", ac97_reg_uncached_read(dev, 0x74)));
	LOG(("0x76 = %#04x\n", ac97_reg_uncached_read(dev, 0x76)));

//	ich_codec_write(config->codecoffset + 0x72, 0x0010); // enable software jack sense
//	ich_codec_write(config->codecoffset + 0x72, 0x0110); // disable hardware line muting

	ac97_reg_cached_write(dev, 0x72, 0x0230);

	LOG(("===\n"));
	LOG(("0x26 = %#04x\n", ac97_reg_uncached_read(dev, 0x26)));
	LOG(("0x2A = %#04x\n", ac97_reg_uncached_read(dev, 0x2A)));
	LOG(("0x3A = %#04x\n", ac97_reg_uncached_read(dev, 0x3A)));
	LOG(("0x72 = %#04x\n", ac97_reg_uncached_read(dev, 0x72)));
	LOG(("0x74 = %#04x\n", ac97_reg_uncached_read(dev, 0x74)));
	LOG(("0x76 = %#04x\n", ac97_reg_uncached_read(dev, 0x76)));

	LOG(("===\n"));
}

void cs4299_amp_enable(ac97_dev *dev, bool yesno)
{
	LOG(("cs4299_amp_enable\n"));
	if (yesno)
		ac97_reg_cached_write(dev, 0x68, 0x8004);
	else
		ac97_reg_cached_write(dev, 0x68, 0);
}


void alc650_init(ac97_dev *dev)
{
	LOG(("alc650_init\n"));
	/* Surround, LFE and Center downmixed into Line-out,
	 * Surround-out is duplicated Line-out.
	 */
	ac97_reg_cached_write(dev, AC97_ALC650_MULTI_CHAN_CTRL, 0x0007);
}
