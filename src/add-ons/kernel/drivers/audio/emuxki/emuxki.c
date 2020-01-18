/*
 * Emuxki BeOS Driver for Creative Labs SBLive!/Audigy series
 *
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
 *
 * Authors:
 *		Alexander Coers		Alexander.Coers@gmx.de
 *		Fredrik Modéen 		fredrik@modeen.se
 *
*/
/* This code is derived from the NetBSD driver for Creative Labs SBLive! series
 *
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yannick Montulet.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <ByteOrder.h>
#include <KernelExport.h>
#include <PCI.h>
#include <driver_settings.h>
#include <fcntl.h>
#include <math.h>
#include <midi_driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "emuxki.h"
#include "debug.h"
#include "config.h"
#include "util.h"
#include "io.h"
#include "multi.h"
#include "ac97.h"

status_t init_hardware(void);
status_t init_driver(void);
void uninit_driver(void);
const char ** publish_devices(void);
device_hooks * find_device(const char *);

pci_module_info	*pci;
generic_mpu401_module * mpu401;
//static char gameport_name[] = "generic/gameport/v2";
//generic_gameport_module * gameport;

int32 num_cards;
emuxki_dev cards[NUM_CARDS];
int32 num_names;
char * names[NUM_CARDS*20+1];

emuxki_settings current_settings = {
		2,      // channels
		16,     // bits per sample
		48000,  // sample rate
		512,    // buffer frames
		2       // buffer count
};

status_t emuxki_init(emuxki_dev *card);
void emuxki_shutdown(emuxki_dev *card);

extern device_hooks multi_hooks;
extern device_hooks midi_hooks;
//extern device_hooks joy_hooks;

/* Hardware Dump */

static void
dump_hardware_regs(device_config *config)
{
	LOG(("EMU_IPR = %#08x\n",emuxki_reg_read_32(config, EMU_IPR)));
	LOG(("EMU_INTE = %#08x\n",emuxki_reg_read_32(config, EMU_INTE)));
	LOG(("EMU_HCFG = %#08x\n",emuxki_reg_read_32(config, EMU_HCFG)));
	snooze(1000);
	/*emuxki_reg_write_8(config, EMU_AC97ADDRESS, EMU_AC97ADDRESS_READY);
	LOG(("EMU_AC97ADDRESS_READY = %#08x\n", emuxki_reg_read_16(config, EMU_AC97DATA)));*/

	/*emuxki_reg_write_8(config, EMU_AC97ADDRESS, EMU_AC97ADDRESS_ADDRESS);
	LOG(("EMU_AC97ADDRESS_ADDRESS = %#08x\n", emuxki_reg_read_16(config, EMU_AC97DATA)));*/

	/*LOG(("EMU_CHAN_CPF_STEREO = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_CPF_STEREO)));
	LOG(("EMU_CHAN_FXRT = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_FXRT)));
	LOG(("EMU_CHAN_PTRX = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_PTRX)));
	LOG(("EMU_CHAN_DSL = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_DSL)));
	LOG(("EMU_CHAN_PSST = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_PSST)));
	LOG(("EMU_CHAN_CCCA = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_CCCA)));
	LOG(("EMU_CHAN_Z1 = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_Z1)));
	LOG(("EMU_CHAN_Z2 = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_Z2)));
	LOG(("EMU_CHAN_MAPA = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_MAPA)));
	LOG(("EMU_CHAN_MAPB = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_MAPB)));
	LOG(("EMU_CHAN_CVCF_CURRENTFILTER = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_CVCF_CURRENTFILTER)));
	LOG(("EMU_CHAN_VTFT_FILTERTARGET = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_VTFT_FILTERTARGET)));
	LOG(("EMU_CHAN_ATKHLDM = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_ATKHLDM)));
	LOG(("EMU_CHAN_DCYSUSM = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_DCYSUSM)));
	LOG(("EMU_CHAN_LFOVAL1 = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_LFOVAL1)));
	LOG(("EMU_CHAN_LFOVAL2 = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_LFOVAL2)));
	LOG(("EMU_CHAN_FMMOD = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_FMMOD)));
	LOG(("EMU_CHAN_TREMFRQ = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_TREMFRQ)));
	LOG(("EMU_CHAN_FM2FRQ2 = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_FM2FRQ2)));
	LOG(("EMU_CHAN_ENVVAL = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_ENVVAL)));
	LOG(("EMU_CHAN_ATKHLDV = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_ATKHLDV)));
	LOG(("EMU_CHAN_ENVVOL = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_ENVVOL)));
	LOG(("EMU_CHAN_PEFE = %#08x\n",emuxki_chan_read(config, 0, EMU_CHAN_PEFE)));*/

}


/*static void
trace_hardware_regs(device_config *config)
{
	TRACE(("EMU_IPR = %#08x\n",emuxki_reg_read_32(config, EMU_IPR)));
	TRACE(("EMU_INTE = %#08x\n",emuxki_reg_read_32(config, EMU_INTE)));
	TRACE(("EMU_HCFG = %#08x\n",emuxki_reg_read_32(config, EMU_HCFG)));
}*/

/* Misc stuff relative to Emuxki */

int             emu10k1_recbuf_sizes[] = {
	0, 384, 448, 512, 640, 768, 896, 1024, 1280, 1536, 1792,
	2048, 2560, 3072, 3584, 4096, 5120, 6144, 7168, 8192, 10240, 12288,
	14366, 16384, 20480, 24576, 28672, 32768, 40960, 49152, 57344, 65536
};


static uint32
emuxki_rate_to_pitch(uint32 rate)
{
	static uint32 logMagTable[128] = {
		0x00000, 0x02dfc, 0x05b9e, 0x088e6, 0x0b5d6, 0x0e26f, 0x10eb3,
		0x13aa2, 0x1663f, 0x1918a, 0x1bc84, 0x1e72e, 0x2118b, 0x23b9a,
		0x2655d, 0x28ed5, 0x2b803, 0x2e0e8, 0x30985, 0x331db, 0x359eb,
		0x381b6, 0x3a93d, 0x3d081, 0x3f782, 0x41e42, 0x444c1, 0x46b01,
		0x49101, 0x4b6c4, 0x4dc49, 0x50191, 0x5269e, 0x54b6f, 0x57006,
		0x59463, 0x5b888, 0x5dc74, 0x60029, 0x623a7, 0x646ee, 0x66a00,
		0x68cdd, 0x6af86, 0x6d1fa, 0x6f43c, 0x7164b, 0x73829, 0x759d4,
		0x77b4f, 0x79c9a, 0x7bdb5, 0x7dea1, 0x7ff5e, 0x81fed, 0x8404e,
		0x86082, 0x88089, 0x8a064, 0x8c014, 0x8df98, 0x8fef1, 0x91e20,
		0x93d26, 0x95c01, 0x97ab4, 0x9993e, 0x9b79f, 0x9d5d9, 0x9f3ec,
		0xa11d8, 0xa2f9d, 0xa4d3c, 0xa6ab5, 0xa8808, 0xaa537, 0xac241,
		0xadf26, 0xafbe7, 0xb1885, 0xb3500, 0xb5157, 0xb6d8c, 0xb899f,
		0xba58f, 0xbc15e, 0xbdd0c, 0xbf899, 0xc1404, 0xc2f50, 0xc4a7b,
		0xc6587, 0xc8073, 0xc9b3f, 0xcb5ed, 0xcd07c, 0xceaec, 0xd053f,
		0xd1f73, 0xd398a, 0xd5384, 0xd6d60, 0xd8720, 0xda0c3, 0xdba4a,
		0xdd3b4, 0xded03, 0xe0636, 0xe1f4e, 0xe384a, 0xe512c, 0xe69f3,
		0xe829f, 0xe9b31, 0xeb3a9, 0xecc08, 0xee44c, 0xefc78, 0xf148a,
		0xf2c83, 0xf4463, 0xf5c2a, 0xf73da, 0xf8b71, 0xfa2f0, 0xfba57,
		0xfd1a7, 0xfe8df
	};
	static uint8 logSlopeTable[128] = {
		0x5c, 0x5c, 0x5b, 0x5a, 0x5a, 0x59, 0x58, 0x58,
		0x57, 0x56, 0x56, 0x55, 0x55, 0x54, 0x53, 0x53,
		0x52, 0x52, 0x51, 0x51, 0x50, 0x50, 0x4f, 0x4f,
		0x4e, 0x4d, 0x4d, 0x4d, 0x4c, 0x4c, 0x4b, 0x4b,
		0x4a, 0x4a, 0x49, 0x49, 0x48, 0x48, 0x47, 0x47,
		0x47, 0x46, 0x46, 0x45, 0x45, 0x45, 0x44, 0x44,
		0x43, 0x43, 0x43, 0x42, 0x42, 0x42, 0x41, 0x41,
		0x41, 0x40, 0x40, 0x40, 0x3f, 0x3f, 0x3f, 0x3e,
		0x3e, 0x3e, 0x3d, 0x3d, 0x3d, 0x3c, 0x3c, 0x3c,
		0x3b, 0x3b, 0x3b, 0x3b, 0x3a, 0x3a, 0x3a, 0x39,
		0x39, 0x39, 0x39, 0x38, 0x38, 0x38, 0x38, 0x37,
		0x37, 0x37, 0x37, 0x36, 0x36, 0x36, 0x36, 0x35,
		0x35, 0x35, 0x35, 0x34, 0x34, 0x34, 0x34, 0x34,
		0x33, 0x33, 0x33, 0x33, 0x32, 0x32, 0x32, 0x32,
		0x32, 0x31, 0x31, 0x31, 0x31, 0x31, 0x30, 0x30,
		0x30, 0x30, 0x30, 0x2f, 0x2f, 0x2f, 0x2f, 0x2f
	};
	int8          i;

	if (rate == 0)
		return 0;	/* Bail out if no leading "1" */
	rate *= 11185;		/* Scale 48000 to 0x20002380 */
	for (i = 31; i > 0; i--) {
		if (rate & 0x80000000) {	/* Detect leading "1" */
			return (((uint32) (i - 15) << 20) +
				logMagTable[0x7f & (rate >> 24)] +
				(0x7f & (rate >> 17)) *
				logSlopeTable[0x7f & (rate >> 24)]);
		}
		rate <<= 1;
	}
	return 0;		/* Should never reach this point */
}

/* Emuxki Memory management */

static emuxki_mem *
emuxki_mem_new(emuxki_dev *card, int ptbidx, size_t size)
{
	emuxki_mem *mem;

	if ((mem = malloc(sizeof(*mem))) == NULL)
		return (NULL);

	mem->ptbidx = ptbidx;
	mem->area = alloc_mem(&mem->phy_base, &mem->log_base, size, "emuxki buffer");
	mem->size = size;
	if (mem->area < B_OK) {
		free(mem);
		return NULL;
	}
	return mem;
}


static void
emuxki_mem_delete(emuxki_mem *mem)
{
	if (mem->area > B_OK)
		delete_area(mem->area);
	free(mem);
}


void *
emuxki_pmem_alloc(emuxki_dev *card, size_t size)
{
	int             i;//, s;
	size_t          numblocks;
	emuxki_mem *mem;
	uint32      j, *ptb, silentpage;

	ptb = card->ptb_log_base;
	silentpage = ((uint32)card->silentpage_phy_base) << 1;
	numblocks = size / EMU_PTESIZE;
	if (size % EMU_PTESIZE)
		numblocks++;

	PRINT(("emuxki_pmem_alloc : numblocks : %ld\n", numblocks));

	for (i = 0; i < EMU_MAXPTE; i++) {
		PRINT(("emuxki_pmem_alloc : %d\n", i));
		if ((B_LENDIAN_TO_HOST_INT32(ptb[i]) & EMU_CHAN_MAP_PTE_MASK) == silentpage) {
			/* We look for a free PTE */
			//s = splaudio();
			for (j = 0; j < numblocks; j++)
				if ((B_LENDIAN_TO_HOST_INT32(ptb[i + j]) & EMU_CHAN_MAP_PTE_MASK)
				    != silentpage)
					break;
			if (j == numblocks) {
				PRINT(("emuxki_pmem_alloc : j == numblocks %" B_PRIu32 "\n",
					j));
				if ((mem = emuxki_mem_new(card, i, size)) == NULL) {
					//splx(s);
					return (NULL);
				}
				PRINT(("emuxki_pmem_alloc : j == numblocks emuxki_mem_new ok\n"));
				for (j = 0; j < numblocks; j++)
					ptb[i + j] = B_HOST_TO_LENDIAN_INT32((uint32) (
						(( ((uint32)mem->phy_base) +
						 j * EMU_PTESIZE) << 1)
						| (i + j)));
				LIST_INSERT_HEAD(&(card->mem), mem, next);
				PRINT(("emuxki_pmem_alloc : j == numblocks returning\n"));

				//splx(s);
				return mem->log_base;
			} else {
				PRINT(("emuxki_pmem_alloc : j != numblocks %" B_PRIu32 "\n",
					j));
				i += j;
			}
			//splx(s);
		}
	}
	return NULL;
}


void *
emuxki_rmem_alloc(emuxki_dev *card, size_t size)
{
	emuxki_mem *mem;
	//int             s;

	mem = emuxki_mem_new(card, EMU_RMEM, size);
	if (mem == NULL)
		return (NULL);

	//s = splaudio();
	LIST_INSERT_HEAD(&(card->mem), mem, next);
	//splx(s);

	return mem->log_base;
}


void
emuxki_mem_free(emuxki_dev *card, void *ptr)
{
	emuxki_mem 		*mem;
	size_t          numblocks;
	uint32      	i, *ptb, silentpage;

	ptb = card->ptb_log_base;
	silentpage = ((uint32)card->silentpage_phy_base) << 1;
	LOG(("emuxki_mem_free 1\n"));
	LIST_FOREACH(mem, &card->mem, next) {
		LOG(("emuxki_mem_free 2\n"));
		if (mem->log_base != ptr)
			continue;
		LOG(("emuxki_mem_free 3\n"));
		//s = splaudio();
		if (mem->ptbidx != EMU_RMEM) {
			LOG(("mem_size : %i\n", mem->size));
			numblocks = mem->size / EMU_PTESIZE;
			if (mem->size % EMU_PTESIZE)
				numblocks++;
			for (i = 0; i < numblocks; i++)
				ptb[mem->ptbidx + i] =
					B_HOST_TO_LENDIAN_INT32(silentpage | (mem->ptbidx + i));
		}
		LIST_REMOVE(mem, next);
		//splx(s);

		LOG(("freeing mem\n"));
		emuxki_mem_delete(mem);
		break;
	}
}

/* Emuxki channel functions */

