/*
 * Auvia BeOS Driver for Via VT82xx Southbridge audio
 *
 * Copyright (c) 2003, Jerome Duval (jerome.duval@free.fr)
 
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tyler C. Sarna
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
 
#include <KernelExport.h>
#include <PCI.h>
#include <string.h>
#include <stdio.h>
#include "auvia.h"
#include "debug.h"
#include "config.h"
#include "util.h"
#include "io.h"
#include <fcntl.h>
#include <unistd.h>
#include "ac97.h"

status_t init_hardware(void);
status_t init_driver(void);
void uninit_driver(void);
const char ** publish_devices(void);
device_hooks * find_device(const char *);

pci_module_info	*pci;

int32 num_cards;
auvia_dev cards[NUM_CARDS];
int32 num_names;
char * names[NUM_CARDS*20+1];

extern device_hooks multi_hooks;

/* Auvia Memory management */

static auvia_mem *
auvia_mem_new(auvia_dev *card, size_t size)
{
	auvia_mem *mem;

	if ((mem = malloc(sizeof(*mem))) == NULL)
		return (NULL);

	mem->area = alloc_mem(&mem->phy_base, &mem->log_base, size, "auvia buffer");
	mem->size = size;
	if (mem->area < B_OK) {
		free(mem);
		return NULL;
	}
	return mem;
}


static void
auvia_mem_delete(auvia_mem *mem)
{
	if(mem->area > B_OK)
		delete_area(mem->area);
	free(mem);
}


static void *
auvia_mem_alloc(auvia_dev *card, size_t size)
{
	auvia_mem *mem;
	
	mem = auvia_mem_new(card, size);
	if (mem == NULL)
		return (NULL);

	LIST_INSERT_HEAD(&(card->mems), mem, next);

	return mem;
}


static void
auvia_mem_free(auvia_dev *card, void *ptr)
{
	auvia_mem 		*mem;
	
	LIST_FOREACH(mem, &card->mems, next) {
		if (mem->log_base != ptr)
			continue;
		LIST_REMOVE(mem, next);
		
		auvia_mem_delete(mem);
		break;
	}
}

/*	Auvia stream functions */

status_t
auvia_stream_set_audioparms(auvia_stream *stream, uint8 channels,
     uint8 b16, uint32 sample_rate)
{
	uint8 			sample_size, frame_size;
	LOG(("auvia_stream_set_audioparms\n"));

	if ((stream->channels == channels) &&
		(stream->b16 == b16) && 
		(stream->sample_rate == sample_rate))
		return B_OK;
	
	if(stream->buffer)
		auvia_mem_free(stream->card, stream->buffer->log_base);
		
	stream->b16 = b16;
	stream->sample_rate = sample_rate;
	stream->channels = channels;
	
	sample_size = stream->b16 + 1;
	frame_size = sample_size * stream->channels;
	
	stream->buffer = auvia_mem_alloc(stream->card, stream->bufframes 
		* frame_size * stream->bufcount);
	
	stream->trigblk = 0;	/* This shouldn't be needed */
	stream->blkmod = stream->bufcount;
	stream->blksize = stream->bufframes * frame_size;
		
	return B_OK;
}


