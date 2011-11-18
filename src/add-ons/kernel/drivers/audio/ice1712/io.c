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

#include "io.h"
#include "ice1712_reg.h"
#include "debug.h"

extern pci_module_info *pci;

static void ak45xx_write_gpio(ice1712 *ice, uint8 reg_addr,
							uint8 data, uint8 chip_select);

static void cs84xx_write_gpio(ice1712 *ice, uint8 reg_addr,
							uint8 data, uint8 chip_select);

static uint8 ak45xx_read_gpio(ice1712 *ice, uint8 reg_addr,
							uint8 chip_select) {return 0;} //Unimplemented

static uint8 cs84xx_read_gpio(ice1712 *ice, uint8 reg_addr,
							uint8 chip_select);

static void write_gpio_byte(ice1712 *ice, uint8 data, uint8 gpio_data);
static uint8 read_gpio_byte(ice1712 *ice, uint8 gpio_data);


//Address are [PCI_10] + xx

uint8
read_ccs_uint8(ice1712 *ice, int8 regno)
{
	return pci->read_io_8(ice->Controller + regno);
};


uint16
read_ccs_uint16(ice1712 *ice, int8 regno)
{
	return pci->read_io_16(ice->Controller + regno);
};


uint32
read_ccs_uint32(ice1712 *ice, int8 regno)
{
	return pci->read_io_32(ice->Controller + regno);
};


void
write_ccs_uint8(ice1712 *ice, int8 regno, uint8 value)
{
	pci->write_io_8(ice->Controller + regno, value);
};


void
write_ccs_uint16(ice1712 *ice, int8 regno, uint16 value)
{
	pci->write_io_16(ice->Controller + regno, value);
};


void
write_ccs_uint32(ice1712 *ice, int8 regno, uint32 value)
{
	pci->write_io_32(ice->Controller + regno, value);
};


uint8
read_cci_uint8(ice1712 *ice, int8 index)
{
	write_ccs_uint8(ice, CCS_CCI_INDEX, index);
	return read_ccs_uint8(ice, CCS_CCI_DATA);
};


void
write_cci_uint8(ice1712 *ice, int8 index, uint8 value)
{
	write_ccs_uint8(ice, CCS_CCI_INDEX, index);
	write_ccs_uint8(ice, CCS_CCI_DATA, value);
};

//--------------------------------------------------
//--------------------------------------------------
//Address are [PCI_14] + xx

uint8
read_ddma_uint8(ice1712 *ice, int8 regno)
{
	return pci->read_io_8(ice->DDMA + regno);
};


uint16
read_ddma_uint16(ice1712 *ice, int8 regno)
{
	return pci->read_io_16(ice->DDMA + regno);
};


uint32
read_ddma_uint32(ice1712 *ice, int8 regno)
{
	return pci->read_io_32(ice->DDMA + regno);
};


void
write_ddma_uint8(ice1712 *ice, int8 regno, uint8 value)
{
	pci->write_io_8(ice->DDMA + regno, value);
};


void
write_ddma_uint16(ice1712 *ice, int8 regno, uint16 value)
{
	pci->write_io_16(ice->DDMA + regno, value);
};


void
write_ddma_uint32(ice1712 *ice, int8 regno, uint32 value)
{
	pci->write_io_32(ice->DDMA + regno, value);
};


//--------------------------------------------------
//--------------------------------------------------
//Address are [PCI_18] + x
uint8
read_ds_uint8(ice1712 *ice, int8 regno)
{
	return pci->read_io_8(ice->DMA_Path + regno);
};


uint16
read_ds_uint16(ice1712 *ice, int8 regno)
{
	return pci->read_io_16(ice->DMA_Path + regno);
};


uint32
read_ds_uint32(ice1712 *ice, int8 regno)
{
	return pci->read_io_32(ice->DMA_Path + regno);
};


void
write_ds_uint8(ice1712 *ice, int8 regno, uint8 value)
{
	pci->write_io_8(ice->DMA_Path + regno, value);
};


void
write_ds_uint16(ice1712 *ice, int8 regno, uint16 value)
{
	pci->write_io_16(ice->DMA_Path + regno, value);
};


void
write_ds_uint32(ice1712 *ice, int8 regno, uint32 value)
{
	pci->write_io_32(ice->DMA_Path + regno, value);
};


