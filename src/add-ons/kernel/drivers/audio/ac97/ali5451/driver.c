/*
 * Copyright 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 *
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <hmulti_audio.h>
#include <string.h>
#include <unistd.h>

#include "ac97.h"
#include "queue.h"
#include "ali_multi.h"
#include "driver.h"
#include "util.h"
#include "ali_hardware.h"
#include "debug.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;

int32 gCardsCount;
ali_dev gCards[MAX_CARDS];
pci_module_info *gPCI;


/* streams handling */


static ali_stream *
ali_stream_new(ali_dev *card, bool rec, uint8 buf_count)
{
	ali_stream *stream;

	if (!buf_count)
		return NULL;

	stream = malloc(sizeof(ali_stream));
	if (!stream)
		return NULL;

	stream->card = card;

	stream->buffer = NULL;

	stream->buf_count = buf_count;
	stream->buf_frames = 0;

	stream->rec = rec;
	stream->channels = 0;
	stream->format.format = 0;
	stream->format.rate = 0;

	stream->is_playing = false;

	stream->used_voice = -1;

	stream->frames_count = 0;
	stream->real_time = 0;
	stream->buffer_cycle = (rec ? buf_count - 1 : 0);

	LOCK(card->lock_sts);
	LIST_INSERT_HEAD(&(card->streams), stream, next);
	UNLOCK(card->lock_sts);

	return stream;
}


static void
ali_stream_unprepare(ali_stream *stream)
{
	ali_stream_stop(stream);

	if (stream->buffer) {
		ali_mem_free(stream->card, stream->buffer->log_base);
		stream->buffer = NULL;
	}

	stream->channels = 0;
	stream->format.format = 0;
	stream->format.rate = 0;

	if (-1 < stream->used_voice) {
		LOCK(stream->card->lock_voices);
		stream->card->used_voices &= ~((uint32) 1 << stream->used_voice);
		UNLOCK(stream->card->lock_voices);
	}
}


static void
ali_stream_delete(ali_stream *stream)
{
	ali_stream_unprepare(stream);

	LOCK(stream->card->lock_sts);
	LIST_REMOVE(stream, next);
	UNLOCK(stream->card->lock_sts);

	free(stream);
}


bool
ali_stream_prepare(ali_stream *stream, uint8 channels, uint32 format,
	uint32 sample_rate)
{
	uint8 frame_size = util_format_to_sample_size(format) * channels, ch;

	TRACE("stream_prepare: frame_size: %d\n", (int) frame_size);

	if (!frame_size)
		return false;

	if (stream->channels == channels && stream->format.format == format &&
			stream->format.rate == sample_rate)
		return true;

	ali_stream_unprepare(stream);

	stream->channels = channels;
	stream->format.format = format;
	stream->format.rate = sample_rate;

	// find and set voice
	LOCK(stream->card->lock_voices);
	if (stream->rec) {
		if ((stream->card->used_voices & (uint32) 1 << ALI_RECORD_VOICE) == 0) {
			stream->used_voice = ALI_RECORD_VOICE;
			stream->card->used_voices |= (uint32) 1 << ALI_RECORD_VOICE;
		}
	} else {
		for (ch = 0; ch < ALI_VOICES; ch++) {
			if ((stream->card->used_voices & (uint32) 1 << ch) == 0) {
				stream->used_voice = ch;
				stream->card->used_voices |= (uint32) 1 << ch;
				break;
			}
		}
	}
	UNLOCK(stream->card->lock_voices);

	if (stream->used_voice == -1) {
		ali_stream_unprepare(stream);
		TRACE("stream_prepare: no free voices\n");
		return false;
	}

	stream->buf_frames = util_get_buffer_length_for_rate(stream->format.rate);

	stream->buffer = ali_mem_alloc(stream->card, stream->buf_frames
		* frame_size * stream->buf_count);
	if (!stream->buffer) {
		ali_stream_unprepare(stream);
		TRACE("stream_prepare: failed to create buffer\n");
		return false;
	}

	return true;
}


bool
ali_stream_start(ali_stream *stream)
{
	if (stream->is_playing)
		return true;

	if (!ali_voice_start(stream)) {
		TRACE("stream_start: trouble starting voice\n");
		return false;
	}

	stream->is_playing = true;

	TRACE("stream_start: %s using voice: %d\n",
		stream->rec?"recording":"playing", stream->used_voice);

	return true;
}


void
ali_stream_stop(ali_stream *stream)
{
	if (!stream->is_playing)
		return;

	ali_voice_stop(stream);

	stream->is_playing = false;
}


