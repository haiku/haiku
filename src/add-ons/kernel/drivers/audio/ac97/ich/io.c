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
#include <KernelExport.h>
#include <OS.h>
#include <PCI.h>
#include "debug.h"
#include "io.h"
#include "ich.h"
#include "config.h"

extern pci_module_info *pci;

status_t ich_codec_wait(void);

uint8
ich_reg_read_8(int regno)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 255) || regno <= 63);
	if (config->type & TYPE_ICH4) 
		return *(uint8 *)(((char *)config->log_mbbar) + regno);
	else
		return pci->read_io_8(config->nabmbar + regno);
}

uint16
ich_reg_read_16(int regno)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 255) || regno <= 63);
	if (config->type & TYPE_ICH4) 
		return *(uint16 *)(((char *)config->log_mbbar) + regno);
	else
		return pci->read_io_16(config->nabmbar + regno);
}

uint32
ich_reg_read_32(int regno)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 255) || regno <= 63);
	if (config->type & TYPE_ICH4) 
		return *(uint32 *)(((char *)config->log_mbbar) + regno);
	else
		return pci->read_io_32(config->nabmbar + regno);
}

void
ich_reg_write_8(int regno, uint8 value)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 255) || regno <= 63);
	if (config->type & TYPE_ICH4) 
		*(uint8 *)(((char *)config->log_mbbar) + regno) = value;
	else
		pci->write_io_8(config->nabmbar + regno, value);
}

void
ich_reg_write_16(int regno, uint16 value)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 255) || regno <= 63);
	if (config->type & TYPE_ICH4) 
		*(uint16 *)(((char *)config->log_mbbar) + regno) = value;
	else
		pci->write_io_16(config->nabmbar + regno, value);
}

void
ich_reg_write_32(int regno, uint32 value)
{
	ASSERT(regno >= 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 255) || regno <= 63);
	if (config->type & TYPE_ICH4) 
		*(uint32 *)(((char *)config->log_mbbar) + regno) = value;
	else
		pci->write_io_32(config->nabmbar + regno, value);
}

status_t
ich_codec_wait(void)
{
	int i;
	for (i = 0; i < 1100; i++) {
		if ((ich_reg_read_8(ICH_REG_ACC_SEMA) & 0x01) == 0)
			return B_OK;
		if (i > 100)
			snooze(1);
	}
	return B_TIMED_OUT;
}

uint16
ich_codec_read(int regno)
{
	status_t rv;
	ASSERT(regno >= 0);
	ASSERT((regno & 1) == 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 511) || regno <= 255);
	if (regno == 0x54) // intel uses 0x54 for GPIO access, we filter it!
		return 0;
	rv = ich_codec_wait();
	if (rv != B_OK)
		PRINT(("semaphore timeout reading register %#x\n",regno));
	if (config->type & TYPE_ICH4) 
		return *(uint16 *)(((char *)config->log_mmbar) + regno);
	else
		return pci->read_io_16(config->nambar + regno);
}

void
ich_codec_write(int regno, uint16 value)
{
	status_t rv;
	ASSERT(regno >= 0);
	ASSERT((regno & 1) == 0);
	ASSERT(((config->type & TYPE_ICH4) != 0 && regno <= 511) || regno <= 255);
	if (regno == 0x54) // intel uses 0x54 for GPIO access, we filter it!
		return;
	rv = ich_codec_wait();
	if (rv != B_OK)
		PRINT(("semaphore timeout writing register %#x\n",regno));
	if (config->type & TYPE_ICH4) 
		*(uint16 *)(((char *)config->log_mmbar) + regno) = value;
	else
		pci->write_io_16(config->nambar + regno, value);
}
