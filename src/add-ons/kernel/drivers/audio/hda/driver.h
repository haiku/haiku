/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 */
#ifndef _HDA_H_
#define _HDA_H_

#include <drivers/KernelExport.h>
#include <drivers/Drivers.h>
#include <drivers/PCI.h>

#include <string.h>
#include <stdlib.h>

#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	#define DEVFS_PATH_FORMAT	"audio/multi/hda/%lu"
	#include <multi_audio.h>
#else
	#define DEVFS_PATH_FORMAT	"audio/hmulti/hda/%lu"
	#include <hmulti_audio.h>
#endif

#include "hda_controller_defs.h"
#include "hda_codec_defs.h"

#define MAXCARDS		4

/* values for the class_sub field for class_base = 0x04 (multimedia device) */
#define PCI_hd_audio		3

#define HDA_MAXAFGS		15
#define HDA_MAXCODECS		15
#define HDA_MAXSTREAMS		16
#define MAX_CODEC_RESPONSES	10
#define MAXINPUTS		32

/* FIXME: Find out why we need so much! */
#define DEFAULT_FRAMESPERBUF	4096

typedef struct hda_controller_s hda_controller;
typedef struct hda_codec_s hda_codec;
typedef struct hda_afg_s hda_afg;

#define STRMAXBUF	10
#define STRMINBUF	2

enum {
	STRM_PLAYBACK,
	STRM_RECORD
};

/* hda_stream_info
 *
 * This structure describes a single stream of audio data,
 * which is can have multiple channels (for stereo or better).
 */

typedef struct hda_stream_info_s {
	uint32		id;				/* HDA controller stream # */
	uint32		off;				/* HDA I/O/B descriptor offset */
	bool		running;			/* Is this stream active? */
	spinlock	lock;				/* Write lock */

	uint32		pin_wid;			/* PIN Widget ID */
	uint32		io_wid;				/* Input/Output Converter Widget ID */

	uint32		samplerate;	
	uint32		sampleformat;

	uint32		num_buffers;
	uint32		num_channels;
	uint32		buffer_length;			/* size of buffer in samples */
	uint32		sample_size;
	void*		buffers[STRMAXBUF];		/* Virtual addresses for buffer */
	uint32		buffers_pa[STRMAXBUF];		/* Physical addresses for buffer */
	sem_id		buffer_ready_sem;
	bigtime_t	real_time;
	uint32		frames_count;
	uint32		buffer_cycle;

	uint32		rate, bps;			/* Samplerate & bits per sample */

	area_id		buffer_area;
	area_id		bdl_area;
	uint32		bdl_pa;				/* BDL physical address */
} hda_stream;

/* hda_afg
 *
 * This structure describes a single Audio Function Group. An afg
 * is a group of audio widgets which can be used to configure multiple
 * streams of audio either from the HDA Link to an output device (= playback)
 * or from an input device to the HDA link (= recording).
 */

struct hda_afg_s {
	hda_codec*		codec;

	/* Multi Audio API data */
	hda_stream*		playback_stream;
	hda_stream*		record_stream;

	
	uint32				root_nid,
					wid_start,
					wid_count;
						
	uint32				deffmts,
					defrates,
					defpm;

	struct {
		uint32			num_inputs;
		int32			active_input;
		uint32			inputs[MAXINPUTS];
		uint32			flags;

		hda_widget_type	type;
		uint32			pm;

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
				uint32		output;
				uint32		input;
				pin_dev_type	device;
			} pin;
		} d;
	} *widgets;
};

/* hda_codec
 *
 * This structure describes a single codec module in the
 * HDA compliant device. This is a discrete component, which
 * can contain both Audio Function Groups, Modem Function Groups,
 * and other customized (vendor specific) Function Groups.
 *
 * NOTE: Atm, only Audio Function Groups are supported.
 */

struct hda_codec_s {
	uint16	vendor_id;
	uint16	product_id;
	uint8	hda_rev;
	uint16	rev_stepping;		
	uint8	addr;

	sem_id	response_sem;
	uint32	responses[MAX_CODEC_RESPONSES];
	uint32	response_count;

	hda_afg*	afgs[HDA_MAXAFGS];
	uint32		num_afgs;
	
	struct hda_controller_s* controller;
};

/* hda_controller
 *
 * This structure describes a single HDA compliant 
 * controller. It contains a list of available streams
 * for use by the codecs contained, and the messaging queue
 * (verb/response) buffers for communication.
 */

struct hda_controller_s {
	struct pci_info	pci_info;
	vuint32		opened;
	const char*	devfs_path;
	
	area_id		regs_area;
	vuint8*		regs;
	uint32		irq;

	uint16		codecsts;
	uint32		num_input_streams;
	uint32		num_output_streams;
	uint32		num_bidir_streams;
	
	uint32		corblen;
	uint32		rirblen;
	uint32		rirbrp;
	uint32		corbwp;
	area_id		rb_area;
	corb_t*		corb;
	rirb_t*		rirb;

	hda_codec*		codecs[HDA_MAXCODECS];
	hda_codec*		active_codec;
	uint32			num_codecs;
	
	hda_stream*		streams[HDA_MAXSTREAMS];
};

/* driver.c */
extern device_hooks gDriverHooks;
extern pci_module_info* gPci;
extern hda_controller gCards[MAXCARDS];
extern uint32 gNumCards;

/* hda_codec.c */
hda_codec* hda_codec_new(hda_controller* controller, uint32 cad);
void hda_codec_delete(hda_codec*);

/* hda_multi_audio.c */
status_t multi_audio_control(void* cookie, uint32 op, void* arg, size_t len);

/* hda_controller.c: Basic controller support */
status_t hda_hw_init(hda_controller* controller);
void hda_hw_stop(hda_controller* controller);
void hda_hw_uninit(hda_controller* controller);
status_t hda_send_verbs(hda_codec* codec, corb_t* verbs, uint32* responses, int count);

/* hda_controller.c: Stream support */
hda_stream* hda_stream_new(hda_controller* controller, int type);
void hda_stream_delete(hda_stream* s);
status_t hda_stream_setup_buffers(hda_afg* afg, hda_stream* s, const char* desc);
status_t hda_stream_start(hda_controller* controller, hda_stream* s);
status_t hda_stream_stop(hda_controller* controller, hda_stream* s);
status_t hda_stream_check_intr(hda_controller* controller, hda_stream* s);

#endif /* _HDA_H_ */