static void
emuxki_chanparms_set_defaults(emuxki_channel *chan)
{
	chan->fxsend.a.level = chan->fxsend.b.level
		= chan->fxsend.c.level = chan->fxsend.d.level
	/* for audigy */
		= chan->fxsend.e.level = chan->fxsend.f.level
		= chan->fxsend.g.level = chan->fxsend.h.level
		= IS_AUDIGY(&chan->voice->stream->card->config) ? 0xc0 : 0xff;	/* not max */

	chan->fxsend.a.dest = 0x0;
	chan->fxsend.b.dest = 0x1;
	chan->fxsend.c.dest = 0x2;
	chan->fxsend.d.dest = 0x3;
	/* for audigy */
	chan->fxsend.e.dest = 0x4;
	chan->fxsend.f.dest = 0x5;
	chan->fxsend.g.dest = 0x6;
	chan->fxsend.h.dest = 0x7;

	chan->pitch.intial = 0x0000;	/* shouldn't it be 0xE000 ? */
	chan->pitch.current = 0x0000;	/* should it be 0x0400 */
	chan->pitch.target = 0x0000;	/* the unity pitch shift ? */
	chan->pitch.envelope_amount = 0x00;	/* none */

	chan->initial_attenuation = 0x00;	/* no attenuation */
	chan->volume.current = 0x0000;	/* no volume */
	chan->volume.target = 0xffff;
	chan->volume.envelope.current_state = 0x8000;	/* 0 msec delay */
	chan->volume.envelope.hold_time = 0x7f;	/* 0 msec */
	chan->volume.envelope.attack_time = 0x7f;	/* 5.5msec */
	chan->volume.envelope.sustain_level = 0x7f;	/* full  */
	chan->volume.envelope.decay_time = 0x7f;	/* 22msec  */

	chan->filter.initial_cutoff_frequency = 0xff;	/* no filter */
	chan->filter.current_cutoff_frequency = 0xffff;	/* no filtering */
	chan->filter.target_cutoff_frequency = 0xffff;	/* no filtering */
	chan->filter.lowpass_resonance_height = 0x0;
	chan->filter.interpolation_ROM = 0x1;	/* full band */
	chan->filter.envelope_amount = 0x7f;	/* none */
	chan->filter.LFO_modulation_depth = 0x00;	/* none */

	chan->loop.start = 0x000000;
	chan->loop.end = 0x000010;	/* Why ? */

	chan->modulation.envelope.current_state = 0x8000;
	chan->modulation.envelope.hold_time = 0x00;	/* 127 better ? */
	chan->modulation.envelope.attack_time = 0x00;	/* infinite */
	chan->modulation.envelope.sustain_level = 0x00;	/* off */
	chan->modulation.envelope.decay_time = 0x7f;	/* 22 msec */
	chan->modulation.LFO_state = 0x8000;

	chan->vibrato_LFO.state = 0x8000;
	chan->vibrato_LFO.modulation_depth = 0x00;	/* none */
	chan->vibrato_LFO.vibrato_depth = 0x00;
	chan->vibrato_LFO.frequency = 0x00;	/* Why set to 24 when
						 * initialized ? */

	chan->tremolo_depth = 0x00;
}


static emuxki_channel *
emuxki_channel_new(emuxki_voice *voice, uint8 num)
{
	emuxki_channel *chan;

	chan = (emuxki_channel *) malloc(sizeof(emuxki_channel));
	if (chan == NULL)
		return NULL;
	chan->voice = voice;
	chan->num = num;
	emuxki_chanparms_set_defaults(chan);
	chan->voice->stream->card->channel[num] = chan;
	return chan;
}


static void
emuxki_channel_delete(emuxki_channel *chan)
{
	chan->voice->stream->card->channel[chan->num] = NULL;
	free(chan);
}


static void
emuxki_channel_set_fxsend(emuxki_channel *chan,
			   emuxki_chanparms_fxsend *fxsend)
{
	/* Could do a memcpy ...*/
	chan->fxsend.a.level = fxsend->a.level;
	chan->fxsend.b.level = fxsend->b.level;
	chan->fxsend.c.level = fxsend->c.level;
	chan->fxsend.d.level = fxsend->d.level;
	chan->fxsend.a.dest = fxsend->a.dest;
	chan->fxsend.b.dest = fxsend->b.dest;
	chan->fxsend.c.dest = fxsend->c.dest;
	chan->fxsend.d.dest = fxsend->d.dest;

	/* for audigy */
	chan->fxsend.e.level = fxsend->e.level;
	chan->fxsend.f.level = fxsend->f.level;
	chan->fxsend.g.level = fxsend->g.level;
	chan->fxsend.h.level = fxsend->h.level;
	chan->fxsend.e.dest = fxsend->e.dest;
	chan->fxsend.f.dest = fxsend->f.dest;
	chan->fxsend.g.dest = fxsend->g.dest;
	chan->fxsend.h.dest = fxsend->h.dest;
}


static void
emuxki_channel_set_srate(emuxki_channel *chan, uint32 srate)
{
	chan->pitch.target = (srate << 8) / 375;
	chan->pitch.target = (chan->pitch.target >> 1) +
		(chan->pitch.target & 1);
	chan->pitch.target &= 0xffff;
	chan->pitch.current = chan->pitch.target;
	chan->pitch.intial =
		(emuxki_rate_to_pitch(srate) >> 8) & EMU_CHAN_IP_MASK;
}


/* voice params must be set before calling this */
static void
emuxki_channel_set_bufparms(emuxki_channel *chan,
			     uint32 start, uint32 end)
{
	chan->loop.start = start & EMU_CHAN_PSST_LOOPSTARTADDR_MASK;
	chan->loop.end = end & EMU_CHAN_DSL_LOOPENDADDR_MASK;
}


static void
emuxki_channel_commit_fx(emuxki_channel *chan)
{
	emuxki_dev   	*card = chan->voice->stream->card;
	uint8			chano = chan->num;

	if (IS_AUDIGY(&card->config)) {
		emuxki_chan_write(&card->config, chano, 0x4c, 0);
		emuxki_chan_write(&card->config, chano, 0x4d, 0);
		emuxki_chan_write(&card->config, chano, 0x4e, 0);
		emuxki_chan_write(&card->config, chano, 0x4f, 0);

		emuxki_chan_write(&card->config, chano, EMU_A_CHAN_FXRT1,
			      (chan->fxsend.d.dest << 24) |
			      (chan->fxsend.c.dest << 16) |
			      (chan->fxsend.b.dest << 8) |
			      (chan->fxsend.a.dest));
		emuxki_chan_write(&card->config, chano, EMU_A_CHAN_FXRT2,
			      (chan->fxsend.h.dest << 24) |
			      (chan->fxsend.g.dest << 16) |
			      (chan->fxsend.f.dest << 8) |
			      (chan->fxsend.e.dest));
		emuxki_chan_write(&card->config, chano, EMU_A_CHAN_SENDAMOUNTS,
			      (chan->fxsend.e.level << 24) |
			      (chan->fxsend.f.level << 16) |
			      (chan->fxsend.g.level << 8) |
			      (chan->fxsend.h.level));
	} else {
		emuxki_chan_write(&card->config, chano, EMU_CHAN_FXRT,
			      (chan->fxsend.d.dest << 28) |
			      (chan->fxsend.c.dest << 24) |
			      (chan->fxsend.b.dest << 20) |
			      (chan->fxsend.a.dest << 16));
	}

	emuxki_chan_write(&card->config, chano, 0x10000000 | EMU_CHAN_PTRX,
		      (chan->fxsend.a.level << 8) | chan->fxsend.b.level);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_DSL,
		      (chan->fxsend.d.level << 24) | chan->loop.end);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_PSST,
		      (chan->fxsend.c.level << 24) | chan->loop.start);
}


static void
emuxki_channel_commit_parms(emuxki_channel *chan)
{
	emuxki_voice *voice = chan->voice;
	emuxki_dev   	*card = chan->voice->stream->card;
	uint32			start, mapval;
	uint8			chano = chan->num;
	//int             s;

	start = chan->loop.start +
		(voice->stereo ? 28 : 30) * (voice->b16 + 1);
	mapval = ((uint32)card->silentpage_phy_base) << 1 | EMU_CHAN_MAP_PTI_MASK;

	//s = splaudio();
	emuxki_chan_write(&card->config, chano, EMU_CHAN_CPF_STEREO, voice->stereo);

	emuxki_channel_commit_fx(chan);

	emuxki_chan_write(&card->config, chano, EMU_CHAN_CCCA,
		      (chan->filter.lowpass_resonance_height << 28) |
		      (chan->filter.interpolation_ROM << 25) |
		   (voice->b16 ? 0 : EMU_CHAN_CCCA_8BITSELECT) | start);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_Z1, 0);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_Z2, 0);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_MAPA, mapval);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_MAPB, mapval);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_CVCF_CURRFILTER,
		      chan->filter.current_cutoff_frequency);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_VTFT_FILTERTARGET,
		      chan->filter.target_cutoff_frequency);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_ATKHLDM,
		      (chan->modulation.envelope.hold_time << 8) |
		      chan->modulation.envelope.attack_time);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_DCYSUSM,
		      (chan->modulation.envelope.sustain_level << 8) |
		      chan->modulation.envelope.decay_time);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_LFOVAL1,
		      chan->modulation.LFO_state);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_LFOVAL2,
		      chan->vibrato_LFO.state);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_FMMOD,
		      (chan->vibrato_LFO.modulation_depth << 8) |
		      chan->filter.LFO_modulation_depth);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_TREMFRQ,
		      (chan->tremolo_depth << 8));
	emuxki_chan_write(&card->config, chano, EMU_CHAN_FM2FRQ2,
		      (chan->vibrato_LFO.vibrato_depth << 8) |
		      chan->vibrato_LFO.frequency);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_ENVVAL,
		      chan->modulation.envelope.current_state);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_ATKHLDV,
		      (chan->volume.envelope.hold_time << 8) |
		      chan->volume.envelope.attack_time);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_ENVVOL,
		      chan->volume.envelope.current_state);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_PEFE,
		      (chan->pitch.envelope_amount << 8) |
		      chan->filter.envelope_amount);
	//splx(s);
}


static void
emuxki_channel_start(emuxki_channel *chan)
{
	emuxki_voice *voice = chan->voice;
	emuxki_dev   *card = chan->voice->stream->card;
	uint8        cache_sample, cache_invalid_size, chano = chan->num;
	uint32       sample;
	//int             s;

	cache_sample = voice->stereo ? 4 : 2;
	sample = voice->b16 ? 0x00000000 : 0x80808080;
	cache_invalid_size = (voice->stereo ? 28 : 30) * (voice->b16 + 1);

	//s = splaudio();
	while (cache_sample--)
		emuxki_chan_write(&card->config, chano,
			      EMU_CHAN_CD0 + cache_sample, sample);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_CCR_CACHEINVALIDSIZE, 0);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_CCR_READADDRESS, 64);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_CCR_CACHEINVALIDSIZE,
		      cache_invalid_size);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_IFATN,
		      (chan->filter.target_cutoff_frequency << 8) |
		      chan->initial_attenuation);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_VTFT_VOLUMETARGET,
		      chan->volume.target);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_CVCF_CURRVOL,
		      chan->volume.current);
	emuxki_chan_write(&card->config, 0, EMU_MKSUBREG(1, chano,
					       EMU_SOLEL + (chano >> 5)),
		      0);	/* Clear stop on loop */
	emuxki_chan_write(&card->config, 0, EMU_MKSUBREG(1, chano,
					       EMU_CLIEL + (chano >> 5)),
		      0);	/* Clear loop interrupt */
	emuxki_chan_write(&card->config, chano, EMU_CHAN_DCYSUSV,
		      (chan->volume.envelope.sustain_level << 8) |
		      chan->volume.envelope.decay_time);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_PTRX_PITCHTARGET,
		      chan->pitch.target);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_CPF_PITCH,
		      chan->pitch.current);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_IP, chan->pitch.intial);

	//splx(s);
}


static void
emuxki_channel_stop(emuxki_channel *chan)
{
	emuxki_dev   	*card = chan->voice->stream->card;
	//int             s;
	uint8           chano = chan->num;

	//s = splaudio();

	emuxki_chan_write(&card->config, chano, EMU_CHAN_PTRX_PITCHTARGET, 0);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_CPF_PITCH, 0);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_IFATN_ATTENUATION, 0xff);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_VTFT_VOLUMETARGET, 0);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_CVCF_CURRVOL, 0);
	emuxki_chan_write(&card->config, chano, EMU_CHAN_IP, 0);

	//splx(s);
}


/*	Emuxki voice functions */
/*static void
emuxki_dump_voice(emuxki_voice *voice)
{
	LOG(("voice->use = %#u\n", voice->use));
	LOG(("voice->state = %#u\n", voice->state));
	LOG(("voice->stereo = %#u\n", voice->stereo));
	LOG(("voice->b16 = %#u\n", voice->b16));
	LOG(("voice->sample_rate = %#lu\n", voice->sample_rate));
	LOG(("voice->buffer = %#08x\n", voice->buffer));
	if (voice->buffer) {
		LOG(("voice->buffer->ptbidx = %#u\n", voice->buffer->ptbidx));
		LOG(("voice->buffer->log_base = %#08x\n", voice->buffer->log_base));
		LOG(("voice->buffer->phy_base = %#08x\n", voice->buffer->phy_base));
		LOG(("voice->buffer->size = %#08x\n", voice->buffer->size));
		LOG(("voice->buffer->area = %#08x\n", voice->buffer->area));
	}
	LOG(("voice->blksize = %#u\n", voice->blksize));
	LOG(("voice->trigblk = %#u\n", voice->trigblk));
	LOG(("voice->blkmod = %#u\n", voice->blkmod));
	LOG(("voice->timerate = %#u\n", voice->timerate));
}*/


/* Allocate channels for voice in case of play voice */
static status_t
emuxki_voice_channel_create(emuxki_voice *voice)
{
	emuxki_channel **channel = voice->stream->card->channel;
	uint8	        i, stereo = voice->stereo;
	//int             s;

	for (i = 0; i < EMU_NUMCHAN - stereo; i += stereo + 1) {
		if ((stereo && (channel[i + 1] != NULL)) ||
		    (channel[i] != NULL))	/* Looking for free channels */
			continue;
		//s = splaudio();
		if (stereo) {
			voice->dataloc.chan[1] =
				emuxki_channel_new(voice, i + 1);
			if (voice->dataloc.chan[1] == NULL) {
				//splx(s);
				return ENOMEM;
			}
		}
		voice->dataloc.chan[0] = emuxki_channel_new(voice, i);
		if (voice->dataloc.chan[0] == NULL) {
			if (stereo) {
				emuxki_channel_delete(voice->dataloc.chan[1]);
				voice->dataloc.chan[1] = NULL;
			}
			//splx(s);
			return ENOMEM;
		}
		//splx(s);
		return B_OK;
	}
	return EAGAIN;
}


/* When calling this function we assume no one can access the voice */
static void
emuxki_voice_channel_destroy(emuxki_voice *voice)
{
	emuxki_channel_delete(voice->dataloc.chan[0]);
	voice->dataloc.chan[0] = NULL;
	if (voice->stereo)
		emuxki_channel_delete(voice->dataloc.chan[1]);
	voice->dataloc.chan[1] = NULL;
}


static status_t
emuxki_voice_dataloc_create(emuxki_voice *voice)
{
	status_t	error;

	if (voice->use & EMU_USE_PLAY) {
		if ((error = emuxki_voice_channel_create(voice)))
			return (error);
	} else {

	}
	return B_OK;
}


static void
emuxki_voice_dataloc_destroy(emuxki_voice *voice)
{
	if (voice->use & EMU_USE_PLAY) {
		if (voice->dataloc.chan[0] != NULL)
			emuxki_voice_channel_destroy(voice);
	} else {
		uint32 buffaddr_reg, buffsize_reg;
		switch (voice->dataloc.source) {
			case EMU_RECSRC_MIC:
				buffaddr_reg = EMU_MICBA;
				buffsize_reg = EMU_MICBS;
				break;
			case EMU_RECSRC_ADC:
				buffaddr_reg = EMU_ADCBA;
				buffsize_reg = EMU_ADCBS;
				break;
			case EMU_RECSRC_FX:
				buffaddr_reg = EMU_FXBA;
				buffsize_reg = EMU_FXBS;
				break;
			default:
				return;
		}
		emuxki_chan_write(&voice->stream->card->config, 0, buffaddr_reg, 0);
		emuxki_chan_write(&voice->stream->card->config, 0, buffsize_reg,
			EMU_RECBS_BUFSIZE_NONE);
	}
}