uint32
read_ds_channel_data(ice1712 *ice, uint8 channel, ds8_register index)
{
	uint8 ds8_channel_index = channel << 4 | index;

	write_ds_uint8(ice,	DS_CHANNEL_INDEX, ds8_channel_index);
	return read_ds_uint32(ice, DS_CHANNEL_DATA);
}


void
write_ds_channel_data(ice1712 *ice, uint8 channel, ds8_register index,
	uint32 data)
{
	uint8 ds8_channel_index = channel << 4 | index;

	write_ds_uint8(ice,	DS_CHANNEL_INDEX, ds8_channel_index);
	write_ds_uint32(ice, DS_CHANNEL_DATA, data);
}


//--------------------------------------------------
//--------------------------------------------------
//Address are [PCI_1C] + xx

uint8
read_mt_uint8(ice1712 *ice,	int8 regno)
{
	return 	pci->read_io_8(ice->Multi_Track + regno);
};


uint16
read_mt_uint16(ice1712 *ice,	int8 regno)
{
	return 	pci->read_io_16(ice->Multi_Track + regno);
};


uint32
read_mt_uint32(ice1712 *ice,	int8 regno)
{
	return pci->read_io_32(ice->Multi_Track + regno);
};

void
write_mt_uint8(ice1712 *ice,	int8 regno,	uint8 value)
{
	pci->write_io_8(ice->Multi_Track + regno, value);
};


void
write_mt_uint16(ice1712 *ice,	int8 regno,	uint16 value)
{
	pci->write_io_16(ice->Multi_Track + regno, value);
};


void
write_mt_uint32(ice1712 *ice,	int8 regno,	uint32 value)
{
	pci->write_io_32(ice->Multi_Track + regno, value);
};


int16
read_i2c(ice1712 *ice, uint8 dev_addr, uint8 byte_addr)
{//return -1 if error else return an uint8

	if (read_ccs_uint8(ice, CCS_I2C_CONTROL_STATUS) != 0x80)
		return -1;
	write_ccs_uint8(ice, CCS_I2C_BYTE_ADDRESS, byte_addr);
	write_ccs_uint8(ice, CCS_I2C_DEV_ADDRESS, dev_addr);
	snooze(1000);
	return read_ccs_uint8(ice, CCS_I2C_DATA);
}


int16
write_i2c(ice1712 *ice, uint8 dev_addr, uint8 byte_addr, uint8 value)
{//return -1 if error else return 0
	if (read_ccs_uint8(ice, CCS_I2C_CONTROL_STATUS) != 0x80)
		return -1;

	write_ccs_uint8(ice, CCS_I2C_BYTE_ADDRESS, byte_addr);
	write_ccs_uint8(ice, CCS_I2C_DEV_ADDRESS, dev_addr);
	write_ccs_uint8(ice, CCS_I2C_DATA, value);
	return 0;
}


int16 read_eeprom(ice1712 *ice, uint8 eeprom[32])
{
	int i;
	int16 tmp;

	for (i = 0; i < 6; i++) {
		tmp = read_i2c(ice, I2C_EEPROM_ADDRESS_READ, i);
		if (tmp >= 0)
			eeprom[i] = (uint8)tmp;
		else
			return -1;
	}
	if (eeprom[4] > 32)
		return -1;
	for (i = 6; i < eeprom[4]; i++) {
		tmp = read_i2c(ice, I2C_EEPROM_ADDRESS_READ, i);
		if (tmp >= 0)
			eeprom[i] = (uint8)tmp;
		else
			return -1;
	}
	return eeprom[4];
}


