
/*-
 * Copyright (c) 2009-2010 Alexander Egorenkov <egorenar@gmail.com>
 * Copyright (c) 2009 Damien Bergamini <damien.bergamini@free.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <dev/rt2860/rt2860_io.h>
#include <dev/rt2860/rt2860_reg.h>

/*
 * Defines and macros
 */

/*
 * RT2860_IO_EEPROM_RAISE_CLK
 */
#define RT2860_IO_EEPROM_RAISE_CLK(sc, val)							\
do																	\
{																	\
	(val) |= RT2860_REG_EESK;										\
																	\
	rt2860_io_mac_write((sc), RT2860_REG_EEPROM_CSR, (val));		\
																	\
	DELAY(1);														\
} while (0)

/*
 * RT2860_IO_EEPROM_LOWER_CLK
 */
#define RT2860_IO_EEPROM_LOWER_CLK(sc, val)							\
do																	\
{																	\
	(val) &= ~RT2860_REG_EESK;										\
																	\
	rt2860_io_mac_write((sc), RT2860_REG_EEPROM_CSR, (val));		\
																	\
	DELAY(1);														\
} while (0)

#define RT2860_IO_BYTE_CRC16(byte, crc)								\
	((uint16_t) (((crc) << 8) ^ rt2860_io_ccitt16[(((crc) >> 8) ^ (byte)) & 255]))

/*
 * Static function prototypes
 */

static void rt2860_io_eeprom_shiftout_bits(struct rt2860_softc *sc,
	uint16_t val, uint16_t count);

static uint16_t rt2860_io_eeprom_shiftin_bits(struct rt2860_softc *sc);

static uint8_t rt2860_io_byte_rev(uint8_t byte);

/* #ifdef RT305X_SOC */
static const uint16_t rt3052_eeprom[] = 
{
	0x3052, 0x0101, 0x0c00, 0x3043, 0x8852, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0x0c00, 0x3043, 0x7752, 0x0c00,
	0x3043, 0x6652, 0x0822, 0x0024, 0xffff, 0x012f, 0x7755, 0xaaa8,
	0x888c, 0xffff, 0x000c, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
	0xffff, 0x0d0d, 0x0d0d, 0x0c0c, 0x0c0c, 0x0c0c, 0x0c0c, 0x0c0c,
	0x1010, 0x1111, 0x1211, 0x1212, 0x1313, 0x1413, 0x1414, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x6666,
	0xaacc, 0x6688, 0xaacc, 0x6688, 0xaacc, 0x6688, 0xaacc, 0x6688,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
};

uint8_t rt3052_rf_default[] = {
	0x50,
	0x01,
	0xF7,
	0x75,
	0x40,
	0x03,
	0x42,
	0x50,
	0x39,
	0x0F,
	0x60,
	0x21,
	0x75,
	0x75,
	0x90,
	0x58,
	0xB3,
	0x92,
	0x2C,
	0x02,
	0xBA,
	0xDB,
	0x00,
	0x31,
	0x08,
	0x01,
	0x25, /* Core Power: 0x25=1.25V */
	0x23, /* RF: 1.35V */
      	0x13, /* ADC: must consist with R27 */
	0x83,
	0x00,
	0x00,
};
/* #endif */


/*
 * Static variables
 */

static const uint16_t rt2860_io_ccitt16[] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

/*
 * rt2860_io_mac_read
 */
uint32_t rt2860_io_mac_read(struct rt2860_softc *sc, uint16_t reg)
{
	bus_space_read_4(sc->bst, sc->bsh, RT2860_REG_MAC_CSR0);
	return bus_space_read_4(sc->bst, sc->bsh, reg);
}

/*
 * rt2860_io_mac_read_multi
 */
void rt2860_io_mac_read_multi(struct rt2860_softc *sc,
	uint16_t reg, void *buf, size_t len)
{
	bus_space_read_4(sc->bst, sc->bsh, RT2860_REG_MAC_CSR0);
	bus_space_read_region_1(sc->bst, sc->bsh, reg, buf, len);
}

