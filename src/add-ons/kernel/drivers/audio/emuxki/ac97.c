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
#include <stdio.h>
#include <MediaDefs.h>
#include "ac97.h"

#define REVERSE_EAMP_POLARITY 0

#include "debug.h"
#include "io.h"

#define B_UTF8_REGISTERED	"\xC2\xAE"

const char * stereo_enhancement_technique[] =
{
	"No 3D stereo enhancement",
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

typedef void (* codec_init)(device_config *);
typedef void (* codec_amp_enable)(device_config *, bool);

typedef struct codec_ops_tag
{
	codec_init init;
	codec_amp_enable amp_enable;
} codec_ops;

typedef struct codec_table_tag
{
	uint32 id;
	uint32 mask;
	codec_ops *ops;
	const char *info;
} codec_table;

void default_init(device_config *);
void ad1886_init(device_config *);

void default_amp_enable(device_config *, bool);
void cs4299_amp_enable(device_config *, bool);

codec_ops default_ops = { default_init, default_amp_enable };
codec_ops ad1886_ops = { ad1886_init, default_amp_enable };
codec_ops cs4299_ops = { default_init, cs4299_amp_enable };

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
	{ 0x41445363, 0xffffffff, &default_ops, "Analog Devices AD1886A SoundMAX"B_UTF8_REGISTERED },
	{ 0x41445371, 0xffffffff, &default_ops, "Analog Devices AD1981A SoundMAX"B_UTF8_REGISTERED },
	{ 0x41445372, 0xffffffff, &default_ops, "Analog Devices AD1981A SoundMAX"B_UTF8_REGISTERED },
	{ 0x414c4320, 0xfffffff0, &default_ops, "Avance Logic (Realtek) ALC100/ALC100P, RL5383/RL5522" },
	{ 0x414c4730, 0xffffffff, &default_ops, "Avance Logic (Realtek) ALC101" },
#if 0
	{ 0x414c4710, 0xffffffff, &default_ops, "Avance Logic (Realtek) ALC200/ALC200A" }, /* datasheet says id2 = 4710 */
	{ 0x414c4710, 0xffffffff, &default_ops, "Avance Logic (Realtek) ALC201/ALC201A" }, /* 4710 or 4720 */
	{ 0x414c4720, 0xffffffff, &default_ops, "Avance Logic (Realtek) ALC650" }, /* datasheet says id2 = 4720 */
#else
	{ 0x414c4710, 0xffffffff, &default_ops, "Avance Logic (Realtek) ALC200/ALC200A or ALC201/ALC201A" },
	{ 0x414c4720, 0xffffffff, &default_ops, "Avance Logic (Realtek) ALC650 or ALC201/ALC201A" },
#endif
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

static codec_table *
find_codec_table(uint32 codecid)
{
	codec_table *codec;
	for (codec = codecs; codec->id; codec++)
		if ((codec->id & codec->mask) == (codecid & codec->mask))
			break;
	return codec;
}

const char *
ac97_get_3d_stereo_enhancement(device_config *config)
{
	uint16 data;
	data = emuxki_codec_read(config, AC97_RESET);
	data = (data >> 10) & 31;
	return stereo_enhancement_technique[data];
}

const char *
ac97_get_vendor_id_description(device_config *config)
{
	uint32 id = ac97_get_vendor_id(config);
	codec_table *codec = find_codec_table(id);
	char f = (id >> 24) & 0xff;
	char s = (id >> 16) & 0xff;
	char t = (id >>  8) & 0xff;
	if (f == 0) f = '?';
	if (s == 0) s = '?';
	if (t == 0) t = '?';
	LOG(("codec %c%c%c %u\n",f,s,t,id & 0xff));
	LOG(("info: %s\n",codec->info));
	return codec->info;
}

uint32
ac97_get_vendor_id(device_config *config)
{
	uint16 data1;
	uint16 data2;
	data1 = emuxki_codec_read(config, AC97_VENDOR_ID1);
	data2 = emuxki_codec_read(config, AC97_VENDOR_ID2);
	return (((uint32)data1) << 16) | data2;
}

void
ac97_amp_enable(device_config *config, bool yesno)
{
	codec_table *codec;
	LOG(("ac97_amp_enable\n"));
	codec = find_codec_table(ac97_get_vendor_id(config));
	codec->ops->amp_enable(config, yesno);
}

void
ac97_init(device_config *config)
{
	codec_table *codec;
	LOG(("ac97_init\n"));
	codec = find_codec_table(ac97_get_vendor_id(config));
	codec->ops->init(config);
}

void default_init(device_config *config)
{
	LOG(("default_init\n"));
}

void ad1886_init(device_config *config)
{
	LOG(("ad1886_init\n"));
	emuxki_codec_write(config, 0x72, 0x0010);
}

void default_amp_enable(device_config *config, bool yesno)
{
	LOG(("default_amp_enable\n"));
	LOG(("powerdown register was = %#04x\n",emuxki_codec_read(config, AC97_POWERDOWN)));
	#if REVERSE_EAMP_POLARITY
		yesno = !yesno;
		LOG(("using reverse eamp polarity\n"));
	#endif
	if (yesno)
		emuxki_codec_write(config, AC97_POWERDOWN, emuxki_codec_read(config, AC97_POWERDOWN) & ~0x8000); /* switch on (low active) */
	else
		emuxki_codec_write(config, AC97_POWERDOWN, emuxki_codec_read(config, AC97_POWERDOWN) | 0x8000); /* switch off */
	LOG(("powerdown register is = %#04x\n", emuxki_codec_read(config, AC97_POWERDOWN)));
}

void cs4299_amp_enable(device_config *config, bool yesno)
{
	LOG(("cs4299_amp_enable\n"));
	if (yesno)
		emuxki_codec_write(config, 0x68, 0x8004);
	else
		emuxki_codec_write(config, 0x68, 0);
}

const ac97_source_info source_info[] = {
	{ "Record", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO|B_MIX_RECORDMUX, 100, AC97_RECORD_GAIN, 0x8000, 4, 0, 1, 0, 0.0, 22.5, 1.5 },
	{ "Master", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 101, AC97_MASTER_VOLUME, 0x8000, 5, 0, 1, 1,-46.5, 0.0, 1.5 },
	//{ "Bass/Trebble", B_MIX_GAIN|B_MIX_STEREO, 102, AC97_MASTER_TONE, 0x0f0f, 4, 0, 1, 1,-12.0, 10.5, 1.5 },
	//{ "Aux out", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 103, AC97_AUX_OUT_VOLUME, 0x8000, 5, 0, 1, 1,-46.5, 0.0, 1.5 },
	{ "PCM out", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 104, AC97_PCM_OUT_VOLUME, 0x8808, 5, 0, 1, 1,-34.5, 12.0, 1.5 },
	{ "CD", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 105, AC97_CD_VOLUME, 0x8808, 5, 0, 1, 1,-34.5, 12.0, 1.5 },
	{ "Aux in", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 106, AC97_AUX_IN_VOLUME, 0x8808, 5, 0, 1, 1,-34.5, 12.0, 1.5 },
	{ "TAD", B_MIX_GAIN|B_MIX_MUTE|B_MIX_MONO, 107, AC97_PHONE_VOLUME, 0x8008, 5, 0, 1, 1,-34.5, 12.0, 1.5 },
	{ "Mic", B_MIX_GAIN|B_MIX_MUTE|B_MIX_MONO|B_MIX_MICBOOST, 108, AC97_MIC_VOLUME, 0x8008, 5, 0, 1, 1,-34.5, 12.0, 1.5 },
	{ "Line in", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 109, AC97_LINE_IN_VOLUME, 0x8808, 5, 0, 1, 1,-34.5, 12.0, 1.5 },
	//{ "Center/Lfe", B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 111, AC97_CENTER_LFE_VOLUME, 0x8080, 5, 0, 1, 1,-46.5, 0.0, 1.5 },
	{ "Center/Lfe" /* should be "Surround" but no */, B_MIX_GAIN|B_MIX_MUTE|B_MIX_STEREO, 110, AC97_SURROUND_VOLUME, 0x8080, 5, 0, 1, 1,-46.5, 0.0, 1.5 }
};

const int32 source_info_size = (sizeof(source_info)/sizeof(source_info[0]));