void
codec_write(ice1712 *ice, uint8 reg_addr, uint8 data)
{
	switch (ice->product) {
		case ICE1712_SUBDEVICE_DELTA66:
		case ICE1712_SUBDEVICE_DELTA44:
			ak45xx_write_gpio(ice, reg_addr, data, DELTA66_CODEC_CS_0);
			ak45xx_write_gpio(ice, reg_addr, data, DELTA66_CODEC_CS_1);
			break;
		case ICE1712_SUBDEVICE_DELTA410:
		case ICE1712_SUBDEVICE_AUDIOPHILE_2496:
		case ICE1712_SUBDEVICE_DELTADIO2496:
			ak45xx_write_gpio(ice, reg_addr, data, AP2496_CODEC_CS);
			break;
		case ICE1712_SUBDEVICE_DELTA1010:
		case ICE1712_SUBDEVICE_DELTA1010LT:
			ak45xx_write_gpio(ice, reg_addr, data, DELTA1010LT_CODEC_CS_0);
			ak45xx_write_gpio(ice, reg_addr, data, DELTA1010LT_CODEC_CS_1);
			ak45xx_write_gpio(ice, reg_addr, data, DELTA1010LT_CODEC_CS_2);
			ak45xx_write_gpio(ice, reg_addr, data, DELTA1010LT_CODEC_CS_3);
			break;
		case ICE1712_SUBDEVICE_VX442:
			ak45xx_write_gpio(ice, reg_addr, data, VX442_CODEC_CS_0);
			ak45xx_write_gpio(ice, reg_addr, data, VX442_CODEC_CS_1);
			break;
	}
}


void
spdif_write(ice1712 *ice, uint8 reg_addr, uint8 data)
{
	switch (ice->product) {
		case ICE1712_SUBDEVICE_DELTA1010:
			break;
		case ICE1712_SUBDEVICE_DELTADIO2496:
			break;
		case ICE1712_SUBDEVICE_DELTA66:
			break;
		case ICE1712_SUBDEVICE_DELTA44:
			break;
		case ICE1712_SUBDEVICE_AUDIOPHILE_2496:
			cs84xx_write_gpio(ice, reg_addr, data, AP2496_SPDIF_CS);
			break;
		case ICE1712_SUBDEVICE_DELTA410:
			break;
		case ICE1712_SUBDEVICE_DELTA1010LT:
			cs84xx_write_gpio(ice, reg_addr, data, DELTA1010LT_SPDIF_CS);
			break;
		case ICE1712_SUBDEVICE_VX442:
			cs84xx_write_gpio(ice, reg_addr, data, VX442_SPDIF_CS);
			break;
	}
}


uint8
codec_read(ice1712 *ice, uint8 reg_addr)
{
	uint8 val = 0xFF;
	switch (ice->product) {
		case ICE1712_SUBDEVICE_DELTA66:
		case ICE1712_SUBDEVICE_DELTA44:
			val = ak45xx_read_gpio(ice, reg_addr, DELTA66_CODEC_CS_0);
			break;
		case ICE1712_SUBDEVICE_DELTA410:
		case ICE1712_SUBDEVICE_AUDIOPHILE_2496:
		case ICE1712_SUBDEVICE_DELTADIO2496:
			val = ak45xx_read_gpio(ice, reg_addr, AP2496_CODEC_CS);
			break;
		case ICE1712_SUBDEVICE_DELTA1010:
		case ICE1712_SUBDEVICE_DELTA1010LT:
			val = ak45xx_read_gpio(ice, reg_addr, DELTA1010LT_CODEC_CS_0);
			break;
		case ICE1712_SUBDEVICE_VX442:
			val = ak45xx_read_gpio(ice, reg_addr, VX442_CODEC_CS_0);
			break;
	}

	return val;
}


uint8
spdif_read(ice1712 *ice, uint8 reg_addr)
{
	uint8 val = 0xFF;
	switch (ice->product) {
		case ICE1712_SUBDEVICE_DELTA1010:
			break;
		case ICE1712_SUBDEVICE_DELTADIO2496:
			break;
		case ICE1712_SUBDEVICE_DELTA66:
			break;
		case ICE1712_SUBDEVICE_DELTA44:
			break;
		case ICE1712_SUBDEVICE_AUDIOPHILE_2496:
			val = cs84xx_read_gpio(ice, reg_addr, AP2496_SPDIF_CS);
			break;
		case ICE1712_SUBDEVICE_DELTA410:
			break;
		case ICE1712_SUBDEVICE_DELTA1010LT:
			val = cs84xx_read_gpio(ice, reg_addr, DELTA1010LT_SPDIF_CS);
			break;
		case ICE1712_SUBDEVICE_VX442:
			val = cs84xx_read_gpio(ice, reg_addr, VX442_SPDIF_CS);
			break;
	}

	return val;
}

