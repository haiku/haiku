/*
 * BeOS Driver for Intel ICH AC'97 Link interface
 *
 * Copyright (c) 2002 - 2004, Marcus Overhagen <marcus@overhagen.de>
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef _ICH_H_
#define _ICH_H_

#include "debug.h"
#include "hardware.h"

#define VERSION_NUMBER "1.8"

#if DEBUG
	#define VERSION_DEBUG " (DEBUG)"
#else
	#define VERSION_DEBUG ""
#endif

#define INFO "Version " VERSION_NUMBER VERSION_DEBUG ", written by Marcus Overhagen. Build " __DATE__ " " __TIME__ 
#define DRIVER_NAME "ich_ac97"

#define BUFFER_SIZE		2048
#define BUFFER_COUNT	2
#define BUFFER_FRAMES_COUNT (BUFFER_SIZE / 4)

/* the software channel descriptor */
typedef struct {
	uint32	regbase;

	volatile int32	lastindex;
	volatile int64	played_frames_count;
	volatile bigtime_t played_real_time;
	volatile bool running;
	volatile void *backbuffer;
	sem_id buffer_ready_sem;

	void	*buffer_log_base;
	void	*buffer_phy_base;
	void 	*buffer[ICH_BD_COUNT];
	void	*bd_phy_base;
	void	*bd_log_base;
	ich_bd 	*bd[ICH_BD_COUNT];
	
	/* 
	 * a second set of buffers, exported to the multiaudio api (bad idea)
	 */
	void	*userbuffer[BUFFER_COUNT];
	void	*userbuffer_base;
	
	area_id buffer_area;
	area_id userbuffer_area;
	area_id bd_area;
} ich_chan;

extern ich_chan * chan_pi;
extern ich_chan * chan_po;
extern ich_chan * chan_mc;

void start_chan(ich_chan *chan);

#endif
