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
#ifndef _AC97_H_
#define _AC97_H_

enum AC97_REGISTER {
	AC97_RESET				= 0x00,
	AC97_MASTER_VOLUME		= 0x02,
	AC97_AUX_OUT_VOLUME		= 0x04,
	AC97_MONO_VOLUME		= 0x06,
	AC97_MASTER_TONE		= 0x08,
	AC97_PC_BEEP_VOLUME		= 0x0A,
	AC97_PHONE_VOLUME		= 0x0C,
	AC97_MIC_VOLUME			= 0x0E,
	AC97_LINE_IN_VOLUME		= 0x10,
	AC97_CD_VOLUME			= 0x12,
	AC97_VIDEO_VOLUME		= 0x14,
	AC97_AUX_IN_VOLUME		= 0x16,
	AC97_PCM_OUT_VOLUME		= 0x18,
	AC97_RECORD_SELECT		= 0x1A,
	AC97_RECORD_GAIN		= 0x1C,
	AC97_RECORD_GAIN_MIC	= 0x1E,
	AC97_GENERAL_PURPOSE	= 0x20,
	AC97_3D_CONTROL			= 0x22,
	AC97_PAGING				= 0x24,
	AC97_POWERDOWN			= 0x26,
	AC97_VENDOR_ID1			= 0x7C,
	AC97_VENDOR_ID2			= 0x7E
};

const char *	ac97_get_3d_stereo_enhancement();
const char *	ac97_get_vendor_id_description();
uint32			ac97_get_vendor_id();
void			ac97_amp_enable(bool yesno);
void			ac97_init();

#endif