void
write_gpio_byte(ice1712 *ice, uint8 data, uint8 gpio_data)
{
	int i;

	for (i = 7; i >= 0; i--) {
		// drop clock and data bits
		gpio_data &= ~(ice->CommLines.clock | ice->CommLines.data_out);

		// set data bit if needed
		if (data & (1 << i))
			gpio_data |= ice->CommLines.data_out;

		write_gpio(ice, gpio_data);
		snooze(GPIO_I2C_DELAY);

		// raise clock
		gpio_data |= ice->CommLines.clock;
		write_gpio(ice, gpio_data);
		snooze(GPIO_I2C_DELAY);
	}
}

uint8
read_gpio_byte(ice1712 *ice, uint8 gpio_data)
{
	int i;
	uint8 data = 0;

	for (i = 7; i >= 0; i--) {
		// drop clock
		gpio_data &= ~(ice->CommLines.clock);
		write_gpio(ice, gpio_data);
		snooze(GPIO_I2C_DELAY);

		if (read_gpio(ice) &  ice->CommLines.data_in)
			data |= 1 << i;

		gpio_data |= ice->CommLines.clock;

		write_gpio(ice, gpio_data);
		snooze(GPIO_I2C_DELAY);
	}

	return data;
}

void
ak45xx_write_gpio(ice1712 *ice, uint8 reg_addr, uint8 data, uint8 chip_select)
{
	uint8 tmp;

	tmp = read_gpio(ice);
	tmp |= ice->CommLines.cs_mask;
	tmp &= ~(chip_select);

	write_gpio(ice, tmp);
	snooze(GPIO_I2C_DELAY);

	write_gpio_byte(ice, ((AK45xx_CHIP_ADDRESS & 0x03) << 6) | 0x20
		| (reg_addr & 0x1F), tmp);
	write_gpio_byte(ice, data, tmp);

	tmp |= chip_select;
	write_gpio(ice, tmp);
	snooze(GPIO_I2C_DELAY);
}

void
cs84xx_write_gpio(ice1712 *ice, uint8 reg_addr, uint8 data, uint8 chip_select)
{
	uint8 tmp;

	tmp = read_gpio(ice);
	tmp |= ice->CommLines.cs_mask;
	tmp &= ~(chip_select);

	write_gpio(ice, tmp);
	snooze(GPIO_I2C_DELAY);

	write_gpio_byte(ice, (CS84xx_CHIP_ADDRESS & 0x7F) << 1, tmp);
	write_gpio_byte(ice, reg_addr & 0x7F, tmp); //Do not Increment
	write_gpio_byte(ice, data, tmp);

	tmp |= chip_select;
	write_gpio(ice, tmp);
	snooze(GPIO_I2C_DELAY);
}

uint8
cs84xx_read_gpio(ice1712 *ice, uint8 reg_addr, uint8 chip_select)
{
	uint8 tmp, data;

	tmp = read_gpio(ice);
	tmp |= ice->CommLines.cs_mask;
	tmp &= ~(chip_select);

	write_gpio(ice, tmp);
	snooze(GPIO_I2C_DELAY);

	write_gpio_byte(ice, (CS84xx_CHIP_ADDRESS & 0x7F) << 1,
		tmp); //For writing the MAP
	write_gpio_byte(ice, reg_addr & 0x7F, tmp); //Do not Increment

	tmp |= chip_select; //Deselect the chip
	write_gpio(ice, tmp);
	snooze(GPIO_I2C_DELAY);

	tmp &= ~(chip_select); //Reselect the chip
	write_gpio(ice, tmp);
	snooze(GPIO_I2C_DELAY);

	write_gpio_byte(ice, (CS84xx_CHIP_ADDRESS & 0x7F) << 1 | 1,
		tmp); //For writing the MAP
	data = read_gpio_byte(ice, tmp); //For reading

	tmp |= chip_select; //Deselect the chip
	write_gpio(ice, tmp);

	return data;
}


uint8
read_gpio(ice1712 *ice)
{//return -1 if error else return an uint8
	return read_cci_uint8(ice, CCI_GPIO_DATA);
}


void
write_gpio(ice1712 *ice, uint8 value)
{//return -1 if error else return 0
	write_cci_uint8(ice, CCI_GPIO_DATA, value);
}

