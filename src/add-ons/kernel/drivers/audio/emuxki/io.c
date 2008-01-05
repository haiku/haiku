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
#include <KernelExport.h>
#include <OS.h>
#include <PCI.h>
#include "io.h"
#include "emuxkireg.h"

extern pci_module_info *pci;

uint8
emuxki_reg_read_8(device_config *config, int regno)
{
	return pci->read_io_8(config->nabmbar + regno);
}

uint16
emuxki_reg_read_16(device_config *config, int regno)
{
	return pci->read_io_16(config->nabmbar + regno);
}

uint32
emuxki_reg_read_32(device_config *config, int regno)
{
	return pci->read_io_32(config->nabmbar + regno);
}

void
emuxki_reg_write_8(device_config *config, int regno, uint8 value)
{
	pci->write_io_8(config->nabmbar + regno, value);
}

void
emuxki_reg_write_16(device_config *config, int regno, uint16 value)
{
	pci->write_io_16(config->nabmbar + regno, value);
}

void
emuxki_reg_write_32(device_config *config, int regno, uint32 value)
{
	pci->write_io_32(config->nabmbar + regno, value);
}

/* Emu10k1 Low level */

uint32
emuxki_chan_read(device_config *config, uint16 chano, uint32 reg)
{
	uint32       ptr, mask = 0xFFFFFFFF;
	uint8        size, offset = 0;
	
	ptr = ((((uint32) reg) << 16) & 
		(IS_AUDIGY(config) ? EMU_A_PTR_ADDR_MASK : EMU_PTR_ADDR_MASK)) |
		(chano & EMU_PTR_CHNO_MASK);
	if (reg & 0xff000000) {
		size = (reg >> 24) & 0x3f;
		offset = (reg >> 16) & 0x1f;
		mask = ((1 << size) - 1) << offset;
	}
	pci->write_io_32(config->nabmbar + EMU_PTR, ptr);
	ptr = (pci->read_io_32(config->nabmbar + EMU_DATA) & mask) >> offset;
	return ptr;
}

void
emuxki_chan_write(device_config *config, uint16 chano,
	      uint32 reg, uint32 data)
{
	uint32       ptr, mask;
	uint8        size, offset;
	
	ptr = ((((uint32) reg) << 16) & 
		(IS_AUDIGY(config) ? EMU_A_PTR_ADDR_MASK : EMU_PTR_ADDR_MASK)) |
		(chano & EMU_PTR_CHNO_MASK);
	if (reg & 0xff000000) {
		size = (reg >> 24) & 0x3f;
		offset = (reg >> 16) & 0x1f;
		mask = ((1 << size) - 1) << offset;
		data = ((data << offset) & mask) |
			(emuxki_chan_read(config, chano, reg & 0xFFFF) & ~mask);
	}
	pci->write_io_32(config->nabmbar + EMU_PTR, ptr);
	pci->write_io_32(config->nabmbar + EMU_DATA, data);
}

/* Microcode */

static void
emuxki_write_micro(device_config *config, uint32 pc, uint32 data)
{
	emuxki_chan_write(config, 0, (IS_AUDIGY(config) ? EMU_A_MICROCODEBASE :
		EMU_MICROCODEBASE ) + pc, data);
}

static uint32
emuxki_read_micro(device_config *config, uint32 pc)
{
	return emuxki_chan_read(config, 0, (IS_AUDIGY(config) ? EMU_A_MICROCODEBASE :
		EMU_MICROCODEBASE ) + pc);
}

