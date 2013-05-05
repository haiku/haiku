/*
 * ice1712 BeOS/Haiku Driver for VIA - VT1712 Multi Channel Audio Controller
 *
 * Copyright (c) 2002, Jerome Duval		(jerome.duval@free.fr)
 * Copyright (c) 2003, Marcus Overhagen	(marcus@overhagen.de)
 * Copyright (c) 2007, Jerome Leveque	(leveque.jerome@neuf.fr)
 *
 * All rights reserved
 * Distributed under the terms of the MIT license.
 */

#ifndef _IO_H_
#define _IO_H_

#include "ice1712.h"

#include <SupportDefs.h>

//------------------------------------------------------
//------------------------------------------------------
//Address are [PCI_10] + xx
uint8	read_ccs_uint8(ice1712 *ice,	int8 regno);
uint16	read_ccs_uint16(ice1712 *ice,	int8 regno);
uint32	read_ccs_uint32(ice1712 *ice,	int8 regno);

void	write_ccs_uint8(ice1712 *ice,	int8 regno,	uint8 value);
void	write_ccs_uint16(ice1712 *ice,	int8 regno,	uint16 value);
void	write_ccs_uint32(ice1712 *ice,	int8 regno,	uint32 value);
//------------------------------------------------------
//------------------------------------------------------
uint8	read_cci_uint8(ice1712 *ice,	int8 index);
void	write_cci_uint8(ice1712 *ice,	int8 index,	uint8 value);
//------------------------------------------------------
//------------------------------------------------------
//Address are [PCI_14] + xx
uint8	read_ddma_uint8(ice1712 *ice,	int8 regno);
uint16	read_ddma_uint16(ice1712 *ice,	int8 regno);
uint32	read_ddma_uint32(ice1712 *ice,	int8 regno);

void	write_ddma_uint8(ice1712 *ice,	int8 regno,	uint8 value);
void	write_ddma_uint16(ice1712 *ice,	int8 regno,	uint16 value);
void	write_ddma_uint32(ice1712 *ice,	int8 regno,	uint32 value);
//------------------------------------------------------
//------------------------------------------------------
//Address are [PCI_18] + x
uint8	read_ds_uint8(ice1712 *ice,		int8 regno);
uint16	read_ds_uint16(ice1712 *ice,	int8 regno);
uint32	read_ds_uint32(ice1712 *ice,	int8 regno);

void	write_ds_uint8(ice1712 *ice,	int8 regno,	uint8 value);
void	write_ds_uint16(ice1712 *ice,	int8 regno,	uint16 value);
void	write_ds_uint32(ice1712 *ice,	int8 regno,	uint32 value);

typedef enum {
	DS8_REGISTER_BUFFER_0_BASE_ADDRESS = 0,
	DS8_REGISTER_BUFFER_0_BASE_COUNT,
	DS8_REGISTER_BUFFER_1_BASE_ADDRESS,
	DS8_REGISTER_BUFFER_1_BASE_COUNT,
	DS8_REGISTER_CONTROL_AND_STATUS,
	DS8_REGISTER_SAMPLING_RATE,
	DS8_REGISTER_LEFT_RIGHT_VOLUME,
} ds8_register;

uint32	read_ds_channel_data(ice1712 *ice, uint8 channel, ds8_register index);
void	write_ds_channel_data(ice1712 *ice, uint8 channel,
			ds8_register index, uint32 data);
//------------------------------------------------------
//------------------------------------------------------
//Address are [PCI_1C] + xx
uint8	read_mt_uint8(ice1712 *ice,		int8 regno);
uint16	read_mt_uint16(ice1712 *ice,	int8 regno);
uint32	read_mt_uint32(ice1712 *ice,	int8 regno);

void	write_mt_uint8(ice1712 *ice,	int8 regno,	uint8 value);
void	write_mt_uint16(ice1712 *ice,	int8 regno,	uint16 value);
void	write_mt_uint32(ice1712 *ice,	int8 regno,	uint32 value);
//------------------------------------------------------
//------------------------------------------------------

int16	read_i2c(ice1712 *ice, uint8 dev_addr, uint8 byte_addr);
//return -1 if error else return an uint8

int16	write_i2c(ice1712 *ice, uint8 dev_addr, uint8 byte_addr, uint8 value);
//return -1 if error else return 0

//------------------------------------------------------

int16 read_eeprom(ice1712 *ice, uint8 eeprom[32]);

//------------------------------------------------------

void codec_write(ice1712 *ice, uint8 reg_addr, uint8 data);
void spdif_write(ice1712 *ice, uint8 reg_addr, uint8 data);
void codec_write_mult(ice1712 *ice, uint8 reg_addr, uint8 datas[], uint8 size);
void spdif_write_mult(ice1712 *ice, uint8 reg_addr, uint8 datas[], uint8 size);

uint8 codec_read(ice1712 *ice, uint8 reg_addr);
uint8 spdif_read(ice1712 *ice, uint8 reg_addr);
uint8 codec_read_mult(ice1712 *ice, uint8 reg_addr, uint8 datas[], uint8 size);
uint8 spdif_read_mult(ice1712 *ice, uint8 reg_addr, uint8 datas[], uint8 size);

//------------------------------------------------------

uint8	read_gpio(ice1712 *ice);
//return -1 if error else return an uint8

void	write_gpio(ice1712 *ice, uint8 value);

#endif