static void
emuxki_voice_fxupdate(emuxki_voice *voice)
{
	emuxki_chanparms_fxsend fxsend;

	uint8 maxlevel =
		IS_AUDIGY(&voice->stream->card->config) ? 0xc0 : 0xff;	/* not max */

	if (voice->use & EMU_USE_PLAY) {
		fxsend.a.dest = 0x3f;
		fxsend.b.dest = 0x3f;
		fxsend.c.dest = 0x3f;
		fxsend.d.dest = 0x3f;
		/* for audigy */
		fxsend.e.dest = 0x3f;
		fxsend.f.dest = 0x3f;
		fxsend.g.dest = 0x3f;
		fxsend.h.dest = 0x3f;

		fxsend.a.level = fxsend.b.level = fxsend.c.level = fxsend.d.level =
		fxsend.e.level = fxsend.g.level = fxsend.f.level = fxsend.h.level = 0x00;

		if (voice->stereo) {
			switch(voice->stream->card->play_mode) {
				case 2:
					if (voice->stream->nstereo == 1) {
						fxsend.a.dest = voice->voicenum * 2;
						fxsend.a.level = maxlevel;
					} else if ((voice->stream->nstereo == 2) ||
						((voice->stream->nstereo == 3)&&(voice->voicenum < 2))) {
						fxsend.a.dest = voice->voicenum * 2;
						fxsend.a.level = maxlevel;
						if (voice->voicenum > 1 - 1)
							fxsend.a.dest-=2;
					} else if (voice->stream->nstereo == 3 && voice->voicenum > 1) {
							fxsend.a.dest = 0x0;
							fxsend.a.level = maxlevel / 2;
							fxsend.b.dest = 0x1;
							fxsend.b.level = maxlevel / 2;
					} else {
						LOG(("emuxki_voice_set_stereo case 2 badly managed\n"));
					}
					break;
				case 4:
					if (voice->stream->nstereo == 1) {
						fxsend.a.dest = voice->voicenum * 2;
						fxsend.a.level = maxlevel;
						fxsend.b.dest = voice->voicenum * 2 + 2;
						fxsend.b.level = maxlevel;
					} else if ((voice->stream->nstereo == 2) ||
						((voice->stream->nstereo == 3)&&(voice->voicenum < 2))) {
						fxsend.a.dest = voice->voicenum * 2;
						fxsend.a.level = maxlevel;
					} else if (voice->stream->nstereo == 3 && voice->voicenum > 1) {
						fxsend.a.dest = 0x0;
						fxsend.a.level = maxlevel / 2;
						fxsend.b.dest = 0x1;
						fxsend.b.level = maxlevel / 2;
					} else {
						LOG(("emuxki_voice_set_stereo case 4 badly managed\n"));
					}
					break;
				case 6: // only on audigy
					if (voice->stream->nstereo == 1) {
						fxsend.a.dest = voice->voicenum * 2;
						fxsend.a.level = maxlevel;
						fxsend.b.dest = voice->voicenum * 2 + 2;
						fxsend.b.level = maxlevel;
						fxsend.c.dest = 0x4;
						fxsend.c.level = maxlevel / 2;
						fxsend.d.dest = 0x5;
						fxsend.d.level = maxlevel / 2;
					} else if (voice->stream->nstereo == 2) {
						fxsend.a.dest = voice->voicenum * 2;
						fxsend.a.level = maxlevel;
						if (voice->voicenum < 1) {
							fxsend.b.dest = 0x4;
							fxsend.b.level = maxlevel / 2;
							fxsend.c.dest = 0x5;
							fxsend.c.level = maxlevel / 2;
						}
					} else if (voice->stream->nstereo == 3) {
						fxsend.a.dest = voice->voicenum * 2;
						fxsend.a.level = maxlevel;
					} else {
						LOG(("emuxki_voice_set_stereo case 6 badly managed\n"));
					}
					break;
			}

			emuxki_channel_set_fxsend(voice->dataloc.chan[0],
						   &fxsend);

			switch(voice->stream->card->play_mode) {
				case 2:
					if (voice->stream->nstereo == 1) {
						fxsend.a.dest = voice->voicenum * 2 + 1;
						fxsend.a.level = maxlevel;
					} else if ((voice->stream->nstereo == 2) ||
						((voice->stream->nstereo == 3)&&(voice->voicenum < 2))) {
						fxsend.a.dest = voice->voicenum * 2 + 1;
						fxsend.a.level = maxlevel;
						if (voice->voicenum > 1 - 1)
							fxsend.a.dest-=2;
					} else if (voice->stream->nstereo == 3 && voice->voicenum > 1) {
							fxsend.a.dest = 0x0;
							fxsend.a.level = maxlevel / 2;
							fxsend.b.dest = 0x1;
							fxsend.b.level = maxlevel / 2;
					} else {
						LOG(("emuxki_voice_set_stereo case 2 badly managed\n"));
					}
					break;
				case 4:
					if (voice->stream->nstereo == 1) {
						fxsend.a.dest = voice->voicenum * 2 + 1;
						fxsend.a.level = maxlevel;
						fxsend.b.dest = voice->voicenum * 2 + 3;
						fxsend.b.level = maxlevel;
					} else if ((voice->stream->nstereo == 2) ||
						((voice->stream->nstereo == 3)&&(voice->voicenum < 2))) {
						fxsend.a.dest = voice->voicenum * 2 + 1;
						fxsend.a.level = maxlevel;
					} else if (voice->stream->nstereo == 3 && voice->voicenum > 1) {
						fxsend.a.dest = 0x0;
						fxsend.a.level = maxlevel / 2;
						fxsend.b.dest = 0x1;
						fxsend.b.level = maxlevel / 2;
					} else {
						LOG(("emuxki_voice_set_stereo case 4 badly managed\n"));
					}
					break;
				case 6: // only on audigy
					if (voice->stream->nstereo == 1) {
						fxsend.a.dest = voice->voicenum * 2 + 1;
						fxsend.a.level = maxlevel;
						fxsend.b.dest = voice->voicenum * 2 + 3;
						fxsend.b.level = maxlevel;
						fxsend.c.dest = 0x4;
						fxsend.c.level = maxlevel / 2;
						fxsend.d.dest = 0x5;
						fxsend.d.level = maxlevel / 2;
					} else if (voice->stream->nstereo == 2) {
						fxsend.a.dest = voice->voicenum * 2 + 1;
						fxsend.a.level = maxlevel;
						if (voice->voicenum < 1) {
							fxsend.b.dest = 0x4;
							fxsend.b.level = maxlevel / 2;
							fxsend.c.dest = 0x5;
							fxsend.c.level = maxlevel / 2;
						}
					} else if (voice->stream->nstereo == 3) {
						fxsend.a.dest = voice->voicenum * 2 + 1;
						fxsend.a.level = maxlevel;
					} else {
						LOG(("emuxki_voice_set_stereo case 6 badly managed\n"));
					}
					break;
			}

			emuxki_channel_set_fxsend(voice->dataloc.chan[1],
						   &fxsend);
		} else {
			switch(voice->stream->card->play_mode) {
				case 2:
					if (voice->stream->nmono == 1) {
						fxsend.a.dest = voice->voicenum;
						fxsend.a.level = maxlevel;
						fxsend.b.dest = voice->voicenum + 1;
						fxsend.b.level = maxlevel;
					} else if (voice->stream->nmono == 2) {
						fxsend.a.dest = voice->voicenum;
						fxsend.a.level = maxlevel;
					} else if ((voice->stream->nmono == 4) ||
						((voice->stream->nmono == 6)&&(voice->voicenum < 4))) {
						fxsend.a.dest = voice->voicenum;
						fxsend.a.level = maxlevel;
						if (voice->voicenum > 2 - 1)
							fxsend.a.dest-=2;
					} else if (voice->stream->nmono == 6 && voice->voicenum > 3) {
							fxsend.a.dest = 0x0;
							fxsend.a.level = maxlevel / 2;
							fxsend.b.dest = 0x1;
							fxsend.b.level = maxlevel / 2;
					} else {
						LOG(("emuxki_voice_set_stereo case 2 badly managed\n"));
					}
					break;
				case 4:
					if (voice->stream->nmono == 1) {
						fxsend.a.dest = voice->voicenum;
						fxsend.a.level = maxlevel;
						fxsend.b.dest = voice->voicenum + 1;
						fxsend.b.level = maxlevel;
						fxsend.c.dest = voice->voicenum + 2;
						fxsend.c.level = maxlevel;
						fxsend.d.dest = voice->voicenum + 3;
						fxsend.d.level = maxlevel;
					} else if (voice->stream->nmono == 2) {
						fxsend.a.dest = voice->voicenum;
						fxsend.a.level = maxlevel;
						fxsend.b.dest = voice->voicenum + 2;
						fxsend.b.level = maxlevel;
					} else if ((voice->stream->nmono == 4) ||
						((voice->stream->nmono == 6)&&(voice->voicenum < 4))) {
						fxsend.a.dest = voice->voicenum;
						fxsend.a.level = maxlevel;
					} else if (voice->stream->nmono == 6 && voice->voicenum > 3) {
							fxsend.a.dest = 0x0;
							fxsend.a.level = maxlevel / 2;
							fxsend.b.dest = 0x1;
							fxsend.b.level = maxlevel / 2;
					} else {
						LOG(("emuxki_voice_set_stereo case 4 badly managed\n"));
					}
					break;
				case 6: // only on audigy
					if (voice->stream->nmono == 1) {
						fxsend.a.dest = voice->voicenum;
						fxsend.a.level = maxlevel;
						fxsend.b.dest = voice->voicenum + 1;
						fxsend.b.level = maxlevel;
						fxsend.c.dest = voice->voicenum + 2;
						fxsend.c.level = maxlevel;
						fxsend.d.dest = voice->voicenum + 3;
						fxsend.d.level = maxlevel;
						fxsend.e.dest = voice->voicenum + 4;
						fxsend.e.level = maxlevel;
						fxsend.f.dest = voice->voicenum + 5;
						fxsend.f.level = maxlevel;
					} else if (voice->stream->nmono == 2) {
						fxsend.a.dest = voice->voicenum;
						fxsend.a.level = maxlevel;
						fxsend.b.dest = voice->voicenum + 2;
						fxsend.b.level = maxlevel;
						fxsend.c.dest = 0x4;
						fxsend.c.level = maxlevel / 2;
						fxsend.d.dest = 0x5;
						fxsend.d.level = maxlevel / 2;
					} else if (voice->stream->nmono == 4) {
						fxsend.a.dest = voice->voicenum;
						fxsend.a.level = maxlevel;
						if (voice->voicenum < 2) {
							fxsend.b.dest = 0x4;
							fxsend.b.level = maxlevel / 2;
							fxsend.c.dest = 0x5;
							fxsend.c.level = maxlevel / 2;
						}
					} else if (voice->stream->nmono == 6) {
						fxsend.a.dest = voice->voicenum;
						fxsend.a.level = maxlevel;
					} else {
						LOG(("emuxki_voice_set_stereo case 6 badly managed\n"));
					}
					break;
			}

			emuxki_channel_set_fxsend(voice->dataloc.chan[0],
						   &fxsend);
		}
	}
}


static status_t
emuxki_voice_set_stereo(emuxki_voice *voice, uint8 stereo)
{
	status_t	error;

	emuxki_voice_dataloc_destroy(voice);
	voice->stereo = stereo;
	if ((error = emuxki_voice_dataloc_create(voice)))
	  return error;
	emuxki_voice_fxupdate(voice);
	return B_OK;
}


static status_t
emuxki_voice_set_srate(emuxki_voice *voice, uint32 srate)
{
	voice->sample_rate = srate;
	if (voice->use & EMU_USE_PLAY) {
		if ((srate < 4000) || (srate > 48000))
			return (EINVAL);
		emuxki_channel_set_srate(voice->dataloc.chan[0], srate);
		if (voice->stereo)
			emuxki_channel_set_srate(voice->dataloc.chan[1],
						  srate);
	}
	return B_OK;
}


status_t
emuxki_voice_set_audioparms(emuxki_voice *voice, uint8 stereo,
			     uint8 b16, uint32 srate)
{
	status_t             error;

	if (voice->stereo == stereo && voice->b16 == b16 &&
	    voice->sample_rate == srate)
		return B_OK;

	if (voice->stereo != stereo) {
		if ((error = emuxki_voice_set_stereo(voice, stereo)))
			return error;
	}
	voice->b16 = b16;
	if (voice->sample_rate != srate)
		emuxki_voice_set_srate(voice, srate);
	return B_OK;
}


status_t
emuxki_voice_set_recparms(emuxki_voice *voice, emuxki_recsrc_t recsrc,
			     	emuxki_recparams *recparams)
{
	if (voice->use & EMU_USE_RECORD) {
		switch(recsrc) {
			case EMU_RECSRC_MIC:
				break;
			case EMU_RECSRC_ADC:
				break;
			case EMU_RECSRC_FX:
				if (!recparams)
					return B_ERROR;
				voice->recparams.efx_voices[0] = recparams->efx_voices[0];
				voice->recparams.efx_voices[1] = recparams->efx_voices[1];
				break;
			default:
				return B_ERROR;
				break;
		}
		voice->dataloc.source = recsrc;
	}
	return B_OK;
}


/* voice audio parms (see just before) must be set prior to this */
status_t
emuxki_voice_set_bufparms(emuxki_voice *voice, void *ptr,
			   uint32 bufsize, uint16 blksize)
{
	emuxki_mem *mem;
	emuxki_channel **chan;
	uint32 start, end;
	uint8 sample_size;
	status_t error = EFAULT;

	LIST_FOREACH(mem, &voice->stream->card->mem, next) {
		if (mem->log_base != ptr)
			continue;

		voice->buffer = mem;
		sample_size = (voice->b16 + 1) * (voice->stereo + 1);
		voice->trigblk = 0;	/* This shouldn't be needed */
		voice->blkmod = bufsize / blksize;
		if (bufsize % blksize) 	  /* This should not happen */
			voice->blkmod++;
		error = 0;

		if (voice->use & EMU_USE_PLAY) {
			voice->blksize = blksize / sample_size;
			chan = voice->dataloc.chan;
			start = (mem->ptbidx << 12) / sample_size;
			end = start + bufsize / sample_size;
			emuxki_channel_set_bufparms(chan[0],
						     start, end);
			if (voice->stereo)
				emuxki_channel_set_bufparms(chan[1],
				     start, end);
			voice->timerate = (uint32) 48000 *
			                voice->blksize / voice->sample_rate / 2;
			if (voice->timerate < 5)
				error = EINVAL;
		} else {
			voice->blksize = blksize;
		}

		break;
	}

	return error;
}