/*
 * rt2860_io_mac_write
 */
void rt2860_io_mac_write(struct rt2860_softc *sc,
	uint16_t reg, uint32_t val)
{
	bus_space_read_4(sc->bst, sc->bsh, RT2860_REG_MAC_CSR0);
	bus_space_write_4(sc->bst, sc->bsh, reg, val);
}

/*
 * rt2860_io_mac_write_multi
 */
void rt2860_io_mac_write_multi(struct rt2860_softc *sc,
	uint16_t reg, const void *buf, size_t len)
{
	int i;
	const uint8_t *p;

	bus_space_read_4(sc->bst, sc->bsh, RT2860_REG_MAC_CSR0);

	p = buf;
	for (i = 0; i < len; i ++)
		bus_space_write_1(sc->bst, sc->bsh, reg + i, *(p+i));
//	bus_space_write_region_1(sc->bst, sc->bsh, reg, buf, len);
}

/*
 * rt2860_io_mac_set_region_4
 */
void rt2860_io_mac_set_region_4(struct rt2860_softc *sc,
	uint16_t reg, uint32_t val, size_t len)
{
	int i;

	for (i = 0; i < len; i += sizeof(uint32_t))
		rt2860_io_mac_write(sc, reg + i, val);
}

/* Read 16-bit from eFUSE ROM (>=RT3071 only.) */
static uint16_t
rt3090_efuse_read_2(struct rt2860_softc *sc, uint16_t addr)
{
	uint32_t tmp;
	uint16_t reg;
	int ntries;

	addr *= 2;
	/*-
	 * Read one 16-byte block into registers EFUSE_DATA[0-3]:
	 * DATA0: F E D C
	 * DATA1: B A 9 8
	 * DATA2: 7 6 5 4
	 * DATA3: 3 2 1 0
	 */
	tmp = rt2860_io_mac_read(sc, RT3070_EFUSE_CTRL);
	tmp &= ~(RT3070_EFSROM_MODE_MASK | RT3070_EFSROM_AIN_MASK);
	tmp |= (addr & ~0xf) << RT3070_EFSROM_AIN_SHIFT | RT3070_EFSROM_KICK;
	rt2860_io_mac_write(sc, RT3070_EFUSE_CTRL, tmp);
	for (ntries = 0; ntries < 500; ntries++) {
		tmp = rt2860_io_mac_read(sc, RT3070_EFUSE_CTRL);
		if (!(tmp & RT3070_EFSROM_KICK))
			break;
		DELAY(2);
	}
	if (ntries == 500)
		return 0xffff;

	if ((tmp & RT3070_EFUSE_AOUT_MASK) == RT3070_EFUSE_AOUT_MASK)
		return 0xffff;	/* address not found */

	/* determine to which 32-bit register our 16-bit word belongs */
	reg = RT3070_EFUSE_DATA3 - (addr & 0xc);
	tmp = rt2860_io_mac_read(sc, reg);

	return (addr & 2) ? tmp >> 16 : tmp & 0xffff;
}


/*
 * rt2860_io_eeprom_read
 */
