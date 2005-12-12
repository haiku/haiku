/*
 * Auich BeOS Driver for Intel Southbridge audio
 *
 * Copyright (c) 2003, Jerome Duval (jerome.duval@free.fr)
 
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
#include "auich.h"
#include "debug.h"
#include "config.h"
#include "util.h"
#include "io.h"
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include "ac97.h"

status_t init_hardware(void);
status_t init_driver(void);
void uninit_driver(void);
const char ** publish_devices(void);
device_hooks * find_device(const char *);
int32 auich_int(void *arg);
status_t auich_init(auich_dev * card);

static char pci_name[] = B_PCI_MODULE_NAME;
pci_module_info	*pci;

int32 num_cards;
auich_dev cards[NUM_CARDS];
int32 num_names;
char * names[NUM_CARDS*20+1];

extern device_hooks multi_hooks;

/* The SIS7012 chipset has SR and PICB registers swapped when compared to Intel */
#define	GET_REG_PICB(x)		(IS_SIS7012(x) ? AUICH_REG_X_SR : AUICH_REG_X_PICB)
#define	GET_REG_SR(x)		(IS_SIS7012(x) ? AUICH_REG_X_PICB : AUICH_REG_X_SR)

static void 
dump_hardware_regs(device_config *config)
{
	LOG(("GLOB_CNT = %#08x\n", auich_reg_read_32(config, AUICH_REG_GLOB_CNT)));
	LOG(("GLOB_STA = %#08x\n", auich_reg_read_32(config, AUICH_REG_GLOB_STA)));
	LOG(("PI AUICH_REG_X_BDBAR = %#x\n", auich_reg_read_32(config, AUICH_REG_X_BDBAR + AUICH_REG_PI_BASE)));
	LOG(("PI AUICH_REG_X_CIV = %#x\n", auich_reg_read_8(config, AUICH_REG_X_CIV + AUICH_REG_PI_BASE)));
	LOG(("PI AUICH_REG_X_LVI = %#x\n", auich_reg_read_8(config, AUICH_REG_X_LVI + AUICH_REG_PI_BASE)));
	LOG(("PI     REG_X_SR = %#x\n", auich_reg_read_16(config, AUICH_REG_X_SR + AUICH_REG_PI_BASE)));
	LOG(("PI     REG_X_PICB = %#x\n", auich_reg_read_16(config, AUICH_REG_X_PICB + AUICH_REG_PI_BASE)));
	LOG(("PI AUICH_REG_X_PIV = %#x\n", auich_reg_read_8(config, AUICH_REG_X_PIV + AUICH_REG_PI_BASE)));
	LOG(("PI AUICH_REG_X_CR = %#x\n", auich_reg_read_8(config, AUICH_REG_X_CR + AUICH_REG_PI_BASE)));
	LOG(("PO AUICH_REG_X_BDBAR = %#x\n", auich_reg_read_32(config, AUICH_REG_X_BDBAR + AUICH_REG_PO_BASE)));
	LOG(("PO AUICH_REG_X_CIV = %#x\n", auich_reg_read_8(config, AUICH_REG_X_CIV + AUICH_REG_PO_BASE)));
	LOG(("PO AUICH_REG_X_LVI = %#x\n", auich_reg_read_8(config, AUICH_REG_X_LVI + AUICH_REG_PO_BASE)));
	LOG(("PO     REG_X_SR = %#x\n", auich_reg_read_16(config, AUICH_REG_X_SR + AUICH_REG_PO_BASE)));
	LOG(("PO     REG_X_PICB = %#x\n", auich_reg_read_16(config, AUICH_REG_X_PICB + AUICH_REG_PO_BASE)));
	LOG(("PO AUICH_REG_X_PIV = %#x\n", auich_reg_read_8(config, AUICH_REG_X_PIV + AUICH_REG_PO_BASE)));
	LOG(("PO AUICH_REG_X_CR = %#x\n", auich_reg_read_8(config, AUICH_REG_X_CR + AUICH_REG_PO_BASE)));
}

/* auich Memory management */