status_t
auvia_stream_commit_parms(auvia_stream *stream)
{
	int             i;
	uint32      	*page;
	uint32			value;
	LOG(("auvia_stream_commit_parms\n"));
	
	page = stream->dmaops_log_base;
	
	for(i = 0; i < stream->bufcount; i++) {
		page[2*i] = ((uint32)stream->buffer->phy_base) + 
			i * stream->blksize;
		page[2*i + 1] = AUVIA_DMAOP_FLAG | stream->blksize;
	}
	
	page[2*stream->bufcount - 1] &= ~AUVIA_DMAOP_FLAG;
	page[2*stream->bufcount - 1] |= AUVIA_DMAOP_EOL;
	
	auvia_reg_write_32(&stream->card->config, stream->base + AUVIA_RP_DMAOPS_BASE, 
		(uint32)stream->dmaops_phy_base);
		
	if(stream->use & AUVIA_USE_RECORD)
		auvia_codec_write(&stream->card->config, AC97_PCM_L_R_ADC_RATE, 
			(uint16)stream->sample_rate);
	else
		auvia_codec_write(&stream->card->config, AC97_PCM_FRONT_DAC_RATE, 
			(uint16)stream->sample_rate);
	
	if(IS_8233(&stream->card->config)) {
		if(stream->base != AUVIA_8233_MP_BASE) {
			value = auvia_reg_read_32(&stream->card->config, stream->base 
				+ AUVIA_8233_RP_RATEFMT);
			value &= ~(AUVIA_8233_RATEFMT_48K | AUVIA_8233_RATEFMT_STEREO 
				| AUVIA_8233_RATEFMT_16BIT);
			if(stream->use & AUVIA_USE_PLAY)
				value |= AUVIA_8233_RATEFMT_48K * (stream->sample_rate / 20) 
					/ (48000 / 20);
			value |= (stream->channels == 2 ? AUVIA_8233_RATEFMT_STEREO : 0)
				| (stream->b16 ? AUVIA_8233_RATEFMT_16BIT : 0);
			auvia_reg_write_32(&stream->card->config, stream->base 
				+ AUVIA_8233_RP_RATEFMT, value);
		} else {
			static const uint32 slottab[7] = {0, 0xff000011, 0xff000021, 
				0xff000521, 0xff004321, 0xff054321, 0xff654321};
			value = (stream->b16 ? AUVIA_8233_MP_FORMAT_16BIT : AUVIA_8233_MP_FORMAT_8BIT)
				| ((stream->channels << 4) & AUVIA_8233_MP_FORMAT_CHANNEL_MASK) ;
			auvia_reg_write_8(&stream->card->config, stream->base 
				+ AUVIA_8233_OFF_MP_FORMAT, value);
			auvia_reg_write_32(&stream->card->config, stream->base 
				+ AUVIA_8233_OFF_MP_STOP, slottab[stream->channels]);
		}
	}
	//auvia_codec_write(&stream->card->config, AC97_SPDIF_CONTROL, (uint16)stream->sample_rate);
		
	return B_OK;
}


status_t
auvia_stream_get_nth_buffer(auvia_stream *stream, uint8 chan, uint8 buf, 
					char** buffer, size_t *stride)
{
	uint8 			sample_size, frame_size;
	LOG(("auvia_stream_get_nth_buffer\n"));
	
	sample_size = stream->b16 + 1;
	frame_size = sample_size * stream->channels;
	
	*buffer = stream->buffer->log_base + (buf * stream->bufframes * frame_size)
		+ chan * sample_size;
	*stride = frame_size;
	
	return B_OK;
}


static uint32
auvia_stream_curaddr(auvia_stream *stream)
{
	uint32 addr;
	if(IS_8233(&stream->card->config)) {
		addr = auvia_reg_read_32(&stream->card->config, stream->base + AUVIA_RP_DMAOPS_BASE);
		TRACE(("stream_curaddr %p, phy_base %p\n", addr, (uint32)stream->dmaops_phy_base));
		return (addr - (uint32)stream->dmaops_phy_base - 4) / 8;
	} else {
		addr = auvia_reg_read_32(&stream->card->config, stream->base + AUVIA_RP_DMAOPS_BASE);
		TRACE(("stream_curaddr %p, phy_base %p\n", addr, (uint32)stream->dmaops_phy_base));
		return (addr - (uint32)stream->dmaops_phy_base - 8) / 8;
	}
}


void
auvia_stream_start(auvia_stream *stream, void (*inth) (void *), void *inthparam)
{
	LOG(("auvia_stream_start\n"));
	
	stream->inth = inth;
	stream->inthparam = inthparam;
		
	stream->state |= AUVIA_STATE_STARTED;
	
	if(IS_8233(&stream->card->config)) {
		if(stream->base != AUVIA_8233_MP_BASE) {
			auvia_reg_write_8(&stream->card->config, stream->base + AUVIA_8233_RP_DXS_LVOL, 0);
			auvia_reg_write_8(&stream->card->config, stream->base + AUVIA_8233_RP_DXS_RVOL, 0);
		}
		auvia_reg_write_8(&stream->card->config, stream->base + AUVIA_RP_CONTROL, 
			AUVIA_RPCTRL_START | AUVIA_RPCTRL_AUTOSTART | AUVIA_RPCTRL_STOP
			| AUVIA_RPCTRL_EOL | AUVIA_RPCTRL_FLAG);
	} else {
		uint8 regvalue = (stream->channels > 1 ? AUVIA_RPMODE_STEREO : 0)
			| (stream->b16 == 1 ? AUVIA_RPMODE_16BIT : 0)
			| AUVIA_RPMODE_INTR_FLAG | AUVIA_RPMODE_INTR_EOL | AUVIA_RPMODE_AUTOSTART;
		auvia_reg_write_8(&stream->card->config, stream->base + AUVIA_RP_MODE, regvalue);
		auvia_reg_write_8(&stream->card->config, stream->base + AUVIA_RP_CONTROL,
			AUVIA_RPCTRL_START);
	}
}


