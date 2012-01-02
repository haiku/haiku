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
#include <unistd.h>
#include "OsSupportBeOS.h"
#include "EchoGalsXface.h"
#include "C3g.h"
#include "CDarla24.h"
#include "CDarla.h"
#include "CGina.h"
#include "CGina24.h"
#include "CIndigo.h"
#include "CIndigoDJ.h"
#include "CIndigoIO.h"
#include "CLayla.h"
#include "CLayla24.h"
#include "CMia.h"
#include "CMona.h"
#include "echo.h"
#include "debug.h"
#include "util.h"

#ifdef CARDBUS
static cb_enabler_module_info	*cbemi;
struct _echodevs				devices;
static char						*names[NUM_CARDS];
int32 							num_names = 0;
static uint32					device_index = 0;
static sem_id					device_lock = 0;

static const cb_device_descriptor	descriptors[] = {
	{VENDOR_ID, DEVICE_ID_56301, 0xff, 0xff, 0xff}
};

#define COUNT_DESCRIPTOR			1

status_t cardbus_device_added(pci_info *info, void **cookie);
void cardbus_device_removed(void *cookie);

static cb_notify_hooks	cardbus_hooks = {
	cardbus_device_added, 		// Add entry point
	cardbus_device_removed 		// Remove entry point
};

#else // CARDBUS
static pci_module_info	*pci;
int32 num_cards;
echo_dev cards[NUM_CARDS];
int32 num_names;
char * names[NUM_CARDS*20+1];
#endif // CARDBUS

extern device_hooks multi_hooks;
#ifdef MIDI_SUPPORT
extern device_hooks midi_hooks;
#endif

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
	if (mem->area > B_OK)
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

extern char *pStatusStrs[ECHOSTATUS_LAST];