static auich_mem *
auich_mem_new(auich_dev *card, size_t size)
{
	auich_mem *mem;

	if ((mem = malloc(sizeof(*mem))) == NULL)
		return (NULL);

	mem->area = alloc_mem(&mem->phy_base, &mem->log_base, size, "auich buffer");
	mem->size = size;
	if (mem->area < B_OK) {
		free(mem);
		return NULL;
	}
	return mem;
}

static void
auich_mem_delete(auich_mem *mem)
{
	if(mem->area > B_OK)
		delete_area(mem->area);
	free(mem);
}

static void *
auich_mem_alloc(auich_dev *card, size_t size)
{
	auich_mem *mem;
	
	mem = auich_mem_new(card, size);
	if (mem == NULL)
		return (NULL);

	LIST_INSERT_HEAD(&(card->mems), mem, next);

	return mem;
}

static void
auich_mem_free(auich_dev *card, void *ptr)
{
	auich_mem 		*mem;
	
	LIST_FOREACH(mem, &card->mems, next) {
		if (mem->log_base != ptr)
			continue;
		LIST_REMOVE(mem, next);
		
		auich_mem_delete(mem);
		break;
	}
}

/*	auich stream functions */

status_t
auich_stream_set_audioparms(auich_stream *stream, uint8 channels,
     uint8 b16, uint32 sample_rate)
{
	uint8 			sample_size, frame_size;
	LOG(("auich_stream_set_audioparms\n"));

	if ((stream->channels == channels) &&
		(stream->b16 == b16) && 
		(stream->sample_rate == sample_rate))
		return B_OK;
	
	if(stream->buffer)
		auich_mem_free(stream->card, stream->buffer->log_base);
		
	stream->b16 = b16;
	stream->sample_rate = sample_rate;
	stream->channels = channels;
	
	sample_size = stream->b16 + 1;
	frame_size = sample_size * stream->channels;
	
	stream->buffer = auich_mem_alloc(stream->card, stream->bufframes * frame_size * stream->bufcount);
	
	stream->trigblk = 0;	/* This shouldn't be needed */
	stream->blkmod = stream->bufcount;
	stream->blksize = stream->bufframes * frame_size;
		
	return B_OK;
}

status_t
auich_stream_commit_parms(auich_stream *stream)
{
	uint32      	*page;
	uint32 			i;
	LOG(("auich_stream_commit_parms\n"));
	
	auich_reg_write_8(&stream->card->config, stream->base + AUICH_REG_X_CR, 0);
	snooze(10000); // 10 ms

	auich_reg_write_8(&stream->card->config, stream->base + AUICH_REG_X_CR, CR_RR);
	for (i = 10000; i>=0; i--) {
		if(0 == auich_reg_read_8(&stream->card->config, stream->base + AUICH_REG_X_CR)) {
			LOG(("channel reset finished, %x, %d\n", stream->base, i));
			break;
		}
		snooze(1);
	}
	
	if(i < 0) {
		LOG(("channel reset failed after 10ms\n"));
	}
	
	page = stream->dmaops_log_base;
	
	for(i=0; i < AUICH_DMALIST_MAX; i++) {
		page[2*i] = ((uint32)stream->buffer->phy_base) + 
			(i % stream->bufcount) * stream->blksize;
		page[2*i + 1] = AUICH_DMAF_IOC | (stream->blksize 
			/ (IS_SIS7012(&stream->card->config) ? 1 : 2));
	}
			
	// set physical buffer descriptor base address
	auich_reg_write_32(&stream->card->config, stream->base + AUICH_REG_X_BDBAR, 
		(uint32)stream->dmaops_phy_base);
		
	if(stream->use & AUICH_USE_RECORD)
		auich_codec_write(&stream->card->config, AC97_PCM_LR_ADC_RATE, (uint16)stream->sample_rate);
	else
		auich_codec_write(&stream->card->config, AC97_PCM_FRONT_DAC_RATE, (uint16)stream->sample_rate);
	
	if(stream->use & AUICH_USE_RECORD)
		LOG(("rate : %d\n", auich_codec_read(&stream->card->config, AC97_PCM_LR_ADC_RATE)));
	else
		LOG(("rate : %d\n", auich_codec_read(&stream->card->config, AC97_PCM_FRONT_DAC_RATE)));
	return B_OK;
}

