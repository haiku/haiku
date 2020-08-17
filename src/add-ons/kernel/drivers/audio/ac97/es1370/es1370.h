/*
 * ES1370 Haiku Driver for ES1370 audio
 *
 * Copyright 2003-2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jerome Duval, jerome.duval@free.fr
 */
 
#ifndef _DEV_PCI_ES1370_H_
#define _DEV_PCI_ES1370_H_

#include <Drivers.h>
#include <SupportDefs.h>
#include <OS.h>
#include <PCI.h>
#include "es1370reg.h"
#include "config.h"
#include "queue.h"
#include "hmulti_audio.h"
#include "multi.h"


#define VERSION "Version alpha 1, Copyright (c) 2007 Jérôme Duval, compiled on " __DATE__ " " __TIME__ 
#define DRIVER_NAME "es1370"
#define FRIENDLY_NAME "ES1370"
#define AUTHOR "Jérôme Duval"

#define FRIENDLY_NAME_ES1370 "ES1370"

#define	ES1370_USE_PLAY		(1 << 0)
#define	ES1370_USE_RECORD	(1 << 1)
#define ES1370_STATE_STARTED	(1 << 0)

/*
 * es1370 memory managment
 */

typedef struct _es1370_mem {
	LIST_ENTRY(_es1370_mem) next;
	void	*log_base;
	phys_addr_t	phy_base;
	area_id area;
	size_t	size;
} es1370_mem;

/*
 * Streams
 */
 
typedef struct _es1370_stream {
	struct _es1370_dev 	*card;
	uint8        		use;
	uint8        		state;
	uint8        		b16;
	uint32       		sample_rate;
	uint8				channels;
	uint32 				bufframes;
	uint8 				bufcount;
	
	uint32				base;
	
	LIST_ENTRY(_es1370_stream)	next;
	
	void            	(*inth) (void *);
	void           		*inthparam;
	
	void	*dmaops_log_base;
	void	*dmaops_phy_base;
	area_id dmaops_area;
	
	es1370_mem *buffer;
	uint16       blksize;	/* in samples */
	uint16       trigblk;	/* blk on which to trigger inth */
	uint16       blkmod;	/* Modulo value to wrap trigblk */
	
	/* multi_audio */
	volatile int64	frames_count;	// for play or record
	volatile bigtime_t real_time;	// for play or record
	volatile int32	buffer_cycle;	// for play or record
	int32 first_channel;
	bool update_needed;
} es1370_stream;

/*
 * Devices
 */

typedef struct _es1370_dev {
	char		name[DEVNAME];	/* used for resources */
	pci_info	info;
	device_config config;
	
	void	*ptb_log_base;
	void	*ptb_phy_base;
	area_id ptb_area;
	
	sem_id buffer_ready_sem;
	
	uint32			interrupt_mask;
	
	LIST_HEAD(, _es1370_stream) streams;
	
	LIST_HEAD(, _es1370_mem) mems;
	
	es1370_stream	*pstream;
	es1370_stream	*rstream;
	
	/* multi_audio */
	multi_dev	multi;
} es1370_dev;

#define ES1370_SETTINGS "es1370.settings"

typedef struct {
	uint32	sample_rate;
	uint32	buffer_frames;
	int32	buffer_count;
} es1370_settings;

extern es1370_settings current_settings;

extern int32 num_cards;
extern es1370_dev cards[NUM_CARDS];

status_t es1370_stream_set_audioparms(es1370_stream *stream, uint8 channels,
			     uint8 b16, uint32 sample_rate);
status_t es1370_stream_commit_parms(es1370_stream *stream);
status_t es1370_stream_get_nth_buffer(es1370_stream *stream, uint8 chan, uint8 buf, 
					char** buffer, size_t *stride);
void es1370_stream_start(es1370_stream *stream, void (*inth) (void *), void *inthparam);
void es1370_stream_halt(es1370_stream *stream);
es1370_stream *es1370_stream_new(es1370_dev *card, uint8 use, uint32 bufframes, uint8 bufcount);
void es1370_stream_delete(es1370_stream *stream);

#endif /* _DEV_PCI_ES1370_H_ */