uint16_t rt2860_io_eeprom_read(struct rt2860_softc *sc, uint16_t addr)
{
	uint32_t tmp;
	uint16_t val;

	addr = (addr >> 1);

	if (sc->mac_rev == 0x28720200) {
		return (rt3052_eeprom[addr]);
	} else if ((sc->mac_rev & 0xffff0000) >= 0x30710000) {
		tmp = rt2860_io_mac_read(sc, RT3070_EFUSE_CTRL);
		if (tmp & RT3070_SEL_EFUSE)
			return (rt3090_efuse_read_2(sc, addr));
	}

	tmp = rt2860_io_mac_read(sc, RT2860_REG_EEPROM_CSR);

	tmp &= ~(RT2860_REG_EEDI | RT2860_REG_EEDO | RT2860_REG_EESK);
	tmp |= RT2860_REG_EECS;

	rt2860_io_mac_write(sc, RT2860_REG_EEPROM_CSR, tmp);

	if (((sc->mac_rev & 0xffff0000) != 0x30710000) &&
		((sc->mac_rev & 0xffff0000) != 0x30900000) &&
		((sc->mac_rev & 0xffff0000) != 0x35720000) &&
		((sc->mac_rev & 0xffff0000) != 0x33900000))
	{
		RT2860_IO_EEPROM_RAISE_CLK(sc, tmp);
		RT2860_IO_EEPROM_LOWER_CLK(sc, tmp);
	}

	rt2860_io_eeprom_shiftout_bits(sc, RT2860_REG_EEOP_READ, 3);
	rt2860_io_eeprom_shiftout_bits(sc, addr, sc->eeprom_addr_num);

	val = rt2860_io_eeprom_shiftin_bits(sc);

	tmp = rt2860_io_mac_read(sc, RT2860_REG_EEPROM_CSR);

	tmp &= ~(RT2860_REG_EECS | RT2860_REG_EEDI);

	rt2860_io_mac_write(sc, RT2860_REG_EEPROM_CSR, tmp);

	RT2860_IO_EEPROM_RAISE_CLK(sc, tmp);
	RT2860_IO_EEPROM_LOWER_CLK(sc, tmp);

	return val;
}

/*
 * rt2860_io_eeprom_read_multi
 */
void rt2860_io_eeprom_read_multi(struct rt2860_softc *sc,
	uint16_t addr, void *buf, size_t len)
{
	uint16_t *ptr;
	int i;

	len += len % sizeof(uint16_t);
	ptr = buf;

	i = 0;

	do
	{
		*ptr++ = rt2860_io_eeprom_read(sc, addr + i);

		i += sizeof(uint16_t);
		len -= sizeof(uint16_t);
	} while (len > 0);
}

/*
 * rt2860_io_bbp_read
 */
uint8_t rt2860_io_bbp_read(struct rt2860_softc *sc, uint8_t reg)
{
	int ntries;
	uint32_t tmp;

	if (sc->mac_rev == 0x28720200)
	{
		for (ntries = 0; ntries < 100; ntries ++) {
			if ( !(rt2860_io_mac_read(sc, RT2860_REG_BBP_CSR_CFG) & 
			    RT2860_REG_BBP_CSR_BUSY) )
				break;
			DELAY(1);
		}
		if (ntries == 100) {
			printf("%s:%s: BBP busy after 100 probes\n",
				device_get_nameunit(sc->dev), __func__);
			return (0);
		}
		rt2860_io_mac_write(sc, RT2860_REG_BBP_CSR_CFG, 
		    RT2860_REG_BBP_CSR_READ |
		    RT2860_REG_BBP_CSR_KICK | RT2860_REG_BBP_RW_MODE_PARALLEL |
		    (reg & RT2860_REG_BBP_REG_MASK) << RT2860_REG_BBP_REG_SHIFT);
		for (ntries = 0; ntries < 100; ntries ++) {
			if ( !(rt2860_io_mac_read(sc, RT2860_REG_BBP_CSR_CFG) & 
			    RT2860_REG_BBP_CSR_BUSY) )
				break;
			DELAY(1);
		}
		if (ntries == 100) {
			printf("%s:%s: BBP busy after 100 probes\n",
				device_get_nameunit(sc->dev), __func__);
			return (0);
		}
		else {
			return 
			    ((rt2860_io_mac_read(sc, RT2860_REG_BBP_CSR_CFG) >>
			    RT2860_REG_BBP_VAL_SHIFT) & 
			    RT2860_REG_BBP_VAL_MASK);
		}
		return (0);
	}

	for (ntries = 0; ntries < 100; ntries++)
	{
		if (!(rt2860_io_mac_read(sc, RT2860_REG_H2M_MAILBOX_BBP_AGENT) &
			RT2860_REG_BBP_CSR_BUSY))
			break;

		DELAY(1);
	}

	if (ntries == 100)
	{
		printf("%s: could not read from BBP through MCU: reg=0x%02x\n",
			device_get_nameunit(sc->dev), reg);
		return 0;
	}

	tmp = RT2860_REG_BBP_RW_MODE_PARALLEL |
		RT2860_REG_BBP_CSR_BUSY |
		RT2860_REG_BBP_CSR_READ |
		((reg & RT2860_REG_BBP_REG_MASK) << RT2860_REG_BBP_REG_SHIFT);

	rt2860_io_mac_write(sc, RT2860_REG_H2M_MAILBOX_BBP_AGENT, tmp);

	rt2860_io_mcu_cmd(sc, RT2860_IO_MCU_CMD_BBP,
		RT2860_REG_H2M_TOKEN_NO_INTR, 0);

	DELAY(1000);

	for (ntries = 0; ntries < 100; ntries++)
	{
		tmp = rt2860_io_mac_read(sc, RT2860_REG_H2M_MAILBOX_BBP_AGENT);
		if (!(tmp & RT2860_REG_BBP_CSR_BUSY))
			return ((tmp >> RT2860_REG_BBP_VAL_SHIFT) &
				RT2860_REG_BBP_VAL_MASK);

		DELAY(1);
	}

	printf("%s: could not read from BBP through MCU: reg=0x%02x\n",
		device_get_nameunit(sc->dev), reg);

	return 0;
}