status_t
auich_stream_get_nth_buffer(auich_stream *stream, uint8 chan, uint8 buf, 
					char** buffer, size_t *stride)
{
	uint8 			sample_size, frame_size;
	LOG(("auich_stream_get_nth_buffer\n"));
	
	sample_size = stream->b16 + 1;
	frame_size = sample_size * stream->channels;
	
	*buffer = stream->buffer->log_base + (buf * stream->bufframes * frame_size)
		+ chan * sample_size;
	*stride = frame_size;
	
	return B_OK;
}

static uint8
auich_stream_curaddr(auich_stream *stream)
{
	uint8 index = auich_reg_read_8(&stream->card->config, stream->base + AUICH_REG_X_CIV);
	TRACE(("stream_curaddr %d\n", index));
	return index;
}

void
auich_stream_start(auich_stream *stream, void (*inth) (void *), void *inthparam)
{
	int32 civ;
	LOG(("auich_stream_start\n"));
	
	stream->inth = inth;
	stream->inthparam = inthparam;
		
	stream->state |= AUICH_STATE_STARTED;
	
	civ = auich_reg_read_8(&stream->card->config, stream->base + AUICH_REG_X_CIV);
	
	// step 1: clear status bits
	auich_reg_write_16(&stream->card->config, 
		stream->base + GET_REG_SR(&stream->card->config), 
		auich_reg_read_16(&stream->card->config, stream->base + GET_REG_SR(&stream->card->config)));
	auich_reg_read_16(&stream->card->config, stream->base + GET_REG_SR(&stream->card->config));
	// step 2: prepare buffer transfer
	auich_reg_write_8(&stream->card->config, stream->base + AUICH_REG_X_LVI, (civ + 2) % AUICH_DMALIST_MAX);
	auich_reg_read_8(&stream->card->config, stream->base + AUICH_REG_X_LVI);
	// step 3: enable interrupts & busmaster transfer
	auich_reg_write_8(&stream->card->config, stream->base + AUICH_REG_X_CR, CR_RPBM | CR_LVBIE | CR_FEIE | CR_IOCE);
	auich_reg_read_8(&stream->card->config, stream->base + AUICH_REG_X_CR);
	
#ifdef DEBUG
	dump_hardware_regs(&stream->card->config);
#endif
}

void
auich_stream_halt(auich_stream *stream)
{
	LOG(("auich_stream_halt\n"));
			
	stream->state &= ~AUICH_STATE_STARTED;
	
	auich_reg_write_8(&stream->card->config, stream->base + AUICH_REG_X_CR, 
		auich_reg_read_8(&stream->card->config, stream->base + AUICH_REG_X_CR) & ~CR_RPBM);
}

