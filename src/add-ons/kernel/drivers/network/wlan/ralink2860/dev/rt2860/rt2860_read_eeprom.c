
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

#include <dev/rt2860/rt2860_read_eeprom.h>
#include <dev/rt2860/rt2860_reg.h>
#include <dev/rt2860/rt2860_eeprom.h>
#include <dev/rt2860/rt2860_io.h>
#include <dev/rt2860/rt2860_debug.h>

/*
 * rt2860_read_eeprom
 */
void rt2860_read_eeprom(struct rt2860_softc *sc)
{
	uint32_t tmp;
	uint16_t val;
	int i;

	/* read EEPROM address number */

	tmp = rt2860_io_mac_read(sc, RT2860_REG_EEPROM_CSR);

	if((tmp & 0x30) == 0)
		sc->eeprom_addr_num = 6;
	else if((tmp & 0x30) == 0x10)
		sc->eeprom_addr_num = 8;
	else
		sc->eeprom_addr_num = 8;

	/* read EEPROM version */

	sc->eeprom_rev = rt2860_io_eeprom_read(sc, RT2860_EEPROM_VERSION);

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM rev=0x%04x\n",
		device_get_nameunit(sc->dev), sc->eeprom_rev);

	/* read MAC address */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_ADDRESS01);

	sc->mac_addr[0] = (val & 0xff);
	sc->mac_addr[1] = (val >> 8);

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_ADDRESS23);

	sc->mac_addr[2] = (val & 0xff);
	sc->mac_addr[3] = (val >> 8);

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_ADDRESS45);

	sc->mac_addr[4] = (val & 0xff);
	sc->mac_addr[5] = (val >> 8);

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM mac address=%s\n",
		device_get_nameunit(sc->dev), ether_sprintf(sc->mac_addr));

	/* read RF information */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_ANTENNA);
	if (val == 0xffff)
	{
		printf("%s: invalid EEPROM antenna info\n",
			device_get_nameunit(sc->dev));

		sc->rf_rev = RT2860_EEPROM_RF_2820;
		sc->ntxpath = 1;
		sc->nrxpath = 2;
	}
	else
	{
		sc->rf_rev = (val >> 8) & 0xf;
		sc->ntxpath = (val >> 4) & 0xf;
		sc->nrxpath = (val & 0xf);
	}

	if ((sc->mac_rev != 0x28830300) && (sc->nrxpath > 2))
	{
		/* only 2 Rx streams for RT2860 series */

		sc->nrxpath = 2;
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM RF rev=0x%04x, paths=%dT%dR\n",
		device_get_nameunit(sc->dev), sc->rf_rev, sc->ntxpath, sc->nrxpath);

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_NIC_CONFIG);
	if ((val & 0xff00) != 0xff00)
		sc->patch_dac = (val >> 15) & 1;

	sc->hw_radio_cntl = ((val & RT2860_EEPROM_HW_RADIO_CNTL) ? 1 : 0);
	sc->tx_agc_cntl = ((val & RT2860_EEPROM_TX_AGC_CNTL) ? 1 : 0);
	sc->ext_lna_2ghz = ((val & RT2860_EEPROM_EXT_LNA_2GHZ) ? 1 : 0);
	sc->ext_lna_5ghz = ((val & RT2860_EEPROM_EXT_LNA_5GHZ) ? 1 : 0);

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM NIC config: HW radio cntl=%d, Tx AGC cntl=%d, ext LNA gains=%d/%d\n",
		device_get_nameunit(sc->dev),
		sc->hw_radio_cntl, sc->tx_agc_cntl, sc->ext_lna_2ghz, sc->ext_lna_5ghz);

	/* read country code */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_COUNTRY);

	sc->country_2ghz = (val >> 8) & 0xff;
	sc->country_5ghz = (val & 0xff);

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM country code=%d/%d\n",
		device_get_nameunit(sc->dev), sc->country_2ghz, sc->country_5ghz);

	/* read RF frequency offset */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_RF_FREQ_OFF);

	if ((val & 0xff) != 0xff)
	{
		sc->rf_freq_off = (val & 0xff);
	}
	else
	{
		printf("%s: invalid EEPROM RF freq offset\n",
			device_get_nameunit(sc->dev));

		sc->rf_freq_off = 0;
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM freq offset=0x%02x\n",
		device_get_nameunit(sc->dev), sc->rf_freq_off);

	/* read LEDs operating mode */

	if (((val >> 8) & 0xff) != 0xff)
	{
		sc->led_cntl = ((val >> 8) & 0xff);
		sc->led_off[0] = rt2860_io_eeprom_read(sc, RT2860_EEPROM_LED1_OFF);
		sc->led_off[1] = rt2860_io_eeprom_read(sc, RT2860_EEPROM_LED2_OFF);
		sc->led_off[2] = rt2860_io_eeprom_read(sc, RT2860_EEPROM_LED3_OFF);
	}
	else
	{
		printf("%s: invalid EEPROM LED settings\n",
			device_get_nameunit(sc->dev));

		sc->led_cntl = RT2860_EEPROM_LED_CNTL_DEFAULT;
		sc->led_off[0] = RT2860_EEPROM_LED1_OFF_DEFAULT;
		sc->led_off[1] = RT2860_EEPROM_LED2_OFF_DEFAULT;
		sc->led_off[2] = RT2860_EEPROM_LED3_OFF_DEFAULT;
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM LED cntl=0x%02x, LEDs=0x%04x/0x%04x/0x%04x\n",
		device_get_nameunit(sc->dev), sc->led_cntl,
		sc->led_off[0], sc->led_off[1], sc->led_off[2]);

	/* read RSSI offsets and LNA gains */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_LNA_GAIN);
	if ((sc->mac_rev & 0xffff0000) >= 0x30710000)
		sc->lna_gain[0] = RT3090_DEF_LNA;
	else				/* channel group 0 */
		sc->lna_gain[0] = val & 0xff;

