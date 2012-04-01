/*
 * ES1370 Haiku Driver for ES1370 audio
 *
 * Copyright 2002-2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jerome Duval, jerome.duval@free.fr
 */
 
#include <KernelExport.h>
#include <PCI.h>
#include <driver_settings.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "es1370.h"
#include "debug.h"
#include "config.h"
#include "util.h"
#include "io.h"
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>

status_t init_hardware(void);
status_t init_driver(void);
void uninit_driver(void);
const char ** publish_devices(void);
device_hooks * find_device(const char *);
status_t es1370_init(es1370_dev * card);

static char pci_name[] = B_PCI_MODULE_NAME;
pci_module_info	*pci;

int32 num_cards;
es1370_dev cards[NUM_CARDS];
int32 num_names;
char * names[NUM_CARDS*20+1];

extern device_hooks multi_hooks;

es1370_settings current_settings = {
	44100,	// sample rate
	512,	// buffer frames
	2,	// buffer count
};


/* es1370 Memory management */

static es1370_mem *
es1370_mem_new(es1370_dev *card, size_t size)
{
	es1370_mem *mem;

	if ((mem = malloc(sizeof(*mem))) == NULL)
		return (NULL);

	mem->area = alloc_mem(&mem->phy_base, &mem->log_base, size, "es1370 buffer");
	mem->size = size;
	if (mem->area < B_OK) {
		free(mem);
		return NULL;
	}
	return mem;
}

static void
es1370_mem_delete(es1370_mem *mem)
{
	if(mem->area > B_OK)
		delete_area(mem->area);
	free(mem);
}

static void *
es1370_mem_alloc(es1370_dev *card, size_t size)
{
	es1370_mem *mem;
	
	mem = es1370_mem_new(card, size);
	if (mem == NULL)
		return (NULL);

	LIST_INSERT_HEAD(&(card->mems), mem, next);

	return mem;
}

static void
es1370_mem_free(es1370_dev *card, void *ptr)
{
	es1370_mem 		*mem;
	
	LIST_FOREACH(mem, &card->mems, next) {
		if (mem->log_base != ptr)
			continue;
		LIST_REMOVE(mem, next);
		
		es1370_mem_delete(mem);
		break;
	}
}

/*	es1370 stream functions */

status_t
es1370_stream_set_audioparms(es1370_stream *stream, uint8 channels,
     uint8 b16, uint32 sample_rate)
{
	uint8 			sample_size, frame_size;
	LOG(("es1370_stream_set_audioparms\n"));

	if ((stream->channels == channels) &&
		(stream->b16 == b16) && 
		(stream->sample_rate == sample_rate))
		return B_OK;
	
	if(stream->buffer)
		es1370_mem_free(stream->card, stream->buffer->log_base);
		
	stream->b16 = b16;
	stream->sample_rate = sample_rate;
	stream->channels = channels;
	
	sample_size = stream->b16 + 1;
	frame_size = sample_size * stream->channels;
	
	stream->buffer = es1370_mem_alloc(stream->card, stream->bufframes * frame_size * stream->bufcount);
	
	stream->trigblk = 0;	/* This shouldn't be needed */
	stream->blkmod = stream->bufcount;
	stream->blksize = stream->bufframes * frame_size;
		
	return B_OK;
}

