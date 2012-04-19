
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

#include <dev/rt2860/rt2860_led.h>
#include <dev/rt2860/rt2860_reg.h>
#include <dev/rt2860/rt2860_eeprom.h>
#include <dev/rt2860/rt2860_io.h>

/*
 * rt2860_led_brightness
 */
void rt2860_led_brightness(struct rt2860_softc *sc, uint8_t brightness)
{
	uint8_t polarity;
	uint16_t tmp;

	polarity = (sc->led_cntl & RT2860_EEPROM_LED_POLARITY) ? 1 : 0;

	tmp = (polarity << 8) | brightness;

	rt2860_io_mcu_cmd(sc, RT2860_IO_MCU_CMD_LED_BRIGHTNESS,
		RT2860_REG_H2M_TOKEN_NO_INTR, tmp);
}

/*
 * rt2860_led_cmd
 */
void rt2860_led_cmd(struct rt2860_softc *sc, uint8_t cmd)
{
	uint16_t tmp;

	tmp = (cmd << 8) | (sc->led_cntl & RT2860_EEPROM_LED_MODE_MASK);

	rt2860_io_mcu_cmd(sc, RT2860_IO_MCU_CMD_LEDS,
		RT2860_REG_H2M_TOKEN_NO_INTR, tmp);
}
