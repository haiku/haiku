#ifndef _HDA_H_
#define _HDA_H_

#include <drivers/KernelExport.h>
#include <drivers/Drivers.h>
#include <drivers/PCI.h>

#include <string.h>
#include <stdlib.h>

#include "multi_audio.h"
#include "hda_controller_defs.h"
#include "hda_codec_defs.h"

#define MAXCARDS		4

/* values for the class_sub field for class_base = 0x04 (multimedia device) */
#define PCI_hd_audio	3

#define DEVFS_PATH_FORMAT	"audio/multi/hda/%lu"
#define MAXWORK				16
#define HDA_MAXCODECS		15
#define HDA_MAXSTREAMS		16
#define MAX_CODEC_RESPONSES	10
#define MAXINPUTS			32

typedef struct hda_controller_s hda_controller;

#define STRMAXBUF	10
#define STRMINBUF	2

enum {
	STRM_PLAYBACK,
	STRM_RECORD
};

typedef struct hda_stream_info_s {
	uint32		id;						/* HDA controller stream # */
	uint32		off;					/* HDA I/O/B descriptor offset */
	bool		running;				/* Is this stream active? */

	uint32		pin_wid;				/* PIN Widget ID */
	uint32		io_wid;					/* Input/Output Converter Widget ID */

	uint32		samplerate;	
	uint32		sampleformat;
	
	uint32		num_buffers;
	uint32		num_channels;
	uint32		buffer_length;			/* size of buffer in samples */
	uint32		sample_size;
	void*		buffers[STRMAXBUF];		/* Virtual addresses for buffer */
	uint32		buffers_pa[STRMAXBUF];	/* Physical addresses for buffer */
	sem_id		buffer_ready_sem;
	bigtime_t	played_real_time;
	uint32		played_frames_count;

	area_id		buffer_area;
	area_id		bdl_area;
	uint32		bdl_pa;					/* BDL physical address */
} hda_stream;

typedef struct hda_codec_s {
	uint16	vendor_id;
	uint16	product_id;
	uint8	hda_rev;
	uint16	rev_stepping;		
	uint8	addr;

	sem_id	response_sem;
	uint32	responses[MAX_CODEC_RESPONSES];
	uint32	response_count;

	/* Multi Audio API data */
	hda_stream*	playback_stream;
	hda_stream*	record_stream;
	
	/* Function Group Data */
	uint32	afg_nid;
	uint32	afg_wid_start, afg_wid_count;
	uint32	afg_deffmts;
	uint32	afg_defrates;
	uint32	afg_defpm;

	struct {
		uint32			num_inputs;
		uint32			active_input;
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
				uint32			output	: 1;
				uint32			input	: 1;
				pin_dev_type	device;
			} pin;
		} d;
	} *afg_widgets;

	struct hda_controller_s* ctrlr;
} hda_codec;

struct hda_controller_s {
	pci_info	pcii;
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
	uint32			num_codecs;
	
	hda_stream*		streams[HDA_MAXSTREAMS];
};

extern device_hooks driver_hooks;

extern pci_module_info* pci;

extern hda_controller cards[MAXCARDS];
extern uint32 num_cards;

status_t hda_hw_init(hda_controller* ctrlr);
void hda_hw_stop(hda_controller* ctrlr);
void hda_hw_uninit(hda_controller* ctrlr);
hda_codec* hda_codec_new(hda_controller* ctrlr, uint32 cad);
status_t hda_send_verbs(hda_codec* codec, corb_t* verbs, uint32* responses, int count);
status_t multi_audio_control(void* cookie, uint32 op, void* arg, size_t len);
hda_stream* hda_stream_alloc(hda_controller* ctrlr, int type);
status_t hda_stream_setup_buffers(hda_codec* codec, hda_stream* s, const char* desc);
status_t hda_stream_start(hda_controller* ctrlr, hda_stream* s);
status_t hda_stream_stop(hda_controller* ctrlr, hda_stream* s);
status_t hda_stream_check_intr(hda_controller* ctrlr, hda_stream* s);

#endif /* _HDA_H_ */
