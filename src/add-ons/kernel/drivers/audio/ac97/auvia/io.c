/*
 * Auvia BeOS Driver for Via VT82xx Southbridge audio
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
#include "auviareg.h"
#include "debug.h"
#include <PCI.h>

extern pci_module_info  *pci;

uint8
auvia_reg_read_8(device_config *config, int regno)
{
	return pci->read_io_8(config->nabmbar + regno);
}

uint16
auvia_reg_read_16(device_config *config, int regno)
{
	return pci->read_io_16(config->nabmbar + regno);
}

uint32
auvia_reg_read_32(device_config *config, int regno)
{
	return pci->read_io_32(config->nabmbar + regno);
}

void
auvia_reg_write_8(device_config *config, int regno, uint8 value)
{
	pci->write_io_8(config->nabmbar + regno, value);
}

void
auvia_reg_write_16(device_config *config, int regno, uint16 value)
{
	pci->write_io_16(config->nabmbar + regno, value);
}

void
auvia_reg_write_32(device_config *config, int regno, uint32 value)
{
	pci->write_io_32(config->nabmbar + regno, value);
}

/* codec */

#define AUVIA_TIMEOUT 	200

static int
auvia_codec_waitready(device_config *config)
{
	int i;
	
	/* poll until codec not busy */
	for(i = 0; (i < AUVIA_TIMEOUT) && (pci->read_io_32(config->nabmbar 
		+ AUVIA_CODEC_CTL) & AUVIA_CODEC_BUSY) ; i++)
		spin(1);
	if(i>=AUVIA_TIMEOUT) {
		//PRINT(("codec busy\n"));
		return B_ERROR;
	}
	return B_OK;	
}

static int
auvia_codec_waitvalid(device_config *config)
{
	int i;
	
	/* poll until codec valid */
	for(i = 0; (i < AUVIA_TIMEOUT) && !(pci->read_io_32(config->nabmbar 
		+ AUVIA_CODEC_CTL) & AUVIA_CODEC_PRIVALID) ; i++)
		spin(1);
	if(i>=AUVIA_TIMEOUT) {
		//PRINT(("codec invalid\n"));
		return B_ERROR;
	}
	return B_OK;	
}

uint16
auvia_codec_read(device_config *config, int regno)
{
	if(auvia_codec_waitready(config)!=B_OK) {
		PRINT(("codec busy (1)\n"));
		return -1;
	}
	pci->write_io_32(config->nabmbar + AUVIA_CODEC_CTL, 
		AUVIA_CODEC_PRIVALID | AUVIA_CODEC_READ | AUVIA_CODEC_INDEX(regno));
	
	if(auvia_codec_waitready(config)!=B_OK) {
		PRINT(("codec busy (2)\n"));
		return -1;
	}
	if(auvia_codec_waitvalid(config)!=B_OK) {
		PRINT(("codec invalid (3)\n"));
		return -1;
	}
	
	return pci->read_io_16(config->nabmbar + AUVIA_CODEC_CTL);
}

void
auvia_codec_write(device_config *config, int regno, uint16 value)
{
	if(auvia_codec_waitready(config)!=B_OK) {
		PRINT(("codec busy (4)\n"));
		return;
	}
	pci->write_io_32(config->nabmbar + AUVIA_CODEC_CTL, 
		AUVIA_CODEC_PRIVALID | AUVIA_CODEC_INDEX(regno) | value);
}