status_t
emuxki_voice_commit_parms(emuxki_voice *voice)
{
	if (voice->use & EMU_USE_PLAY) {
		emuxki_channel_commit_parms(voice->dataloc.chan[0]);
		if (voice->stereo)
			emuxki_channel_commit_parms(voice->dataloc.chan[1]);
	} else {
		uint32 buffaddr_reg, buffsize_reg, idx_reg;
		switch (voice->dataloc.source) {
			case EMU_RECSRC_MIC:
				buffaddr_reg = EMU_MICBA;
				buffsize_reg = EMU_MICBS;
				idx_reg = EMU_MICIDX;
				break;
			case EMU_RECSRC_ADC:
				buffaddr_reg = EMU_ADCBA;
				buffsize_reg = EMU_ADCBS;
				idx_reg = EMU_ADCIDX;
				break;
			case EMU_RECSRC_FX:
				buffaddr_reg = EMU_FXBA;
				buffsize_reg = EMU_FXBS;
				idx_reg = EMU_FXIDX;
				break;
			default:
				return B_ERROR;
		}
		emuxki_chan_write(&voice->stream->card->config, 0, buffaddr_reg, (uint32)voice->buffer->phy_base);
		emuxki_chan_write(&voice->stream->card->config, 0, buffsize_reg, EMU_RECBS_BUFSIZE_NONE);
		emuxki_chan_write(&voice->stream->card->config, 0, buffsize_reg, EMU_RECBS_BUFSIZE_4096);

		LOG(("emuxki_voice_commit_parms idx_reg : %u\n", idx_reg));

		idx_reg = EMU_RECIDX(idx_reg);
		while (emuxki_chan_read(&voice->stream->card->config, 0, idx_reg))
			snooze(5);
	}
	return B_OK;
}


static uint32
emuxki_voice_curaddr(emuxki_voice *voice)
{
	if (voice->use & EMU_USE_PLAY)
		return (emuxki_chan_read(&voice->stream->card->config,
				     voice->dataloc.chan[0]->num,
				     EMU_CHAN_CCCA_CURRADDR) -
			voice->dataloc.chan[0]->loop.start);
	else {
		uint32 idx_reg = 0;
		switch (voice->dataloc.source) {
			case EMU_RECSRC_MIC:
				idx_reg = IS_AUDIGY(&voice->stream->card->config) ? EMU_A_MICIDX : EMU_MICIDX;
				break;
			case EMU_RECSRC_ADC:
				idx_reg = IS_AUDIGY(&voice->stream->card->config) ? EMU_A_ADCIDX : EMU_ADCIDX;
				break;
			case EMU_RECSRC_FX:
				idx_reg = EMU_FXIDX;
				break;
			default:
				TRACE(("emuxki_voice_curaddr : BUG!!\n"));
		}
		//TRACE(("emuxki_voice_curaddr idx_reg : %u\n", idx_reg));
		//TRACE(("emuxki_voice_curaddr : %lu\n", emuxki_chan_read(&voice->stream->card->config, 0, idx_reg)));
		return emuxki_chan_read(&voice->stream->card->config,
				     0,
				     idx_reg);
	}
}


static void
emuxki_resched_timer(emuxki_dev *card)
{
	emuxki_voice 	*voice;
	emuxki_stream 	*stream;
	uint16			timerate = 1024;
	uint8			active = 0;
	//int             s = splaudio();

	LOG(("emuxki_resched_timer begin\n"));

	LIST_FOREACH(stream, &card->streams, next) {
		LIST_FOREACH(voice, &stream->voices, next) {
			if ((voice->use & EMU_USE_PLAY) == 0 ||
				(voice->state & EMU_STATE_STARTED) == 0)
				continue;
			active = 1;
			if (voice->timerate < timerate)
				timerate = voice->timerate;
		}
	}
	if (timerate & ~EMU_TIMER_RATE_MASK)
		timerate = 0;

	if (card->timerate > timerate) {
		LOG(("emuxki_resched_timer written (old %u, new %u)\n", card->timerate, timerate));
		card->timerate = timerate;
		emuxki_reg_write_16(&card->config, EMU_TIMER, timerate);
	}
	if (!active && (card->timerstate & EMU_TIMER_STATE_ENABLED)) {
		emuxki_inte_disable(&card->config, EMU_INTE_INTERTIMERENB);
		card->timerstate &= ~EMU_TIMER_STATE_ENABLED;
		LOG(("emuxki_resched_timer : timer disabled\n"));
	} else
	  if (active && !(card->timerstate & EMU_TIMER_STATE_ENABLED)) {
	  	emuxki_inte_enable(&card->config, EMU_INTE_INTERTIMERENB);
		card->timerstate |= EMU_TIMER_STATE_ENABLED;
		LOG(("emuxki_resched_timer : timer enabled\n"));
	  }
	LOG(("emuxki_resched_timer state : %x\n", card->timerstate));
	//splx(s);
}


static uint32
emuxki_voice_adc_rate(emuxki_voice *voice)
{
	switch(voice->sample_rate) {
		case 48000:
			return EMU_ADCCR_SAMPLERATE_48;
			break;
		case 44100:
			return EMU_ADCCR_SAMPLERATE_44;
			break;
		case 32000:
			return EMU_ADCCR_SAMPLERATE_32;
			break;
		case 24000:
			return EMU_ADCCR_SAMPLERATE_24;
			break;
		case 22050:
			return EMU_ADCCR_SAMPLERATE_22;
			break;
		case 16000:
			return EMU_ADCCR_SAMPLERATE_16;
			break;
		case 12000:
			if (IS_AUDIGY(&voice->stream->card->config))
				return EMU_A_ADCCR_SAMPLERATE_12;
			else
				PRINT(("recording sample_rate not supported : %" B_PRIu32 "\n",
					voice->sample_rate));
			break;
		case 11000:
			if (IS_AUDIGY(&voice->stream->card->config))
				return EMU_A_ADCCR_SAMPLERATE_11;
			else
				return EMU_ADCCR_SAMPLERATE_11;
			break;
		case 8000:
			if (IS_AUDIGY(&voice->stream->card->config))
				return EMU_A_ADCCR_SAMPLERATE_8;
			else
				return EMU_ADCCR_SAMPLERATE_8;
			break;
		default:
			PRINT(("recording sample_rate not supported : %" B_PRIu32 "\n",
				voice->sample_rate));
	}
	return 0;
}


void
emuxki_voice_start(emuxki_voice *voice)
{
	LOG(("emuxki_voice_start\n"));

	if (voice->use & EMU_USE_PLAY) {
		voice->trigblk = 1;
		emuxki_channel_start(voice->dataloc.chan[0]);
		if (voice->stereo)
			emuxki_channel_start(voice->dataloc.chan[1]);
	} else {

		switch (voice->dataloc.source) {
			case EMU_RECSRC_MIC:
				emuxki_inte_enable(&voice->stream->card->config, EMU_INTE_MICBUFENABLE);
				break;
			case EMU_RECSRC_ADC: {
				uint32 adccr_value = 0;
				adccr_value = emuxki_voice_adc_rate(voice);
				LOG(("emuxki_voice_start adccr_value : %u\n", adccr_value));
				if (voice->stereo)
					adccr_value |= ( (IS_AUDIGY(&voice->stream->card->config) ? EMU_A_ADCCR_LCHANENABLE : EMU_ADCCR_LCHANENABLE )
						| ( IS_AUDIGY(&voice->stream->card->config) ? EMU_A_ADCCR_RCHANENABLE : EMU_ADCCR_RCHANENABLE ));
				else
					adccr_value |= IS_AUDIGY(&voice->stream->card->config) ? EMU_A_ADCCR_LCHANENABLE : EMU_ADCCR_LCHANENABLE;

				LOG(("emuxki_voice_start adccr_value : %u, %u\n", adccr_value, EMU_ADCCR_LCHANENABLE | EMU_ADCCR_RCHANENABLE));
				emuxki_chan_write(&voice->stream->card->config, 0, EMU_ADCCR, adccr_value);

				emuxki_inte_enable(&voice->stream->card->config, EMU_INTE_ADCBUFENABLE);
				}
				break;
			case EMU_RECSRC_FX:
				if (IS_AUDIGY(&voice->stream->card->config)) {
					emuxki_chan_write(&voice->stream->card->config, 0, EMU_A_FXWC1,
						voice->recparams.efx_voices[0]);
					emuxki_chan_write(&voice->stream->card->config, 0, EMU_A_FXWC2,
						voice->recparams.efx_voices[1]);
				} else {
					emuxki_chan_write(&voice->stream->card->config, 0, EMU_FXWC,
						voice->recparams.efx_voices[0]);
				}
				emuxki_inte_enable(&voice->stream->card->config, EMU_INTE_EFXBUFENABLE);
				break;
			default:
				PRINT(("emuxki_voice_start BUG\n"));
		}
	}
	voice->state |= EMU_STATE_STARTED;
	if (voice->use & EMU_USE_PLAY) {
		emuxki_resched_timer(voice->stream->card);
	}
}


void
emuxki_voice_halt(emuxki_voice *voice)
{
	LOG(("emuxki_voice_halt\n"));

	if (voice->use & EMU_USE_PLAY) {
		emuxki_channel_stop(voice->dataloc.chan[0]);
		if (voice->stereo)
			emuxki_channel_stop(voice->dataloc.chan[1]);
	} else {

		switch (voice->dataloc.source) {
			case EMU_RECSRC_MIC:
				emuxki_inte_disable(&voice->stream->card->config, EMU_INTE_MICBUFENABLE);
				break;
			case EMU_RECSRC_ADC:
				emuxki_chan_write(&voice->stream->card->config, 0, EMU_ADCCR, 0);
				emuxki_inte_disable(&voice->stream->card->config, EMU_INTE_ADCBUFENABLE);
				break;
			case EMU_RECSRC_FX:
				if (IS_AUDIGY(&voice->stream->card->config)) {
					emuxki_chan_write(&voice->stream->card->config, 0, EMU_A_FXWC1, 0);
					emuxki_chan_write(&voice->stream->card->config, 0, EMU_A_FXWC2, 0);
				} else
					emuxki_chan_write(&voice->stream->card->config, 0, EMU_FXWC, 0);
				emuxki_inte_disable(&voice->stream->card->config, EMU_INTE_EFXBUFENABLE);
				break;
			default:
				PRINT(("emuxki_voice_halt BUG\n"));
		}
	}
	voice->state &= ~EMU_STATE_STARTED;
	if (voice->use & EMU_USE_PLAY) {
		emuxki_resched_timer(voice->stream->card);
	}
}


emuxki_voice *
emuxki_voice_new(emuxki_stream *stream, uint8 use, uint8 voicenum)
{
	emuxki_voice *voice;
	//int             s;

	LOG(("emuxki_voice_new\n"));

	voice = malloc(sizeof(emuxki_voice));
	if (voice == NULL)
		return (NULL);
	voice->stream = stream;
	voice->use = use;
	voice->state = !EMU_STATE_STARTED;
	voice->stereo = EMU_STEREO_NOTSET;
	voice->b16 = 0;
	voice->sample_rate = 0;
	if (use & EMU_USE_PLAY)
		voice->dataloc.chan[0] = voice->dataloc.chan[1] = NULL;
	else
		voice->dataloc.source = EMU_RECSRC_NOTSET;
	voice->buffer = NULL;
	voice->blksize = 0;
	voice->trigblk = 0;
	voice->blkmod = 0;
	voice->timerate = 0;
	voice->voicenum = voicenum;
	return voice;
}


void
emuxki_voice_delete(emuxki_voice *voice)
{
	if (voice->state & EMU_STATE_STARTED)
		emuxki_voice_halt(voice);
	emuxki_voice_dataloc_destroy(voice);
	free(voice);
}

/*	Emuxki stream functions */

status_t
emuxki_stream_set_audioparms(emuxki_stream *stream, bool stereo, uint8 channels,
     uint8 b16, uint32 sample_rate)
{
	status_t 		error;
	emuxki_voice 	*voice;
	uint8			i, nvoices;
	char 			*buffer;
	uint8 			sample_size, frame_size;
	LOG(("emuxki_stream_set_audioparms\n"));

	if (stream->stereo == stereo &&
		((stream->nmono + 2*stream->nstereo) == channels) &&
		(stream->b16 == b16) &&
		(stream->sample_rate == sample_rate))
		return B_OK;

	LIST_FOREACH(voice, &stream->voices, next) {
		if (voice->buffer)
			emuxki_mem_free(stream->card, voice->buffer->log_base);
		emuxki_voice_delete(voice);
	}
	stream->first_voice = NULL;
	LIST_INIT(&(stream->voices));

	stream->b16 = b16;
	stream->sample_rate = sample_rate;

	if (stereo && (channels % 2 == 0)) {
		stream->stereo = true;
		stream->nstereo = channels / 2;
		stream->nmono = 0;
		nvoices = stream->nstereo;
	} else {
		stream->stereo = false;
		stream->nstereo = 0;
		stream->nmono = channels;
		nvoices = stream->nmono;
	}

	sample_size = stream->b16 + 1;
	frame_size = sample_size * (stream->stereo ? 2 : 1);

	for (i = 0; i < nvoices; i++) {
		voice = emuxki_voice_new(stream, stream->use, i);
		if (voice) {
			if (!stream->first_voice)
				stream->first_voice = voice;
			LIST_INSERT_HEAD((&stream->voices), voice, next);
			if ((error = emuxki_voice_set_audioparms(voice, stream->stereo,
				stream->b16, stream->sample_rate)))
				return error;

			if (stream->use & EMU_USE_PLAY)
				buffer = emuxki_pmem_alloc(stream->card, stream->bufframes
					* frame_size * stream->bufcount);
			else
				buffer = emuxki_rmem_alloc(stream->card, stream->bufframes
					* frame_size * stream->bufcount);

			emuxki_voice_set_bufparms(voice, buffer,
				stream->bufframes * frame_size * stream->bufcount,
				stream->bufframes * frame_size);
		}
	}

	return B_OK;
}


status_t
emuxki_stream_set_recparms(emuxki_stream *stream, emuxki_recsrc_t recsrc,
			     	emuxki_recparams *recparams)
{
	emuxki_voice *voice;
	LOG(("emuxki_stream_set_recparms\n"));

	if (stream->use & EMU_USE_RECORD) {
		switch(recsrc) {
			case EMU_RECSRC_MIC:
				break;
			case EMU_RECSRC_ADC:
				break;
			case EMU_RECSRC_FX:
				if (!recparams)
					return B_ERROR;
				LIST_FOREACH(voice, &stream->voices, next) {
					voice->recparams.efx_voices[0] = recparams->efx_voices[0];
					voice->recparams.efx_voices[1] = recparams->efx_voices[1];
				}
				break;
			default:
				return B_ERROR;
				break;
		}
		LIST_FOREACH(voice, &stream->voices, next)
			voice->dataloc.source = recsrc;
	}
	return B_OK;
}


status_t
emuxki_stream_commit_parms(emuxki_stream *stream)
{
	emuxki_voice 	*voice;
	status_t 		error;
	LOG(("emuxki_stream_commit_parms\n"));

	LIST_FOREACH(voice, &stream->voices, next)
		if ((error = emuxki_voice_commit_parms(voice)))
			return error;

	return B_OK;
}


status_t
emuxki_stream_get_nth_buffer(emuxki_stream *stream, uint8 chan, uint8 buf,
					char** buffer, size_t *stride)
{
	emuxki_voice *voice = NULL;
	uint8 i, sample_size;
	LOG(("emuxki_stream_get_nth_buffer\n"));

	sample_size = stream->b16 + 1;
	if (buf >= stream->bufcount)
		return B_BAD_INDEX;

	if (stream->stereo) {
		i = stream->nstereo - 1;
		if (chan/2 > i)
			return B_BAD_INDEX;
		LIST_FOREACH(voice, &stream->voices, next)
			if (i != chan/2)
				i--;
			else
				break;
		if (voice) {
			*buffer = (char*)voice->buffer->log_base
				+ (buf * stream->bufframes * sample_size * 2);
			if (chan % 2 == 1)
				*buffer += sample_size;
			*stride = sample_size * 2;
		} else
			return B_ERROR;
	} else {
		i = stream->nmono - 1;
		if (chan > i)
			return B_BAD_INDEX;
		LIST_FOREACH(voice, &stream->voices, next)
			if (i != chan)
				i--;
			else
				break;
		if (voice) {
			*buffer = (char*)voice->buffer->log_base
				+ (buf * stream->bufframes * sample_size);
			*stride = sample_size;
		} else
			return B_ERROR;
	}

	return B_OK;
}