void
auvia_stream_halt(auvia_stream *stream)
{
	LOG(("auvia_stream_halt\n"));
			
	stream->state &= ~AUVIA_STATE_STARTED;
	
	auvia_reg_write_8(&stream->card->config, stream->base + AUVIA_RP_CONTROL, 
		AUVIA_RPCTRL_TERMINATE);
}


auvia_stream *
auvia_stream_new(auvia_dev *card, uint8 use, uint32 bufframes, uint8 bufcount)
{
	auvia_stream *stream;
	cpu_status status;
	LOG(("auvia_stream_new\n"));

	stream = malloc(sizeof(auvia_stream));
	if (stream == NULL)
		return (NULL);
	stream->card = card;
	stream->use = use;
	stream->state = !AUVIA_STATE_STARTED;
	stream->b16 = 0;
	stream->sample_rate = 0;
	stream->channels = 0;
	stream->bufframes = bufframes;
	stream->bufcount = bufcount;
	stream->inth = NULL;
	stream->inthparam = NULL;
	stream->buffer = NULL;
	stream->blksize = 0;
	stream->trigblk = 0;
	stream->blkmod = 0;
	
	if(use & AUVIA_USE_PLAY) {
		if(IS_8233(&card->config))
			stream->base = AUVIA_8233_MP_BASE;
			//stream->base = AUVIA_PLAY_BASE;
		else
			stream->base = AUVIA_PLAY_BASE;
	} else {
		if(IS_8233(&card->config))
			stream->base = AUVIA_8233_RECORD_BASE;
		else
			stream->base = AUVIA_RECORD_BASE;
	}
		
	stream->frames_count = 0;
	stream->real_time = 0;
	stream->buffer_cycle = 0;
	stream->update_needed = false;
	
	/* allocate memory for our dma ops */
	stream->dmaops_area = alloc_mem(&stream->dmaops_phy_base, &stream->dmaops_log_base, 
		VIA_TABLE_SIZE, "auvia dmaops");
		
	if (stream->dmaops_area < B_OK) {
		PRINT(("couldn't allocate memory\n"));
		free(stream);
		return NULL;	
	}

	status = lock();
	LIST_INSERT_HEAD((&card->streams), stream, next);
	unlock(status);
	
	return stream;
}


void
auvia_stream_delete(auvia_stream *stream)
{
	cpu_status status;
	LOG(("auvia_stream_delete\n"));
	
	auvia_stream_halt(stream);
	
	auvia_reg_write_32(&stream->card->config, stream->base + AUVIA_RP_DMAOPS_BASE, 0);
	
	if (stream->dmaops_area > B_OK)
		delete_area(stream->dmaops_area);
		
	if(stream->buffer)
		auvia_mem_free(stream->card, stream->buffer->log_base);
	
	status = lock();
	LIST_REMOVE(stream, next);
	unlock(status);
	
	free(stream);
}

/* Auvia interrupt */

static int32 
auvia_int(void *arg)
{
	auvia_dev	 *card = arg;
	bool gotone = false;
	uint32       curblk;
	auvia_stream *stream;
	
	if(auvia_reg_read_32(&card->config, AUVIA_SGD_SHADOW) 
		& card->interrupt_mask) {
	
		LIST_FOREACH(stream, &card->streams, next)
			if(auvia_reg_read_8(&card->config, stream->base + AUVIA_RP_STAT) & AUVIA_RPSTAT_INTR) {
				gotone = true;
				//TRACE(("interrupt\n"));
				
				curblk = auvia_stream_curaddr(stream);
				TRACE(("RPSTAT_INTR at trigblk %lu, stream->trigblk %lu\n", curblk, stream->trigblk));
				if (curblk == stream->trigblk) {
					//TRACE(("AUVIA_RPSTAT_INTR at trigblk %lu\n", curblk));
						
					if(stream->inth)
						stream->inth(stream->inthparam);
						
					stream->trigblk++;
					stream->trigblk %= stream->blkmod;
				}				
				
				auvia_reg_write_8(&card->config, stream->base + AUVIA_RP_STAT, AUVIA_RPSTAT_INTR);
			}
	} else {
		TRACE(("SGD_SHADOW %x %x\n", card->interrupt_mask, 
			auvia_reg_read_32(&card->config, AUVIA_SGD_SHADOW)));
	}
	
	if(gotone)
		return B_INVOKE_SCHEDULER;

	TRACE(("Got unhandled interrupt\n"));
	return B_UNHANDLED_INTERRUPT;
}

