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
#include "config.h"

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

typedef void (* codec_init)(void);
typedef void (* codec_amp_enable)(bool);

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

void default_init(void);
void ad1886_init(void);

void default_amp_enable(bool);
void cs4299_amp_enable(bool);

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
	{ 0x41445363, 0xffffffff, &ad1886_ops,  "Analog Devices AD1886A SoundMAX"B_UTF8_REGISTERED },
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

codec_table *
find_codec_table(uint32 codecid)
{
	codec_table *codec;
	for (codec = codecs; codec->id; codec++)
		if ((codec->id & codec->mask) == (codecid & codec->mask))
			break;
	return codec;
}

const char *
ac97_get_3d_stereo_enhancement()
{
	uint16 data;
	data = ich_codec_read(AC97_RESET);
	data = (data >> 10) & 31;
	return stereo_enhancement_technique[data];
}

const char *
ac97_get_vendor_id_description()
{
	uint32 id = ac97_get_vendor_id();
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
ac97_get_vendor_id()
{
	uint16 data1;
	uint16 data2;
	data1 = ich_codec_read(AC97_VENDOR_ID1);
	data2 = ich_codec_read(AC97_VENDOR_ID2);
	return (((uint32)data1) << 16) | data2;
}

void
ac97_amp_enable(bool yesno)
{
	codec_table *codec;
	LOG(("ac97_amp_enable\n"));
	codec = find_codec_table(ac97_get_vendor_id());
	codec->ops->amp_enable(yesno);
}

void
ac97_init()
{
	codec_table *codec;
	LOG(("ac97_init\n"));
	codec = find_codec_table(ac97_get_vendor_id());
	codec->ops->init();
}

void default_init(void)
{
	LOG(("default_init\n"));
}

void ad1886_init(void)
{
	LOG(("ad1886_init\n"));

	LOG(("===\n"));
	LOG(("codecoffset = %d\n",config->codecoffset));
	LOG(("0x26 = %#04x\n",ich_codec_read(config->codecoffset + 0x26)));
	LOG(("0x2A = %#04x\n",ich_codec_read(config->codecoffset + 0x2A)));
	LOG(("0x3A = %#04x\n",ich_codec_read(config->codecoffset + 0x3A)));
	LOG(("0x72 = %#04x\n",ich_codec_read(config->codecoffset + 0x72)));
	LOG(("0x74 = %#04x\n",ich_codec_read(config->codecoffset + 0x74)));
	LOG(("0x76 = %#04x\n",ich_codec_read(config->codecoffset + 0x76)));

//	ich_codec_write(config->codecoffset + 0x72, 0x0010); // enable software jack sense
//	ich_codec_write(config->codecoffset + 0x72, 0x0110); // disable hardware line muting

	ich_codec_write(config->codecoffset + 0x72, 0x0230);

	LOG(("===\n"));
	LOG(("0x26 = %#04x\n",ich_codec_read(config->codecoffset + 0x26)));
	LOG(("0x2A = %#04x\n",ich_codec_read(config->codecoffset + 0x2A)));
	LOG(("0x3A = %#04x\n",ich_codec_read(config->codecoffset + 0x3A)));
	LOG(("0x72 = %#04x\n",ich_codec_read(config->codecoffset + 0x72)));
	LOG(("0x74 = %#04x\n",ich_codec_read(config->codecoffset + 0x74)));
	LOG(("0x76 = %#04x\n",ich_codec_read(config->codecoffset + 0x76)));

	LOG(("===\n"));
}

void default_amp_enable(bool yesno)
{
	LOG(("default_amp_enable\n"));
	LOG(("powerdown register was = %#04x\n",ich_codec_read(config->codecoffset + AC97_POWERDOWN)));
	#if REVERSE_EAMP_POLARITY
		yesno = !yesno;
		LOG(("using reverse eamp polarity\n"));
	#endif
	if (yesno)
		ich_codec_write(config->codecoffset + AC97_POWERDOWN, ich_codec_read(AC97_POWERDOWN) & ~0x8000); /* switch on (low active) */
	else
		ich_codec_write(config->codecoffset + AC97_POWERDOWN, ich_codec_read(AC97_POWERDOWN) | 0x8000); /* switch off */
	LOG(("powerdown register is = %#04x\n",ich_codec_read(config->codecoffset + AC97_POWERDOWN)));
}

void cs4299_amp_enable(bool yesno)
{
	LOG(("cs4299_amp_enable\n"));
	if (yesno)
		ich_codec_write(config->codecoffset + 0x68, 0x8004);
	else
		ich_codec_write(config->codecoffset + 0x68, 0);
}
