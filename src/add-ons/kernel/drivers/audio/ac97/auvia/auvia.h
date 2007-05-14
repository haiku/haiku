/*
 * Auvia BeOS Driver for Via VT82xx Southbridge audio
 *
 * Copyright (c) 2003, Jerome Duval (jerome.duval@free.fr)
 
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tyler C. Sarna
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef _DEV_PCI_AUVIA_H_
#define _DEV_PCI_AUVIA_H_

#include <Drivers.h>
#include <SupportDefs.h>
#include <OS.h>
#include <PCI.h>
#include "auviareg.h"
#include "config.h"
#include "queue.h"
#include "hmulti_audio.h"
#include "multi.h"

#define VIATECH_VENDOR_ID	0x1106	/* Via Technologies */
#define VIATECH_82C686_AC97_DEVICE_ID	0x3058	/* Via Technologies VT82C686 AC97 */
#define VIATECH_8233_AC97_DEVICE_ID		0x3059	/* Via Technologies VT8233 AC97 */

#define VIATECH_8233_AC97_REV_8233_10	0x10	/* rev 10 */
#define VIATECH_8233_AC97_REV_8233C		0x20
#define VIATECH_8233_AC97_REV_8233		0x30
#define VIATECH_8233_AC97_REV_8233A		0x40
#define VIATECH_8233_AC97_REV_8235		0x50
#define VIATECH_8233_AC97_REV_8237		0x60	//this is the 5.1 Card in the new Athlon64 boards

#define VERSION "Version alpha 2, Copyright (c) 2003 Jérôme Duval, compiled on " ## __DATE__ ## " " ## __TIME__ 
#define DRIVER_NAME "auvia"
#define FRIENDLY_NAME "Auvia"
#define FRIENDLY_NAME_686 FRIENDLY_NAME" 82C686"
#define FRIENDLY_NAME_8233C FRIENDLY_NAME" 8233C"
#define FRIENDLY_NAME_8233 FRIENDLY_NAME" 8233"
#define FRIENDLY_NAME_8233A FRIENDLY_NAME" 8233A"
#define FRIENDLY_NAME_8235 FRIENDLY_NAME" 8235"
#define FRIENDLY_NAME_8237 FRIENDLY_NAME" 8237"
#define AUTHOR "Jérôme Duval"

#define VIA_TABLE_SIZE	255

#define	AUVIA_USE_PLAY		(1 << 0)
#define	AUVIA_USE_RECORD	(1 << 1)
#define AUVIA_STATE_STARTED	(1 << 0)

/*
 * Auvia memory managment
 */

typedef struct _auvia_mem {
	LIST_ENTRY(_auvia_mem) next;
	void	*log_base;
	void	*phy_base;
	area_id area;
	size_t	size;
} auvia_mem;

/*
 * Streams
 */
 
typedef struct _auvia_stream {
	struct _auvia_dev 	*card;
	uint8        		use;
	uint8        		state;
	uint8        		b16;
	uint32       		sample_rate;
	uint8				channels;
	uint32 				bufframes;
	uint8 				bufcount;
	
	uint32				base;
	
	LIST_ENTRY(_auvia_stream)	next;
	
	void            	(*inth) (void *);
	void           		*inthparam;
	
	void	*dmaops_log_base;
	void	*dmaops_phy_base;
	area_id dmaops_area;
	
	auvia_mem *buffer;
	uint16       blksize;	/* in samples */
	uint16       trigblk;	/* blk on which to trigger inth */
	uint16       blkmod;	/* Modulo value to wrap trigblk */
	
	/* multi_audio */
	volatile int64	frames_count;	// for play or record
	volatile bigtime_t real_time;	// for play or record
	volatile int32	buffer_cycle;	// for play or record
	int32 first_channel;
	bool update_needed;
} auvia_stream;

/*
 * Devices
 */

typedef struct _auvia_dev {
	char		name[DEVNAME];	/* used for resources */
	pci_info	info;
	device_config config;
	
	void	*ptb_log_base;
	void	*ptb_phy_base;
	area_id ptb_area;
	
	sem_id buffer_ready_sem;
	
	uint32			interrupt_mask;
	
	LIST_HEAD(, _auvia_stream) streams;
	
	LIST_HEAD(, _auvia_mem) mems;
	
	auvia_stream	*pstream;
	auvia_stream	*rstream;
	
	/* multi_audio */
	multi_dev	multi;
} auvia_dev;

extern int32 num_cards;
extern auvia_dev cards[NUM_CARDS];

status_t auvia_stream_set_audioparms(auvia_stream *stream, uint8 channels,
			     uint8 b16, uint32 sample_rate);
status_t auvia_stream_commit_parms(auvia_stream *stream);
status_t auvia_stream_get_nth_buffer(auvia_stream *stream, uint8 chan, uint8 buf, 
					char** buffer, size_t *stride);
void auvia_stream_start(auvia_stream *stream, void (*inth) (void *), void *inthparam);
void auvia_stream_halt(auvia_stream *stream);
auvia_stream *auvia_stream_new(auvia_dev *card, uint8 use, uint32 bufframes, uint8 bufcount);
void auvia_stream_delete(auvia_stream *stream);

#endif /* _DEV_PCI_AUVIA_H_ */