auich_stream *
auich_stream_new(auich_dev *card, uint8 use, uint32 bufframes, uint8 bufcount)
{
	auich_stream *stream;
	cpu_status status;
	LOG(("auich_stream_new\n"));

	stream = malloc(sizeof(auich_stream));
	if (stream == NULL)
		return (NULL);
	stream->card = card;
	stream->use = use;
	stream->state = !AUICH_STATE_STARTED;
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
	
	if(use & AUICH_USE_PLAY) {
		stream->base = AUICH_REG_PO_BASE;
		stream->sta = STA_POINT;
	} else {
		stream->base = AUICH_REG_PI_BASE;
		stream->sta = STA_PIINT;
	}
		
	stream->frames_count = 0;
	stream->real_time = 0;
	stream->update_needed = false;
	
	/* allocate memory for our dma ops */
	stream->dmaops_area = alloc_mem(&stream->dmaops_phy_base, &stream->dmaops_log_base, 
		sizeof(auich_dmalist) * AUICH_DMALIST_MAX, "auich dmaops");
		
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
auich_stream_delete(auich_stream *stream)
{
	cpu_status status;
	int32 i;
	LOG(("auich_stream_delete\n"));
	
	auich_stream_halt(stream);
	
	auich_reg_write_8(&stream->card->config, stream->base + AUICH_REG_X_CR, 0);
	snooze(10000); // 10 ms

	auich_reg_write_8(&stream->card->config, stream->base + AUICH_REG_X_CR, CR_RR);
	for (i = 10000; i>=0; i--) {
		if(0 == auich_reg_read_8(&stream->card->config, stream->base + AUICH_REG_X_CR)) {
			LOG(("channel reset finished, %x, %d\n", stream->base, i));
			break;
		}
		snooze(1);
	}
	
	if(i < 0) {
		LOG(("channel reset failed after 10ms\n"));
	}
	
	auich_reg_write_32(&stream->card->config, stream->base + AUICH_REG_X_BDBAR, 0);
	
	if (stream->dmaops_area > B_OK)
		delete_area(stream->dmaops_area);
		
	if(stream->buffer)
		auich_mem_free(stream->card, stream->buffer->log_base);
	
	status = lock();
	LIST_REMOVE(stream, next);
	unlock(status);
	
	free(stream);
}

/* auich interrupt */

int32 
auich_int(void *arg)
{
	auich_dev	 	*card = arg;
	bool 			gotone 	= false;
	uint8       	curblk;
	auich_stream 	*stream = NULL;
	uint16 			sr, sta;
	
	//TRACE(("auich_int(%p)\n", card));
	
	sta = auich_reg_read_16(&card->config, AUICH_REG_GLOB_STA);
	if(sta & card->interrupt_mask) {
		
		if (sta & (STA_S0RI | STA_S1RI | STA_S2RI)) {
			// ignore and clear resume interrupt(s)
			auich_reg_write_16(&card->config, AUICH_REG_GLOB_STA, sta & (STA_S0RI | STA_S1RI | STA_S2RI));
			TRACE(("interrupt !! %x\n", sta));
			gotone = true;
		}
		
		//TRACE(("interrupt !! %x\n", sta));
		
		LIST_FOREACH(stream, &card->streams, next) 
			if (sta & stream->sta) {
				sr = auich_reg_read_16(&card->config, 
					stream->base + GET_REG_SR(&stream->card->config));
				sr &= SR_MASK;
				
				if(!sr)
					continue;
				
				gotone = true;
								
				if (sr & SR_BCIS) {
					curblk = auich_stream_curaddr(stream);
									
					auich_reg_write_8(&card->config, stream->base + AUICH_REG_X_LVI, 
						(curblk + 2) % AUICH_DMALIST_MAX);
							
					stream->trigblk = (curblk) % stream->blkmod;
											
					if(stream->inth)
						stream->inth(stream->inthparam);
				} else {
					TRACE(("interrupt !! sta %x, sr %x\n", sta, sr));
				}
								
				auich_reg_write_16(&card->config, 
					stream->base + GET_REG_SR(&stream->card->config), sr);		
			}
	} else {
		TRACE(("interrupt masked %x, ", card->interrupt_mask));
		TRACE(("sta %x\n", sta));
	}
	
	if(gotone)
		return B_HANDLED_INTERRUPT;

	TRACE(("Got unhandled interrupt\n"));
	return B_UNHANDLED_INTERRUPT;
}

/*	auich driver functions */

static status_t 
map_io_memory(device_config *config)
{
	if ((config->type & TYPE_ICH4) == 0)
		return B_OK;
	
	config->area_mmbar = map_mem(&config->log_mmbar, (void *)config->mmbar, ICH4_MMBAR_SIZE, "auich mmbar io");
	if (config->area_mmbar <= B_OK) {
		LOG(("mapping of mmbar io failed, error = %#x\n",config->area_mmbar));
		return B_ERROR;
	}
	LOG(("mapping of mmbar: area %#x, phys %#x, log %#x\n", config->area_mmbar, config->mmbar, config->log_mmbar));

	config->area_mbbar = map_mem(&config->log_mbbar, (void *)config->mbbar, ICH4_MBBAR_SIZE, "auich mbbar io");
	if (config->area_mbbar <= B_OK) {
		LOG(("mapping of mbbar io failed, error = %#x\n",config->area_mbbar));
		delete_area(config->area_mmbar);
		config->area_mmbar = -1;
		return B_ERROR;
	}
	LOG(("mapping of mbbar: area %#x, phys %#x, log %#x\n", config->area_mbbar, config->mbbar, config->log_mbbar));

	return B_OK;
}

static status_t 
unmap_io_memory(device_config *config)
{
	status_t rv;
	if ((config->type & TYPE_ICH4) == 0)
		return B_OK;
	rv  = delete_area(config->area_mmbar);
	rv |= delete_area(config->area_mbbar);
	return rv;
}

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
		if ((info.vendor_id == INTEL_VENDOR_ID &&
			(info.device_id == INTEL_82443MX_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801AA_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801AB_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801BA_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801CA_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801DB_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801EB_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801FB_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801GB_AC97_DEVICE_ID
			|| info.device_id == INTEL_6300ESB_AC97_DEVICE_ID
			))
		|| (info.vendor_id == SIS_VENDOR_ID &&
			(info.device_id == SIS_SI7012_AC97_DEVICE_ID
			))
		|| (info.vendor_id == NVIDIA_VENDOR_ID &&
			(info.device_id == NVIDIA_nForce_AC97_DEVICE_ID
			|| info.device_id == NVIDIA_nForce2_AC97_DEVICE_ID
			|| info.device_id == NVIDIA_nForce3_AC97_DEVICE_ID
			)) 
		|| (info.vendor_id == AMD_VENDOR_ID &&
			(info.device_id == AMD_AMD8111_AC97_DEVICE_ID
			|| info.device_id == AMD_AMD768_AC97_DEVICE_ID
			))
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
	auich_dev * card)
{
	sprintf(card->name, "audio/multi/auich/%ld", card-cards+1);
	names[num_names++] = card->name;

	names[num_names] = NULL;
}


status_t
auich_init(auich_dev * card)
{
	card->interrupt_mask = STA_PIINT | STA_POINT; //STA_INTMASK; 
		
	/* Init streams list */
	LIST_INIT(&(card->streams));
	
	/* Init mems list */
	LIST_INIT(&(card->mems));
	
	return B_OK;
}

static status_t
auich_setup(auich_dev * card)
{
	status_t err = B_OK;
	status_t rv;
	unsigned char cmd;
	
	PRINT(("auich_setup(%p)\n", card));

	make_device_names(card);
	
	card->config.nabmbar = card->info.u.h0.base_registers[0];
	card->config.irq = card->info.u.h0.interrupt_line;
	card->config.type = 0;
	if ((card->info.device_id == INTEL_82801DB_AC97_DEVICE_ID)
		|| (card->info.device_id == INTEL_82801EB_AC97_DEVICE_ID)
		|| (card->info.device_id == INTEL_82801FB_AC97_DEVICE_ID)
		|| (card->info.device_id == INTEL_82801GB_AC97_DEVICE_ID)
		|| (card->info.device_id == INTEL_6300ESB_AC97_DEVICE_ID))
		card->config.type |= TYPE_ICH4;
	if (card->info.device_id == SIS_SI7012_AC97_DEVICE_ID)
		card->config.type |= TYPE_SIS7012;
	
	PRINT(("%s deviceid = %#04x chiprev = %x model = %x enhanced at %lx\n", card->name, card->info.device_id,
		card->info.revision, card->info.u.h0.subsystem_id, card->config.nabmbar));

	if (IS_ICH4(&card->config)) {
		// memory mapped access
		card->config.mmbar = 0xfffffffe & (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, 0x18, 4);
		card->config.mbbar = 0xfffffffe & (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, 0x1C, 4);
	} else {
		// pio access
		card->config.nambar = 0xfffffffe & (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, 0x10, 4);
		card->config.nabmbar = 0xfffffffe & (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, 0x14, 4);
	}

	/* before doing anything else, map the IO memory */
	rv = map_io_memory(&card->config);
	if (rv != B_OK) {
		PRINT(("mapping of memory IO space failed\n"));
		return B_ERROR;
	}
	
	cmd = (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, PCI_command, 2);
	PRINT(("PCI command before: %x\n", cmd));
	(*pci->write_pci_config)(card->info.bus, card->info.device, card->info.function, PCI_command, 2, cmd | PCI_command_io);
	cmd = (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, PCI_command, 2);
	PRINT(("PCI command after: %x\n", cmd));
	
	/* do a cold reset */
	LOG(("cold reset\n"));
	auich_reg_write_32(&card->config, AUICH_REG_GLOB_CNT, 0);
	snooze(50000); // 50 ms
	auich_reg_write_32(&card->config, AUICH_REG_GLOB_CNT, CNT_COLD | CNT_PRIE);
	LOG(("cold reset finished\n"));
	rv = auich_reg_read_32(&card->config, AUICH_REG_GLOB_CNT);
	if ((rv & CNT_COLD) == 0) {
		LOG(("cold reset failed\n"));
	}

	/* reset the codec */	
	PRINT(("codec reset\n"));
	auich_codec_write(&card->config, 0x00, 0x0000);
	snooze(50000); // 50 ms

	ac97_init(&card->config);
	ac97_amp_enable(&card->config, true);
	
	rv = auich_reg_read_32(&card->config, AUICH_REG_GLOB_STA);
	if (!(rv & STA_S0CR)) { /* reset failure */
		/* It never return STA_S0CR in some cases */
		PRINT(("reset failure\n"));
	}
	/* Print capabilities though there are no supports for now */
	if ((rv & STA_SAMPLE_CAP) == STA_POM20) {
		LOG(("20 bit precision support\n"));
	}
	if ((rv & STA_CHAN_CAP) == STA_PCM4) {
		LOG(("4ch PCM output support\n"));
	}
	if ((rv & STA_CHAN_CAP) == STA_PCM6) {
		LOG(("6ch PCM output support\n"));
	}

	PRINT(("codec vendor id      = %#08lx\n",ac97_get_vendor_id(&card->config)));
	PRINT(("codec description     = %s\n",ac97_get_vendor_id_description(&card->config)));
	PRINT(("codec 3d enhancement = %s\n",ac97_get_3d_stereo_enhancement(&card->config)));
	
	PRINT(("installing interrupt : %lx\n", card->config.irq));
	install_io_interrupt_handler(card->config.irq, auich_int, card, 0);
		
	/*PRINT(("codec master output = %#04x\n",auich_codec_read(&card->config, 0x02)));
	PRINT(("codec aux output    = %#04x\n",auich_codec_read(&card->config, 0x04)));
	PRINT(("codec mono output   = %#04x\n",auich_codec_read(&card->config, 0x06)));
	PRINT(("codec pcm output    = %#04x\n",auich_codec_read(&card->config, 0x18)));
	PRINT(("codec line in	    = %#04x\n",auich_codec_read(&card->config, 0x10)));
	PRINT(("codec record line in= %#04x\n",auich_codec_read(&card->config, 0x1a)));
	PRINT(("codec record gain   = %#04x\n",auich_codec_read(&card->config, 0x1c)));*/
	
	PRINT(("writing codec registers\n"));
	// TODO : to move with AC97
	/* enable master output */
	auich_codec_write(&card->config, AC97_MASTER_VOLUME, 0x0000);
	/* enable aux output */
	auich_codec_write(&card->config, AC97_AUX_OUT_VOLUME, 0x0000);
	/* enable mono output */
	//auich_codec_write(&card->config, AC97_MONO_VOLUME, 0x0004);
	/* enable pcm output */
	auich_codec_write(&card->config, AC97_PCM_OUT_VOLUME, 0x0808);
	/* enable line in */
	//auich_codec_write(&card->config, AC97_LINE_IN_VOLUME, 0x8808);
	/* set record line in */
	auich_codec_write(&card->config, AC97_RECORD_SELECT, 0x0404);
	/* set record gain */
	//auich_codec_write(&card->config, AC97_RECORD_GAIN, 0x0000);

	PRINT(("codec master output = %#04x\n",auich_codec_read(&card->config, AC97_MASTER_VOLUME)));
	PRINT(("codec aux output    = %#04x\n",auich_codec_read(&card->config, AC97_AUX_OUT_VOLUME)));
	PRINT(("codec mono output   = %#04x\n",auich_codec_read(&card->config, AC97_MONO_VOLUME)));
	PRINT(("codec pcm output    = %#04x\n",auich_codec_read(&card->config, AC97_PCM_OUT_VOLUME)));
	PRINT(("codec line in	    = %#04x\n",auich_codec_read(&card->config, AC97_LINE_IN_VOLUME)));
	PRINT(("codec record line in= %#04x\n",auich_codec_read(&card->config, AC97_RECORD_SELECT)));
	PRINT(("codec record gain   = %#04x\n",auich_codec_read(&card->config, AC97_RECORD_GAIN)));
		
	if ((err = auich_init(card)))
		return (err);
		
	PRINT(("init_driver done\n"));

	return err;
}


status_t
init_driver(void)
{
	int ix=0;
	
	pci_info info;
	num_cards = 0;
	
	PRINT(("init_driver()\n"));
	load_driver_symbols("auich");

	if (get_module(pci_name, (module_info **) &pci))
		return ENOSYS;
		
	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if((info.vendor_id == INTEL_VENDOR_ID &&
			(info.device_id == INTEL_82443MX_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801AA_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801AB_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801BA_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801CA_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801DB_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801EB_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801FB_AC97_DEVICE_ID
			|| info.device_id == INTEL_82801GB_AC97_DEVICE_ID
			|| info.device_id == INTEL_6300ESB_AC97_DEVICE_ID
			))
		|| (info.vendor_id == SIS_VENDOR_ID &&
			(info.device_id == SIS_SI7012_AC97_DEVICE_ID
			))
		|| (info.vendor_id == NVIDIA_VENDOR_ID &&
			(info.device_id == NVIDIA_nForce_AC97_DEVICE_ID
			|| info.device_id == NVIDIA_nForce2_AC97_DEVICE_ID
			|| info.device_id == NVIDIA_nForce3_AC97_DEVICE_ID
			)) 
		|| (info.vendor_id == AMD_VENDOR_ID &&
			(info.device_id == AMD_AMD8111_AC97_DEVICE_ID
			|| info.device_id == AMD_AMD768_AC97_DEVICE_ID
			))
			) {
			if (num_cards == NUM_CARDS) {
				PRINT(("Too many auich cards installed!\n"));
				break;
			}
			memset(&cards[num_cards], 0, sizeof(auich_dev));
			cards[num_cards].info = info;
			if (auich_setup(&cards[num_cards])) {
				PRINT(("Setup of auich %ld failed\n", num_cards+1));
			}
			else {
				num_cards++;
			}
		}
		ix++;
	}
	if (!num_cards) {
		PRINT(("no cards\n"));
		put_module(pci_name);
		PRINT(("no suitable cards found\n"));
		return ENODEV;
	}

	
#if DEBUG
	//add_debugger_command("auich", auich_debug, "auich [card# (1-n)]");
#endif
	return B_OK;
}


static void
auich_shutdown(auich_dev *card)
{
	PRINT(("shutdown(%p)\n", card));
	card->interrupt_mask = 0;
	
	remove_io_interrupt_handler(card->config.irq, auich_int, card);
	
	unmap_io_memory(&card->config);
}


void
uninit_driver(void)
{
	int ix, cnt = num_cards;
	num_cards = 0;

	PRINT(("uninit_driver()\n"));
	//remove_debugger_command("auich", auich_debug);

	for (ix=0; ix<cnt; ix++) {
		auich_shutdown(&cards[ix]);
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