status_t
es1370_stream_commit_parms(es1370_stream *stream)
{
	uint8 			sample_size, frame_size;
	uint32	ctrl;
	es1370_dev *card = stream->card;
	LOG(("es1370_stream_commit_parms\n"));
	
	ctrl = es1370_reg_read_32(&card->config, ES1370_REG_CONTROL) & ~CTRL_PCLKDIV;
	ctrl |= DAC2_SRTODIV((uint16)stream->sample_rate) << CTRL_SH_PCLKDIV;
	es1370_reg_write_32(&card->config, ES1370_REG_CONTROL, ctrl);
	
	sample_size = stream->b16 + 1;
	frame_size = sample_size * stream->channels;

	if (stream->use & ES1370_USE_RECORD) {
		es1370_reg_write_32(&card->config, ES1370_REG_MEMPAGE, 0xd);
		es1370_reg_write_32(&card->config, ES1370_REG_ADC_FRAMEADR & 0xff, (uint32)stream->buffer->phy_base);
		es1370_reg_write_32(&card->config, ES1370_REG_ADC_FRAMECNT & 0xff, ((stream->blksize * stream->bufcount) >> 2) - 1);
		es1370_reg_write_32(&card->config, ES1370_REG_ADC_SCOUNT & 0xff, stream->bufframes - 1);
	} else {
		es1370_reg_write_32(&card->config, ES1370_REG_MEMPAGE, 0xc);
		es1370_reg_write_32(&card->config, ES1370_REG_DAC2_FRAMEADR & 0xff, (uint32)stream->buffer->phy_base);
		es1370_reg_write_32(&card->config, ES1370_REG_DAC2_FRAMECNT & 0xff, ((stream->blksize * stream->bufcount) >> 2) - 1);
		es1370_reg_write_32(&card->config, ES1370_REG_DAC2_SCOUNT & 0xff, stream->bufframes - 1);
		LOG(("es1370_stream_commit_parms %ld %ld\n", ((stream->blksize * stream->bufcount) >> 2) - 1, (stream->blksize / frame_size) - 1));
	}

	return B_OK;
}

status_t
es1370_stream_get_nth_buffer(es1370_stream *stream, uint8 chan, uint8 buf, 
					char** buffer, size_t *stride)
{
	uint8 			sample_size, frame_size;
	LOG(("es1370_stream_get_nth_buffer\n"));
	
	sample_size = stream->b16 + 1;
	frame_size = sample_size * stream->channels;
	
	*buffer = stream->buffer->log_base + (buf * stream->bufframes * frame_size)
		+ chan * sample_size;
	*stride = frame_size;
	
	return B_OK;
}

static uint32
es1370_stream_curaddr(es1370_stream *stream)
{
	es1370_dev *card = stream->card;
	uint32 reg = 0, cnt = 0;
	if (stream->use & ES1370_USE_RECORD) {
		reg = ES1370_REG_ADC_FRAMECNT;
	} else {
		reg = ES1370_REG_DAC2_FRAMECNT;
	}
	es1370_reg_write_32(&card->config, ES1370_REG_MEMPAGE, reg >> 8);
	cnt = es1370_reg_read_32(&card->config, reg & 0xff) >> 16;
	//TRACE(("stream_curaddr %lx\n", (cnt << 2) / stream->blksize));
	return (cnt << 2) / stream->blksize;
}

void
es1370_stream_start(es1370_stream *stream, void (*inth) (void *), void *inthparam)
{
	uint32 sctrl = 0, ctrl = 0;
	es1370_dev *card = stream->card;
	LOG(("es1370_stream_start\n"));
	
	stream->inth = inth;
	stream->inthparam = inthparam;
		
	stream->state |= ES1370_STATE_STARTED;

	sctrl = es1370_reg_read_32(&card->config, ES1370_REG_SERIAL_CONTROL);
	ctrl = es1370_reg_read_32(&card->config, ES1370_REG_CONTROL);
	
	if (stream->use & ES1370_USE_RECORD) {
		sctrl &= ~(SCTRL_R1SEB | SCTRL_R1SMB);
		if (stream->b16)
			sctrl |= SCTRL_R1SEB;
		if (stream->channels == 2)
			sctrl |= SCTRL_R1SMB;
		es1370_reg_write_32(&card->config, ES1370_REG_SERIAL_CONTROL, sctrl & ~SCTRL_R1INTEN);
		es1370_reg_write_32(&card->config, ES1370_REG_SERIAL_CONTROL, sctrl | SCTRL_R1INTEN);

		ctrl |= CTRL_ADC_EN;
	} else {
		sctrl &= ~(SCTRL_P2SEB | SCTRL_P2SMB | 0x003f0000);
		if (stream->b16)
			sctrl |= SCTRL_P2SEB;
		if (stream->channels == 2)
			sctrl |= SCTRL_P2SMB;
		sctrl |= (stream->b16 + 1) << SCTRL_SH_P2ENDINC;
		es1370_reg_write_32(&card->config, ES1370_REG_SERIAL_CONTROL, sctrl & ~SCTRL_P2INTEN);
		es1370_reg_write_32(&card->config, ES1370_REG_SERIAL_CONTROL, sctrl | SCTRL_P2INTEN);

		ctrl |= CTRL_DAC2_EN;
	}

	es1370_reg_write_32(&card->config, ES1370_REG_SERIAL_CONTROL, sctrl);
	es1370_reg_write_32(&card->config, ES1370_REG_CONTROL, ctrl);
	
#ifdef DEBUG
	//dump_hardware_regs(&stream->card->config);
#endif
}

