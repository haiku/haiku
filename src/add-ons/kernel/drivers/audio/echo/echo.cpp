//------------------------------------------------------------------------------
//
//  EchoGals/Echo24 BeOS Driver for Echo audio cards
//
//	Copyright (c) 2003, Jérôme Duval
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.

#include <KernelExport.h>
#include <Drivers.h>
#include <malloc.h>
#include <unistd.h>
#include "OsSupportBeOS.h"
#include "EchoGalsXface.h"
#include "CDarla24.h"
#include "CDarla.h"
#include "CGina.h"
#include "CGina24.h"
#include "CLayla.h"
#include "CLayla24.h"
#include "CMia.h"
#include "CMona.h"
#include "echo.h"
#include "debug.h"
#include "util.h"

static char pci_name[] = B_PCI_MODULE_NAME;
static pci_module_info	*pci;
int32 num_cards;
echo_dev cards[NUM_CARDS];
int32 num_names;
char * names[NUM_CARDS*20+1];

extern device_hooks multi_hooks;

int32 echo_int(void *arg);
status_t init_hardware(void);
status_t init_driver(void);
static void make_device_names(echo_dev * card);

static status_t echo_setup(echo_dev * card);
static void echo_shutdown(echo_dev *card);

void uninit_driver(void);
const char ** publish_devices(void);
device_hooks * find_device(const char * name);


/* Echo Memory management */

echo_mem *
echo_mem_new(echo_dev *card, size_t size)
{
	echo_mem *mem;

	if ((mem = (echo_mem *) malloc(sizeof(*mem))) == NULL)
		return (NULL);

	mem->area = alloc_mem(&mem->phy_base, &mem->log_base, size, "echo buffer");
	mem->size = size;
	if (mem->area < B_OK) {
		free(mem);
		return NULL;
	}
	return mem;
}

void
echo_mem_delete(echo_mem *mem)
{
	if(mem->area > B_OK)
		delete_area(mem->area);
	free(mem);
}

echo_mem *
echo_mem_alloc(echo_dev *card, size_t size)
{
	echo_mem *mem;
	
	mem = echo_mem_new(card, size);
	if (mem == NULL)
		return (NULL);

	LIST_INSERT_HEAD(&(card->mems), mem, next);

	return mem;
}

void
echo_mem_free(echo_dev *card, void *ptr)
{
	echo_mem 		*mem;
	
	LIST_FOREACH(mem, &card->mems, next) {
		if (mem->log_base != ptr)
			continue;
		LIST_REMOVE(mem, next);
		
		echo_mem_delete(mem);
		break;
	}
}

/*	Echo stream functions */

extern char *      pStatusStrs[ECHOSTATUS_LAST];

