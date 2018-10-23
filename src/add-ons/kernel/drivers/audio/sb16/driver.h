#ifndef DRIVER_H
#define DRIVER_H

#include <drivers/driver_settings.h>
#include <drivers/KernelExport.h>
#include <drivers/Drivers.h>

#include <string.h>
#include <stdlib.h>

#define DEVFS_PATH	"audio/hmulti"
#include <hmulti_audio.h>


#define STRMINBUF		2
#define STRMAXBUF		2
#define DEFAULT_FRAMESPERBUF	512

#define SB16_MULTI_CONTROL_FIRSTID       1024
#define SB16_MULTI_CONTROL_MASTERID      0

typedef struct {
	int		running;
	spinlock	lock;
	int bits;

	void*		buffers[STRMAXBUF];
	uint32		num_buffers;
	uint32		num_channels;
	uint32		sample_size;
	uint32		sampleformat;
	uint32		samplerate;
	uint32		buffer_length;
	sem_id		buffer_ready_sem;
	uint32		frames_count;
	uint32		buffer_cycle;
	bigtime_t	real_time;
} sb16_stream_t;

typedef struct {
	int port, irq, dma8, dma16, midiport;
	int opened;

	sb16_stream_t playback_stream;
	sb16_stream_t record_stream;
} sb16_dev_t;

extern device_hooks driver_hooks;

status_t sb16_hw_init(sb16_dev_t* dev);
void sb16_hw_stop(sb16_dev_t* dev);
void sb16_hw_uninit(sb16_dev_t* dev);

status_t sb16_stream_setup_buffers(sb16_dev_t* dev, sb16_stream_t* s, const char* desc);
status_t sb16_stream_start(sb16_dev_t* dev, sb16_stream_t* s);
status_t sb16_stream_stop(sb16_dev_t* dev, sb16_stream_t* s);
void sb16_stream_buffer_done(sb16_stream_t* stream);

status_t multi_audio_control(void* cookie, uint32 op, void* arg, size_t len);

#endif /* DRIVER_H */

