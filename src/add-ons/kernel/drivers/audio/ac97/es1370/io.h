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
#ifndef _IO_H_
#define _IO_H_

#include "config.h"

uint8  es1370_reg_read_8(device_config *config, int regno);
uint16 es1370_reg_read_16(device_config *config, int regno);
uint32 es1370_reg_read_32(device_config *config, int regno);

void es1370_reg_write_8(device_config *config, int regno, uint8 value);
void es1370_reg_write_16(device_config *config, int regno, uint16 value);
void es1370_reg_write_32(device_config *config, int regno, uint32 value);

uint16 es1370_codec_read(device_config *config, int regno);
void es1370_codec_write(device_config *config, int regno, uint16 value);
	       
#endif
