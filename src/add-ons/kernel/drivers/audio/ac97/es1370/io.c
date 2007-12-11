/*
 * ES1370 Haiku Driver for ES1370 audio
 *
 * Copyright 2002-2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen, marcus@overhagen.de
 *		Jerome Duval, jerome.duval@free.fr
 */
#include <KernelExport.h>
#include <OS.h>
#include "io.h"
#include "es1370reg.h"
#include "debug.h"
#include <PCI.h>

extern pci_module_info  *pci;

uint8
es1370_reg_read_8(device_config *config, int regno)
{
	ASSERT(regno >= 0);
	return pci->read_io_8(config->base + regno);
}

uint16
es1370_reg_read_16(device_config *config, int regno)
{
	ASSERT(regno >= 0);
	return pci->read_io_16(config->base + regno);
}

uint32
es1370_reg_read_32(device_config *config, int regno)
{
	ASSERT(regno >= 0);
	return pci->read_io_32(config->base + regno);
}

void
es1370_reg_write_8(device_config *config, int regno, uint8 value)
{
	ASSERT(regno >= 0);
	pci->write_io_8(config->base + regno, value);
}

void
es1370_reg_write_16(device_config *config, int regno, uint16 value)
{
	ASSERT(regno >= 0);
	pci->write_io_16(config->base + regno, value);
}

void
es1370_reg_write_32(device_config *config, int regno, uint32 value)
{
	ASSERT(regno >= 0);
	pci->write_io_32(config->base + regno, value);
}

/* codec */
static int
es1370_codec_wait(device_config *config)
{
	int i;
	for (i = 0; i < 1100; i++) {
		if ((es1370_reg_read_32(config, ES1370_REG_STATUS) & STAT_CWRIP) == 0)
			return B_OK;
		if (i > 100)
			spin(1);
	}
	return B_TIMED_OUT;
}

uint16
es1370_codec_read(device_config *config, int regno)
{
	ASSERT(regno >= 0);
	if(es1370_codec_wait(config)!=B_OK) {
		PRINT(("codec busy (2)\n"));
		return -1;
	}
	
	return pci->read_io_32(config->base + ES1370_REG_CODEC);
}

void
es1370_codec_write(device_config *config, int regno, uint16 value)
{
	ASSERT(regno >= 0);
	if(es1370_codec_wait(config)!=B_OK) {
		PRINT(("codec busy (4)\n"));
		return;
	}
	pci->write_io_32(config->base + ES1370_REG_CODEC, (regno << 8) | value);
}