void
es1370_stream_halt(es1370_stream *stream)
{
	uint32 ctrl;
	es1370_dev *card = stream->card;
	LOG(("es1370_stream_halt\n"));
			
	stream->state &= ~ES1370_STATE_STARTED;

	ctrl = es1370_reg_read_32(&card->config, ES1370_REG_CONTROL);
	if (stream->use & ES1370_USE_RECORD)
		ctrl &= ~CTRL_ADC_EN;
	else
		ctrl &= ~CTRL_DAC2_EN;
	es1370_reg_write_32(&card->config, ES1370_REG_CONTROL, ctrl);
}

es1370_stream *
es1370_stream_new(es1370_dev *card, uint8 use, uint32 bufframes, uint8 bufcount)
{
	es1370_stream *stream;
	cpu_status status;
	LOG(("es1370_stream_new\n"));

	stream = malloc(sizeof(es1370_stream));
	if (stream == NULL)
		return (NULL);
	stream->card = card;
	stream->use = use;
	stream->state = !ES1370_STATE_STARTED;
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
	
	stream->buffer_cycle = 0;
	stream->frames_count = 0;
	stream->real_time = 0;
	stream->update_needed = false;
	
	status = lock();
	LIST_INSERT_HEAD((&card->streams), stream, next);
	unlock(status);
	
	return stream;
}

void
es1370_stream_delete(es1370_stream *stream)
{
	cpu_status status;
	LOG(("es1370_stream_delete\n"));
	
	es1370_stream_halt(stream);
			
	if(stream->buffer)
		es1370_mem_free(stream->card, stream->buffer->log_base);
	
	status = lock();
	LIST_REMOVE(stream, next);
	unlock(status);
	
	free(stream);
}

/* es1370 interrupt */