void
emuxki_stream_start(emuxki_stream *stream, void (*inth) (void *), void *inthparam)
{
	emuxki_voice *voice;
	LOG(("emuxki_stream_start\n"));

	stream->inth = inth;
	stream->inthparam = inthparam;

	LIST_FOREACH(voice, &stream->voices, next) {
		emuxki_voice_start(voice);
	}
	stream->state |= EMU_STATE_STARTED;
}


void
emuxki_stream_halt(emuxki_stream *stream)
{
	emuxki_voice *voice;
	LOG(("emuxki_stream_halt\n"));

	LIST_FOREACH(voice, &stream->voices, next) {
		emuxki_voice_halt(voice);
	}
	stream->state &= ~EMU_STATE_STARTED;
}


emuxki_stream *
emuxki_stream_new(emuxki_dev *card, uint8 use, uint32 bufframes, uint8 bufcount)
{
	emuxki_stream *stream;
	cpu_status status;
	LOG(("emuxki_stream_new\n"));

	stream = malloc(sizeof(emuxki_stream));
	if (stream == NULL)
		return (NULL);
	stream->card = card;
	stream->use = use;
	stream->state = !EMU_STATE_STARTED;
	stream->stereo = EMU_STEREO_NOTSET;
	stream->b16 = 0;
	stream->sample_rate = 0;
	stream->nmono = 0;
	stream->nstereo = 0;
	stream->bufframes = bufframes;
	stream->bufcount = bufcount;
	stream->first_voice = NULL;
	stream->inth = NULL;
	stream->inthparam = NULL;

	stream->frames_count = 0;
	stream->real_time = 0;
	stream->buffer_cycle = 0;
	stream->update_needed = false;

	/* Init voices list */
	LIST_INIT(&(stream->voices));

	status = lock();
	LIST_INSERT_HEAD((&card->streams), stream, next);
	unlock(status);

	return stream;
}


void
emuxki_stream_delete(emuxki_stream *stream)
{
	emuxki_voice *voice;
	cpu_status status;
	LOG(("emuxki_stream_delete\n"));

	emuxki_stream_halt(stream);

	status = lock();
	LIST_REMOVE(stream, next);
	unlock(status);

	while (!LIST_EMPTY(&stream->voices)) {
		voice = LIST_FIRST(&stream->voices);
		LIST_REMOVE(voice, next);
		if (voice->buffer)
			emuxki_mem_free(stream->card, voice->buffer->log_base);
		emuxki_voice_delete(voice);
	}

	free(stream);
}


/* Emuxki gprs */
// 87 values from 0.0dB to -xdB (-0.75dB each)
static uint32 db_table[] = {
	2147483647,	1969835071,	1806882308,	1657409659,	1520301995,	1394536435,	1279174712,
	1173356181,	1076291388,	987256190,	905586345,	830672562,	761955951,	698923858,
	641106035,	588071138,	539423503,	494800198,	453868315,	416322483,	381882595,
	350291714,	321314160,	294733747,	270352173,	247987542,	227473005,	208655513,
	191394681,	175561735,	161038555,	147716791,	135497057,	124288190,	114006566,
	104575479,	95924570,	87989300,	80710468,	74033770,	67909395,	62291654,
	57138635,	52411895,	48076170,	44099114,	40451056,	37104780,	34035322,
	31219781,	28637154,	26268172,	24095162,	22101913,	20273552,	18596442,
	17058068,	15646956,	14352576,	13165272,	12076188,	11077196,	10160845,
	9320299,	8549286,	7842054,	7193328,	6598266,	6052431,	5551749,
	5092486,	4671215,	4284793,	3930337,	3605204,	3306967,	3033401,
	2782465,	2552289,	2341153,	2147483,	1969835,	1806882,	1657409,
	1520301,	1394536,	1279174
};


void
emuxki_gpr_set(emuxki_dev *card, emuxki_gpr *gpr, int32 type, float *values)
{
	uint8 count = gpr->type & EMU_MIX_STEREO ? 2 : 1;
	uint8 i;
	uint32 index;

	LOG(("emuxki_set_gpr\n"));

	switch(type) {
		case EMU_MIX_MUTE:
			gpr->mute = (values[0] == 1.0);
			if (gpr->mute) {
				for (i = 0; i < count; i++)
					emuxki_write_gpr(&card->config, gpr->gpr + i, 0);
				break;
			}
			for (i = 0; i < count; i++) {
				values[i] = gpr->current[i];
			}
		case EMU_MIX_GAIN:
			for (i = 0; i < count; i++) {
				if (values[i]>gpr->max_gain || values[i]<gpr->min_gain)
					return;
				index = values[i] / gpr->granularity;
				if (index > sizeof(db_table)/sizeof(db_table[0]))
					index = sizeof(db_table)/sizeof(db_table[0]);
				LOG(("emuxki_set_gpr gpr: %d \n", gpr->gpr + i));
				LOG(("emuxki_set_gpr values[i]: %g \n", values[i]));
				LOG(("emuxki_set_gpr index: %u \n", index));
				if (!gpr->mute)
					emuxki_write_gpr(&card->config, gpr->gpr + i, db_table[index]);
				gpr->current[i] = index * gpr->granularity;
			}
			break;
	}
}


void
emuxki_gpr_get(emuxki_dev *card, emuxki_gpr *gpr, int32 type, float *values)
{
	uint8 count = gpr->type & EMU_MIX_STEREO ? 2 : 1;
	uint16 i;

	LOG(("emuxki_get_gpr\n"));

	switch(type) {
		case EMU_MIX_GAIN:
			for (i = 0; i < count; i++) {
				values[i] = gpr->current[i];
			}
			break;
		case EMU_MIX_MUTE:
			values[0] = (gpr->mute ? 1.0 : 0.0);
			break;
	}
}


void
emuxki_gpr_dump(emuxki_dev * card, uint16 count)
{
	uint16      pc;
	uint32		value;

	LOG(("emuxki_dump_gprs\n"));

	for (pc = 0; pc < count; pc++) {
		value = emuxki_read_gpr(&card->config, pc);
		LOG(("dsp_gpr pc=%x, value=%x\n", pc, value));
	}
}


static emuxki_gpr *
emuxki_gpr_new(emuxki_dev *card, const char *name, emuxki_gpr_type type, uint16 *gpr_num,
			float default_value, float default_mute, float min_gain, float max_gain, float granularity)
{
	emuxki_gpr *gpr;
	float	values[2];

	LOG(("emuxki_gpr_new\n"));

	gpr = &card->gpr[*gpr_num];
	strncpy(gpr->name, name, 32);
	gpr->type = type;
	gpr->gpr = *gpr_num;
	gpr->default_value = default_value;
	gpr->min_gain = min_gain;
	gpr->max_gain = max_gain;
	gpr->granularity = granularity;
	gpr->mute = false;
	(*gpr_num)++;
	if (gpr->type & EMU_MIX_STEREO)
		(*gpr_num)++;

	if (default_mute == 1.0) {
		values[0] = default_mute;
		emuxki_gpr_set(card, gpr, EMU_MIX_MUTE, values);
	}

	values[0] = gpr->default_value;
	if (gpr->type & EMU_MIX_STEREO)
		values[1] = gpr->default_value;
	emuxki_gpr_set(card, gpr, EMU_MIX_GAIN, values);


	return gpr;
}

/* Emuxki parameter */

void
emuxki_parameter_set(emuxki_dev *card, const void* cookie, int32 type, int32 *value)
{
	emuxki_stream *stream;
	emuxki_voice *voice;
	LOG(("emuxki_parameter_set\n"));

	switch(type) {
		case EMU_DIGITAL_MODE:
			card->digital_enabled = *value == 1;
			if (IS_AUDIGY(&card->config))
				if (IS_AUDIGY2(&card->config)) {
					// this disables analog, not enough
					emuxki_reg_write_32(&card->config, EMU_A_IOCFG,
						(card->digital_enabled ? 0 : EMU_A_IOCFG_GPOUT0) |
							(emuxki_reg_read_32(&card->config, EMU_A_IOCFG)
							& ~(EMU_A_IOCFG_GPOUT0 | EMU_A_IOCFG_GPOUT1) ) );
				} else {
					// this disables analog, and enables digital
					emuxki_reg_write_32(&card->config, EMU_A_IOCFG,
						(card->digital_enabled ? EMU_A_IOCFG_GPOUT0 | EMU_A_IOCFG_GPOUT1 : 0) |
							(emuxki_reg_read_32(&card->config, EMU_A_IOCFG)
							& ~(EMU_A_IOCFG_GPOUT0 | EMU_A_IOCFG_GPOUT1) ) );
				}
			else {
				// this enables digital, not enough
				emuxki_reg_write_32(&card->config, EMU_HCFG,
					(card->digital_enabled ? EMU_HCFG_GPOUTPUT0 : 0) |
					(emuxki_reg_read_32(&card->config, EMU_HCFG) & ~EMU_HCFG_GPOUTPUT0));
			}

			break;
		case EMU_AUDIO_MODE:
			if (*value!=0 && *value!=1 && *value!=2) {
				PRINT(("emuxki_parameter_set error value unexpected\n"));
				return;
			}
			card->play_mode = (*value + 1) * 2;
			LIST_FOREACH(stream, &card->streams, next) {
				if ((stream->use & EMU_USE_PLAY) == 0 ||
					(stream->state & EMU_STATE_STARTED) == 0)
						continue;
				LIST_FOREACH(voice, &stream->voices, next) {
					emuxki_voice_fxupdate(voice);
					emuxki_channel_commit_fx(voice->dataloc.chan[0]);
					if (voice->stereo)
						emuxki_channel_commit_fx(voice->dataloc.chan[1]);
				}
			}
			break;
	}
}


void
emuxki_parameter_get(emuxki_dev *card, const void* cookie, int32 type, int32 *value)
{
	LOG(("emuxki_parameter_get\n"));

	switch(type) {
		case EMU_DIGITAL_MODE:
			*value = card->digital_enabled ? 1 : 0;
			break;
		case EMU_AUDIO_MODE:
			*value = card->play_mode / 2 - 1;
			break;
	}
}


/* Emuxki interrupt */
static int32
emuxki_int(void *arg)
{
	emuxki_dev	 *card = arg;
	uint32       ipr, curblk;
	bool gotone = false;
	emuxki_voice *voice;
	emuxki_stream *stream;

	while ((ipr = emuxki_reg_read_32(&card->config, EMU_IPR))) {
		gotone = true;
		if (ipr & EMU_IPR_INTERVALTIMER) {
			//TRACE(("EMU_IPR_INTERVALTIMER\n"));
			LIST_FOREACH(stream, &card->streams, next) {
				if ((stream->use & EMU_USE_PLAY) == 0 ||
					(stream->state & EMU_STATE_STARTED) == 0 ||
					(stream->inth == NULL))
						continue;

				voice = stream->first_voice;
				//TRACE(("voice %p\n", voice));
				curblk = emuxki_voice_curaddr(voice) /
				       voice->blksize;
				//TRACE(("EMU_IPR_INTERVALTIMER at trigblk %lu\n", curblk));
				//TRACE(("EMU_IPR_INTERVALTIMER at voice->trigblk %lu\n", voice->trigblk));
				if (curblk == voice->trigblk) {
					//TRACE(("EMU_IPR_INTERVALTIMER at trigblk %lu\n", curblk));
					//dump_voice(voice);
					//trace_hardware_regs(&card->config);
					//TRACE(("voice pointer %p\n", voice));

					if (stream->inth)
						stream->inth(stream->inthparam);

					voice->trigblk++;
					voice->trigblk %= voice->blkmod;
				}
			}
		}
#if MIDI
		if (ipr & (EMU_IPR_MIDIRECVBUFE)) {
			midi_interrupt(card);          /* Gameport */
		}

		if (ipr & (EMU_IPR_MIDITRANSBUFE)) {
			if (!midi_interrupt(card)) {
				emuxki_inte_disable(&card->config, EMU_INTE_MIDITXENABLE);
				TRACE(("EMU_INTE_MIDITXENABLE disabled\n"));
			}
		}
#endif
		if (ipr & (EMU_IPR_ADCBUFHALFFULL | EMU_IPR_ADCBUFFULL
			| EMU_IPR_MICBUFHALFFULL | EMU_IPR_MICBUFFULL
			| EMU_IPR_EFXBUFHALFFULL | EMU_IPR_EFXBUFFULL)) {
			//TRACE(("EMU_IPR_ADCBUF\n"));
			LIST_FOREACH(stream, &card->streams, next) {
				if ((stream->use & EMU_USE_RECORD) == 0 ||
					(stream->state & EMU_STATE_STARTED) == 0 ||
					(stream->inth == NULL) ||
					(stream->first_voice == NULL))
						continue;
				voice = stream->first_voice;
				curblk = emuxki_voice_curaddr(voice) /
				       voice->blksize;
				//TRACE(("EMU_IPR_ADCBUF at trigblk %lu\n", curblk));
				//TRACE(("EMU_IPR_ADCBUF at voice->trigblk %lu\n", voice->trigblk));
				if (curblk == voice->trigblk) {
					//TRACE(("EMU_IPR_ADCBUF at trigblk %lu\n", curblk));
					//dump_voice(voice);
					//trace_hardware_regs(&card->config);

					if (stream->inth)
						stream->inth(stream->inthparam);

					voice->trigblk++;
					voice->trigblk %= voice->blkmod;
				}
			}
		}

		/*if (ipr & (EMU_IPR_CHANNELLOOP)) {
			TRACE(("EMU_IPR_CHANNELLOOP pending channel : %u\n", ipr & EMU_IPR_CHNOMASK));
			LIST_FOREACH(stream, &card->streams, next)
			LIST_FOREACH(voice, &stream->voices, next) {
				if ((voice->use & EMU_USE_PLAY) == 0 ||
					(voice->state & EMU_STATE_STARTED) == 0)
						continue;
				TRACE(("EMU_IPR_CHANNELLOOP at trigblk %lu\n", emuxki_voice_curaddr(voice)));
				TRACE(("EMU_IPR_CHANNELLOOP read %x\n", emuxki_chan_read(&voice->card->config, 0, EMU_CLIPL)));
				emuxki_chan_write(&voice->card->config, 0, EMU_CLIPL, emuxki_chan_read(&voice->card->config, 0, EMU_CLIPL));
			}
		}*/

		if (ipr & ~(EMU_IPR_RATETRCHANGE | EMU_IPR_INTERVALTIMER
				| EMU_IPR_MIDITRANSBUFE | EMU_IPR_MIDIRECVBUFE
				| EMU_IPR_ADCBUFHALFFULL | EMU_IPR_ADCBUFFULL
				| EMU_IPR_MICBUFHALFFULL | EMU_IPR_MICBUFFULL
				| EMU_IPR_EFXBUFHALFFULL | EMU_IPR_EFXBUFFULL))
			TRACE(("Got interrupt 0x%08x !!!\n",
			       ipr & ~(EMU_IPR_RATETRCHANGE |
				       EMU_IPR_INTERVALTIMER)));

		emuxki_reg_write_32(&card->config, EMU_IPR, ipr);
	}

	if (IS_AUDIGY2(&card->config)) {
		while ((ipr = emuxki_reg_read_32(&card->config, EMU_A2_IPR2))) {
			emuxki_reg_write_32(&card->config, EMU_A2_IPR2, ipr);
			break;	// avoid loop
		}

		if (!IS_AUDIGY2_VALUE(&card->config)) {
			while ((ipr = emuxki_reg_read_32(&card->config, EMU_A2_IPR3))) {
				emuxki_reg_write_32(&card->config, EMU_A2_IPR3, ipr);
				break; // avoid loop
			}
		}
	}

	if (gotone)
		return B_INVOKE_SCHEDULER;

	TRACE(("Got unhandled interrupt\n"));
	return B_UNHANDLED_INTERRUPT;
}