void
ali_stream_get_buffer_part(ali_stream *stream, uint8 ch_ndx, uint8 buf_ndx,
	char **buffer, size_t *stride)
{
	uint8 sample_size = util_format_to_sample_size(stream->format.format),
		frame_size = sample_size * stream->channels;

	*buffer = stream->buffer->log_base + frame_size * stream->buf_frames
		* buf_ndx +	sample_size * ch_ndx;
	*stride = frame_size;
}


static int32
ali_interrupt_handler(void *cookie)
{
	ali_dev *card = (ali_dev *) cookie;
	bool handled = false;
	uint32 aina;
	ali_stream *stream;

	aina = ali_read_int(card);
	if (!aina)
		goto exit_;

	LIST_FOREACH(stream, &card->streams, next) {
		if (!stream->is_playing || stream->used_voice == -1)
			continue;

		if ((aina & (uint32) 1 << stream->used_voice) == 0)
			continue;

		ali_voice_clr_int(stream);
			// clear interrupt flag

		// interrupts are disabled already when entering handler, lock only
		acquire_spinlock(&card->lock_sts);
		stream->frames_count += stream->buf_frames;
		stream->real_time = system_time();
		stream->buffer_cycle = (stream->buffer_cycle + 1) % stream->buf_count;
		release_spinlock(&card->lock_sts);

		release_sem_etc(card->sem_buf_ready, 1, B_DO_NOT_RESCHEDULE);

		handled = true;
	}

exit_:
	if (!handled)
		TRACE("unhandled interrupt\n");

	return handled ? B_INVOKE_SCHEDULER : B_UNHANDLED_INTERRUPT;
}


static void
make_device_names(ali_dev *card)
{
	sprintf(card->name, DEVFS_PATH_FORMAT, card - gCards + 1);
}


static void
ali_terminate(ali_dev *card)
{
	TRACE("terminate\n");

	remove_io_interrupt_handler(card->irq, ali_interrupt_handler, card);

	if (card->sem_buf_ready != -1)
		delete_sem(card->sem_buf_ready);

	hardware_terminate(card);
}


static status_t
ali_setup(ali_dev *card)
{
	uint32 cmd_reg;
	status_t res;

	make_device_names(card);

	cmd_reg = (*gPCI->read_pci_config)(card->info.bus, card->info.device,
		card->info.function, PCI_command, 2);
	cmd_reg |= PCI_command_io | PCI_command_memory | PCI_command_master;
	(*gPCI->write_pci_config)(card->info.bus, card->info.device,
		card->info.function, PCI_command, 2, cmd_reg);

	card->io_base = card->info.u.h0.base_registers[0];
	TRACE("setup: base io is: %08x\n", (unsigned int) card->io_base);

	card->irq = card->info.u.h0.interrupt_line;
	TRACE("setup: irq is: %02x\n", card->irq);

	card->codec = NULL;
	card->lock_hw = 0;
	card->lock_sts = 0;
	card->lock_voices = 0;

	card->sem_buf_ready = create_sem(0, "ali5451_buf_ready");
	if (card->sem_buf_ready < B_OK) {
		TRACE("setup: failed to create semaphore\n");
		return -1;
	}

	TRACE("setup: revision: %02x\n", card->info.revision);

	card->playback_stream = NULL;
	card->record_stream = NULL;

	card->used_voices = 0;

	card->global_output_format.format = B_FMT_16BIT;
	card->global_output_format.rate = B_SR_48000;
	card->global_input_format = card->global_output_format;

	res = install_io_interrupt_handler(card->irq, ali_interrupt_handler, card,
		0);
	if (res != B_OK) {
		TRACE("setup: cannot install interrupt handler\n");
		return res;
	}

	res = hardware_init(card);
	if (res != B_OK)
		ali_terminate(card);

	return res;
}


/* driver's public functions */


status_t
init_hardware(void)
{
	status_t res = ENODEV;
	pci_info info;
	int32 i;

	if (get_module(B_PCI_MODULE_NAME, (module_info**) &gPCI) != B_OK)
		return res;

	for (i = 0; (*gPCI->get_nth_pci_info)(i, &info) == B_OK; i++) {
		if (info.class_base == PCI_multimedia &&
			info.vendor_id == ALI_VENDOR_ID &&
			info.device_id == ALI_DEVICE_ID) {
			TRACE("hardware found\n");
			res = B_OK;
			break;
		}
	}

	put_module(B_PCI_MODULE_NAME);

	return res;
}