/*
 * rt2860_io_bbp_write
 */
void rt2860_io_bbp_write(struct rt2860_softc *sc, uint8_t reg, uint8_t val)
{
	int ntries;
	uint32_t tmp;

	if (sc->mac_rev == 0x28720200)
	{
		for (ntries = 0; ntries < 100; ntries ++) {
			if ( !(rt2860_io_mac_read(sc, RT2860_REG_BBP_CSR_CFG) & 
			    RT2860_REG_BBP_CSR_BUSY) )
				break;
			DELAY(1);
		}
		if (ntries == 100) {
			printf("%s:%s: BBP busy after 100 probes\n",
				device_get_nameunit(sc->dev), __func__);
			return;
		}
		rt2860_io_mac_write(sc, RT2860_REG_BBP_CSR_CFG, 
		    RT2860_REG_BBP_CSR_KICK | RT2860_REG_BBP_RW_MODE_PARALLEL |
		    (reg & RT2860_REG_BBP_REG_MASK) << RT2860_REG_BBP_REG_SHIFT | 
		    (val & RT2860_REG_BBP_VAL_MASK) << RT2860_REG_BBP_VAL_SHIFT );
		rt2860_io_bbp_read(sc, reg);
		return;
	}

	for (ntries = 0; ntries < 100; ntries++)
	{
		if (!(rt2860_io_mac_read(sc, RT2860_REG_H2M_MAILBOX_BBP_AGENT) &
			RT2860_REG_BBP_CSR_BUSY))
			break;

		DELAY(1);
	}

	if (ntries == 100)
	{
		printf("%s: could not write to BBP through MCU: reg=0x%02x\n",
			device_get_nameunit(sc->dev), reg);
		return;
	}

	tmp = RT2860_REG_BBP_RW_MODE_PARALLEL |
		RT2860_REG_BBP_CSR_BUSY |
		((reg & RT2860_REG_BBP_REG_MASK) << RT2860_REG_BBP_REG_SHIFT) |
		((val & RT2860_REG_BBP_VAL_MASK) << RT2860_REG_BBP_VAL_SHIFT);

	rt2860_io_mac_write(sc, RT2860_REG_H2M_MAILBOX_BBP_AGENT, tmp);

	rt2860_io_mcu_cmd(sc, RT2860_IO_MCU_CMD_BBP,
		RT2860_REG_H2M_TOKEN_NO_INTR, 0);

	DELAY(1000);
}