status_t
echo_stream_set_audioparms(echo_stream *stream, uint8 channels,
     uint8 bitsPerSample, uint32 sample_rate)
{
	int32 			i;
	uint8 			sample_size, frame_size;
	ECHOGALS_OPENAUDIOPARAMETERS	open_params;
	ECHOGALS_CLOSEAUDIOPARAMETERS  close_params;
	ECHOGALS_AUDIOFORMAT			format_params;
	ECHOSTATUS status;
	
	LOG(("echo_stream_set_audioparms\n"));
	
	close_params.wPipeIndex = stream->pipe;	
	status = stream->card->pEG->CloseAudio(&close_params);
	if(status!=ECHOSTATUS_OK && status!=ECHOSTATUS_CHANNEL_NOT_OPEN) {
		PRINT(("echo_stream_set_audioparms : CloseAudio failed\n"));
		PRINT(("  status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}
	
	open_params.bIsCyclic = TRUE;
	open_params.Pipe.nPipe = 0;
	open_params.Pipe.bIsInput = stream->use == ECHO_USE_RECORD ? TRUE : FALSE;
	open_params.Pipe.wInterleave = channels;
	open_params.ProcessId = NULL;
	
	status = stream->card->pEG->OpenAudio(&open_params, &stream->pipe);
	if(status!=ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : OpenAudio failed\n"));
		PRINT(("  status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}

	PRINT(("VerifyAudioOpen\n"));
	status = stream->card->pEG->VerifyAudioOpen(stream->pipe);
	if(status!=ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : VerifyAudioOpen failed\n"));
		PRINT(("  status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}
	
	if ((stream->channels == channels) &&
		(stream->bitsPerSample == bitsPerSample) && 
		(stream->sample_rate == sample_rate))
		return B_OK;

	format_params.wBitsPerSample = bitsPerSample;
	format_params.byDataAreBigEndian = 0;
	format_params.byMonoToStereo = 0;
	format_params.wDataInterleave = channels == 1 ? 1 : 2;

	status = stream->card->pEG->QueryAudioFormat(stream->pipe, &format_params);
	if(status!=ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : bad format when querying\n"));
		PRINT(("  status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}
	
	status = stream->card->pEG->SetAudioFormat(stream->pipe, &format_params);
	if(status!=ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : bad format when setting\n"));
		PRINT(("  status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}
	
	/* XXXX : setting sample rate is global in this driver */
	status = stream->card->pEG->QueryAudioSampleRate(sample_rate);
	if(status!=ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : bad sample rate when querying\n"));
		PRINT(("  status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}
		
	/* XXXX : setting sample rate is global in this driver */
	status = stream->card->pEG->SetAudioSampleRate(sample_rate);
	if(status!=ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : bad sample rate when setting\n"));
		PRINT(("  status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}
	
	if(stream->buffer)
		echo_mem_free(stream->card, stream->buffer->log_base);
		
	stream->bitsPerSample = bitsPerSample;
	stream->sample_rate = sample_rate;
	stream->channels = channels;
	
	sample_size = stream->bitsPerSample / 8;
	frame_size = sample_size * stream->channels;
	
	stream->buffer = echo_mem_alloc(stream->card, stream->bufframes * frame_size * stream->bufcount);
	
	stream->trigblk = 0;	/* This shouldn't be needed */
	stream->blkmod = stream->bufcount;
	stream->blksize = stream->bufframes * frame_size;
	
	CDaffyDuck *duck = stream->card->pEG->GetDaffyDuck(stream->pipe);
	if(duck == NULL) {
		PRINT(("echo_stream_set_audioparms : Could not get daffy duck pointer\n"));
		return B_ERROR;
	}
	
	uint32 dwNumFreeEntries = 0;

	for(i=0; i<stream->bufcount; i++) {
		duck->AddMapping(((uint32)stream->buffer->phy_base) + 
			i * stream->blksize, stream->blksize, 0, TRUE, dwNumFreeEntries);
	}
	
	duck->Wrap();
	
	if(stream->card->pEG->GetAudioPositionPtr(stream->pipe, stream->position)!=ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : Could not get audio position ptr\n"));
		return B_ERROR;
	}
	
	return B_OK;
}


status_t
echo_stream_get_nth_buffer(echo_stream *stream, uint8 chan, uint8 buf, 
					char** buffer, size_t *stride)
{
	uint8 			sample_size, frame_size;
	LOG(("echo_stream_get_nth_buffer\n"));
	
	sample_size = stream->bitsPerSample / 8;
	frame_size = sample_size * stream->channels;
	
	*buffer = (char*)stream->buffer->log_base + (buf * stream->bufframes * frame_size)
		+ chan * sample_size;
	*stride = frame_size;
	
	return B_OK;
}


static uint32
echo_stream_curaddr(echo_stream *stream)
{
	uint32 addr = *stream->position - (uint32)stream->buffer->phy_base;
	TRACE(("stream_curaddr %p, phy_base %p\n", addr, (uint32)stream->buffer->phy_base));
	return addr;
}


void
echo_stream_start(echo_stream *stream, void (*inth) (void *), void *inthparam)
{
	LOG(("echo_stream_start\n"));
	
	stream->inth = inth;
	stream->inthparam = inthparam;
		
	stream->state |= ECHO_STATE_STARTED;

	if(stream->card->pEG->Start(stream->pipe)!=ECHOSTATUS_OK) {
		PRINT(("echo_stream_start : Could not start the pipe\n"));
	}	
}

void
echo_stream_halt(echo_stream *stream)
{
	LOG(("echo_stream_halt\n"));
			
	stream->state &= ~ECHO_STATE_STARTED;
	
	if(stream->card->pEG->Stop(stream->pipe)!=ECHOSTATUS_OK) {
		PRINT(("echo_stream_halt : Could not stop the pipe\n"));
	}	
}

echo_stream *
echo_stream_new(echo_dev *card, uint8 use, uint32 bufframes, uint8 bufcount)
{
	echo_stream *stream;
	cpu_status status;
	LOG(("echo_stream_new\n"));

	stream = (echo_stream *) malloc(sizeof(echo_stream));
	if (stream == NULL)
		return (NULL);
	stream->card = card;
	stream->use = use;
	stream->state = !ECHO_STATE_STARTED;
	stream->bitsPerSample = 0;
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
	
	stream->pipe = card->pEG->MakePipeIndex(0, (use == ECHO_USE_RECORD));
	
	stream->frames_count = 0;
	stream->real_time = 0;
	stream->update_needed = false;
	
	status = lock();
	LIST_INSERT_HEAD((&card->streams), stream, next);
	unlock(status);
	
	return stream;
}

void
echo_stream_delete(echo_stream *stream)
{
	cpu_status status;
	LOG(("echo_stream_delete\n"));
	
	echo_stream_halt(stream);
	
	if(stream->buffer)
		echo_mem_free(stream->card, stream->buffer->log_base);
	
	status = lock();
	LIST_REMOVE(stream, next);
	unlock(status);
	
	free(stream);
}


/* Echo interrupt */

int32 echo_int(void *arg)
{
	echo_dev	 	*card = (echo_dev*)arg;
	BOOL			midiReceived;
	ECHOSTATUS 		err;
	
	echo_stream *stream;
	uint32       curblk;
			
	err = card->pEG->ServiceIrq(midiReceived);
	
	if(err == ECHOSTATUS_OK) {
		LIST_FOREACH(stream, &card->streams, next) {
			if ((stream->use & ECHO_USE_PLAY) == 0 ||
				(stream->state & ECHO_STATE_STARTED) == 0 ||
				(stream->inth == NULL))
					continue;
			
			TRACE(("echo_int stream %p\n", stream));
			curblk = echo_stream_curaddr(stream) / stream->blksize;
			TRACE(("echo_int at trigblk %lu\n", curblk));
			TRACE(("echo_int at stream->trigblk %lu\n", stream->trigblk));
			if (curblk == stream->trigblk) {
				if(stream->inth)
					stream->inth(stream->inthparam);

				stream->trigblk++;
				stream->trigblk %= stream->blkmod;
			}
		}
	
		return B_HANDLED_INTERRUPT;
	} else
		return B_UNHANDLED_INTERRUPT;
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
		
		ushort card_type = info.u.h0.subsystem_id & 0xfff0;
		
		if (info.vendor_id == VENDOR_ID &&
			((info.device_id == DEVICE_ID_56301)
			|| (info.device_id == DEVICE_ID_56361)) &&
			(info.u.h0.subsystem_vendor_id == SUBVENDOR_ID) &&
			(
#ifdef ECHOGALS_FAMILY
			(card_type == DARLA)
			|| (card_type == GINA)
			|| (card_type == LAYLA)
			|| (card_type == DARLA24)
#endif
#ifdef ECHO24_FAMILY
			(card_type == GINA24)
			|| (card_type == LAYLA24)
			|| (card_type == MONA)
			|| (card_type == MIA)
			|| (card_type == INDIGO)
#endif
			 )) {
			err = B_OK;
		}
		ix++;
	}
		
	put_module(pci_name);

	if(err!=B_OK) {
		PRINT(("no card found\n"));
	}

	return err;
}


status_t
init_driver(void)
{
	int ix=0;
	
	pci_info info;
	num_cards = 0;
	
	PRINT(("init_driver()\n"));
	load_driver_symbols(DRIVER_NAME);

	if (get_module(pci_name, (module_info **) &pci))
		return ENOSYS;
		
	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		ushort card_type = info.u.h0.subsystem_id & 0xfff0;
		
		if (info.vendor_id == VENDOR_ID &&
			((info.device_id == DEVICE_ID_56301)
			|| (info.device_id == DEVICE_ID_56361)) &&
			(info.u.h0.subsystem_vendor_id == SUBVENDOR_ID) &&
			(
#ifdef ECHOGALS_FAMILY
			(card_type == DARLA)
			|| (card_type == GINA)
			|| (card_type == LAYLA)
			|| (card_type == DARLA24)
#endif
#ifdef ECHO24_FAMILY
			(card_type == GINA24)
			|| (card_type == LAYLA24)
			|| (card_type == MONA)
			|| (card_type == MIA)
			|| (card_type == INDIGO)
#endif
			)) {			
			
			if (num_cards == NUM_CARDS) {
				PRINT(("Too many "DRIVER_NAME" cards installed!\n"));
				break;
			}
			memset(&cards[num_cards], 0, sizeof(echo_dev));
			cards[num_cards].info = info;
			cards[num_cards].type = card_type;
			if (echo_setup(&cards[num_cards])) {
				PRINT(("Setup of "DRIVER_NAME" %ld failed\n", num_cards+1));
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
	
	return B_OK;
}


static void
make_device_names(
	echo_dev * card)
{
	sprintf(card->name, "audio/multi/"DRIVER_NAME"/%ld", card-cards+1);
	names[num_names++] = card->name;

	names[num_names] = NULL;
}


static status_t
echo_setup(echo_dev * card)
{
	status_t err = B_OK;
	unsigned char cmd;
	
	PRINT(("echo_setup(%p)\n", card));
	
	(*pci->write_pci_config)(card->info.bus, card->info.device, card->info.function, 
		PCI_latency, 1, 0xc0 );
	
	make_device_names(card);
	
	card->bmbar = card->info.u.h0.base_registers[0];
	card->irq = card->info.u.h0.interrupt_line;

	card->pOSS = new COsSupport(card->info.device_id);
	if(card->pOSS == NULL)
		return B_ERROR;

	switch (card->type) {
#ifdef ECHOGALS_FAMILY
		case DARLA:
			card->pEG = new CDarla(card->pOSS);
			break;
		case GINA:
			card->pEG = new CGina(card->pOSS);
			break;
		case LAYLA:
			card->pEG = new CLayla(card->pOSS);
			break;
		case DARLA24:
			card->pEG = new CDarla24(card->pOSS);
			break;
#endif
#ifdef ECHO24_FAMILY
		case GINA24:
			card->pEG = new CGina24(card->pOSS);
			break;
		case LAYLA24:
			card->pEG = new CLayla24(card->pOSS);
			break;
		case MONA:
			card->pEG = new CMona(card->pOSS);
			break;
		case MIA:
			card->pEG = new CMia(card->pOSS);
			break;
#endif
		default:
			PRINT(("card type 0x%x not supported by "DRIVER_NAME"\n", card->type));
			delete card->pOSS;
			return B_ERROR;
	}

	if (card->pEG == NULL)
		return B_ERROR;
		
	card->area_bmbar = map_mem(&card->log_bmbar, (void *)card->bmbar, 
		card->info.u.h0.base_register_sizes[0], DRIVER_NAME" bmbar io");
	if (card->area_bmbar <= B_OK) {
		LOG(("mapping of bmbar io failed, error = %#x\n",card->area_bmbar));
		return B_ERROR;
	}
	LOG(("mapping of bmbar: area %#x, phys %#x, log %#x\n", card->area_bmbar, card->bmbar, card->log_bmbar));

	cmd = (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, PCI_command, 2);
	PRINT(("PCI command before: %x\n", cmd));
	(*pci->write_pci_config)(card->info.bus, card->info.device, card->info.function, PCI_command, 2, cmd | PCI_command_io);
	cmd = (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, PCI_command, 2);
	PRINT(("PCI command after: %x\n", cmd));

	card->pEG->AssignResources(card->log_bmbar, "no name");

	ECHOSTATUS status;
	status = card->pEG->InitHw();
	if(status != ECHOSTATUS_OK)
		return B_ERROR;
		
	/* Init streams list */
	LIST_INIT(&(card->streams));
			
	/* Init mems list */
	LIST_INIT(&(card->mems));
	
	PRINT(("installing interrupt : %x\n", card->irq));
	install_io_interrupt_handler(card->irq, echo_int, card, 0);
	
	PRINT(("echo_setup done\n"));

	return err;
}

static void
echo_shutdown(echo_dev *card)
{
	PRINT(("shutdown(%p)\n", card));
	remove_io_interrupt_handler(card->irq, echo_int, card);
	
	delete card->pEG;
	delete card->pOSS;
	
	delete_area(card->area_bmbar);
}



void
uninit_driver(void)
{
	int ix, cnt = num_cards;
	num_cards = 0;

	PRINT(("uninit_driver()\n"));
	
	for (ix=0; ix<cnt; ix++) {
		echo_shutdown(&cards[ix]);
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
