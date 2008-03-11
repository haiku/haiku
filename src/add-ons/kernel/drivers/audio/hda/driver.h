/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _HDA_H_
#define _HDA_H_

#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>

#include <string.h>
#include <stdlib.h>

#ifndef HAIKU_TARGET_PLATFORM_HAIKU
#	define DEVFS_PATH_FORMAT	"audio/multi/hda/%lu"
#	include <multi_audio.h>
#else
#	define DEVFS_PATH_FORMAT	"audio/hmulti/hda/%lu"
#	include <hmulti_audio.h>
#endif

#include "hda_controller_defs.h"
#include "hda_codec_defs.h"

#define MAX_CARDS				4

/* values for the class_sub field for class_base = 0x04 (multimedia device) */
#define PCI_hd_audio			3

#define HDA_MAX_AUDIO_GROUPS	15
#define HDA_MAX_CODECS			15
#define HDA_MAX_STREAMS			16
#define MAX_CODEC_RESPONSES		10
#define MAX_INPUTS				32

/* FIXME: Find out why we need so much! */
#define DEFAULT_FRAMES_PER_BUFFER	4096

#define STREAM_MAX_BUFFERS	10
#define STREAM_MIN_BUFFERS	2

enum {
	STREAM_PLAYBACK,
	STREAM_RECORD
};

struct hda_codec;

/*!	This structure describes a single stream of audio data,
	which is can have multiple channels (for stereo or better).
*/
struct hda_stream {
	uint32		id;					/* HDA controller stream # */
	uint32		off;				/* HDA I/O/B descriptor offset */
	bool		running;			/* Is this stream active? */
	spinlock	lock;				/* Write lock */
	uint32		type;

	uint32		pin_widget;			/* PIN Widget ID */
	uint32		io_widget;			/* Input/Output Converter Widget ID */

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
	uint32		frames_count;
	uint32		buffer_cycle;

	uint32		rate, bps;			/* Samplerate & bits per sample */

	area_id		buffer_area;
	area_id		buffer_descriptors_area;
	uint32		physical_buffer_descriptors;	/* BDL physical address */
};

struct hda_widget {
	uint32			node_id;

	uint32			num_inputs;
	int32			active_input;
	uint32			inputs[MAX_INPUTS];
	uint32			flags;

	hda_widget_type	type;
	uint32			pm;

	struct {
		uint32		audio;
		uint32		output_amplifier;
		uint32		input_amplifier;
	} capabilities;

	union {
		struct {
			uint32	formats;
			uint32	rates;
		} output;
		struct {
			uint32	formats;
			uint32	rates;
		} input;
		struct {
		} mixer;
		struct {
			uint32	output;
			uint32	input;
			pin_dev_type device;
		} pin;
	} d;
};

/*!	This structure describes a single Audio Function Group. An AFG
	is a group of audio widgets which can be used to configure multiple
	streams of audio either from the HDA Link to an output device (= playback)
	or from an input device to the HDA link (= recording).
*/
struct hda_audio_group {
	hda_codec*		codec;

	/* Multi Audio API data */
	hda_stream*		playback_stream;
	hda_stream*		record_stream;

	uint32			root_node_id;
	uint32			widget_start;
	uint32			widget_count;

	uint32			supported_formats;
	uint32			supported_rates;
	uint32			supported_pm;

	uint32			input_amplifier_capabilities;
	uint32			output_amplifier_capabilities;

	hda_widget*		widgets;
};

/*!	This structure describes a single codec module in the
	HDA compliant device. This is a discrete component, which
	can contain both Audio Function Groups, Modem Function Groups,
	and other customized (vendor specific) Function Groups.

	NOTE: ATM, only Audio Function Groups are supported.
*/
struct hda_codec {
	uint16		vendor_id;
	uint16		product_id;
	uint8		revision;
	uint16		stepping;		
	uint8		addr;

	sem_id		response_sem;
	uint32		responses[MAX_CODEC_RESPONSES];
	uint32		response_count;

	hda_audio_group* audio_groups[HDA_MAX_AUDIO_GROUPS];
	uint32		num_audio_groups;

	struct hda_controller* controller;
};

/*!	This structure describes a single HDA compliant 
	controller. It contains a list of available streams
	for use by the codecs contained, and the messaging queue
	(verb/response) buffers for communication.
*/
struct hda_controller {
	struct pci_info	pci_info;
	vint32			opened;
	const char*		devfs_path;

	area_id			regs_area;
	vuint8*			regs;
	uint32			irq;

	uint16			codec_status;
	uint32			num_input_streams;
	uint32			num_output_streams;
	uint32			num_bidir_streams;
	
	uint32			corb_length;
	uint32			rirb_length;
	uint32			rirb_read_pos;
	uint32			corb_write_pos;
	area_id			corb_rirb_pos_area;
	corb_t*			corb;
	rirb_t*			rirb;
	uint32*			stream_positions;

	hda_codec*		codecs[HDA_MAX_CODECS + 1];
	hda_codec*		active_codec;
	uint32			num_codecs;
	
	hda_stream*		streams[HDA_MAX_STREAMS];
};

/* driver.c */
extern device_hooks gDriverHooks;
extern pci_module_info* gPci;
extern hda_controller gCards[MAX_CARDS];
extern uint32 gNumCards;

/* hda_codec.c */
hda_codec* hda_codec_new(hda_controller* controller, uint32 cad);
void hda_codec_delete(hda_codec* codec);

/* hda_multi_audio.c */
status_t multi_audio_control(void* cookie, uint32 op, void* arg, size_t length);

/* hda_controller.c: Basic controller support */
status_t hda_hw_init(hda_controller* controller);
void hda_hw_stop(hda_controller* controller);
void hda_hw_uninit(hda_controller* controller);
status_t hda_send_verbs(hda_codec* codec, corb_t* verbs, uint32* responses,
	uint32 count);

/* hda_controller.c: Stream support */
hda_stream* hda_stream_new(hda_controller* controller, int type);
void hda_stream_delete(hda_stream* stream);
status_t hda_stream_setup_buffers(hda_audio_group* audioGroup,
	hda_stream* stream, const char* desc);
status_t hda_stream_start(hda_controller* controller, hda_stream* stream);
status_t hda_stream_stop(hda_controller* controller, hda_stream* stream);

#endif	/* _HDA_H_ */