static int32 
es1370_int(void *arg)
{
	es1370_dev	 	*card = arg;
	bool 			gotone 	= false;
	uint32       	curblk;
	es1370_stream 	*stream = NULL;
	uint32		sta, sctrl;
	
	// TRACE(("es1370_int(%p)\n", card));
	
	sta = es1370_reg_read_32(&card->config, ES1370_REG_STATUS);
	if (sta & card->interrupt_mask) {
		
		//TRACE(("interrupt !! %x\n", sta));
		sctrl = es1370_reg_read_32(&card->config, ES1370_REG_SERIAL_CONTROL);
		
		LIST_FOREACH(stream, &card->streams, next) {
			if (stream->use & ES1370_USE_RECORD) {
				if ((sta & STAT_ADC) == 0)
					continue;
				es1370_reg_write_32(&card->config, ES1370_REG_SERIAL_CONTROL, sctrl & ~SCTRL_R1INTEN);
				es1370_reg_write_32(&card->config, ES1370_REG_SERIAL_CONTROL, sctrl | SCTRL_R1INTEN);
			} else {
				if ((sta & STAT_DAC2) == 0)
					continue;
				es1370_reg_write_32(&card->config, ES1370_REG_SERIAL_CONTROL, sctrl & ~SCTRL_P2INTEN);
				es1370_reg_write_32(&card->config, ES1370_REG_SERIAL_CONTROL, sctrl | SCTRL_P2INTEN);
			}

			curblk = es1370_stream_curaddr(stream);
			// TRACE(("INTR at trigblk %lu, stream->trigblk %lu\n", curblk, stream->trigblk));
			if (curblk == stream->trigblk) {
				stream->trigblk++;
				stream->trigblk = stream->trigblk % stream->blkmod;
				if (stream->inth)
					stream->inth(stream->inthparam);
			}
			gotone = true;
		}
	} else {
		//TRACE(("interrupt masked %x, ", card->interrupt_mask));
		//TRACE(("sta %x\n", sta));
	}
	
	if (gotone)
		return B_INVOKE_SCHEDULER;

	//TRACE(("Got unhandled interrupt\n"));
	return B_UNHANDLED_INTERRUPT;
}


/*	es1370 driver functions */


/* detect presence of our hardware */
status_t 
init_hardware(void)
{
	int ix=0;
	pci_info info;
	status_t err = ENODEV;
	
	LOG_CREATE();

	PRINT(("init_hardware()\n"));

	if (get_module(pci_name, (module_info **)&pci))
		return ENOSYS;

	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if (info.vendor_id == 0x1274 
			&& (info.device_id == 0x5000
			/*|| info.device_id == 0x1371
			|| info.device_id == 0x5880*/)
			)
		 {
			err = B_OK;
		}
		ix++;
	}
		
	put_module(pci_name);

	return err;
}

static void
make_device_names(
	es1370_dev * card)
{
	sprintf(card->name, "audio/hmulti/es1370/%ld", card-cards+1);
	names[num_names++] = card->name;

	names[num_names] = NULL;
}


status_t
es1370_init(es1370_dev * card)
{
	card->interrupt_mask = STAT_DAC2 | STAT_ADC;
		
	/* Init streams list */
	LIST_INIT(&(card->streams));
	
	/* Init mems list */
	LIST_INIT(&(card->mems));
	
	return B_OK;
}

static status_t
es1370_setup(es1370_dev * card)
{
	status_t err = B_OK;
	unsigned char cmd;
	
	PRINT(("es1370_setup(%p)\n", card));

	make_device_names(card);
	
	card->config.base = card->info.u.h0.base_registers[0];
	card->config.irq = card->info.u.h0.interrupt_line;
	card->config.type = 0;
	
	PRINT(("%s deviceid = %#04x chiprev = %x model = %x enhanced at %lx\n", card->name, card->info.device_id,
		card->info.revision, card->info.u.h0.subsystem_id, card->config.base));

	cmd = (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, PCI_command, 2);
	PRINT(("PCI command before: %x\n", cmd));
	(*pci->write_pci_config)(card->info.bus, card->info.device, card->info.function, PCI_command, 2, cmd | PCI_command_io);
	cmd = (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, PCI_command, 2);
	PRINT(("PCI command after: %x\n", cmd));
	
	es1370_reg_write_32(&card->config, ES1370_REG_SERIAL_CONTROL, SCTRL_P2INTEN | SCTRL_R1INTEN);
	es1370_reg_write_32(&card->config, ES1370_REG_CONTROL, CTRL_CDC_EN);
	
	/* reset the codec */	
	PRINT(("codec reset\n"));
	es1370_codec_write(&card->config, CODEC_RESET_PWRDWN, 0x2);
	snooze (20);
	es1370_codec_write(&card->config, CODEC_RESET_PWRDWN, 0x3);
	snooze (20);
	es1370_codec_write(&card->config, CODEC_CLOCK_SEL, 0x0);

	/* set max volume on master and mixer outputs */
	es1370_codec_write(&card->config, CODEC_MASTER_VOL_L, 0x0);
	es1370_codec_write(&card->config, CODEC_MASTER_VOL_R, 0x0);
	es1370_codec_write(&card->config, CODEC_VOICE_VOL_L, 0x0);
	es1370_codec_write(&card->config, CODEC_VOICE_VOL_R, 0x0);

	/* unmute CD playback */
	es1370_codec_write(&card->config, CODEC_OUTPUT_MIX1, ES1370_OUTPUT_MIX1_CDL | ES1370_OUTPUT_MIX1_CDR);
	/* unmute mixer output */
	es1370_codec_write(&card->config, CODEC_OUTPUT_MIX2, ES1370_OUTPUT_MIX2_VOICEL | ES1370_OUTPUT_MIX2_VOICER);

	snooze(50000); // 50 ms

	PRINT(("installing interrupt : %lx\n", card->config.irq));
	err = install_io_interrupt_handler(card->config.irq, es1370_int, card, 0);
	if (err != B_OK) {
		PRINT(("failed to install interrupt\n"));
		return err;
	}
		
	if ((err = es1370_init(card)))
		return (err);
		
	PRINT(("init_driver done\n"));

	return err;
}


