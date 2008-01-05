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
#ifndef _IO_H_
#define _IO_H_

#include "config.h"

uint8  emuxki_reg_read_8(device_config *config, int regno);
uint16 emuxki_reg_read_16(device_config *config, int regno);
uint32 emuxki_reg_read_32(device_config *config, int regno);

void emuxki_reg_write_8(device_config *config, int regno, uint8 value);
void emuxki_reg_write_16(device_config *config, int regno, uint16 value);
void emuxki_reg_write_32(device_config *config, int regno, uint32 value);

uint32 emuxki_chan_read(device_config *config, uint16 chano, uint32 reg);
void emuxki_chan_write(device_config *config, uint16 chano, uint32 reg, uint32 data);

void emuxki_dsp_addop(device_config *config, uint16 *pc, uint8 op,
		  uint16 r, uint16 a, uint16 x, uint16 y);
void emuxki_dsp_getop(device_config *config, uint16 *pc, uint8 *op,
		  uint16 *r, uint16 *a, uint16 *x, uint16 *y);

void emuxki_write_gpr(device_config *config, uint32 pc, uint32 data);
uint32 emuxki_read_gpr(device_config *config, uint32 pc);

uint16 emuxki_codec_read(device_config *config, int regno);
void emuxki_codec_write(device_config *config, int regno, uint16 value);

void emuxki_inte_enable(device_config *config, uint32 value);
void emuxki_inte_disable(device_config *config, uint32 value);

uint32 emuxki_p16v_read(device_config *config, uint16 chano, uint16 reg);
void emuxki_p16v_write(device_config *config, uint16 chano, uint16 reg, uint32 data);

#endif
