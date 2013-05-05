
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

#ifndef _RT2860_IO_H_
#define _RT2860_IO_H_

#include <dev/rt2860/rt2860_softc.h>

#define RT2860_IO_MCU_CMD_SLEEP				0x30
#define RT2860_IO_MCU_CMD_WAKEUP			0x31
#define RT2860_IO_MCU_CMD_RADIOOFF			0x35
#define RT2860_IO_MCU_CMD_LEDS				0x50
#define RT2860_IO_MCU_CMD_LED_BRIGHTNESS		0x51
#define RT2860_IO_MCU_CMD_LED1				0x52
#define RT2860_IO_MCU_CMD_LED2				0x53
#define RT2860_IO_MCU_CMD_LED3				0x54
#define RT2860_IO_MCU_CMD_BOOT				0x72
#define RT2860_IO_MCU_CMD_BBP				0x80
#define RT2860_IO_MCU_CMD_POWERSAVE_LEVEL		0x83

void rt2860_io_rf_load_defaults(struct rt2860_softc *sc);
uint32_t rt2860_io_mac_read(struct rt2860_softc *sc, uint16_t reg);

void rt2860_io_mac_read_multi(struct rt2860_softc *sc,
	uint16_t reg, void *buf, size_t len);

void rt2860_io_mac_write(struct rt2860_softc *sc,
	uint16_t reg, uint32_t val);

void rt2860_io_mac_write_multi(struct rt2860_softc *sc,
	uint16_t reg, const void *buf, size_t len);

void rt2860_io_mac_set_region_4(struct rt2860_softc *sc,
	uint16_t reg, uint32_t val, size_t len);

uint16_t rt2860_io_eeprom_read(struct rt2860_softc *sc, uint16_t addr);

void rt2860_io_eeprom_read_multi(struct rt2860_softc *sc,
	uint16_t addr, void *buf, size_t len);

uint8_t rt2860_io_bbp_read(struct rt2860_softc *sc, uint8_t reg);

void rt2860_io_bbp_write(struct rt2860_softc *sc, uint8_t reg, uint8_t val);

void rt2860_io_rf_write(struct rt2860_softc *sc, uint8_t reg, uint32_t val);

int32_t rt2860_io_rf_read(struct rt2860_softc *sc, uint8_t reg);

void rt2860_io_mcu_cmd(struct rt2860_softc *sc, uint8_t cmd,
	uint8_t token, uint16_t arg);

int rt2860_io_mcu_cmd_check(struct rt2860_softc *sc, uint8_t cid);

int rt2860_io_mcu_load_ucode(struct rt2860_softc *sc,
	const uint8_t *ucode, size_t len);


#endif /* #ifndef _RT2860_IO_H_ */
