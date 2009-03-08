/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	Jérôme Duval (korli@users.berlios.de)
 */
#ifndef _DRIVER_H_
#define _DRIVER_H_

#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>

#include <string.h>
#include <stdlib.h>

#define DEVFS_PATH_FORMAT	"audio/hmulti/geode/%lu"
#include <hmulti_audio.h>

#include "ac97.h"
#include "gcscaudioreg.h"

#define MAX_CARDS				4
#define GEODE_MAX_STREAMS			4
#define STREAM_MAX_BUFFERS			4
#define STREAM_MIN_BUFFERS			2

#define DEFAULT_FRAMES_PER_BUFFER	2048


#define AMD_VENDOR_ID			0x1022
#define AMD_CS5536_AUDIO_DEVICE_ID	0x2093

#define NS_VENDOR_ID			0x100b
#define NS_CS5535_AUDIO_DEVICE_ID	0x002e

enum {
	STREAM_PLAYBACK,
	STREAM_RECORD
};

struct geode_stream;
struct geode_multi;
extern pci_module_info* gPci;

/*!	This structure describes a controller.
*/
struct geode_controller {
	struct pci_info	pci_info;
	vint32			opened;
	const char*		devfs_path;

	uint32			nabmbar;
	uint32			irq;

	uint32			num_streams;
	geode_stream*		streams[GEODE_MAX_STREAMS];

	/* Multi Audio API data */
	geode_stream*		playback_stream;
	geode_stream*		record_stream;

	geode_multi*		multi;
	
	ac97_dev *		ac97;

	uint8 Read8(uint32 reg)
	{
		return gPci->read_io_8(nabmbar + reg);
	}

	uint16 Read16(uint32 reg)
	{
		return gPci->read_io_16(nabmbar + reg);
	}

	uint32 Read32(uint32 reg)
	{
		return gPci->read_io_32(nabmbar + reg);
	}

	void Write8(uint32 reg, uint8 value)
	{
		gPci->write_io_8(nabmbar + reg, value);
	}

	void Write16(uint32 reg, uint16 value)
	{
		gPci->write_io_16(nabmbar + reg, value);
	}

	void Write32(uint32 reg, uint32 value)
	{
		gPci->write_io_32(nabmbar + reg, value);
	}
};

/*!	This structure describes a single stream of audio data,
	which is can have multiple channels (for stereo or better).
*/
struct geode_stream {
	uint16		status;				/* Interrupt status */
	uint8		offset;				/* Stream offset */
	uint8		dma_offset;				/* Stream dma offset */
	uint16		ac97_rate_reg;			/* AC97 rate register */
	bool		running;
	spinlock	lock;				/* Write lock */
	uint32		type;

	geode_controller* controller;

	uint32		sample_rate;	
	uint32		sample_format;

	uint32		num_buffers;
	uint32		num_channels;
	uint32		buffer_length;		/* size of buffer in samples */
	uint32		sample_size;
	uint8*		buffers[STREAM_MAX_BUFFERS];
					/* Virtual addresses for buffer */
	uint32		physical_buffers[STREAM_MAX_BUFFERS];
					/* Physical addresses for buffer */
	sem_id		buffer_ready_sem;
	bigtime_t	real_time;
	uint64		frames_count;
	int32		buffer_cycle;

	uint32		rate;			/* Samplerate */

	area_id		buffer_area;
	area_id		buffer_descriptors_area;
	uint32		physical_buffer_descriptors;	/* BDL physical address */

	uint8 Read8(uint32 reg)
	{
		return controller->Read8(ACC_BM0_CMD + offset + reg);
	}

	uint16 Read16(uint32 reg)
	{
		return controller->Read16(ACC_BM0_CMD + offset + reg);
	}

	uint32 Read32(uint32 reg)
	{
		return controller->Read32(ACC_BM0_CMD + offset + reg);
	}

	void Write8(uint32 reg, uint8 value)
	{
		controller->Write8(ACC_BM0_CMD + offset + reg, value);
	}

	void Write16(uint32 reg, uint16 value)
	{
		controller->Write16(ACC_BM0_CMD + offset + reg, value);
	}

	void Write32(uint32 reg, uint32 value)
	{
		controller->Write32(ACC_BM0_CMD + offset + reg, value);
	}
};

#define MULTI_CONTROL_FIRSTID	1024
#define MULTI_CONTROL_MASTERID	0
#define MULTI_MAX_CONTROLS 128
#define MULTI_MAX_CHANNELS 128

struct multi_mixer_control {
	geode_multi	*multi;
	void	(*get) (geode_controller *card, const void *cookie, int32 type, float *values);
	void	(*set) (geode_controller *card, const void *cookie, int32 type, float *values);
	const void    *cookie;
	int32 type;
	multi_mix_control	mix_control;
};


struct geode_multi {
	geode_controller *controller;
	multi_mixer_control controls[MULTI_MAX_CONTROLS];
	uint32 control_count;
	
	multi_channel_info chans[MULTI_MAX_CHANNELS];
	uint32 output_channel_count;
	uint32 input_channel_count;
	uint32 output_bus_channel_count;
	uint32 input_bus_channel_count;
	uint32 aux_bus_channel_count;
};


/* driver.cpp */
extern device_hooks gDriverHooks;
extern geode_controller gCards[MAX_CARDS];
extern uint32 gNumCards;

/* geode_multi.cpp */
status_t multi_audio_control(geode_controller* controller, uint32 op, void* arg, size_t length);

/* geode_controller.cpp: Basic controller support */
status_t geode_hw_init(geode_controller* controller);
void geode_hw_stop(geode_controller* controller);
void geode_hw_uninit(geode_controller* controller);

uint16 geode_codec_read(geode_controller *controller, uint8 regno);
void geode_codec_write(geode_controller *controller, uint8 regno, uint16 value);

/* geode_controller.cpp: Stream support */
geode_stream* geode_stream_new(geode_controller* controller, int type);
void geode_stream_delete(geode_stream* stream);
status_t geode_stream_setup_buffers(geode_stream* stream, const char* desc);
status_t geode_stream_start(geode_stream* stream);
status_t geode_stream_stop(geode_stream* stream);

#endif	/* _DRIVER_H_ */