/*	Emu10k1 driver functions */


/* detect presence of our hardware */
status_t
init_hardware(void)
{
	int ix = 0;
	pci_info info;
//	uint32 buffer;
	status_t err = ENODEV;

	LOG_CREATE();

	PRINT(("init_hardware()\n"));

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci))
		return ENOSYS;

	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if (info.vendor_id == CREATIVELABS_VENDOR_ID &&
			(info.device_id == CREATIVELABS_SBLIVE_DEVICE_ID
#if AUDIGY
			|| info.device_id == CREATIVELABS_AUDIGY_DEVICE_ID
			|| info.device_id == CREATIVELABS_AUDIGY2_VALUE_DEVICE_ID
#endif
			)) {
			err = B_OK;

/*
			Joystick suport
			if (!(info.u.h0.subsystem_id == 0x20 ||
					info.u.h0.subsystem_id == 0xc400 ||
 					(info.u.h0.subsystem_id == 0x21 && info.revision < 6))) {
	    		buffer = (*pci->read_io_32)(info.u.h0.base_registers[0] + HCFG);
	    		buffer |= HCFG_JOYENABLE;
	    		(*pci->write_io_32)(info.u.h0.base_registers[0] + HCFG, buffer);
	   		}*/
		}
		ix++;
	}

	put_module(B_PCI_MODULE_NAME);

	return err;
}


static void
make_device_names(
	emuxki_dev * card)
{
#if MIDI
	sprintf(card->midi.name, "midi/emuxki/%ld", card-cards+1);
	names[num_names++] = card->midi.name;
#endif

//	sprintf(card->joy.name1, "joystick/"DRIVER_NAME "/%x", card-cards+1);
//	names[num_names++] = card->joy.name1;

	sprintf(card->name, "audio/hmulti/emuxki/%ld", card-cards+1);
	names[num_names++] = card->name;

	names[num_names] = NULL;
}


static status_t
emuxki_setup(emuxki_dev * card)
{
	status_t err = B_OK;
	unsigned char cmd;
	//int32 base;

	PRINT(("setup_emuxki(%p)\n", card));

	make_device_names(card);
	card->config.nabmbar = card->info.u.h0.base_registers[0];
	card->config.irq = card->info.u.h0.interrupt_line;
	card->config.type = 0;
	if (card->info.device_id == CREATIVELABS_AUDIGY_DEVICE_ID) {
		card->config.type |= TYPE_AUDIGY;
		if (card->info.revision == 4)
			card->config.type |= TYPE_AUDIGY2;
	} else if (card->info.device_id == CREATIVELABS_AUDIGY2_VALUE_DEVICE_ID)
		card->config.type |= TYPE_AUDIGY | TYPE_AUDIGY2 | TYPE_AUDIGY2_VALUE;

	PRINT(("%s deviceid = %#04x chiprev = %x model = %x "
		"enhanced at %" B_PRIx32 "\n",
		card->name, card->info.device_id, card->info.revision,
		card->info.u.h0.subsystem_id, card->config.nabmbar));

	cmd = (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, PCI_command, 2);
	PRINT(("PCI command before: %x\n", cmd));
	(*pci->write_pci_config)(card->info.bus, card->info.device, card->info.function, PCI_command, 2, cmd | PCI_command_io);
	cmd = (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, PCI_command, 2);
	PRINT(("PCI command after: %x\n", cmd));

	dump_hardware_regs(&card->config);

	emuxki_reg_write_32(&card->config, EMU_HCFG, EMU_HCFG_LOCKSOUNDCACHE | EMU_HCFG_LOCKTANKCACHE_MASK|
		EMU_HCFG_MUTEBUTTONENABLE);

	dump_hardware_regs(&card->config);

#if MIDI
	//SBLIVE : EMU_MUDATA, workaround 0, AUDIGY, AUDIGY2: 0, workaround 0x11020004
	if ((err = (*mpu401->create_device)((card->config.nabmbar + (!IS_AUDIGY(&card->config) ? EMU_MUDATA : 0)),
		&card->midi.driver, !IS_AUDIGY(&card->config) ? 0 : 0x11020004, midi_interrupt_op, &card->midi)) < B_OK)
		return (err);

	card->midi.card = card;
#endif

	// begin Joystick part
/*	base = card->info.u.h0.base_registers[0];
	(*pci->write_pci_config) (card->info.bus,card->info.device,
			card->info.function, 0x10, 2, base);

	if ((*gameport->create_device)(base, &card->joy.driver) < B_OK) {
		dprintf("Audigy joystick - Error creating device\n");
		(*gameport->delete_device)(card->joy.driver);
	}*/
	// end Joystick part

	/* reset the codec */
	PRINT(("codec reset\n"));
	emuxki_codec_write(&card->config, 0x00, 0x0000);
	snooze(50000); // 50 ms

	ac97_init(&card->config);
	ac97_amp_enable(&card->config, true);

	PRINT(("codec vendor id      = %#08" B_PRIx32 "\n",
		ac97_get_vendor_id(&card->config)));
	PRINT(("codec description     = %s\n",
		ac97_get_vendor_id_description(&card->config)));
	PRINT(("codec 3d enhancement = %s\n",
		ac97_get_3d_stereo_enhancement(&card->config)));

	if (IS_AUDIGY2(&card->config)) {
		emuxki_reg_write_32(&card->config, EMU_A_IOCFG,
			EMU_A_IOCFG_GPOUT0 | emuxki_reg_read_32(&card->config, EMU_A_IOCFG));
	}

	dump_hardware_regs(&card->config);

	/*PRINT(("codec master output = %#04x\n",emuxki_codec_read(&card->config, 0x02)));
	PRINT(("codec aux output    = %#04x\n",emuxki_codec_read(&card->config, 0x04)));
	PRINT(("codec mono output   = %#04x\n",emuxki_codec_read(&card->config, 0x06)));
	PRINT(("codec pcm output    = %#04x\n",emuxki_codec_read(&card->config, 0x18)));
	PRINT(("codec line in	    = %#04x\n",emuxki_codec_read(&card->config, 0x10)));
	PRINT(("codec record line in= %#04x\n",emuxki_codec_read(&card->config, 0x1a)));
	PRINT(("codec record gain   = %#04x\n",emuxki_codec_read(&card->config, 0x1c)));*/

	/*PRINT(("adc index	    = %#08x\n",emuxki_chan_read(&card->config, EMU_ADCIDX, 0)));
	PRINT(("micro index 	= %#08x\n",emuxki_chan_read(&card->config, EMU_MICIDX, 0)));
	PRINT(("fx index   		= %#08x\n",emuxki_chan_read(&card->config, EMU_FXIDX, 0)));
	PRINT(("adc addr	    = %#08x\n",emuxki_chan_read(&card->config, EMU_ADCBA, 0)));
	PRINT(("micro addr 		= %#08x\n",emuxki_chan_read(&card->config, EMU_MICBA, 0)));
	PRINT(("fx addr   		= %#08x\n",emuxki_chan_read(&card->config, EMU_FXBA, 0)));
	PRINT(("adc size	    = %#08x\n",emuxki_chan_read(&card->config, EMU_ADCBS, 0)));
	PRINT(("micro size 		= %#08x\n",emuxki_chan_read(&card->config, EMU_MICBS, 0)));
	PRINT(("fx size   		= %#08x\n",emuxki_chan_read(&card->config, EMU_FXBS, 0)));

	PRINT(("EMU_ADCCR   		= %#08x\n",emuxki_chan_read(&card->config, EMU_ADCCR, 0)));
	PRINT(("EMU_FXWC   		= %#08x\n",emuxki_chan_read(&card->config, EMU_FXWC, 0)));
	PRINT(("EMU_FXWC   		= %#08x\n",emuxki_reg_read_32(&card->config, EMU_FXWC)));*/

	PRINT(("writing codec registers\n"));
	// TODO : to move with AC97
	/* enable master output */
	emuxki_codec_write(&card->config, AC97_MASTER_VOLUME, 0x0000);
	/* enable aux output */
	emuxki_codec_write(&card->config, AC97_AUX_OUT_VOLUME, 0x0000);
	/* enable mono output */
	//emuxki_codec_write(&card->config, AC97_MONO_VOLUME, 0x0004);
	/* enable pcm output */
	emuxki_codec_write(&card->config, AC97_PCM_OUT_VOLUME, 0x0808);
	/* enable line in */
	//emuxki_codec_write(&card->config, AC97_LINE_IN_VOLUME, 0x8808);
	/* set record line in */
	emuxki_codec_write(&card->config, AC97_RECORD_SELECT, 0x0404);
	/* set record gain */
	//emuxki_codec_write(&card->config, AC97_RECORD_GAIN, 0x0000);

	PRINT(("codec master output = %#04x\n",emuxki_codec_read(&card->config, AC97_MASTER_VOLUME)));
	PRINT(("codec aux output    = %#04x\n",emuxki_codec_read(&card->config, AC97_AUX_OUT_VOLUME)));
	PRINT(("codec mono output   = %#04x\n",emuxki_codec_read(&card->config, AC97_MONO_VOLUME)));
	PRINT(("codec pcm output    = %#04x\n",emuxki_codec_read(&card->config, AC97_PCM_OUT_VOLUME)));
	PRINT(("codec line in	    = %#04x\n",emuxki_codec_read(&card->config, AC97_LINE_IN_VOLUME)));
	PRINT(("codec record line in= %#04x\n",emuxki_codec_read(&card->config, AC97_RECORD_SELECT)));
	PRINT(("codec record gain   = %#04x\n",emuxki_codec_read(&card->config, AC97_RECORD_GAIN)));

	if (emuxki_codec_read(&card->config, AC97_EXTENDED_AUDIO_ID) & 0x0080) {
		card->config.type |= TYPE_LIVE_5_1;
		emuxki_chan_write(&card->config, 0, EMU_AC97SLOT, EMU_AC97SLOT_CENTER | EMU_AC97SLOT_LFE);
		emuxki_codec_write(&card->config, AC97_SURROUND_VOLUME, 0x0000);
	}

	if ((err = emuxki_init(card)))
		return (err);

	if (IS_AUDIGY(&card->config) || IS_LIVE_5_1(&card->config)) {
		card->play_mode = 6;	// mode 5.1
	} else {
		card->play_mode = 4;	// mode 4.0
	}

	emuxki_reg_write_32(&card->config, EMU_INTE, EMU_INTE_SAMPLERATER | EMU_INTE_PCIERRENABLE);
	if (IS_AUDIGY2(&card->config)) {
		emuxki_reg_write_32(&card->config, EMU_A2_INTE2, 0);
		if (!IS_AUDIGY2_VALUE(&card->config)) {
			emuxki_reg_write_32(&card->config, EMU_A2_INTE3, 0);
		}
	}

	PRINT(("installing interrupt : %" B_PRIx32 "\n", card->config.irq));
	err = install_io_interrupt_handler(card->config.irq, emuxki_int, card, 0);
	if (err != B_OK) {
		PRINT(("failed to install interrupt\n"));
		emuxki_shutdown(card);
		return err;
	}

	emuxki_inte_enable(&card->config, EMU_INTE_VOLINCRENABLE | EMU_INTE_VOLDECRENABLE
		| EMU_INTE_MUTEENABLE | EMU_INTE_FXDSPENABLE);
	if (IS_AUDIGY2(&card->config)) {
		emuxki_reg_write_32(&card->config, EMU_HCFG, EMU_HCFG_AUDIOENABLE |
			EMU_HCFG_AC3ENABLE_CDSPDIF | EMU_HCFG_AC3ENABLE_GPSPDIF|
			EMU_HCFG_JOYENABLE | EMU_HCFG_AUTOMUTE);
	} else if (IS_AUDIGY(&card->config)) {
		emuxki_reg_write_32(&card->config, EMU_HCFG, EMU_HCFG_AUDIOENABLE |
			EMU_HCFG_JOYENABLE | EMU_HCFG_AUTOMUTE);
	} else {
		emuxki_reg_write_32(&card->config, EMU_HCFG, EMU_HCFG_AUDIOENABLE |
			EMU_HCFG_LOCKTANKCACHE_MASK | EMU_HCFG_JOYENABLE | EMU_HCFG_AUTOMUTE);
	}

	PRINT(("setup_emuxki done\n"));

	return err;
}


void
emuxki_dump_fx(emuxki_dev * card)
{
	uint16      pc = 0;
	uint8		op;
	uint16		r,a,x,y,zero;

	LOG(("emuxki_dump_fx\n"));

	zero = IS_AUDIGY(&card->config) ? EMU_A_DSP_CST(0) : EMU_DSP_CST(0);

	while (pc < 512) {
		emuxki_dsp_getop(&card->config, &pc, &op, &r, &a, &x, &y);
		if (op!=EMU_DSP_OP_ACC3 || r!=zero || a!=zero || x!=zero || y!=zero) {
			LOG(("dsp_op pc=%u, op=%x, r=%x, a=%x, x=%x, y=%x\n",
				pc, op, r, a, x, y));
		}
	}
}


