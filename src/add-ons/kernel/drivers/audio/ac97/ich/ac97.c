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
#include <MediaDefs.h>
#include "ac97.h"

//#define DEBUG 1

#include "debug.h"
#include "io.h"

const char *
ac97_get_3d_stereo_enhancement()
{
	const char * technique[] = {
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
	uint16 data;
	data = ich_codec_read(AC97_RESET);
	data = (data >> 10) & 31;
	return technique[data];
}

const char *
ac97_get_vendor_id_description()
{
	static char desc[10];
	uint32 id = ac97_get_vendor_id();
	char f = (id >> 24) & 0xff;
	char s = (id >> 16) & 0xff;
	char t = (id >>  8) & 0xff;
	if (f == 0) f = '?';
	if (s == 0) s = '?';
	if (t == 0) t = '?';
	sprintf(desc,"%c%c%c %u",f,s,t,id & 0xff);
	return desc;
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
	switch (ac97_get_vendor_id())
	{
		case 0x43525931: /* Cirrus Logic CS4299 rev A */
		case 0x43525933: /* Cirrus Logic CS4299 rev C */
		case 0x43525934: /* Cirrus Logic CS4299 rev D */
		{
			LOG(("using Cirrus enable"));
			if (yesno)
				ich_codec_write(0x68, 0x8004);
			else
				ich_codec_write(0x68, 0);
			break;
		}

		default:
		{
			LOG(("powerdown register was = %#04x\n",ich_codec_read(AC97_POWERDOWN)));
			if (yesno)
				ich_codec_write(AC97_POWERDOWN, ich_codec_read(AC97_POWERDOWN) & ~0x8000); /* switch on (low active) */
			else
				ich_codec_write(AC97_POWERDOWN, ich_codec_read(AC97_POWERDOWN) | 0x8000); /* switch off */
			LOG(("powerdown register is = %#04x\n",ich_codec_read(AC97_POWERDOWN)));
			break;
		}
	}
}

void
ac97_init()
{
	switch (ac97_get_vendor_id())
	{
		case 0x41445461: /* Analog Devices AD1886 */
			LOG(("using AD1886 init"));
			ich_codec_write(0x72, 0x0010);
			break;
		default:
			break;
	}
}
