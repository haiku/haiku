/*
 * BeOS Driver for Intel ICH AC'97 Link interface
 *
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

#include "ac97.h"

typedef struct
{
	const char *name;
	uint32	nambar;
	uint32	nabmbar;
	uint32	irq;
	uint32	type;
	uint32	mmbar; // ich4
	uint32	mbbar; // ich4
	void *	log_mmbar; // ich4
	void *	log_mbbar; // ich4
	area_id area_mmbar; // ich4
	area_id area_mbbar; // ich4
	uint32	codecoffset;
	ac97_dev *ac97;

	uint32 input_rate;
	uint32 output_rate;
} device_config;

extern device_config *config;

status_t probe_device(void);

#define TYPE_DEFAULT		0x00
#define TYPE_ICH4			0x01
#define TYPE_SIS7012		0x02

/* The SIS7012 chipset has SR and PICB registers swapped when compared to Intel */
#define	GET_REG_X_PICB(cfg)		(((cfg)->type == TYPE_SIS7012) ? _ICH_REG_X_SR : _ICH_REG_X_PICB)
#define	GET_REG_X_SR(cfg)		(((cfg)->type == TYPE_SIS7012) ? _ICH_REG_X_PICB : _ICH_REG_X_SR)
/* Each 16 bit sample is counted as 1 in SIS7012 chipsets, 2 in all others */
#define GET_HW_SAMPLE_SIZE(cfg)	(((cfg)->type == TYPE_SIS7012) ? 1 : 2)

#endif