static void
emuxki_initfx(emuxki_dev * card)
{
	uint16       pc, gpr;
	emuxki_gpr *a_front_gpr, *a_rear_gpr, *a_center_sub_gpr = NULL;
	emuxki_gpr *p_ac97_in_gpr, *p_cd_in_gpr, *r_ac97_in_gpr, *r_cd_in_gpr, *r_fx_out_gpr;
	emuxki_gpr *d_front_gpr, *d_rear_gpr, *d_center_sub_gpr;

	/* Set all GPRs to 0 */
	for (pc = 0; pc < 256; pc++) {
		emuxki_write_gpr(&card->config, pc, 0);
		card->gpr[pc].gpr = -1;
	}

	for (pc = 0; pc < 160; pc++) {
		emuxki_chan_write(&card->config, 0, EMU_TANKMEMDATAREGBASE + pc, 0);
		emuxki_chan_write(&card->config, 0, EMU_TANKMEMADDRREGBASE + pc, 0);
	}
	pc = 0;
	gpr = EMU_GPR_FIRST_MIX;	// we reserve 16 gprs for processing
#define EMU_DSP_TMPGPR_FRONT_LEFT 0
#define EMU_DSP_TMPGPR_FRONT_RIGHT 1
#define EMU_DSP_TMPGPR_REAR_LEFT 2
#define EMU_DSP_TMPGPR_REAR_RIGHT 3
#define EMU_DSP_TMPGPR_CENTER 4
#define EMU_DSP_TMPGPR_SUB 5
#define EMU_DSP_TMPGPR_DSP_IN_L	6
#define EMU_DSP_TMPGPR_DSP_IN_R	7

	a_front_gpr = emuxki_gpr_new(card, "Analog Front",
			EMU_MIX_GAIN|EMU_MIX_STEREO|EMU_MIX_MUTE|EMU_MIX_PLAYBACK, &gpr, 0.0, 0.0, -46.5, 0.0, -0.75);
	a_rear_gpr = emuxki_gpr_new(card, "Analog Rear",
			EMU_MIX_GAIN|EMU_MIX_STEREO|EMU_MIX_MUTE|EMU_MIX_PLAYBACK, &gpr, 0.0, 0.0, -46.5, 0.0, -0.75);
	if (IS_AUDIGY(&card->config) || IS_LIVE_5_1(&card->config))
		a_center_sub_gpr = emuxki_gpr_new(card, "Analog Center/Sub",
			EMU_MIX_GAIN|EMU_MIX_STEREO|EMU_MIX_MUTE|EMU_MIX_PLAYBACK, &gpr, 0.0, 0.0, -46.5, 0.0, -0.75);

	d_front_gpr = emuxki_gpr_new(card, "Digital Front",
			EMU_MIX_GAIN|EMU_MIX_STEREO|EMU_MIX_MUTE|EMU_MIX_PLAYBACK, &gpr, 0.0, 0.0, -46.5, 0.0, -0.75);
	d_rear_gpr = emuxki_gpr_new(card, "Digital Rear",
			EMU_MIX_GAIN|EMU_MIX_STEREO|EMU_MIX_MUTE|EMU_MIX_PLAYBACK, &gpr, 0.0, 0.0, -46.5, 0.0, -0.75);
	d_center_sub_gpr = emuxki_gpr_new(card, "Digital Center/Sub",
			EMU_MIX_GAIN|EMU_MIX_STEREO|EMU_MIX_MUTE|EMU_MIX_PLAYBACK, &gpr, 0.0, 0.0, -46.5, 0.0, -0.75);

	/* playback in gprs */
	p_ac97_in_gpr = emuxki_gpr_new(card, "AC97 Record In",
			EMU_MIX_GAIN|EMU_MIX_STEREO|EMU_MIX_MUTE|EMU_MIX_PLAYBACK, &gpr, 0.0, 1.0, -46.5, 0.0, -0.75);
	p_cd_in_gpr = emuxki_gpr_new(card, "CD Spdif In",
			EMU_MIX_GAIN|EMU_MIX_STEREO|EMU_MIX_MUTE|EMU_MIX_PLAYBACK, &gpr, 0.0, 1.0, -46.5, 0.0, -0.75);

	/* record in gprs */
	r_ac97_in_gpr = emuxki_gpr_new(card, "AC97 Record In",
			EMU_MIX_GAIN|EMU_MIX_STEREO|EMU_MIX_MUTE|EMU_MIX_RECORD, &gpr, 0.0, 0.0, -46.5, 0.0, -0.75);
	r_cd_in_gpr = emuxki_gpr_new(card, "CD Spdif In",
			EMU_MIX_GAIN|EMU_MIX_STEREO|EMU_MIX_MUTE|EMU_MIX_RECORD, &gpr, 0.0, 0.0, -46.5, 0.0, -0.75);
	r_fx_out_gpr = emuxki_gpr_new(card, "FX 0/1",
			EMU_MIX_GAIN|EMU_MIX_STEREO|EMU_MIX_MUTE|EMU_MIX_RECORD, &gpr, 0.0, 0.0, -46.5, 0.0, -0.75);

	card->gpr_count = gpr;

	if (IS_AUDIGY(&card->config)) {
		/* DSP_IN_GPR(l/r) = 0 + AC97In(l/r) * P_AC97_IN_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_INL(EMU_DSP_IN_AC97), EMU_A_DSP_GPR(p_ac97_in_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_INR(EMU_DSP_IN_AC97), EMU_A_DSP_GPR(p_ac97_in_gpr->gpr + 1));

		/* DSP_IN_GPR(l/r) = DSP_IN_GPR(l/r) + CDIn(l/r) * P_CD_IN_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_A_DSP_INL(EMU_DSP_IN_CDSPDIF), EMU_A_DSP_GPR(p_cd_in_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_A_DSP_INR(EMU_DSP_IN_CDSPDIF), EMU_A_DSP_GPR(p_cd_in_gpr->gpr + 1));

		/* Front GPR(l/r) = DSP_IN_GPR(l/r) + FX(0/1) * 4 */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_FRONT_LEFT),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_FX(0), EMU_A_DSP_CST(4));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_FRONT_RIGHT),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_FX(1), EMU_A_DSP_CST(4));

		/* Rear GPR(l/r) = DSP_IN_GPR(l/r) + FX(2/3) * 4 */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_REAR_LEFT),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_FX(2), EMU_A_DSP_CST(4));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_REAR_RIGHT),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_FX(3), EMU_A_DSP_CST(4));

		/* Center/Sub GPR = 0 + FX(4/5) * 4 */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_CENTER),
				  EMU_A_DSP_CST(0),
				  EMU_DSP_FX(4), EMU_A_DSP_CST(4));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_SUB),
				  EMU_A_DSP_CST(0),
				  EMU_DSP_FX(5), EMU_A_DSP_CST(4));

		/* Analog Front Output l/r = 0 + Front GPR(l/r) * A_FRONT_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTL(EMU_A_DSP_OUT_A_FRONT),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_FRONT_LEFT), EMU_A_DSP_GPR(a_front_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTR(EMU_A_DSP_OUT_A_FRONT),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_FRONT_RIGHT), EMU_A_DSP_GPR(a_front_gpr->gpr+1));

		/* Analog Rear Output l/r = 0 + Rear GPR(l/r) * A_REAR_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTL(EMU_A_DSP_OUT_A_REAR),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_REAR_LEFT), EMU_A_DSP_GPR(a_rear_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTR(EMU_A_DSP_OUT_A_REAR),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_REAR_RIGHT), EMU_A_DSP_GPR(a_rear_gpr->gpr+1));

		/* Analog Center/Sub = 0 + Center/Sub GPR(l/r) * A_CENTER_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTL(EMU_A_DSP_OUT_A_CENTER),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_CENTER), EMU_A_DSP_GPR(a_center_sub_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTR(EMU_A_DSP_OUT_A_CENTER),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_SUB), EMU_A_DSP_GPR(a_center_sub_gpr->gpr+1));

		/* Digital Front Output l/r = 0 + Front GPR(l/r) * D_FRONT_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTL(EMU_A_DSP_OUT_D_FRONT),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_FRONT_LEFT), EMU_A_DSP_GPR(d_front_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTR(EMU_A_DSP_OUT_D_FRONT),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_FRONT_RIGHT), EMU_A_DSP_GPR(d_front_gpr->gpr+1));

		/* Digital Rear Output l/r = 0 + Rear GPR(l/r) * D_REAR_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTL(EMU_A_DSP_OUT_D_REAR),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_REAR_LEFT), EMU_A_DSP_GPR(d_rear_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTR(EMU_A_DSP_OUT_D_REAR),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_REAR_RIGHT), EMU_A_DSP_GPR(d_rear_gpr->gpr+1));

		/* Digital Center/Sub = 0 + Center/Sub GPR(l/r) * D_CENTER_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTL(EMU_A_DSP_OUT_D_CENTER),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_CENTER), EMU_A_DSP_GPR(d_center_sub_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTR(EMU_A_DSP_OUT_D_CENTER),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_SUB), EMU_A_DSP_GPR(d_center_sub_gpr->gpr+1));

		/* DSP_IN_GPR(l/r) = 0 + AC97In(l/r) * R_AC97_IN_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_INL(EMU_DSP_IN_AC97), EMU_A_DSP_GPR(r_ac97_in_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_INR(EMU_DSP_IN_AC97), EMU_A_DSP_GPR(r_ac97_in_gpr->gpr + 1));

		/* DSP_IN_GPR (l/r) = DSP_IN_GPR(l/r) + FX(0/1) * R_FX_OUT_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_FX(0), EMU_A_DSP_GPR(r_fx_out_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_FX(1), EMU_A_DSP_GPR(r_fx_out_gpr->gpr + 1));

		/* DSP_IN_GPR(l/r) = 0 + DSP_IN_GPR(l/r) * 4 */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L), EMU_A_DSP_CST(4));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_A_DSP_CST(0),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R), EMU_A_DSP_CST(4));

		/* ADC recording buffer (l/r) = DSP_IN_GPR(l/r) + CDIn(l/r) * R_CD_IN_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTL(EMU_A_DSP_OUT_ADC),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_A_DSP_INL(EMU_DSP_IN_CDSPDIF), EMU_A_DSP_GPR(r_cd_in_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_A_DSP_OUTR(EMU_A_DSP_OUT_ADC),
				  EMU_A_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_A_DSP_INR(EMU_DSP_IN_CDSPDIF), EMU_A_DSP_GPR(r_cd_in_gpr->gpr + 1));



		/* zero out the rest of the microcode */
		while (pc < 512)
			emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_ACC3,
				  EMU_A_DSP_CST(0), EMU_A_DSP_CST(0),
				  EMU_A_DSP_CST(0), EMU_A_DSP_CST(0));

		emuxki_chan_write(&card->config, 0, EMU_A_DBG, 0);	/* Is it really necessary ? */

	} else {
		/* DSP_IN_GPR(l/r) = 0 + AC97In(l/r) * P_AC97_IN_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_CST(0),
				  EMU_DSP_INL(EMU_DSP_IN_AC97), EMU_DSP_GPR(p_ac97_in_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_CST(0),
				  EMU_DSP_INR(EMU_DSP_IN_AC97), EMU_DSP_GPR(p_ac97_in_gpr->gpr + 1));

		/* DSP_IN_GPR(l/r) = DSP_IN_GPR(l/r) + CDIn(l/r) * P_CD_IN_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_INL(EMU_DSP_IN_CDSPDIF), EMU_DSP_GPR(p_cd_in_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_INR(EMU_DSP_IN_CDSPDIF), EMU_DSP_GPR(p_cd_in_gpr->gpr + 1));

		/* Front GPR(l/r) = DSP_IN_GPR(l/r) + FX(0/1) * 4 */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_FRONT_LEFT),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_FX(0), EMU_DSP_CST(4));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_FRONT_RIGHT),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_FX(1), EMU_DSP_CST(4));

		/* Rear GPR(l/r) = DSP_IN_GPR(l/r) + FX(2/3) * 4 */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_REAR_LEFT),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_FX(2), EMU_DSP_CST(4));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_REAR_RIGHT),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_FX(3), EMU_DSP_CST(4));

		/* Center/Sub GPR = 0 + FX(4/5) * 4 */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_CENTER),
				  EMU_DSP_CST(0),
				  EMU_DSP_FX(4), EMU_DSP_CST(4));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_SUB),
				  EMU_DSP_CST(0),
				  EMU_DSP_FX(5), EMU_DSP_CST(4));

		/* Analog Front Output l/r = 0 + Front GPR(l/r) * A_FRONT_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_OUTL(EMU_DSP_OUT_A_FRONT),
				  EMU_DSP_CST(0),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_FRONT_LEFT), EMU_DSP_GPR(a_front_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_OUTR(EMU_DSP_OUT_A_FRONT),
				  EMU_DSP_CST(0),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_FRONT_RIGHT), EMU_DSP_GPR(a_front_gpr->gpr+1));

		/* Analog Rear Output l/r = 0 + Rear GPR(l/r) * A_REAR_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_OUTL(EMU_DSP_OUT_AD_REAR),
				  EMU_DSP_CST(0),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_REAR_LEFT), EMU_DSP_GPR(a_rear_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_OUTR(EMU_DSP_OUT_AD_REAR),
				  EMU_DSP_CST(0),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_REAR_RIGHT), EMU_DSP_GPR(a_rear_gpr->gpr+1));

		/* Analog Center/Sub = 0 + Center/Sub GPR(l/r) * A_CENTER_GPR(l/r) */
		if (IS_LIVE_5_1(&card->config)) {
			emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
					  EMU_DSP_OUT_A_CENTER,
					  EMU_DSP_CST(0),
					  EMU_DSP_GPR(EMU_DSP_TMPGPR_CENTER), EMU_DSP_GPR(a_center_sub_gpr->gpr));
			emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
					  EMU_DSP_OUT_A_SUB,
					  EMU_DSP_CST(0),
					  EMU_DSP_GPR(EMU_DSP_TMPGPR_SUB), EMU_DSP_GPR(a_center_sub_gpr->gpr+1));
		}

		/* Digital Front Output l/r = 0 + Front GPR(l/r) * D_FRONT_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_OUTL(EMU_DSP_OUT_D_FRONT),
				  EMU_DSP_CST(0),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_FRONT_LEFT), EMU_DSP_GPR(d_front_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_OUTR(EMU_DSP_OUT_D_FRONT),
				  EMU_DSP_CST(0),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_FRONT_RIGHT), EMU_DSP_GPR(d_front_gpr->gpr+1));

		/* Digital Rear Output l/r = 0 + Rear GPR(l/r) * D_REAR_GPR(l/r) */
		/*emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_OUTL(EMU_DSP_OUT_D_REAR),
				  EMU_DSP_CST(0),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_REAR_LEFT), EMU_DSP_GPR(d_rear_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_OUTR(EMU_DSP_OUT_D_REAR),
				  EMU_DSP_CST(0),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_REAR_RIGHT), EMU_DSP_GPR(d_rear_gpr->gpr+1));*/

		/* Digital Center/Sub = 0 + Center/Sub GPR(l/r) * D_CENTER_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_OUTL(EMU_DSP_OUT_D_CENTER),
				  EMU_DSP_CST(0),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_CENTER), EMU_DSP_GPR(d_center_sub_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_OUTR(EMU_DSP_OUT_D_CENTER),
				  EMU_DSP_CST(0),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_SUB), EMU_DSP_GPR(d_center_sub_gpr->gpr+1));

		/* DSP_IN_GPR(l/r) = 0 + AC97In(l/r) * R_AC97_IN_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_CST(0),
				  EMU_DSP_INL(EMU_DSP_IN_AC97), EMU_DSP_GPR(r_ac97_in_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_CST(0),
				  EMU_DSP_INR(EMU_DSP_IN_AC97), EMU_DSP_GPR(r_ac97_in_gpr->gpr + 1));

		/* DSP_IN_GPR (l/r) = DSP_IN_GPR(l/r) + FX(0/1) * R_FX_OUT_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_FX(0), EMU_DSP_GPR(r_fx_out_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_FX(1), EMU_DSP_GPR(r_fx_out_gpr->gpr + 1));

		/* DSP_IN_GPR(l/r) = 0 + DSP_IN_GPR(l/r) * 4 */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_CST(0),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L), EMU_DSP_CST(4));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACINTS,
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_CST(0),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R), EMU_DSP_CST(4));

		/* ADC recording buffer (l/r) = DSP_IN_GPR(l/r) + CDIn(l/r) * R_CD_IN_GPR(l/r) */
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_OUTL(EMU_DSP_OUT_ADC),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_L),
				  EMU_DSP_INL(EMU_DSP_IN_CDSPDIF), EMU_DSP_GPR(r_cd_in_gpr->gpr));
		emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_MACS,
				  EMU_DSP_OUTR(EMU_DSP_OUT_ADC),
				  EMU_DSP_GPR(EMU_DSP_TMPGPR_DSP_IN_R),
				  EMU_DSP_INR(EMU_DSP_IN_CDSPDIF), EMU_DSP_GPR(r_cd_in_gpr->gpr + 1));

		/* zero out the rest of the microcode */
		while (pc < 512)
			emuxki_dsp_addop(&card->config, &pc, EMU_DSP_OP_ACC3,
				  EMU_DSP_CST(0), EMU_DSP_CST(0),
				  EMU_DSP_CST(0), EMU_DSP_CST(0));

		emuxki_chan_write(&card->config, 0, EMU_DBG, 0);	/* Is it really necessary ? */
	}

	emuxki_dump_fx(card);
}