/*	Auvia driver functions */

/* detect presence of our hardware */
status_t 
init_hardware(void)
{
	int ix=0;
	pci_info info;
	status_t err = ENODEV;
	
	LOG_CREATE();

	PRINT(("init_hardware()\n"));

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci))
		return ENOSYS;

	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if (info.vendor_id == VIATECH_VENDOR_ID &&
			(info.device_id == VIATECH_82C686_AC97_DEVICE_ID
			|| info.device_id == VIATECH_8233_AC97_DEVICE_ID
			)) {
			err = B_OK;
		}
		ix++;
	}
		
	put_module(B_PCI_MODULE_NAME);

	return err;
}

static void
make_device_names(
	auvia_dev * card)
{
	sprintf(card->name, "audio/hmulti/auvia/%ld", card-cards+1);
	names[num_names++] = card->name;

	names[num_names] = NULL;
}


static status_t
auvia_init(auvia_dev * card)
{
	uint32 pr;

	pr = (*pci->read_pci_config)(card->info.bus, card->info.device, 
		card->info.function, AUVIA_PCICONF_JUNK, 4);
	PRINT(("AUVIA_PCICONF_JUNK before: %" B_PRIx32 "\n", pr));
	pr &= ~AUVIA_PCICONF_ENABLES;
	pr |= AUVIA_PCICONF_ACLINKENAB | AUVIA_PCICONF_ACNOTRST 
		| AUVIA_PCICONF_ACVSR | AUVIA_PCICONF_ACSGD;
	pr &= ~(AUVIA_PCICONF_ACFM | AUVIA_PCICONF_ACSB);
	(*pci->write_pci_config)(card->info.bus, card->info.device, 
		card->info.function, AUVIA_PCICONF_JUNK, 4, pr );
	snooze(100); 
	pr = (*pci->read_pci_config)(card->info.bus, card->info.device, 
		card->info.function, AUVIA_PCICONF_JUNK, 4);
	PRINT(("AUVIA_PCICONF_JUNK after: %" B_PRIx32 "\n", pr));

	if(IS_8233(&card->config)) {
		card->interrupt_mask = 
			AUVIA_8233_SGD_STAT_FLAG_EOL |
			AUVIA_8233_SGD_STAT_FLAG_EOL << 4 |
			AUVIA_8233_SGD_STAT_FLAG_EOL << 8 |
			AUVIA_8233_SGD_STAT_FLAG_EOL << 12 |
			AUVIA_8233_SGD_STAT_FLAG_EOL << 16 |
			AUVIA_8233_SGD_STAT_FLAG_EOL << 24 |
			AUVIA_8233_SGD_STAT_FLAG_EOL << 28;
	} else {
		card->interrupt_mask = AUVIA_SGD_STAT_ALL | (AUVIA_SGD_STAT_ALL << 4);
	}

	
	/* Init streams list */
	LIST_INIT(&(card->streams));
	
	/* Init mems list */
	LIST_INIT(&(card->mems));
	
	return B_OK;
}


static void
auvia_shutdown(auvia_dev *card)
{
	PRINT(("shutdown(%p)\n", card));
	ac97_detach(card->config.ac97);
	remove_io_interrupt_handler(card->config.irq, auvia_int, card);
}