//	sc->lna_gain[0] = (val & 0xff);
	sc->lna_gain[1] = (val >> 8) & 0xff;

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_RSSI_OFF_2GHZ_BASE);

	sc->rssi_off_2ghz[0] = (val & 0xff);
	sc->rssi_off_2ghz[1] = (val >> 8) & 0xff;

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_RSSI_OFF_2GHZ_BASE + 2);

	//sc->rssi_off_2ghz[2] = (val & 0xff);
	sc->lna_gain[2] = (val >> 8) & 0xff;
	if ((sc->mac_rev & 0xffff0000) >= 0x30710000) {
		/*
		 * On RT3090 chips (limited to 2 Rx chains), this ROM
		 * field contains the Tx mixer gain for the 2GHz band.
		 */
		if ((val & 0xff) != 0xff)
			sc->txmixgain_2ghz = val & 0x7;
		//DPRINTF(("tx mixer gain=%u (2GHz)\n", sc->txmixgain_2ghz));
	} else
		sc->rssi_off_2ghz[2] = val & 0xff;	/* Ant C */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_RSSI_OFF_5GHZ_BASE);

	sc->rssi_off_5ghz[0] = (val & 0xff);
	sc->rssi_off_5ghz[1] = (val >> 8) & 0xff;

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_RSSI_OFF_5GHZ_BASE + 2);

	sc->rssi_off_5ghz[2] = (val & 0xff);
	sc->lna_gain[3] = (val >> 8) & 0xff;

	for (i = 2; i < RT2860_SOFTC_LNA_GAIN_COUNT; i++)
	{
		if (sc->lna_gain[i] == 0x00 || sc->lna_gain[i] == (int8_t) 0xff)
		{
			printf("%s: invalid EEPROM LNA gain #%d: 0x%02x\n",
				device_get_nameunit(sc->dev), i, sc->lna_gain[i]);

			sc->lna_gain[i] = sc->lna_gain[1];
		}
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM LNA gains=0x%02x/0x%02x/0x%02x/0x%02x\n",
		device_get_nameunit(sc->dev),
		sc->lna_gain[0], sc->lna_gain[1], sc->lna_gain[2], sc->lna_gain[3]);

	for (i = 0; i < RT2860_SOFTC_RSSI_OFF_COUNT; i++)
	{
		if (sc->rssi_off_2ghz[i] < RT2860_EEPROM_RSSI_OFF_MIN ||
			sc->rssi_off_2ghz[i] > RT2860_EEPROM_RSSI_OFF_MAX)
		{
			printf("%s: invalid EEPROM RSSI offset #%d (2GHz): 0x%02x\n",
				device_get_nameunit(sc->dev), i, sc->rssi_off_2ghz[i]);

			sc->rssi_off_2ghz[i] = 0;
		}

		if (sc->rssi_off_5ghz[i] < RT2860_EEPROM_RSSI_OFF_MIN ||
			sc->rssi_off_5ghz[i] > RT2860_EEPROM_RSSI_OFF_MAX)
		{
			printf("%s: invalid EEPROM RSSI offset #%d (5GHz): 0x%02x\n",
				device_get_nameunit(sc->dev), i, sc->rssi_off_5ghz[i]);

			sc->rssi_off_5ghz[i] = 0;
		}
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM RSSI offsets 2GHz=%d/%d/%d\n",
		device_get_nameunit(sc->dev),
		sc->rssi_off_2ghz[0], sc->rssi_off_2ghz[1], sc->rssi_off_2ghz[2]);

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM RSSI offsets 5GHz=%d/%d/%d\n",
		device_get_nameunit(sc->dev),
		sc->rssi_off_5ghz[0], sc->rssi_off_5ghz[1], sc->rssi_off_5ghz[2]);

	/* read Tx power settings for 2GHz channels */

	for (i = 0; i < 14; i += 2)
	{
		val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TXPOW1_2GHZ_BASE + i / 2);

		sc->txpow1[i + 0] = (int8_t) (val & 0xff);
		sc->txpow1[i + 1] = (int8_t) (val >> 8);

		val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TXPOW2_2GHZ_BASE + i / 2);

		sc->txpow2[i + 0] = (int8_t) (val & 0xff);
		sc->txpow2[i + 1] = (int8_t) (val >> 8);
	}

	/* read Tx power settings for 5GHz channels */

	for (; i < RT2860_SOFTC_TXPOW_COUNT; i += 2)
	{
		val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TXPOW1_5GHZ_BASE + i / 2);

		sc->txpow1[i + 0] = (int8_t) (val & 0xff);
		sc->txpow1[i + 1] = (int8_t) (val >> 8);

		val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TXPOW2_5GHZ_BASE + i / 2);

		sc->txpow2[i + 0] = (int8_t) (val & 0xff);
		sc->txpow2[i + 1] = (int8_t) (val >> 8);
	}

	/* fix broken Tx power settings */

    for (i = 0; i < 14; i++)
	{
		if (sc->txpow1[i] < RT2860_EEPROM_TXPOW_2GHZ_MIN ||
			sc->txpow1[i] > RT2860_EEPROM_TXPOW_2GHZ_MAX)
		{
			printf("%s: invalid EEPROM Tx power1 #%d (2GHz): 0x%02x\n",
				device_get_nameunit(sc->dev), i, sc->txpow1[i]);

			sc->txpow1[i] = RT2860_EEPROM_TXPOW_2GHZ_DEFAULT;
		}

		if (sc->txpow2[i] < RT2860_EEPROM_TXPOW_2GHZ_MIN ||
			sc->txpow2[i] > RT2860_EEPROM_TXPOW_2GHZ_MAX)
		{
			printf("%s: invalid EEPROM Tx power2 #%d (2GHz): 0x%02x\n",
				device_get_nameunit(sc->dev), i, sc->txpow2[i]);

			sc->txpow2[i] = RT2860_EEPROM_TXPOW_2GHZ_DEFAULT;
		}
	}

	for (; i < RT2860_SOFTC_TXPOW_COUNT; i++)
	{
		if (sc->txpow1[i] < RT2860_EEPROM_TXPOW_5GHZ_MIN ||
			sc->txpow1[i] > RT2860_EEPROM_TXPOW_5GHZ_MAX)
		{
			printf("%s: invalid EEPROM Tx power1 #%d (5GHz): 0x%02x\n",
				device_get_nameunit(sc->dev), i, sc->txpow1[i]);

			sc->txpow1[i] = RT2860_EEPROM_TXPOW_5GHZ_DEFAULT;
		}

		if (sc->txpow2[i] < RT2860_EEPROM_TXPOW_5GHZ_MIN ||
			sc->txpow2[i] > RT2860_EEPROM_TXPOW_5GHZ_MAX)
		{
			printf("%s: invalid EEPROM Tx power2 #%d (5GHz): 0x%02x\n",
				device_get_nameunit(sc->dev), i, sc->txpow2[i]);

			sc->txpow2[i] = RT2860_EEPROM_TXPOW_5GHZ_DEFAULT;
		}
	}

	/* read Tx power per rate deltas */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TXPOW_RATE_DELTA);

	sc->txpow_rate_delta_2ghz = 0;
	sc->txpow_rate_delta_5ghz = 0;

	if ((val & 0xff) != 0xff)
	{
		if (val & 0x80)
			sc->txpow_rate_delta_2ghz = (val & 0xf);

		if (!(val & 0x40))
			sc->txpow_rate_delta_2ghz = -sc->txpow_rate_delta_2ghz;
	}

	val >>= 8;

	if ((val & 0xff) != 0xff)
	{
		if (val & 0x80)
			sc->txpow_rate_delta_5ghz = (val & 0xf);

		if (!(val & 0x40))
			sc->txpow_rate_delta_5ghz = -sc->txpow_rate_delta_5ghz;
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM Tx power per rate deltas=%d(2MHz), %d(5MHz)\n",
		device_get_nameunit(sc->dev),
		sc->txpow_rate_delta_2ghz, sc->txpow_rate_delta_5ghz);

	/* read Tx power per rate */

	for (i = 0; i < RT2860_SOFTC_TXPOW_RATE_COUNT; i++)
	{
		rt2860_io_eeprom_read_multi(sc, RT2860_EEPROM_TXPOW_RATE_BASE + i * sizeof(uint32_t),
			&tmp, sizeof(uint32_t));

		sc->txpow_rate_20mhz[i] = tmp;
		sc->txpow_rate_40mhz_2ghz[i] =
			rt2860_read_eeprom_txpow_rate_add_delta(tmp, sc->txpow_rate_delta_2ghz);
		sc->txpow_rate_40mhz_5ghz[i] =
			rt2860_read_eeprom_txpow_rate_add_delta(tmp, sc->txpow_rate_delta_5ghz);

		RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
			"%s: EEPROM Tx power per rate #%d=0x%08x(20MHz), 0x%08x(40MHz/2GHz), 0x%08x(40MHz/5GHz)\n",
			device_get_nameunit(sc->dev), i,
			sc->txpow_rate_20mhz[i], sc->txpow_rate_40mhz_2ghz[i], sc->txpow_rate_40mhz_5ghz[i]);
	}

	if (sc->tx_agc_cntl)
		sc->tx_agc_cntl_2ghz = sc->tx_agc_cntl_5ghz = 1;

	/* read factory-calibrated samples for temperature compensation */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TSSI_2GHZ_BASE);

	sc->tssi_2ghz[0] = (val & 0xff);	/* [-4] */
	sc->tssi_2ghz[1] = (val >> 8);		/* [-3] */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TSSI_2GHZ_BASE + 2);

	sc->tssi_2ghz[2] = (val & 0xff);	/* [-2] */
	sc->tssi_2ghz[3] = (val >> 8);		/* [-1] */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TSSI_2GHZ_BASE + 2 * 2);

	sc->tssi_2ghz[4] = (val & 0xff);	/* [0] */
	sc->tssi_2ghz[5] = (val >> 8);		/* [+1] */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TSSI_2GHZ_BASE + 3 * 2);

	sc->tssi_2ghz[6] = (val & 0xff);	/* [+2] */
	sc->tssi_2ghz[7] = (val >> 8);		/* [+3] */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TSSI_2GHZ_BASE + 4 * 2);

	sc->tssi_2ghz[8] = (val & 0xff);	/* [+4] */
	sc->tssi_step_2ghz = (val >> 8);

	if (sc->tssi_2ghz[4] == 0xff)
		sc->tx_agc_cntl_2ghz = 0;

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM TSSI 2GHz: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, "
		"0x%02x, 0x%02x, step=%d\n",
		device_get_nameunit(sc->dev),
		sc->tssi_2ghz[0], sc->tssi_2ghz[1], sc->tssi_2ghz[2],
		sc->tssi_2ghz[3], sc->tssi_2ghz[4], sc->tssi_2ghz[5],
		sc->tssi_2ghz[6], sc->tssi_2ghz[7], sc->tssi_2ghz[8],
		sc->tssi_step_2ghz);

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TSSI_5GHZ_BASE);

	sc->tssi_5ghz[0] = (val & 0xff);	/* [-4] */
	sc->tssi_5ghz[1] = (val >> 8);		/* [-3] */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TSSI_5GHZ_BASE + 2);

	sc->tssi_5ghz[2] = (val & 0xff);	/* [-2] */
	sc->tssi_5ghz[3] = (val >> 8);		/* [-1] */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TSSI_5GHZ_BASE + 2 * 2);

	sc->tssi_5ghz[4] = (val & 0xff);	/* [0] */
	sc->tssi_5ghz[5] = (val >> 8);		/* [+1] */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TSSI_5GHZ_BASE + 3 * 2);

	sc->tssi_5ghz[6] = (val & 0xff);	/* [+2] */
	sc->tssi_5ghz[7] = (val >> 8);		/* [+3] */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_TSSI_5GHZ_BASE + 4 * 2);

	sc->tssi_5ghz[8] = (val & 0xff);	/* [+4] */
	sc->tssi_step_5ghz = (val >> 8);

	if (sc->tssi_5ghz[4] == 0xff)
		sc->tx_agc_cntl_5ghz = 0;

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM TSSI 5GHz: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, "
		"0x%02x, 0x%02x, step=%d\n",
		device_get_nameunit(sc->dev),
		sc->tssi_5ghz[0], sc->tssi_5ghz[1], sc->tssi_5ghz[2],
		sc->tssi_5ghz[3], sc->tssi_5ghz[4], sc->tssi_5ghz[5],
		sc->tssi_5ghz[6], sc->tssi_5ghz[7], sc->tssi_5ghz[8],
		sc->tssi_step_5ghz);

	/* read default BBP settings */

	rt2860_io_eeprom_read_multi(sc, RT2860_EEPROM_BBP_BASE,
		sc->bbp_eeprom, RT2860_SOFTC_BBP_EEPROM_COUNT * 2);

	if ((sc->mac_rev & 0xffff0000) >= 0x30710000) {
		/* read vendor RF settings */
		for (i = 0; i < 10; i++) {
			val = rt2860_io_eeprom_read(sc, RT3071_EEPROM_RF_BASE + i);
			sc->rf[i].val = val & 0xff;
			sc->rf[i].reg = val >> 8;
//			DPRINTF(("RF%d=0x%02x\n", sc->rf[i].reg,
//			    sc->rf[i].val));
		}
	}
	/* read powersave level */

	val = rt2860_io_eeprom_read(sc, RT2860_EEPROM_POWERSAVE_LEVEL);

	sc->powersave_level = val & 0xff;

	if ((sc->powersave_level & 0xff) == 0xff)
		printf("%s: invalid EEPROM powersave level\n",
			device_get_nameunit(sc->dev));

	RT2860_DPRINTF(sc, RT2860_DEBUG_EEPROM,
		"%s: EEPROM powersave level=0x%02x\n",
		device_get_nameunit(sc->dev), sc->powersave_level);
}

/*
 * rt2860_read_eeprom_txpow_rate_add_delta
 */
uint32_t rt2860_read_eeprom_txpow_rate_add_delta(uint32_t txpow_rate,
	int8_t delta)
{
	int8_t b4;
	int i;

	for (i = 0; i < 8; i++)
	{
		b4 = txpow_rate & 0xf;
		b4 += delta;

		if (b4 < 0)
			b4 = 0;
		else if (b4 > 0xf)
			b4 = 0xf;

		txpow_rate = (txpow_rate >> 4) | (b4 << 28);
	}

	return txpow_rate;
}