status_t
emuxki_init(emuxki_dev * card)
{
	uint16       i;
	uint32       spcs, *ptb;
	uint32	 	 silentpage;

	/* disable any channel interrupt */
	emuxki_chan_write(&card->config, 0, EMU_CLIEL, 0);
	emuxki_chan_write(&card->config, 0, EMU_CLIEH, 0);
	emuxki_chan_write(&card->config, 0, EMU_SOLEL, 0);
	emuxki_chan_write(&card->config, 0, EMU_SOLEH, 0);

	/* Set recording buffers sizes to zero */
	emuxki_chan_write(&card->config, 0, EMU_MICBS, EMU_RECBS_BUFSIZE_NONE);
	emuxki_chan_write(&card->config, 0, EMU_MICBA, 0);
	emuxki_chan_write(&card->config, 0, EMU_FXBS, EMU_RECBS_BUFSIZE_NONE);
	emuxki_chan_write(&card->config, 0, EMU_FXBA, 0);
	emuxki_chan_write(&card->config, 0, EMU_ADCBS, EMU_RECBS_BUFSIZE_NONE);
	emuxki_chan_write(&card->config, 0, EMU_ADCBA, 0);

	if (IS_AUDIGY(&card->config)) {
		emuxki_chan_write(&card->config, 0, EMU_SPBYPASS, EMU_SPBYPASS_24_BITS);
		emuxki_chan_write(&card->config, 0, EMU_AC97SLOT, EMU_AC97SLOT_CENTER | EMU_AC97SLOT_LFE);
	}

	/* Initialize all channels to stopped and no effects */
	for (i = 0; i < EMU_NUMCHAN; i++) {
		emuxki_chan_write(&card->config, i, EMU_CHAN_DCYSUSV, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_IP, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_VTFT, 0xffff);
		emuxki_chan_write(&card->config, i, EMU_CHAN_CVCF, 0xffff);
		emuxki_chan_write(&card->config, i, EMU_CHAN_PTRX, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_CPF, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_CCR, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_PSST, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_DSL, 0x10);	/* Why 16 ? */
		emuxki_chan_write(&card->config, i, EMU_CHAN_CCCA, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_Z1, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_Z2, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_FXRT, 0x32100000);
		emuxki_chan_write(&card->config, i, EMU_CHAN_ATKHLDM, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_DCYSUSM, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_IFATN, 0xffff);
		emuxki_chan_write(&card->config, i, EMU_CHAN_PEFE, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_FMMOD, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_TREMFRQ, 24);
		emuxki_chan_write(&card->config, i, EMU_CHAN_FM2FRQ2, 24);
		emuxki_chan_write(&card->config, i, EMU_CHAN_TEMPENV, 0);
		/*** these are last so OFF prevents writing ***/
		emuxki_chan_write(&card->config, i, EMU_CHAN_LFOVAL2, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_LFOVAL1, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_ATKHLDV, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_ENVVOL, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_ENVVAL, 0);
	}

	/* set digital outputs format */
	spcs = (EMU_SPCS_CLKACCY_1000PPM | EMU_SPCS_SAMPLERATE_48 |
	      EMU_SPCS_CHANNELNUM_LEFT | EMU_SPCS_SOURCENUM_UNSPEC |
		EMU_SPCS_GENERATIONSTATUS | 0x00001200 /* Cat code. */ |
		0x00000000 /* IEC-958 Mode */ | EMU_SPCS_EMPHASIS_NONE |
		EMU_SPCS_COPYRIGHT);
	emuxki_chan_write(&card->config, 0, EMU_SPCS0, spcs);
	emuxki_chan_write(&card->config, 0, EMU_SPCS1, spcs);
	emuxki_chan_write(&card->config, 0, EMU_SPCS2, spcs);

	if (IS_AUDIGY2(&card->config)) {
		emuxki_chan_write(&card->config, 0, EMU_A2_SPDIF_SAMPLERATE, EMU_A2_SPDIF_UNKNOWN);

		emuxki_p16v_write(&card->config, 0, EMU_A2_SRCSEL,
			EMU_A2_SRCSEL_ENABLE_SPDIF | EMU_A2_SRCSEL_ENABLE_SRCMULTI);

		if (IS_AUDIGY2_VALUE(&card->config)) {
			emuxki_p16v_write(&card->config, 0, EMU_A2_P17V_I2S, EMU_A2_P17V_I2S_ENABLE);
			emuxki_p16v_write(&card->config, 0, EMU_A2_P17V_SPDIF, EMU_A2_P17V_SPDIF_ENABLE);

			emuxki_reg_write_32(&card->config, EMU_A_IOCFG,
				emuxki_reg_read_32(&card->config, EMU_A_IOCFG) & ~0x8);
		} else {
			emuxki_p16v_write(&card->config, 0, EMU_A2_SRCMULTI, EMU_A2_SRCMULTI_ENABLE_INPUT);
		}
	}

	/* Let's play with sound processor */
	emuxki_initfx(card);

	/* allocate memory for our Page Table */
	card->ptb_area = alloc_mem(&card->ptb_phy_base, &card->ptb_log_base,
		EMU_MAXPTE * sizeof(uint32), "emuxki ptb");

	/* This is necessary unless you like Metallic noise... */
	card->silentpage_area = alloc_mem(&card->silentpage_phy_base, &card->silentpage_log_base,
		EMU_PTESIZE, "emuxki sp");

	if (card->ptb_area < B_OK || card->silentpage_area < B_OK) {
		PRINT(("couldn't allocate memory\n"));
		if (card->ptb_area > B_OK)
			delete_area(card->ptb_area);
		if (card->silentpage_area > B_OK)
			delete_area(card->silentpage_area);
		return B_ERROR;
	}

	/* Zero out the silent page */
	/* This might not be always true, it might be 128 for 8bit channels */
	memset(card->ptb_log_base, 0, EMU_PTESIZE);

	/*
	 * Set all the PTB Entries to the silent page We shift the physical
	 * address by one and OR it with the page number. I don't know what
	 * the ORed index is for, might be a very useful unused feature...
	 */
	silentpage = ((uint32)card->silentpage_phy_base) << 1;
	ptb = card->ptb_log_base;
	for (i = 0; i < EMU_MAXPTE; i++)
		ptb[i] = B_HOST_TO_LENDIAN_INT32(silentpage | i);

	/* Write PTB address and set TCB to none */
	emuxki_chan_write(&card->config, 0, EMU_PTB, (uint32)card->ptb_phy_base);
	emuxki_chan_write(&card->config, 0, EMU_TCBS, 0);	/* This means 16K TCB */
	emuxki_chan_write(&card->config, 0, EMU_TCB, 0);	/* No TCB use for now */

	/*
	 * Set channels MAPs to the silent page.
	 * I don't know what MAPs are for.
	 */
	silentpage |= EMU_CHAN_MAP_PTI_MASK;
	for (i = 0; i < EMU_NUMCHAN; i++) {
		emuxki_chan_write(&card->config, i, EMU_CHAN_MAPA, silentpage);
		emuxki_chan_write(&card->config, i, EMU_CHAN_MAPB, silentpage);
		card->channel[i] = NULL;
	}

	/* Init streams list */
	LIST_INIT(&(card->streams));

	/* Init mems list */
	LIST_INIT(&(card->mem));

	/* Timer is stopped */
	card->timerstate &= ~EMU_TIMER_STATE_ENABLED;
	card->timerate = 0xffff;

	return B_OK;
}


status_t
init_driver(void)
{
	void *settings_handle;
	pci_info info;
	status_t err;
	int ix = 0;
	num_cards = 0;

	PRINT(("init_driver()\n"));

	// get driver settings
	settings_handle = load_driver_settings("emuxki.settings");
	if (settings_handle != NULL) {
		const char *item;
		char       *end;
		uint32      value;

		item = get_driver_parameter (settings_handle, "channels", NULL, NULL);
		if (item) {
			value = strtoul (item, &end, 0);
			if (*end == '\0') current_settings.channels = value;
		}

		item = get_driver_parameter (settings_handle, "bitsPerSample", NULL, NULL);
		if (item) {
			value = strtoul (item, &end, 0);
			if (*end == '\0') current_settings.bitsPerSample = value;
		}

		item = get_driver_parameter (settings_handle, "sample_rate", NULL, NULL);
		if (item) {
			value = strtoul (item, &end, 0);
			if (*end == '\0') current_settings.sample_rate = value;
		}

		item = get_driver_parameter (settings_handle, "buffer_frames", NULL, NULL);
		if (item) {
			value = strtoul (item, &end, 0);
			if (*end == '\0') current_settings.buffer_frames = value;
		}

		item = get_driver_parameter (settings_handle, "buffer_count", NULL, NULL);
		if (item) {
			value = strtoul (item, &end, 0);
			if (*end == '\0') current_settings.buffer_count = value;
		}

		unload_driver_settings (settings_handle);
	}

	if (get_module(B_PCI_MODULE_NAME, (module_info **) &pci))
		return ENOSYS;

//	if (get_module (gameport_name, (module_info **)&gameport)) {
//		put_module (B_PCI_MODULE_NAME);
//		return ENOSYS;
//	}

	if (get_module(B_MPU_401_MODULE_NAME, (module_info **) &mpu401)) {
		//put_module(gameport_name);
		put_module(B_PCI_MODULE_NAME);
		return ENOSYS;
	}

	while ((*pci->get_nth_pci_info)(ix++, &info) == B_OK) {
		if (info.vendor_id == CREATIVELABS_VENDOR_ID &&
			(info.device_id == CREATIVELABS_SBLIVE_DEVICE_ID
#if AUDIGY
			|| info.device_id == CREATIVELABS_AUDIGY_DEVICE_ID
			|| info.device_id == CREATIVELABS_AUDIGY2_VALUE_DEVICE_ID
#endif
			)) {
			if (num_cards == NUM_CARDS) {
				PRINT(("Too many emuxki cards installed!\n"));
				break;
			}
			memset(&cards[num_cards], 0, sizeof(emuxki_dev));
			cards[num_cards].info = info;
#ifdef __HAIKU__
			if ((err = (*pci->reserve_device)(info.bus, info.device, info.function,
				DRIVER_NAME, &cards[num_cards])) < B_OK) {
				dprintf("%s: failed to reserve_device(%d, %d, %d,): %s\n",
					DRIVER_NAME, info.bus, info.device, info.function,
					strerror(err));
				continue;
			}
#endif
			if (emuxki_setup(&cards[num_cards])) {
				PRINT(("Setup of emuxki %" B_PRId32 " failed\n",
					num_cards + 1));
#ifdef __HAIKU__
				(*pci->unreserve_device)(info.bus, info.device, info.function,
					DRIVER_NAME, &cards[num_cards]);
#endif
			} else {
				num_cards++;
			}
		}
	}
	if (!num_cards) {
		put_module(B_MPU_401_MODULE_NAME);
//		put_module(gameport_name);
		put_module(B_PCI_MODULE_NAME);
		PRINT(("emuxki: no suitable cards found\n"));
		return ENODEV;
	}

	return B_OK;
}


void
emuxki_shutdown(emuxki_dev *card)
{
	uint32       i;

	PRINT(("shutdown(%p)\n", card));

	emuxki_reg_write_32(&card->config, EMU_HCFG, EMU_HCFG_LOCKSOUNDCACHE |
		  EMU_HCFG_LOCKTANKCACHE_MASK | EMU_HCFG_MUTEBUTTONENABLE);
	emuxki_reg_write_32(&card->config, EMU_INTE, 0);

	dump_hardware_regs(&card->config);

	/* Disable any Channels interrupts */
	emuxki_chan_write(&card->config, 0, EMU_CLIEL, 0);
	emuxki_chan_write(&card->config, 0, EMU_CLIEH, 0);
	emuxki_chan_write(&card->config, 0, EMU_SOLEL, 0);
	emuxki_chan_write(&card->config, 0, EMU_SOLEH, 0);

	/* Stop all channels */
	/* This shouldn't be necessary, i'll remove once everything works */
	for (i = 0; i < EMU_NUMCHAN; i++)
		emuxki_chan_write(&card->config, i, EMU_CHAN_DCYSUSV, 0);
	for (i = 0; i < EMU_NUMCHAN; i++) {
		emuxki_chan_write(&card->config, i, EMU_CHAN_VTFT, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_CVCF, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_PTRX, 0);
		emuxki_chan_write(&card->config, i, EMU_CHAN_CPF, 0);
	}

	remove_io_interrupt_handler(card->config.irq, emuxki_int, card);

	/*
	 * deallocate Emu10k1 caches and recording buffers Again it will be
	 * removed because it will be done in voice shutdown.
	 */
	emuxki_chan_write(&card->config, 0, EMU_MICBS, EMU_RECBS_BUFSIZE_NONE);
	emuxki_chan_write(&card->config, 0, EMU_MICBA, 0);
	emuxki_chan_write(&card->config, 0, EMU_FXBS, EMU_RECBS_BUFSIZE_NONE);
	emuxki_chan_write(&card->config, 0, EMU_FXBA, 0);
	if (IS_AUDIGY(&card->config)) {
		emuxki_chan_write(&card->config, 0, EMU_A_FXWC1, 0);
		emuxki_chan_write(&card->config, 0, EMU_A_FXWC2, 0);
	} else {
		emuxki_chan_write(&card->config, 0, EMU_FXWC, 0);
	}
	emuxki_chan_write(&card->config, 0, EMU_ADCBS, EMU_RECBS_BUFSIZE_NONE);
	emuxki_chan_write(&card->config, 0, EMU_ADCBA, 0);

	/*
	 * I don't know yet how i will handle tank cache buffer,
	 * I don't even clearly  know what it is for
	 */
	emuxki_chan_write(&card->config, 0, EMU_TCB, 0);	/* 16K again */
	emuxki_chan_write(&card->config, 0, EMU_TCBS, 0);

	emuxki_chan_write(&card->config, 0, EMU_DBG, 0x8000);	/* necessary ? */

	PRINT(("freeing ptb_area\n"));
	if (card->ptb_area > B_OK)
		delete_area(card->ptb_area);
	PRINT(("freeing silentpage_area\n"));
	if (card->silentpage_area > B_OK)
		delete_area(card->silentpage_area);

//	(*gameport->delete_device)(card->joy.driver);
}


void
uninit_driver(void)
{
	int ix, cnt = num_cards;

	PRINT(("uninit_driver()\n"));

	for (ix=0; ix<cnt; ix++) {
		emuxki_shutdown(&cards[ix]);
#ifdef __HAIKU__
		(*pci->unreserve_device)(cards[ix].info.bus,
			cards[ix].info.device, cards[ix].info.function,
			DRIVER_NAME, &cards[ix]);
#endif
	}
	memset(&cards, 0, sizeof(cards));
	put_module(B_MPU_401_MODULE_NAME);
//	put_module(gameport_name);
	put_module(B_PCI_MODULE_NAME);
	num_cards = 0;
}


const char **
publish_devices(void)
{
	int ix = 0;
	PRINT(("publish_devices()\n"));

	for (ix=0; names[ix]; ix++) {
		PRINT(("publish %s\n", names[ix]));
	}
	return (const char **)names;
}


device_hooks *
find_device(const char * name)
{
	int ix;

	PRINT(("emuxki: find_device(%s)\n", name));

	for (ix=0; ix<num_cards; ix++) {
#if MIDI
		if (!strcmp(cards[ix].midi.name, name)) {
			return &midi_hooks;
		}
#endif
//		if (!strcmp(cards[ix].joy.name1, name)) {
//			return &joy_hooks;
//		}

		if (!strcmp(cards[ix].name, name)) {
			return &multi_hooks;
		}
	}
	PRINT(("emuxki: find_device(%s) failed\n", name));
	return NULL;
}

int32	api_version = B_CUR_DRIVER_API_VERSION;