static status_t
auvia_setup(auvia_dev * card)
{
	status_t err = B_OK;
	unsigned char cmd;
	
	PRINT(("auvia_setup(%p)\n", card));

	make_device_names(card);
	
	card->config.subvendor_id = card->info.u.h0.subsystem_vendor_id;
	card->config.subsystem_id = card->info.u.h0.subsystem_id;
	card->config.nabmbar = card->info.u.h0.base_registers[0];
	card->config.irq = card->info.u.h0.interrupt_line;
	card->config.type = 0;
	if(card->info.device_id == VIATECH_82C686_AC97_DEVICE_ID)
		card->config.type |= TYPE_686;
	if(card->info.device_id == VIATECH_8233_AC97_DEVICE_ID)
		card->config.type |= TYPE_8233;
	
	PRINT(("%s deviceid = %#04x chiprev = %x model = %x enhanced "
		"at %" B_PRIx32 "\n",
		card->name, card->info.device_id, card->info.revision,
		card->info.u.h0.subsystem_id, card->config.nabmbar));
		
	cmd = (*pci->read_pci_config)(card->info.bus, card->info.device, 
		card->info.function, PCI_command, 2);
	PRINT(("PCI command before: %x\n", cmd));
	(*pci->write_pci_config)(card->info.bus, card->info.device, 
		card->info.function, PCI_command, 2, cmd | PCI_command_io);
	cmd = (*pci->read_pci_config)(card->info.bus, card->info.device, 
		card->info.function, PCI_command, 2);
	PRINT(("PCI command after: %x\n", cmd));

	/* attach the codec */
	PRINT(("codec attach\n"));
	ac97_attach(&card->config.ac97, (codec_reg_read)auvia_codec_read,
		(codec_reg_write)auvia_codec_write, &card->config,
		card->config.subvendor_id, card->config.subsystem_id);

	PRINT(("installing interrupt : %" B_PRIx32 "\n", card->config.irq));
	err = install_io_interrupt_handler(card->config.irq, auvia_int, card, 0);
	if (err != B_OK) {
		PRINT(("failed to install interrupt\n"));
		ac97_detach(card->config.ac97);
		return err;
	}
		
	if ((err = auvia_init(card))) {
		auvia_shutdown(card);
		return err;
	}
		
	PRINT(("init_driver done\n"));

	return err;
}


status_t
init_driver(void)
{
	pci_info info;
	status_t err;
	int ix = 0;
	num_cards = 0;

	PRINT(("init_driver()\n"));

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci))
		return ENOSYS;
		
	while ((*pci->get_nth_pci_info)(ix++, &info) == B_OK) {
		if (info.vendor_id == VIATECH_VENDOR_ID &&
			(info.device_id == VIATECH_82C686_AC97_DEVICE_ID 
			|| info.device_id == VIATECH_8233_AC97_DEVICE_ID
			)) {
			if (num_cards == NUM_CARDS) {
				PRINT(("Too many auvia cards installed!\n"));
				break;
			}
			memset(&cards[num_cards], 0, sizeof(auvia_dev));
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
			if (auvia_setup(&cards[num_cards])) {
				PRINT(("Setup of auvia %" B_PRId32 " failed\n", num_cards + 1));
#ifdef __HAIKU__
				(*pci->unreserve_device)(info.bus, info.device, info.function,
					DRIVER_NAME, &cards[num_cards]);
#endif
			}
			else {
				num_cards++;
			}
		}
	}
	if (!num_cards) {
		PRINT(("no cards\n"));
		put_module(B_PCI_MODULE_NAME);
		PRINT(("no suitable cards found\n"));
		return ENODEV;
	}

	
#if DEBUG
	//add_debugger_command("auvia", auvia_debug, "auvia [card# (1-n)]");
#endif
	return B_OK;
}


void
uninit_driver(void)
{
	int ix, cnt = num_cards;
	num_cards = 0;

	PRINT(("uninit_driver()\n"));
	//remove_debugger_command("auvia", auvia_debug);

	for (ix=0; ix<cnt; ix++) {
		auvia_shutdown(&cards[ix]);
#ifdef __HAIKU__
		(*pci->unreserve_device)(cards[ix].info.bus,
			cards[ix].info.device, cards[ix].info.function,
			DRIVER_NAME, &cards[ix]);
#endif
	}
	memset(&cards, 0, sizeof(cards));
	put_module(B_PCI_MODULE_NAME);
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

	PRINT(("find_device(%s)\n", name));

	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(cards[ix].name, name)) {
			return &multi_hooks;
		}
	}
	PRINT(("find_device(%s) failed\n", name));
	return NULL;
}

int32	api_version = B_CUR_DRIVER_API_VERSION;
