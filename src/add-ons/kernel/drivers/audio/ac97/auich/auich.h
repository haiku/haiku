/*
 * auich BeOS Driver for Intel Southbridge audio
 *
 * Copyright (c) 2003, Jerome Duval (jerome.duval@free.fr)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _DEV_PCI_AUICH_H_
#define _DEV_PCI_AUICH_H_

#include <Drivers.h>
#include <SupportDefs.h>
#include <OS.h>
#include <PCI.h>
#include "auichreg.h"
#include "config.h"
#include "queue.h"
#include "hmulti_audio.h"
#include "multi.h"

#define INTEL_VENDOR_ID		0x8086	/* Intel */
#define INTEL_82443MX_AC97_DEVICE_ID	0x7195
#define INTEL_82801AA_AC97_DEVICE_ID	0x2415
#define INTEL_82801AB_AC97_DEVICE_ID	0x2425
#define INTEL_82801BA_AC97_DEVICE_ID	0x2445
#define INTEL_82801CA_AC97_DEVICE_ID	0x2485
#define INTEL_82801DB_AC97_DEVICE_ID	0x24c5
#define INTEL_82801EB_AC97_DEVICE_ID	0x24d5
#define INTEL_82801FB_AC97_DEVICE_ID	0x266e
#define INTEL_82801GB_AC97_DEVICE_ID	0x27de
#define INTEL_6300ESB_AC97_DEVICE_ID	0x25a6
#define SIS_VENDOR_ID		0x1039	/* Sis */
#define SIS_SI7012_AC97_DEVICE_ID		0x7012
#define NVIDIA_VENDOR_ID	0x10de	/* Nvidia */
#define NVIDIA_nForce_AC97_DEVICE_ID	0x01b1
#define NVIDIA_nForce2_AC97_DEVICE_ID	0x006a
#define NVIDIA_nForce2_400_AC97_DEVICE_ID	0x008a
#define NVIDIA_nForce3_AC97_DEVICE_ID	0x00da
#define NVIDIA_nForce3_250_AC97_DEVICE_ID	0x00ea
#define NVIDIA_CK804_AC97_DEVICE_ID		0x0059
#define NVIDIA_MCP51_AC97_DEVICE_ID		0x026b
#define NVIDIA_MCP04_AC97_DEVICE_ID		0x003a
#define AMD_VENDOR_ID		0x1022	/* Amd */
#define AMD_AMD8111_AC97_DEVICE_ID		0x764d
#define AMD_AMD768_AC97_DEVICE_ID		0x7445

#define VERSION "Version alpha 1, Copyright (c) 2003 Jérôme Duval, compiled on " ## __DATE__ ## " " ## __TIME__
#define DRIVER_NAME "auich"
#define FRIENDLY_NAME "Auich"
#define AUTHOR "Jérôme Duval"

#define FRIENDLY_NAME_ICH "Auich ICH"
#define FRIENDLY_NAME_SIS "Auich SiS"
#define FRIENDLY_NAME_NVIDIA "Auich nVidia"
#define FRIENDLY_NAME_AMD "Auich AMD"

#define	AUICH_DMALIST_MAX	32
typedef struct _auich_dmalist {
	uint32	base;
	uint32	len;
#define	AUICH_DMAF_IOC		0x80000000	/* 1-int on complete */
#define	AUICH_DMAF_BUP		0x40000000	/* 0-retrans last, 1-transmit 0 */
} auich_dmalist;

#define	AUICH_USE_PLAY		(1 << 0)
#define	AUICH_USE_RECORD	(1 << 1)
#define AUICH_STATE_STARTED	(1 << 0)

/*
 * auich memory managment
 */

typedef struct _auich_mem {
	LIST_ENTRY(_auich_mem) next;
	void	*log_base;
	phys_addr_t	phy_base;
	area_id area;
	size_t	size;
} auich_mem;

/*
 * Streams
 */

typedef struct _auich_stream {
	struct _auich_dev 	*card;
	uint8        		use;
	uint8        		state;
	uint8        		b16;
	uint32       		sample_rate;
	uint8				channels;
	uint32 				bufframes;
	uint8 				bufcount;

	uint32				base;

	LIST_ENTRY(_auich_stream)	next;

	void            	(*inth) (void *);
	void           		*inthparam;

	void		*dmaops_log_base;
	phys_addr_t	dmaops_phy_base;
	area_id dmaops_area;

	auich_mem *buffer;
	uint16       blksize;	/* in samples */
	uint16       trigblk;	/* blk on which to trigger inth */
	uint16       blkmod;	/* Modulo value to wrap trigblk */

	uint16 		 		sta; /* GLOB_STA int bit */

	/* multi_audio */
	volatile int64	frames_count;	// for play or record
	volatile bigtime_t real_time;	// for play or record
	volatile int32	buffer_cycle;	// for play or record
	int32 first_channel;
	bool update_needed;
} auich_stream;

/*
 * Devices
 */

typedef struct _auich_dev {
	char		name[DEVNAME];	/* used for resources */
	pci_info	info;
	device_config config;

	void	*ptb_log_base;
	void	*ptb_phy_base;
	area_id ptb_area;

	sem_id buffer_ready_sem;

	uint32			interrupt_mask;

	LIST_HEAD(, _auich_stream) streams;

	LIST_HEAD(, _auich_mem) mems;

	auich_stream	*pstream;
	auich_stream	*rstream;

	/* multi_audio */
	multi_dev	multi;
} auich_dev;


#define AUICH_SETTINGS "auich.settings"

typedef struct {
	uint32	sample_rate;
	uint32	buffer_frames;
	int32	buffer_count;
	bool	use_thread;
} auich_settings;

extern auich_settings current_settings;

extern int32 num_cards;
extern auich_dev cards[NUM_CARDS];

status_t auich_stream_set_audioparms(auich_stream *stream, uint8 channels,
			     uint8 b16, uint32 sample_rate);
status_t auich_stream_commit_parms(auich_stream *stream);
status_t auich_stream_get_nth_buffer(auich_stream *stream, uint8 chan, uint8 buf,
					char** buffer, size_t *stride);
void auich_stream_start(auich_stream *stream, void (*inth) (void *), void *inthparam);
void auich_stream_halt(auich_stream *stream);
auich_stream *auich_stream_new(auich_dev *card, uint8 use, uint32 bufframes, uint8 bufcount);
void auich_stream_delete(auich_stream *stream);

#endif /* _DEV_PCI_AUICH_H_ */