/*
 * rt2860_io_rf_write
 */
void rt2860_io_rf_write(struct rt2860_softc *sc, uint8_t reg, uint32_t val)
{
	int ntries;
	if (sc->mac_rev == 0x28720200)
	{
		for (ntries = 0; ntries < 100; ntries ++) {
			if ( !(rt2860_io_mac_read(sc, RT2872_REG_RF_CSR_CFG) & 
			    RT2872_REG_RF_CSR_BUSY) )
				break;
			DELAY(1);
		}
		if (ntries == 100) {
			printf("%s:%s: RF busy after 100 probes\n",
				device_get_nameunit(sc->dev), __func__);
			return;
		}
		rt2860_io_mac_write(sc, RT2872_REG_RF_CSR_CFG, 
		    RT2872_REG_RF_CSR_KICK | RT2872_REG_RF_CSR_WRITE |
		    (reg & RT2872_REG_RF_ID_MASK) << RT2872_REG_RF_ID_SHIFT | 
		    (val & RT2872_REG_RF_VAL_MASK) << RT2872_REG_RF_VAL_SHIFT );
		rt2860_io_rf_read(sc, reg);
		return;
	}


	for (ntries = 0; ntries < 100; ntries++)
		if (!(rt2860_io_mac_read(sc, RT2860_REG_RF_CSR_CFG0) &
			RT2860_REG_RF_BUSY))
			break;

	if (ntries == 100)
	{
		printf("%s: could not write to RF: reg=0x%02x\n",
			device_get_nameunit(sc->dev), reg);
		return;
	}

	rt2860_io_mac_write(sc, RT2860_REG_RF_CSR_CFG0, val);
}

/*
 * rt2860_io_rf_read
 */
int32_t rt2860_io_rf_read(struct rt2860_softc *sc, uint8_t reg)
{
	int ntries;
	if (sc->mac_rev == 0x28720200)
	{
		for (ntries = 0; ntries < 100; ntries ++) {
			if ( !(rt2860_io_mac_read(sc, RT2872_REG_RF_CSR_CFG) & 
			    RT2872_REG_RF_CSR_BUSY) )
				break;
			DELAY(1);
		}
		if (ntries == 100) {
			printf("%s:%s: RF busy after 100 probes\n",
				device_get_nameunit(sc->dev), __func__);
			return (-1);
		}
		rt2860_io_mac_write(sc, RT2872_REG_RF_CSR_CFG, 
		    RT2872_REG_RF_CSR_KICK |
		    (reg & RT2872_REG_RF_ID_MASK) << RT2872_REG_RF_ID_SHIFT );

		for (ntries = 0; ntries < 100; ntries ++) {
			if ( !(rt2860_io_mac_read(sc, RT2872_REG_RF_CSR_CFG) & 
			    RT2872_REG_RF_CSR_BUSY) )
				break;
			DELAY(1);
		}
		if (ntries == 100) {
			printf("%s:%s: RF busy after 100 probes\n",
				device_get_nameunit(sc->dev), __func__);
		}

		return (rt2860_io_mac_read(sc, RT2872_REG_RF_CSR_CFG) & RT2872_REG_RF_VAL_MASK);
	}
	return (-1);
}

/*
 * rt2860_io_rf_load_defaults
 */
void rt2860_io_rf_load_defaults(struct rt2860_softc *sc)
{
	int i;

	if (sc->mac_rev == 0x28720200) {
		for (i = 0; i < 32; i ++)
			rt2860_io_rf_write(sc, i, rt3052_rf_default[i]);
	}
}

/*
 * rt2860_io_mcu_cmd
 */