status_t
init_driver(void)
{
	void *settings_handle;
	pci_info info;
	int ix = 0;
	status_t err;
	num_cards = 0;

	PRINT(("init_driver()\n"));

	// get driver settings
	settings_handle  = load_driver_settings ("es1370.settings");
	if (settings_handle != NULL) {
		const char *item;
		char       *end;
		uint32      value;

		item = get_driver_parameter (settings_handle, "sample_rate", "44100", "44100");
		value = strtoul (item, &end, 0);
		if (*end == '\0') current_settings.sample_rate = value;

		item = get_driver_parameter (settings_handle, "buffer_frames", "512", "512");
		value = strtoul (item, &end, 0);
		if (*end == '\0') current_settings.buffer_frames = value;

		item = get_driver_parameter (settings_handle, "buffer_count", "2", "2");
		value = strtoul (item, &end, 0);
		if (*end == '\0') current_settings.buffer_count = value;

		unload_driver_settings (settings_handle);
	}

	if (get_module(pci_name, (module_info **) &pci))
		return ENOSYS;
		
	while ((*pci->get_nth_pci_info)(ix++, &info) == B_OK) {
		if (info.vendor_id == 0x1274
                        && (info.device_id == 0x5000
                        /*|| info.device_id == 0x1371
                        || info.device_id == 0x5880*/)
			) {
			if (num_cards == NUM_CARDS) {
				PRINT(("Too many es1370 cards installed!\n"));
				break;
			}
			memset(&cards[num_cards], 0, sizeof(es1370_dev));
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
			if (es1370_setup(&cards[num_cards])) {
				PRINT(("Setup of es1370 %ld failed\n", num_cards+1));
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
		put_module(pci_name);
		PRINT(("no suitable cards found\n"));
		return ENODEV;
	}

	return B_OK;
}


static void
es1370_shutdown(es1370_dev *card)
{
	PRINT(("shutdown(%p)\n", card));
	//ac97_amp_enable(&card->config, false);
	card->interrupt_mask = 0;
	
	remove_io_interrupt_handler(card->config.irq, es1370_int, card);
}


void
uninit_driver(void)
{
	int ix, cnt = num_cards;
	num_cards = 0;

	PRINT(("uninit_driver()\n"));
	for (ix=0; ix<cnt; ix++) {
		es1370_shutdown(&cards[ix]);
#ifdef __HAIKU__
		(*pci->unreserve_device)(cards[ix].info.bus,
			cards[ix].info.device, cards[ix].info.function,
			DRIVER_NAME, &cards[ix]);
#endif
	}
	memset(&cards, 0, sizeof(cards));
	put_module(pci_name);
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

