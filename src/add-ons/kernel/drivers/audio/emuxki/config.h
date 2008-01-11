/*
 * Emuxki BeOS Driver for Creative Labs SBLive!/Audigy series
 *
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
 *
 * Original code : BeOS Driver for Intel ICH AC'97 Link interface
 * Copyright (c) 2002, Marcus Overhagen <marcus@overhagen.de>
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
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <PCI.h>

#define NUM_CARDS 3
#define DEVNAME 32

typedef struct
{
	const char *name;
	uint32	nambar;
	uint32	nabmbar;
	uint32	irq;
	int		sample_size;
	bool	swap_reg;
	uint32	type;
} device_config;

#define TYPE_AUDIGY			0x01
#define TYPE_AUDIGY2		0x02
#define TYPE_AUDIGY2_VALUE	0x04
#define TYPE_LIVE_5_1		0x08

#define IS_AUDIGY(x) ((x)->type & TYPE_AUDIGY)
#define IS_AUDIGY2(x) ((x)->type & TYPE_AUDIGY2)
#define IS_AUDIGY2_VALUE(x) ((x)->type & TYPE_AUDIGY2_VALUE)
#define IS_LIVE_5_1(x) ((x)->type & TYPE_LIVE_5_1)
#endif
