/*
 * Copyright 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 *
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ALI_AUDIO_DRIVER_H
#define _ALI_AUDIO_DRIVER_H


#define MULTI_AUDIO_BASE_ID 1024
#define MULTI_AUDIO_DEV_PATH "audio/hmulti"
#define MULTI_AUDIO_MASTER_ID 0

#define DEVFS_PATH_FORMAT MULTI_AUDIO_DEV_PATH "/ali5451/%lu"

#define MAX_CARDS 3
#define DEVNAME 32

#define ALI_VENDOR_ID 0x10b9
#define ALI_DEVICE_ID 0x5451

#define ALI_BUF_CNT 2
#define ALI_MAX_CHN 2
#define ALI_VOICES 32
#define ALI_RECORD_VOICE 31

#define DRIVER_NAME "ali5451"

typedef struct _ali_mem
{
	LIST_ENTRY(_ali_mem) next;

	void *log_base;
	void *phy_base;

	area_id area;
	size_t size;
} ali_mem;

typedef struct {
	uint32 format;
	uint32 rate;
} ali_format;

typedef struct _ali_stream
{
	LIST_ENTRY(_ali_stream) next;

	struct _ali_dev *card;

	ali_mem *buffer;

	uint8 buf_count;
	uint32 buf_frames;

	bool rec;
	uint8 channels;
	ali_format format;

	bool is_playing;

	int8 used_voice;

	// multi_audio
	volatile int64 frames_count;
	volatile bigtime_t real_time;
	volatile int32 buffer_cycle;
} ali_stream;

typedef struct _ali_dev
{
	char name[DEVNAME];
	pci_info info;
	ulong io_base;
	uint8 irq;

	ac97_dev *codec;

	spinlock lock_hw;
	spinlock lock_sts;
	spinlock lock_voices;
	sem_id sem_buf_ready;

	ali_stream *playback_stream;
	ali_stream *record_stream;

	uint32 used_voices;

	ali_format global_output_format;
	ali_format global_input_format;

	LIST_HEAD(, _ali_stream) streams;
	LIST_HEAD(, _ali_mem) mems;
} ali_dev;

#define LOCK(_lock)								\
	{											\
		cpu_status cp = disable_interrupts();	\
		acquire_spinlock(&_lock)
#define UNLOCK(_lock)							\
		release_spinlock(&_lock);				\
		restore_interrupts(cp);					\
	}

bool ali_stream_prepare(ali_stream *stream, uint8 channels, uint32 format,
	uint32 sample_rate);
bool ali_stream_start(ali_stream *stream);
void ali_stream_stop(ali_stream *stream);
void ali_stream_get_buffer_part(ali_stream *stream, uint8 ch_ndx, uint8 buf_ndx,
	char ** buffer, size_t *stride);

#endif // _ALI_AUDIO_DRIVER_H