status_t
init_driver(void)
{
	pci_info info;
	int32 i;
	status_t err;

	if (get_module(B_PCI_MODULE_NAME, (module_info**) &gPCI) != B_OK)
		return ENODEV;

	gCardsCount = 0;

	for (i = 0; (*gPCI->get_nth_pci_info)(i, &info) == B_OK; i++) {
		if (info.class_base == PCI_multimedia &&
			info.vendor_id == ALI_VENDOR_ID &&
			info.device_id == ALI_DEVICE_ID) {
			if (gCardsCount == MAX_CARDS) {
				TRACE("init_driver: too many cards\n");
				break;
			}

			memset(&gCards[gCardsCount], 0, sizeof(ali_dev));
			gCards[gCardsCount].info = info;
			if ((err = (*gPCI->reserve_device)(info.bus, info.device, info.function,
				DRIVER_NAME, &gCards[gCardsCount])) < B_OK) {
				dprintf("%s: failed to reserve_device(%d, %d, %d,): %s\n",
					DRIVER_NAME, info.bus, info.device, info.function,
					strerror(err));
				continue;
			}
			if (ali_setup(&gCards[gCardsCount])) {
				TRACE("init_driver: setup of ali %ld failed\n", gCardsCount + 1);
				(*gPCI->unreserve_device)(info.bus, info.device, info.function,
					DRIVER_NAME, &gCards[gCardsCount]);
			}
			else
				gCardsCount++;
		}
	}

	if (!gCardsCount) {
		put_module(B_PCI_MODULE_NAME);
		TRACE("init_driver: no cards found\n");
		return ENODEV;
	}

	return B_OK;
}


void
uninit_driver(void)
{
	int32 i = gCardsCount;

	while (i--) {
		ali_terminate(&gCards[i]);
		(*gPCI->unreserve_device)(gCards[i].info.bus, gCards[i].info.device,
			gCards[i].info.function, DRIVER_NAME, &gCards[i]);
	}
	memset(&gCards, 0, sizeof(gCards));

	put_module(B_PCI_MODULE_NAME);

	gCardsCount = 0;
}


const char **
publish_devices(void)
{
	static const char *names[MAX_CARDS + 1];
	int32 i;

	for (i = 0; i < gCardsCount; i++)
		names[i] = gCards[i].name;
	names[gCardsCount] = NULL;

	return (const char **) names;
}


/* driver's hooks */


static status_t
ali_audio_open(const char *name, uint32 flags, void **cookie)
{
	ali_dev *card = NULL;
	int32 i;

	for (i = 0; i < gCardsCount; i++) {
		if (!strcmp(gCards[i].name, name)) {
			card = &gCards[i];
		}
	}

	if (!card) {
		TRACE("audio_open: card not found\n");
		return B_ERROR;
	}

	if (card->playback_stream || card->record_stream)
		return B_ERROR;

	*cookie = card;

	card->playback_stream = ali_stream_new(card, false, ALI_BUF_CNT);
	card->record_stream = ali_stream_new(card, true, ALI_BUF_CNT);

	if (!ali_stream_prepare(card->playback_stream, 2,
			card->global_output_format.format,
			util_sample_rate_in_bits(card->global_output_format.rate)))
		return B_ERROR;
	if (!ali_stream_prepare(card->record_stream, 2,
			card->global_input_format.format,
			util_sample_rate_in_bits(card->global_input_format.rate)))
		return B_ERROR;

	return B_OK;
}


static status_t
ali_audio_read(void *cookie, off_t a, void *b, size_t *num_bytes)
{
	TRACE("audio_read attempt\n");

	*num_bytes = 0;
	return B_IO_ERROR;
}


static status_t
ali_audio_write(void *cookie, off_t a, const void *b, size_t *num_bytes)
{
	TRACE("audio_write attempt\n");

	*num_bytes = 0;
	return B_IO_ERROR;
}


static status_t
ali_audio_control(void *cookie, uint32 op, void *arg, size_t len)
{
	if (!cookie)
		TRACE("audio_control: no cookie\n");

	return cookie?multi_audio_control(cookie, op, arg, len):B_BAD_VALUE;
}


static status_t
ali_audio_close(void *cookie)
{
	ali_dev *card = (ali_dev *) cookie;
	ali_stream *stream;

	LIST_FOREACH(stream, &card->streams, next) {
		ali_stream_stop(stream);
	}

	return B_OK;
}


static status_t
ali_audio_free(void *cookie)
{
	ali_dev *card = (ali_dev *) cookie;

	while (!LIST_EMPTY(&card->streams))
		ali_stream_delete(LIST_FIRST(&card->streams));

	card->playback_stream = NULL;
	card->record_stream = NULL;

	return B_OK;
}


device_hooks driver_hooks = {
	ali_audio_open,
	ali_audio_close,
	ali_audio_free,
	ali_audio_control,
	ali_audio_read,
	ali_audio_write
};


device_hooks *
find_device(const char *name)
{
	return &driver_hooks;
}