void
emuxki_dsp_addop(device_config *config, uint16 *pc, uint8 op,
		  uint16 r, uint16 a, uint16 x, uint16 y)
{
	if(IS_AUDIGY(config)) {
		emuxki_write_micro(config, *pc << 1,
			    ((x << 12) & EMU_A_DSP_LOWORD_OPX_MASK) |
			    (y & EMU_A_DSP_LOWORD_OPY_MASK));
		emuxki_write_micro(config, (*pc << 1) + 1,
			    ((op << 24) & EMU_A_DSP_HIWORD_OPCODE_MASK) |
			    ((r << 12) & EMU_A_DSP_HIWORD_RESULT_MASK) |
			    (a & EMU_A_DSP_HIWORD_OPA_MASK));
	} else {
		emuxki_write_micro(config, *pc << 1,
			    ((x << 10) & EMU_DSP_LOWORD_OPX_MASK) |
			    (y & EMU_DSP_LOWORD_OPY_MASK));
		emuxki_write_micro(config, (*pc << 1) + 1,
			    ((op << 20) & EMU_DSP_HIWORD_OPCODE_MASK) |
			    ((r << 10) & EMU_DSP_HIWORD_RESULT_MASK) |
			    (a & EMU_DSP_HIWORD_OPA_MASK));
	}
	(*pc)++;
}

void
emuxki_dsp_getop(device_config *config, uint16 *pc, uint8 *op,
		  uint16 *r, uint16 *a, uint16 *x, uint16 *y)
{
	uint32 value;
	if(IS_AUDIGY(config)) {
		value = emuxki_read_micro(config, *pc << 1);
		*x = (value & EMU_A_DSP_LOWORD_OPX_MASK) >> 12;
		*y = value & EMU_A_DSP_LOWORD_OPY_MASK;
		value = emuxki_read_micro(config, (*pc << 1) + 1);
		*op = (value & EMU_A_DSP_HIWORD_OPCODE_MASK) >> 24;
		*r = (value & EMU_A_DSP_HIWORD_RESULT_MASK) >> 12;
		*a = value & EMU_A_DSP_HIWORD_OPA_MASK;
	} else {
		value = emuxki_read_micro(config, *pc << 1);
		*x = (value & EMU_DSP_LOWORD_OPX_MASK) >> 10;
		*y = value & EMU_DSP_LOWORD_OPY_MASK;
		value = emuxki_read_micro(config, (*pc << 1) + 1);
		*op = (value & EMU_DSP_HIWORD_OPCODE_MASK) >> 20;
		*r = (value & EMU_DSP_HIWORD_RESULT_MASK) >> 10;
		*a = value & EMU_DSP_HIWORD_OPA_MASK;
	}
	(*pc)++;
}

/* Gpr */

void
emuxki_write_gpr(device_config *config, uint32 pc, uint32 data)
{
	emuxki_chan_write(config, 0, IS_AUDIGY(config) ? EMU_A_DSP_GPR(pc) : 
		EMU_DSP_GPR(pc), data);
}

uint32
emuxki_read_gpr(device_config *config, uint32 pc)
{
	return emuxki_chan_read(config, 0, IS_AUDIGY(config) ? EMU_A_DSP_GPR(pc) :
		EMU_DSP_GPR(pc));
}

/* codec */

uint16
emuxki_codec_read(device_config *config, int regno)
{
	pci->write_io_8(config->nabmbar + EMU_AC97ADDR, regno);
	return pci->read_io_16(config->nabmbar + EMU_AC97DATA);
}

void
emuxki_codec_write(device_config *config, int regno, uint16 value)
{
	pci->write_io_8(config->nabmbar + EMU_AC97ADDR, regno);
	pci->write_io_16(config->nabmbar + EMU_AC97DATA, value);
}

/* inte */

void
emuxki_inte_enable(device_config *config, uint32 value)
{
	emuxki_reg_write_32(config, EMU_INTE,
		emuxki_reg_read_32(config, EMU_INTE) | value);
}

void
emuxki_inte_disable(device_config *config, uint32 value)
{
	emuxki_reg_write_32(config, EMU_INTE,
		emuxki_reg_read_32(config, EMU_INTE) & ~value);
}

/* p16v */
uint32
emuxki_p16v_read(device_config *config, uint16 chano, uint16 reg)
{
	emuxki_reg_write_32(config, EMU_A2_PTR, reg << 16 | chano);
	return emuxki_reg_read_32(config, EMU_A2_DATA);
}


void 
emuxki_p16v_write(device_config *config, uint16 chano, uint16 reg, uint32 data)
{
	emuxki_reg_write_32(config, EMU_A2_PTR, reg << 16 | chano);
	emuxki_reg_write_32(config, EMU_A2_DATA, data);
}