void rt2860_io_mcu_cmd(struct rt2860_softc *sc, uint8_t cmd,
	uint8_t token, uint16_t arg)
{
	uint32_t tmp;
	int ntries;

	if (sc->mac_rev == 0x28720200)
		return;

	for (ntries = 0; ntries < 100; ntries++)
	{
		if (!(rt2860_io_mac_read(sc, RT2860_REG_H2M_MAILBOX) &
			RT2860_REG_H2M_BUSY))
			break;

		DELAY(2);
	}

	if (ntries == 100)
	{
		printf("%s: could not read H2M: cmd=0x%02x\n",
			device_get_nameunit(sc->dev), cmd);
		return;
	}

	tmp = RT2860_REG_H2M_BUSY | (token << 16) | arg;

	rt2860_io_mac_write(sc, RT2860_REG_H2M_MAILBOX, tmp);
	rt2860_io_mac_write(sc, RT2860_REG_H2M_HOST_CMD, cmd);
}

/*
 * rt2860_io_mcu_cmd_check
 */
int rt2860_io_mcu_cmd_check(struct rt2860_softc *sc, uint8_t cid)
{
	uint32_t tmp, mask, status;
	int result, ntries;

	result = -1;

	for (ntries = 0; ntries < 200; ntries++)
	{
		tmp = rt2860_io_mac_read(sc, RT2860_REG_H2M_MAILBOX_CID);

		if (((cid >> RT2860_REG_H2M_CID0_SHIFT) & RT2860_REG_H2M_CID_MASK) == cid)
		{
			mask = (RT2860_REG_H2M_CID_MASK << RT2860_REG_H2M_CID0_SHIFT);
			break;
		}
		else if (((tmp >> RT2860_REG_H2M_CID1_SHIFT) & RT2860_REG_H2M_CID_MASK) == cid)
		{
			mask = (RT2860_REG_H2M_CID_MASK << RT2860_REG_H2M_CID1_SHIFT);
			break;
		}
		else if (((tmp >> RT2860_REG_H2M_CID2_SHIFT) & RT2860_REG_H2M_CID_MASK) == cid)
		{
			mask = (RT2860_REG_H2M_CID_MASK << RT2860_REG_H2M_CID2_SHIFT);
			break;
		}
		else if (((tmp >> RT2860_REG_H2M_CID3_SHIFT) & RT2860_REG_H2M_CID_MASK) == cid)
		{
			mask = (RT2860_REG_H2M_CID_MASK << RT2860_REG_H2M_CID3_SHIFT);
			break;
		}

		DELAY(100);
	}

	status = rt2860_io_mac_read(sc, RT2860_REG_H2M_MAILBOX_STATUS);

	if (ntries < 200)
	{
		status &= mask;

		if ((status == 0x1) ||
			(status == 0x100) ||
			(status == 0x10000) ||
			(status == 0x1000000))
			result = 0;
	}

	rt2860_io_mac_write(sc, RT2860_REG_H2M_MAILBOX_STATUS, 0xffffffff);
	rt2860_io_mac_write(sc, RT2860_REG_H2M_MAILBOX_CID, 0xffffffff);

	return result;
}

/*
 * rt2860_io_mcu_load_ucode
 */
