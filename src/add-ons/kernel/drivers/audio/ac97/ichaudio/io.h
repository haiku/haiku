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
#ifndef _IO_H_
#define _IO_H_

#include "lala/lala.h"

uint8	ich_reg_read_8(ichaudio_cookie *cookie, int regno);
uint16	ich_reg_read_16(ichaudio_cookie *cookie, int regno);
uint32	ich_reg_read_32(ichaudio_cookie *cookie, int regno);

void	ich_reg_write_8(ichaudio_cookie *cookie, int regno, uint8 value);
void	ich_reg_write_16(ichaudio_cookie *cookie, int regno, uint16 value);
void	ich_reg_write_32(ichaudio_cookie *cookie, int regno, uint32 value);

uint16	ich_codec_read(void *_cookie, int regno);
void	ich_codec_write(void *_cookie, int regno, uint16 value);

#endif
