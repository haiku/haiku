//------------------------------------------------------------------------------
//	EchoGals/Echo24 BeOS Driver for Echo audio cards
//
//  Copyright (c) 2003, Jérôme Duval
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

#ifndef _ECHO_H_
#define _ECHO_H_

#ifdef CARDBUS
#include <pcmcia/cb_enabler.h>
#else
#include <PCI.h>
#endif
#include "OsSupportBeOS.h"
#include "CEchoGals.h"
#include "hmulti_audio.h"
#include "multi.h"
#include "queue.h"

#define AUTHOR "Jérôme Duval"
#define DEVNAME 32
#define NUM_CARDS 3

/*
 * Echo midi
 */

typedef struct _midi_dev {
	int32		count;
	char		name[64];
	sem_id 		midi_ready_sem;
	ECHOGALS_MIDI_IN_CONTEXT	context;
} midi_dev;

#define	ECHO_USE_PLAY		(1 << 0)
#define	ECHO_USE_RECORD	(1 << 1)
#define ECHO_STATE_STARTED	(1 << 0)

/*
 * Streams
 */

typedef struct _echo_stream {
	struct _echo_dev 	*card;
	uint8        		use;
	uint8        		state;
	uint8        		bitsPerSample;
	uint32       		sample_rate;
	uint8				channels;
	uint32 				bufframes;
	uint8 				bufcount;

	WORD				pipe;
	PDWORD				position;

	LIST_ENTRY(_echo_stream)	next;

	void            	(*inth) (void *);
	void           		*inthparam;

	echo_mem 	*buffer;
	uint16       blksize;	/* in samples */
	uint16       trigblk;	/* blk on which to trigger inth */
	uint16       blkmod;	/* Modulo value to wrap trigblk */

	/* multi_audio */
	volatile int64	frames_count;	// for play or record
	volatile bigtime_t real_time;	// for play or record
	volatile int32	buffer_cycle;	// for play or record
	int32 first_channel;
	bool update_needed;
} echo_stream;


typedef struct _echo_dev {
	char		name[DEVNAME];	/* used for resources */
	pci_info	info;

	uint32	bmbar;
	void *	log_bmbar;
	area_id area_bmbar;
	uint32	irq;
	uint16	type;

	PCEchoGals	pEG;
	ECHOGALS_CAPS	caps;
	NUINT		mixer;
	PCOsSupport pOSS;

	void	*ptb_log_base;
	phys_addr_t	ptb_phy_base;
	area_id ptb_area;

	sem_id buffer_ready_sem;

	LIST_HEAD(, _echo_stream) streams;
	LIST_HEAD(, _echo_mem) mems;

	echo_stream		*pstream;
	echo_stream		*rstream;

	/* multi_audio */
	multi_dev	multi;
#ifdef MIDI_SUPPORT
	midi_dev 	midi;
#endif
#ifdef CARDBUS
	bool				plugged;					// Device plugged
	bool				opened;						// Device opened
	int32				index;
	LIST_ENTRY(_echo_dev)	next;
#endif
} echo_dev;

extern int32 num_cards;
#ifdef CARDBUS
LIST_HEAD(_echodevs, _echo_dev);
extern struct _echodevs	devices;
#else
extern echo_dev cards[NUM_CARDS];
#endif

#ifdef __cplusplus
extern "C" {
#endif

status_t echo_stream_set_audioparms(echo_stream *stream, uint8 channels,
			     uint8 bitsPerSample, uint32 sample_ratei, uint8 index);
status_t echo_stream_get_nth_buffer(echo_stream *stream, uint8 chan, uint8 buf,
					char** buffer, size_t *stride);
void echo_stream_start(echo_stream *stream, void (*inth) (void *), void *inthparam);
void echo_stream_halt(echo_stream *stream);
echo_stream *echo_stream_new(echo_dev *card, uint8 use, uint32 bufframes, uint8 bufcount);
void echo_stream_delete(echo_stream *stream);

#ifdef __cplusplus
}
#endif


#endif /* _ECHO_H_ */