int rt2860_io_mcu_load_ucode(struct rt2860_softc *sc,
	const uint8_t *ucode, size_t len)
{
	int i, ntries;
	uint16_t crc;

	for (i = 0, crc = 0xffff; i < len - 2; i++)
		crc = RT2860_IO_BYTE_CRC16(rt2860_io_byte_rev(ucode[i]), crc);

	if (ucode[len - 2] != rt2860_io_byte_rev(crc >> 8) ||
		ucode[len - 1] != rt2860_io_byte_rev(crc))
	{
		printf("%s: wrong microcode crc\n",
			device_get_nameunit(sc->dev));
		return EINVAL;
	}

	rt2860_io_mac_write(sc, RT2860_REG_PBF_SYS_CTRL, RT2860_REG_HST_PM_SEL);

	for(i = 0; i < len; i += 4)
	{
		rt2860_io_mac_write(sc, RT2860_REG_MCU_UCODE_BASE + i, 
		    (ucode[i+3] << 24) | (ucode[i+2] << 16) | 
		    (ucode[i+1] << 8) | ucode[i]);
	}

	if (sc->mac_rev != 0x28720200)
		rt2860_io_mac_write_multi(sc, RT2860_REG_MCU_UCODE_BASE,
		    ucode, len);

	rt2860_io_mac_write(sc, RT2860_REG_PBF_SYS_CTRL, 0);

	if (sc->mac_rev != 0x28720200)
		rt2860_io_mac_write(sc, RT2860_REG_PBF_SYS_CTRL,
		    RT2860_REG_MCU_RESET);

	DELAY(10000);

	/* initialize BBP R/W access agent */

	rt2860_io_mac_write(sc, RT2860_REG_H2M_MAILBOX_BBP_AGENT, 0);
	rt2860_io_mac_write(sc, RT2860_REG_H2M_MAILBOX, 0);

	if (sc->mac_rev != 0x28720200) {

		for (ntries = 0; ntries < 1000; ntries++)
		{
			if (rt2860_io_mac_read(sc, RT2860_REG_PBF_SYS_CTRL) &
				RT2860_REG_MCU_READY)
				break;

			DELAY(1000);
		}

		if (ntries == 1000)
		{
			printf("%s: timeout waiting for MCU to initialize\n",
				device_get_nameunit(sc->dev));
			return ETIMEDOUT;
		}
	}

	return 0;
}

/*
 * rt2860_io_eeprom_shiftout_bits
 */
static void rt2860_io_eeprom_shiftout_bits(struct rt2860_softc *sc,
	uint16_t val, uint16_t count)
{
	uint32_t mask, tmp; 

	mask = (1 << (count - 1));

	tmp = rt2860_io_mac_read(sc, RT2860_REG_EEPROM_CSR);

	tmp &= ~(RT2860_REG_EEDO | RT2860_REG_EEDI);

	do
	{
	    tmp &= ~RT2860_REG_EEDI;

	    if(val & mask)
			tmp |= RT2860_REG_EEDI;

		rt2860_io_mac_write(sc, RT2860_REG_EEPROM_CSR, tmp);

		RT2860_IO_EEPROM_RAISE_CLK(sc, tmp);
		RT2860_IO_EEPROM_LOWER_CLK(sc, tmp);

	    mask = (mask >> 1);
	} while (mask);

	tmp &= ~RT2860_REG_EEDI;

	rt2860_io_mac_write(sc, RT2860_REG_EEPROM_CSR, tmp);
}

/*
 * rt2860_io_eeprom_shiftin_bits
 */
static uint16_t rt2860_io_eeprom_shiftin_bits(struct rt2860_softc *sc)
{
	uint32_t tmp; 
	uint16_t val;
	int i;

	val = 0;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_EEPROM_CSR);

	tmp &= ~(RT2860_REG_EEDO | RT2860_REG_EEDI);

	for(i = 0; i < 16; i++)
	{
		val = (val << 1);

		RT2860_IO_EEPROM_RAISE_CLK(sc, tmp);

		tmp = rt2860_io_mac_read(sc, RT2860_REG_EEPROM_CSR);

		RT2860_IO_EEPROM_LOWER_CLK(sc, tmp);
		
		tmp &= ~RT2860_REG_EEDI;
		if(tmp & RT2860_REG_EEDO)
		    val |= 1;
	}

	return val;
}

/*
 * rt2860_io_byte_rev
 */
static uint8_t rt2860_io_byte_rev(uint8_t byte)
{
	int i;
	uint8_t tmp;

	for(i = 0, tmp = 0; ; i++)
	{
		if(byte & 0x80)
			tmp |= 0x80;

		if(i == 7)
			break;

		byte <<= 1;
		tmp >>= 1;
	}

	return tmp;
}
