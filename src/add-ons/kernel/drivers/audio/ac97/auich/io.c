/*
 * Auich BeOS Driver for Intel Southbridge audio
 *
 * Copyright (c) 2003, Jerome Duval (jerome.duval@free.fr)
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
#include "io.h"
#include "auichreg.h"
#include "debug.h"
#include <PCI.h>

extern pci_module_info  *pci;

uint8
auich_reg_read_8(device_config *config, int regno)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 255) || regno <= 63);
	if (config->type & TYPE_ICH4) 
		return *(uint8 *)(((char *)config->log_mbbar) + regno);
	else
		return pci->read_io_8(config->nabmbar + regno);
}

uint16
auich_reg_read_16(device_config *config, int regno)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 255) || regno <= 63);
	if (config->type & TYPE_ICH4) 
		return *(uint16 *)(((char *)config->log_mbbar) + regno);
	else
		return pci->read_io_16(config->nabmbar + regno);
}

uint32
auich_reg_read_32(device_config *config, int regno)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 255) || regno <= 63);
	if (config->type & TYPE_ICH4) 
		return *(uint32 *)(((char *)config->log_mbbar) + regno);
	else
		return pci->read_io_32(config->nabmbar + regno);
}

void
auich_reg_write_8(device_config *config, int regno, uint8 value)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 255) || regno <= 63);
	if (config->type & TYPE_ICH4) 
		*(uint8 *)(((char *)config->log_mbbar) + regno) = value;
	else
		pci->write_io_8(config->nabmbar + regno, value);
}

void
auich_reg_write_16(device_config *config, int regno, uint16 value)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 255) || regno <= 63);
	if (config->type & TYPE_ICH4) 
		*(uint16 *)(((char *)config->log_mbbar) + regno) = value;
	else
		pci->write_io_16(config->nabmbar + regno, value);
}

void
auich_reg_write_32(device_config *config, int regno, uint32 value)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 255) || regno <= 63);
	if (config->type & TYPE_ICH4) 
		*(uint32 *)(((char *)config->log_mbbar) + regno) = value;
	else
		pci->write_io_32(config->nabmbar + regno, value);
}

/* codec */
static int
auich_codec_wait(device_config *config)
{
	int i;
	for (i = 0; i < 1100; i++) {
		if ((auich_reg_read_8(config, AUICH_REG_ACC_SEMA) & 0x01) == 0)
			return B_OK;
		if (i > 100)
			snooze(1);
	}
	return B_TIMED_OUT;
}

uint16
auich_codec_read(device_config *config, int regno)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 511) || regno <= 255);
	if(auich_codec_wait(config)!=B_OK) {
		PRINT(("codec busy (2)\n"));
		return -1;
	}
	
	if (config->type & TYPE_ICH4) 
		return *(uint16 *)(((char *)config->log_mmbar) + regno);
	else
		return pci->read_io_16(config->nambar + regno);
}

void
auich_codec_write(device_config *config, int regno, uint16 value)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 511) || regno <= 255);
	if(auich_codec_wait(config)!=B_OK) {
		PRINT(("codec busy (4)\n"));
		return;
	}
	if (config->type & TYPE_ICH4) 
		*(uint16 *)(((char *)config->log_mmbar) + regno) = value;
	else
		pci->write_io_16(config->nambar + regno, value);
}