status_t
echo_stream_set_audioparms(echo_stream *stream, uint8 channels,
	uint8 bitsPerSample, uint32 sample_rate, uint8 index)
{
	int32 			i;
	uint8 			sample_size, frame_size;
	ECHOGALS_OPENAUDIOPARAMETERS	open_params;
	ECHOGALS_CLOSEAUDIOPARAMETERS  close_params;
	ECHOGALS_AUDIOFORMAT			format_params;
	ECHOSTATUS status;

	LOG(("echo_stream_set_audioparms\n"));

	if (stream->pipe >= 0) {
		close_params.wPipeIndex = stream->pipe;
		status = stream->card->pEG->CloseAudio(&close_params);
		if (status != ECHOSTATUS_OK && status != ECHOSTATUS_CHANNEL_NOT_OPEN) {
			PRINT(("echo_stream_set_audioparms : CloseAudio failed\n"));
			PRINT((" status: %s \n", pStatusStrs[status]));
			return B_ERROR;
		}
	}

	open_params.bIsCyclic = TRUE;
	open_params.Pipe.nPipe = index;
	open_params.Pipe.bIsInput = stream->use == ECHO_USE_RECORD ? TRUE : FALSE;
	open_params.Pipe.wInterleave = channels;
	open_params.ProcessId = NULL;

	status = stream->card->pEG->OpenAudio(&open_params, &stream->pipe);
	if (status != ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : OpenAudio failed\n"));
		PRINT((" status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}

	//PRINT(("VerifyAudioOpen\n"));
	status = stream->card->pEG->VerifyAudioOpen(stream->pipe);
	if (status != ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : VerifyAudioOpen failed\n"));
		PRINT(("  status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}

	if (bitsPerSample == 24)
		bitsPerSample = 32;

	if ((stream->channels == channels)
		&& (stream->bitsPerSample == bitsPerSample)
		&& (stream->sample_rate == sample_rate))
		return B_OK;

	format_params.wBitsPerSample = bitsPerSample;
	format_params.byDataAreBigEndian = 0;
	format_params.byMonoToStereo = 0;
	format_params.wDataInterleave = channels == 1 ? 1 : 2;

	status = stream->card->pEG->QueryAudioFormat(stream->pipe, &format_params);
	if (status != ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : bad format when querying\n"));
		PRINT(("  status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}

	status = stream->card->pEG->SetAudioFormat(stream->pipe, &format_params);
	if (status != ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : bad format when setting\n"));
		PRINT(("  status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}

	/* XXXX : setting sample rate is global in this driver */
	status = stream->card->pEG->QueryAudioSampleRate(sample_rate);
	if (status != ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : bad sample rate when querying\n"));
		PRINT(("  status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}

	/* XXXX : setting sample rate is global in this driver */
	status = stream->card->pEG->SetAudioSampleRate(sample_rate);
	if (status != ECHOSTATUS_OK) {
		PRINT(("echo_stream_set_audioparms : bad sample rate when setting\n"));
		PRINT(("  status: %s \n", pStatusStrs[status]));
		return B_ERROR;
	}

	if (stream->buffer)
		echo_mem_free(stream->card, stream->buffer->log_base);

	stream->bitsPerSample = bitsPerSample;
	stream->sample_rate = sample_rate;
	stream->channels = channels;

	sample_size = stream->bitsPerSample / 8;
	frame_size = sample_size * stream->channels;

	stream->buffer = echo_mem_alloc(stream->card,
		stream->bufframes * frame_size * stream->bufcount);

	stream->trigblk = 1;
	stream->blkmod = stream->bufcount;
	stream->blksize = stream->bufframes * frame_size;

	CDaffyDuck *duck = stream->card->pEG->GetDaffyDuck(stream->pipe);
	if (duck == NULL) {
		PRINT(("echo_stream_set_audioparms : Could not get daffy duck pointer\n"));
		return B_ERROR;
	}

	uint32 dwNumFreeEntries = 0;

	for (i = 0; i < stream->bufcount; i++) {
		duck->AddMapping(((uint32)stream->buffer->phy_base) +
			i * stream->blksize, stream->blksize, 0, TRUE, dwNumFreeEntries);
	}

	duck->Wrap();

	if (stream->card->pEG->GetAudioPositionPtr(stream->pipe, stream->position)!=ECHOSTATUS_OK) {
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
	uint32 addr = B_LENDIAN_TO_HOST_INT32(*stream->position);
//	TRACE(("stream_curaddr %p, phy_base %p\n", addr));
	return (addr / stream->blksize) % stream->blkmod;
}


void
echo_stream_start(echo_stream *stream, void (*inth) (void *), void *inthparam)
{
	LOG(("echo_stream_start\n"));
	ECHOSTATUS status;

	stream->inth = inth;
	stream->inthparam = inthparam;

	stream->state |= ECHO_STATE_STARTED;

	status = stream->card->pEG->Start(stream->pipe);
	if (status!=ECHOSTATUS_OK) {
		PRINT(("echo_stream_start : Could not start the pipe %s\n", pStatusStrs[status]));
	}
}


void
echo_stream_halt(echo_stream *stream)
{
	LOG(("echo_stream_halt\n"));
	ECHOSTATUS status;

	stream->state &= ~ECHO_STATE_STARTED;

	status = stream->card->pEG->Stop(stream->pipe);
	if (status!=ECHOSTATUS_OK) {
		PRINT(("echo_stream_halt : Could not stop the pipe %s\n", pStatusStrs[status]));
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

	stream->pipe = -1;

	stream->frames_count = 0;
	stream->real_time = 0;
	stream->buffer_cycle = 0;
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
	ECHOGALS_CLOSEAUDIOPARAMETERS close_params;
	LOG(("echo_stream_delete\n"));

	echo_stream_halt(stream);

	if (stream->pipe >= 0) {
		close_params.wPipeIndex = stream->pipe;
		status = stream->card->pEG->CloseAudio(&close_params);
		if (status != ECHOSTATUS_OK && status != ECHOSTATUS_CHANNEL_NOT_OPEN) {
			PRINT(("echo_stream_set_audioparms : CloseAudio failed\n"));
			PRINT((" status: %s \n", pStatusStrs[status]));
		}
	}

	if (stream->buffer)
		echo_mem_free(stream->card, stream->buffer->log_base);

	status = lock();
	LIST_REMOVE(stream, next);
	unlock(status);

	free(stream);
}


/* Echo interrupt */

int32 echo_int(void *arg)
{
	echo_dev* card = (echo_dev*)arg;
	BOOL midiReceived;
	ECHOSTATUS err;
	echo_stream* stream;
	uint32 curblk;

	err = card->pEG->ServiceIrq(midiReceived);

	if (err != ECHOSTATUS_OK) {
		return B_UNHANDLED_INTERRUPT;
	}

#ifdef MIDI_SUPPORT
	if (midiReceived)
		release_sem_etc(card->midi.midi_ready_sem, 1, B_DO_NOT_RESCHEDULE);
#endif

	LIST_FOREACH(stream, &card->streams, next) {
		if ((stream->state & ECHO_STATE_STARTED) == 0 ||
			(stream->inth == NULL))
				continue;

		curblk = echo_stream_curaddr(stream);
		//TRACE(("echo_int stream %p at trigblk %lu at stream->trigblk %lu\n",
		//	   stream, curblk, stream->trigblk));
		if (curblk == stream->trigblk) {
			if (stream->inth)
				stream->inth(stream->inthparam);

			stream->trigblk++;
			stream->trigblk %= stream->blkmod;
		}
	}

	return B_INVOKE_SCHEDULER;
}

/* dumps card capabilities */
void
echo_dump_caps(echo_dev *card)
{
	PECHOGALS_CAPS caps = &card->caps;
	PRINT(("name: %s\n", caps->szName));
	PRINT(("out pipes: %d, in pipes: %d, out busses: %d, in busses: %d, out midi: %d, in midi: %d\n",
		caps->wNumPipesOut, caps->wNumPipesIn, caps->wNumBussesOut, caps->wNumBussesIn, caps->wNumMidiOut, caps->wNumMidiIn));

}


/* detect presence of our hardware */
status_t
init_hardware(void)
{
#ifdef CARDBUS
	return B_OK;
#else
	int ix = 0;
	pci_info info;
	status_t err = ENODEV;

	LOG_CREATE();

	PRINT(("init_hardware()\n"));

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci))
		return ENOSYS;

	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {

		ushort card_type = info.u.h0.subsystem_id & 0xfff0;

		if (info.vendor_id == VENDOR_ID
			&& ((info.device_id == DEVICE_ID_56301)
			|| (info.device_id == DEVICE_ID_56361))
			&& (info.u.h0.subsystem_vendor_id == SUBVENDOR_ID)
			&& (
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
#endif
#ifdef INDIGO_FAMILY
			(card_type == INDIGO)
			|| (card_type == INDIGO_IO)
			|| (card_type == INDIGO_DJ)
#endif
#ifdef ECHO3G_FAMILY
			(card_type == ECHO3G)
#endif
			 )) {
			err = B_OK;
		}
		ix++;
	}

	put_module(B_PCI_MODULE_NAME);

	if (err != B_OK) {
		PRINT(("no card found\n"));
	}

	return err;
#endif
}


status_t
init_driver(void)
{
	PRINT(("init_driver()\n"));

#ifdef CARDBUS
	// Get card services client module
	if (get_module(CB_ENABLER_MODULE_NAME, (module_info **)&cbemi) != B_OK) {
		dprintf(DRIVER_NAME ": cardbus enabler module error\n");
		return B_ERROR;
	}
	// Create the devices lock
	device_lock = create_sem(1, DRIVER_NAME " device");
	if (device_lock < B_OK) {
		dprintf(DRIVER_NAME ": create device semaphore error 0x%.8x\n", device_lock);
		put_module(CB_ENABLER_MODULE_NAME);
		return B_ERROR;
	}
	// Register driver
	cbemi->register_driver(DRIVER_NAME, descriptors, COUNT_DESCRIPTOR);
	cbemi->install_notify(DRIVER_NAME, &cardbus_hooks);
	LIST_INIT(&(devices));
	return B_OK;
#else
	int ix = 0;

	pci_info info;
	status_t err;
	num_cards = 0;

	if (get_module(B_PCI_MODULE_NAME, (module_info **) &pci))
		return ENOSYS;

	while ((*pci->get_nth_pci_info)(ix++, &info) == B_OK) {
		ushort card_type = info.u.h0.subsystem_id & 0xfff0;

		if (info.vendor_id == VENDOR_ID
			&& ((info.device_id == DEVICE_ID_56301)
			|| (info.device_id == DEVICE_ID_56361))
			&& (info.u.h0.subsystem_vendor_id == SUBVENDOR_ID)
			&& (
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
#endif
#ifdef INDIGO_FAMILY
			(card_type == INDIGO)
			|| (card_type == INDIGO_IO)
			|| (card_type == INDIGO_DJ)
#endif
#ifdef ECHO3G_FAMILY
			(card_type == ECHO3G)
#endif
			)) {

			if (num_cards == NUM_CARDS) {
				PRINT(("Too many "DRIVER_NAME" cards installed!\n"));
				break;
			}
			memset(&cards[num_cards], 0, sizeof(echo_dev));
			cards[num_cards].info = info;
			cards[num_cards].type = card_type;
#ifdef __HAIKU__
			if ((err = (*pci->reserve_device)(info.bus, info.device, info.function,
				DRIVER_NAME, &cards[num_cards])) < B_OK) {
				dprintf("%s: failed to reserve_device(%d, %d, %d,): %s\n",
					DRIVER_NAME, info.bus, info.device, info.function,
					strerror(err));
				continue;
			}
#endif
			if (echo_setup(&cards[num_cards])) {
				PRINT(("Setup of "DRIVER_NAME" %ld failed\n", num_cards + 1));
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

	return B_OK;
#endif
}


#ifndef CARDBUS
static void
make_device_names(echo_dev * card)
{
#ifdef MIDI_SUPPORT
	sprintf(card->midi.name, "midi/"DRIVER_NAME"/%ld", card-cards + 1);
	names[num_names++] = card->midi.name;
#endif
	sprintf(card->name, "audio/hmulti/"DRIVER_NAME"/%ld", card-cards + 1);
	names[num_names++] = card->name;

	names[num_names] = NULL;
}
#else

status_t
cardbus_device_added(pci_info *info, void **cookie) {
	echo_dev 			* card, *dev;
	uint32				index;
	char				buffer[32];

	LOG(("cardbus_device_added at %.2d:%.2d:%.2d\n", info->bus, info->device, info->function));
	// Allocate cookie
	if (!(*cookie = card = (echo_dev *)malloc(sizeof(echo_dev)))) {
		return B_NO_MEMORY;
	}
	// Clear cookie
	memset(card, 0, sizeof(echo_dev));
	// Initialize cookie
	card->info = *info;
	card->plugged = true;
	card->index = 0;

	LIST_FOREACH(dev, &devices, next) {
		if (dev->index == card->index) {
			card->index++;
			dev = LIST_FIRST(&devices);
		}
	}

	// Format device name
	sprintf(card->name, "audio/hmulti/" DRIVER_NAME "/%ld", card->index);
	// Lock the devices
	acquire_sem(device_lock);
	LIST_INSERT_HEAD((&devices), card, next);
	// Unlock the devices
	release_sem(device_lock);

	echo_setup(card);
	return B_OK;
}


// cardbus_device_removed - handle cardbus device removal.
// status : OK
void
cardbus_device_removed(void *cookie)
{
	echo_dev		*card = (echo_dev *) cookie;

	LOG((": cardbus_device_removed\n"));
	// Check
	if (card == NULL) {
		LOG((": null device 0x%.8x\n", card));
		return;
	}

	echo_shutdown(card);

	// Lock the devices
	acquire_sem(device_lock);
	// Finalize
	card->plugged = false;
	// Check if the device is opened
	if (card->opened) {
		LOG(("device 0x%.8x %s still in use\n", card, card->name));
	} else {
		LOG(("free device 0x%.8x %s\n", card, card->name));
		LIST_REMOVE(card, next);
		free(card);
	}
	// Unlock the devices
	release_sem(device_lock);
}

#endif


static status_t
echo_setup(echo_dev * card)
{
	unsigned char cmd;
	char *name;

	PRINT(("echo_setup(%p)\n", card));

#ifndef CARDBUS
	(*pci->write_pci_config)(card->info.bus, card->info.device,
		card->info.function, PCI_latency, 1, 0xc0 );

	make_device_names(card);
#endif
	card->bmbar = card->info.u.h0.base_registers[0];
	card->irq = card->info.u.h0.interrupt_line;

	card->area_bmbar = map_mem(&card->log_bmbar, card->bmbar,
		card->info.u.h0.base_register_sizes[0], DRIVER_NAME" bmbar io");
	if (card->area_bmbar <= B_OK) {
		LOG(("mapping of bmbar io failed, error = %#x\n",card->area_bmbar));
		goto err5;
	}
	LOG(("mapping of bmbar: area %#x, phys %#x, log %#x\n", card->area_bmbar,
		card->bmbar, card->log_bmbar));

	card->pOSS = new COsSupport(card->info.device_id, card->info.revision);
	if (card->pOSS == NULL)
		goto err4;

	switch (card->type) {
#ifdef ECHOGALS_FAMILY
		case DARLA:
			card->pEG = new CDarla(card->pOSS);
			name = "Echo Darla";
			break;
		case GINA:
			card->pEG = new CGina(card->pOSS);
			name = "Echo Gina";
			break;
		case LAYLA:
			card->pEG = new CLayla(card->pOSS);
			name = "Echo Layla";
			break;
		case DARLA24:
			card->pEG = new CDarla24(card->pOSS);
			name = "Echo Darla24";
			break;
#endif
#ifdef ECHO24_FAMILY
		case GINA24:
			card->pEG = new CGina24(card->pOSS);
			name = "Echo Gina24";
			break;
		case LAYLA24:
			card->pEG = new CLayla24(card->pOSS);
			name = "Echo Layla24";
			break;
		case MONA:
			card->pEG = new CMona(card->pOSS);
			name = "Echo Mona";
			break;
		case MIA:
			card->pEG = new CMia(card->pOSS);
			name = "Echo Mia";
			break;
#endif
#ifdef INDIGO_FAMILY
		case INDIGO:
			card->pEG = new CIndigo(card->pOSS);
			name = "Echo Indigo";
			break;
		case INDIGO_IO:
			card->pEG = new CIndigoIO(card->pOSS);
			name = "Echo IndigoIO";
			break;
		case INDIGO_DJ:
			card->pEG = new CIndigoDJ(card->pOSS);
			name = "Echo IndigoDJ";
			break;
#endif
#ifdef ECHO3G_FAMILY
		case ECHO3G:
			card->pEG = new C3g(card->pOSS);
			name = "Echo 3g";
			break;
#endif
		default:
			PRINT(("card type 0x%x not supported by "DRIVER_NAME"\n",
				card->type));
	}

	if (card->pEG == NULL)
		goto err2;

#ifndef CARDBUS
	cmd = (*pci->read_pci_config)(card->info.bus, card->info.device,
		card->info.function, PCI_command, 2);
	PRINT(("PCI command before: %x\n", cmd));
	(*pci->write_pci_config)(card->info.bus, card->info.device,
		card->info.function, PCI_command, 2, cmd | PCI_command_io);
	cmd = (*pci->read_pci_config)(card->info.bus, card->info.device,
		card->info.function, PCI_command, 2);
	PRINT(("PCI command after: %x\n", cmd));
#endif

	card->pEG->AssignResources(card->log_bmbar, name);

	ECHOSTATUS status;
	status = card->pEG->InitHw();
	if (status != ECHOSTATUS_OK)
		goto err3;

	card->pEG->GetCapabilities(&card->caps);

	/* Init streams list */
	LIST_INIT(&(card->streams));

	/* Init mems list */
	LIST_INIT(&(card->mems));

#ifdef MIDI_SUPPORT
	card->midi.midi_ready_sem = create_sem(0, "midi sem");
#endif

	PRINT(("installing interrupt : %x\n", card->irq));
	status = install_io_interrupt_handler(card->irq, echo_int, card, 0);
	if (status != B_OK) {
		PRINT(("failed to install interrupt\n"));
		goto err2;
	}

	PRINT(("echo_setup done\n"));

	echo_dump_caps(card);

#ifdef ECHO3G_FAMILY
	if (card->type == ECHO3G) {
		strlcpy(card->caps.szName, ((C3g*)card->pEG)->Get3gBoxName(),
			ECHO_MAXNAMELEN);
	}
#endif

	status = card->pEG->OpenMixer(card->mixer);
	if (status != ECHOSTATUS_OK) {
		PRINT(("failed to open mixer\n"));
		goto err1;
	}

	return B_OK;

err1:
	remove_io_interrupt_handler(card->irq, echo_int, card);
err2:
#ifdef MIDI_SUPPORT
	delete_sem(card->midi.midi_ready_sem);
#endif
err3:
	delete card->pEG;
err4:
	delete card->pOSS;
err5:
	delete_area(card->area_bmbar);
	return B_ERROR;
}

static void
echo_shutdown(echo_dev *card)
{
	ECHOSTATUS status;

	PRINT(("shutdown(%p)\n", card));
	status = card->pEG->CloseMixer(card->mixer);
	if (status != ECHOSTATUS_OK)
		PRINT(("echo_shutdown: error when CloseMixer\n"));

	remove_io_interrupt_handler(card->irq, echo_int, card);

#ifdef MIDI_SUPPORT
	delete_sem(card->midi.midi_ready_sem);
#endif

	delete card->pEG;
	delete card->pOSS;

	delete_area(card->area_bmbar);
}



void
uninit_driver(void)
{
	PRINT(("uninit_driver()\n"));

#ifdef CARDBUS
	echo_dev			*dev;

	LIST_FOREACH(dev, &devices, next) {
		echo_shutdown(dev);
	}
	put_module(CB_ENABLER_MODULE_NAME);
#else
	int ix, cnt = num_cards;
	num_cards = 0;
	for (ix=0; ix<cnt; ix++) {
		echo_shutdown(&cards[ix]);
#ifdef __HAIKU__
		(*pci->unreserve_device)(cards[ix].info.bus,
			cards[ix].info.device, cards[ix].info.function,
			DRIVER_NAME, &cards[ix]);
#endif
	}

	memset(&cards, 0, sizeof(cards));
	put_module(B_PCI_MODULE_NAME);
#endif
}


const char **
publish_devices(void)
{
#ifdef CARDBUS
	echo_dev			*dev;
	int			ix = 0;

	// Lock the devices
	acquire_sem(device_lock);
	// Loop
	LIST_FOREACH(dev, &devices, next) {
		if (dev->plugged == true) {
			names[ix] = dev->name;
			ix++;
		}
	}
	names[ix] = NULL;
	release_sem(device_lock);
#else
	int ix = 0;
	PRINT(("publish_devices()\n"));

	for (ix=0; names[ix]; ix++) {
		PRINT(("publish %s\n", names[ix]));
	}
#endif
	return (const char **)names;
}


device_hooks *
find_device(const char * name)
{
	echo_dev *dev;
#ifdef CARDBUS
	LIST_FOREACH(dev, &devices, next) {
		if (!strcmp(dev->name, name)) {
			return &multi_hooks;
		}
	}

#else
	int ix;

	PRINT(("find_device(%s)\n", name));

	for (ix=0; ix<num_cards; ix++) {
#ifdef MIDI_SUPPORT
		if (!strcmp(cards[ix].midi.name, name)) {
			return &midi_hooks;
		}
#endif
		if (!strcmp(cards[ix].name, name)) {
			return &multi_hooks;
		}
	}
#endif
	PRINT(("find_device(%s) failed\n", name));
	return NULL;
}

int32	api_version = B_CUR_DRIVER_API_VERSION;
