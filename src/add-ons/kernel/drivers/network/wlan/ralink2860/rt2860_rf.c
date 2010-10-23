
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

#include "rt2860_rf.h"
#include "rt2860_reg.h"
#include "rt2860_eeprom.h"
#include "rt2860_io.h"
#include "rt2860_debug.h"

/*
 * Static variables
 */

static const struct rt2860_rf_prog
{
	uint8_t chan;
	uint32_t r1, r2, r3, r4;
} rt2860_rf_2850[] =
{
	{   1, 0x98402ecc, 0x984c0786, 0x9816b455, 0x9800510b },
	{   2, 0x98402ecc, 0x984c0786, 0x98168a55, 0x9800519f },
	{   3, 0x98402ecc, 0x984c078a, 0x98168a55, 0x9800518b },
	{   4, 0x98402ecc, 0x984c078a, 0x98168a55, 0x9800519f },
	{   5, 0x98402ecc, 0x984c078e, 0x98168a55, 0x9800518b },
	{   6, 0x98402ecc, 0x984c078e, 0x98168a55, 0x9800519f },
	{   7, 0x98402ecc, 0x984c0792, 0x98168a55, 0x9800518b },
	{   8, 0x98402ecc, 0x984c0792, 0x98168a55, 0x9800519f },
	{   9, 0x98402ecc, 0x984c0796, 0x98168a55, 0x9800518b },
	{  10, 0x98402ecc, 0x984c0796, 0x98168a55, 0x9800519f },
	{  11, 0x98402ecc, 0x984c079a, 0x98168a55, 0x9800518b },
	{  12, 0x98402ecc, 0x984c079a, 0x98168a55, 0x9800519f },
	{  13, 0x98402ecc, 0x984c079e, 0x98168a55, 0x9800518b },
	{  14, 0x98402ecc, 0x984c07a2, 0x98168a55, 0x98005193 },
	{  36, 0x98402ecc, 0x984c099a, 0x98158a55, 0x980ed1a3 },
	{  38, 0x98402ecc, 0x984c099e, 0x98158a55, 0x980ed193 },
	{  40, 0x98402ec8, 0x984c0682, 0x98158a55, 0x980ed183 },
	{  44, 0x98402ec8, 0x984c0682, 0x98158a55, 0x980ed1a3 },
	{  46, 0x98402ec8, 0x984c0686, 0x98158a55, 0x980ed18b },
	{  48, 0x98402ec8, 0x984c0686, 0x98158a55, 0x980ed19b },
	{  52, 0x98402ec8, 0x984c068a, 0x98158a55, 0x980ed193 },
	{  54, 0x98402ec8, 0x984c068a, 0x98158a55, 0x980ed1a3 },
	{  56, 0x98402ec8, 0x984c068e, 0x98158a55, 0x980ed18b },
	{  60, 0x98402ec8, 0x984c0692, 0x98158a55, 0x980ed183 },
	{  62, 0x98402ec8, 0x984c0692, 0x98158a55, 0x980ed193 },
	{  64, 0x98402ec8, 0x984c0692, 0x98158a55, 0x980ed1a3 },
	{ 100, 0x98402ec8, 0x984c06b2, 0x98178a55, 0x980ed783 },
	{ 102, 0x98402ec8, 0x985c06b2, 0x98578a55, 0x980ed793 },
	{ 104, 0x98402ec8, 0x985c06b2, 0x98578a55, 0x980ed1a3 },
	{ 108, 0x98402ecc, 0x985c0a32, 0x98578a55, 0x980ed193 },
	{ 110, 0x98402ecc, 0x984c0a36, 0x98178a55, 0x980ed183 },
	{ 112, 0x98402ecc, 0x984c0a36, 0x98178a55, 0x980ed19b },
	{ 116, 0x98402ecc, 0x984c0a3a, 0x98178a55, 0x980ed1a3 },
	{ 118, 0x98402ecc, 0x984c0a3e, 0x98178a55, 0x980ed193 },
	{ 120, 0x98402ec4, 0x984c0382, 0x98178a55, 0x980ed183 },
	{ 124, 0x98402ec4, 0x984c0382, 0x98178a55, 0x980ed193 },
	{ 126, 0x98402ec4, 0x984c0382, 0x98178a55, 0x980ed15b },
	{ 128, 0x98402ec4, 0x984c0382, 0x98178a55, 0x980ed1a3 },
	{ 132, 0x98402ec4, 0x984c0386, 0x98178a55, 0x980ed18b },
	{ 134, 0x98402ec4, 0x984c0386, 0x98178a55, 0x980ed193 },
	{ 136, 0x98402ec4, 0x984c0386, 0x98178a55, 0x980ed19b },
	{ 140, 0x98402ec4, 0x984c038a, 0x98178a55, 0x980ed183 },
	{ 149, 0x98402ec4, 0x984c038a, 0x98178a55, 0x980ed1a7 },
	{ 151, 0x98402ec4, 0x984c038e, 0x98178a55, 0x980ed187 },
	{ 153, 0x98402ec4, 0x984c038e, 0x98178a55, 0x980ed18f },
	{ 157, 0x98402ec4, 0x984c038e, 0x98178a55, 0x980ed19f },
	{ 159, 0x98402ec4, 0x984c038e, 0x98178a55, 0x980ed1a7 },
	{ 161, 0x98402ec4, 0x984c0392, 0x98178a55, 0x980ed187 },
	{ 165, 0x98402ec4, 0x984c0392, 0x98178a55, 0x980ed197 },
	{ 184, 0x95002ccc, 0x9500491e, 0x9509be55, 0x950c0a0b },
	{ 188, 0x95002ccc, 0x95004922, 0x9509be55, 0x950c0a13 },
	{ 192, 0x95002ccc, 0x95004926, 0x9509be55, 0x950c0a1b },
	{ 196, 0x95002ccc, 0x9500492a, 0x9509be55, 0x950c0a23 },
	{ 208, 0x95002ccc, 0x9500493a, 0x9509be55, 0x950c0a13 },
	{ 212, 0x95002ccc, 0x9500493e, 0x9509be55, 0x950c0a1b },
	{ 216, 0x95002ccc, 0x95004982, 0x9509be55, 0x950c0a23 },
};

/*
 * rt2860_rf_name
 */
const char *rt2860_rf_name(int rf_rev)
{
	switch (rf_rev)
	{
		case RT2860_EEPROM_RF_2820:
			return "RT2820";

		case RT2860_EEPROM_RF_2850:
			return "RT2850";

		case RT2860_EEPROM_RF_2720:
			return "RT2720";

		case RT2860_EEPROM_RF_2750:
			return "RT2750";

		default:
			return "unknown";
	}
}

/*
 * rt2860_rf_select_chan_group
 */
void rt2860_rf_select_chan_group(struct rt2860_softc *sc,
	struct ieee80211_channel *c)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	int chan, group;
	uint32_t tmp;

	ifp = sc->ifp;
	ic = ifp->if_l2com;

	chan = ieee80211_chan2ieee(ic, c);
	if (chan == 0 || chan == IEEE80211_CHAN_ANY)
		return;

	if (chan <= 14)
		group = 0;
	else if (chan <= 64)
		group = 1;
	else if (chan <= 128)
		group = 2;
	else
		group = 3;

	rt2860_io_bbp_write(sc, 62, 0x37 - sc->lna_gain[group]);
	rt2860_io_bbp_write(sc, 63, 0x37 - sc->lna_gain[group]);
	rt2860_io_bbp_write(sc, 64, 0x37 - sc->lna_gain[group]);
	rt2860_io_bbp_write(sc, 86, 0x00);

	if (group == 0)
	{
		if (sc->ext_lna_2ghz)
		{
			rt2860_io_bbp_write(sc, 82, 0x62);
			rt2860_io_bbp_write(sc, 75, 0x46);
		}
		else
		{
			rt2860_io_bbp_write(sc, 82, 0x84);
			rt2860_io_bbp_write(sc, 75, 0x50);
		}
	}
	else
	{
		rt2860_io_bbp_write(sc, 82, 0xf2);

		if (sc->ext_lna_5ghz)
			rt2860_io_bbp_write(sc, 75, 0x46);
		else
			rt2860_io_bbp_write(sc, 75, 0x50);
	}

	if (group == 0)
	{
		tmp = 0x2e + sc->lna_gain[group];
	}
	else
	{
		if ((ic->ic_flags & IEEE80211_F_SCAN) || !IEEE80211_IS_CHAN_HT40(c))
			tmp = 0x32 + sc->lna_gain[group] * 5 / 3;
		else
			tmp = 0x3a + sc->lna_gain[group] * 5 / 3;
	}

	rt2860_io_bbp_write(sc, 66, tmp);

	tmp = RT2860_REG_RFTR_ENABLE |
		RT2860_REG_TRSW_ENABLE |
		RT2860_REG_LNA_PE_G1_ENABLE |
		RT2860_REG_LNA_PE_A1_ENABLE |
		RT2860_REG_LNA_PE_G0_ENABLE |
		RT2860_REG_LNA_PE_A0_ENABLE;

	if (group == 0)
		tmp |= RT2860_REG_PA_PE_G1_ENABLE |
			RT2860_REG_PA_PE_G0_ENABLE;
	else
		tmp |= RT2860_REG_PA_PE_A1_ENABLE |
			RT2860_REG_PA_PE_A0_ENABLE;

	if (sc->ntxpath == 1)
		tmp &= ~(RT2860_REG_PA_PE_G1_ENABLE | RT2860_REG_PA_PE_A1_ENABLE);

	if (sc->nrxpath == 1)
		tmp &= ~(RT2860_REG_LNA_PE_G1_ENABLE | RT2860_REG_LNA_PE_A1_ENABLE);

	rt2860_io_mac_write(sc, RT2860_REG_TX_PIN_CFG, tmp);

	tmp = rt2860_io_mac_read(sc, RT2860_REG_TX_BAND_CFG);

	tmp &= ~(RT2860_REG_TX_BAND_BG | RT2860_REG_TX_BAND_A | RT2860_REG_TX_BAND_HT40_ABOVE);

	if (group == 0)
		tmp |= RT2860_REG_TX_BAND_BG;
	else
		tmp |= RT2860_REG_TX_BAND_A;

	/* set central channel position */

	if (IEEE80211_IS_CHAN_HT40U(c))
		tmp |= RT2860_REG_TX_BAND_HT40_BELOW;
	else if (IEEE80211_IS_CHAN_HT40D(c))
		tmp |= RT2860_REG_TX_BAND_HT40_ABOVE;
	else
		tmp |= RT2860_REG_TX_BAND_HT40_BELOW;

	rt2860_io_mac_write(sc, RT2860_REG_TX_BAND_CFG, tmp);

	/* set bandwidth (20MHz or 40MHz) */

	tmp = rt2860_io_bbp_read(sc, 4);

	tmp &= ~0x18;

	if (IEEE80211_IS_CHAN_HT40(c))
		tmp |= 0x10;

	rt2860_io_bbp_write(sc, 4, tmp);

	/* set central channel position */

	tmp = rt2860_io_bbp_read(sc, 3);

	tmp &= ~0x20;

	if (IEEE80211_IS_CHAN_HT40D(c))
		tmp |= 0x20;

	rt2860_io_bbp_write(sc, 3, tmp);

	if (sc->mac_rev == 0x28600100)
	{
		if (!IEEE80211_IS_CHAN_HT40(c))
		{
			rt2860_io_bbp_write(sc, 69, 0x16);
			rt2860_io_bbp_write(sc, 70, 0x08);
			rt2860_io_bbp_write(sc, 73, 0x12);
		}
		else
		{
			rt2860_io_bbp_write(sc, 69, 0x1a);
			rt2860_io_bbp_write(sc, 70, 0x0a);
			rt2860_io_bbp_write(sc, 73, 0x16);
		}
	}
}

/*
 * rt2860_rf_set_chan
 */
void rt2860_rf_set_chan(struct rt2860_softc *sc,
	struct ieee80211_channel *c)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	const struct rt2860_rf_prog *prog;
	uint32_t r1, r2, r3, r4;
	int8_t txpow1, txpow2;
	int i, chan;

	ifp = sc->ifp;
	ic = ifp->if_l2com;
	prog = rt2860_rf_2850;

	/* get central channel position */

	chan = ieee80211_chan2ieee(ic, c);

	if (IEEE80211_IS_CHAN_HT40U(c))
		chan += 2;
	else if (IEEE80211_IS_CHAN_HT40D(c))
		chan -= 2;

	RT2860_DPRINTF(sc, RT2860_DEBUG_CHAN,
		"%s: RF set channel: channel=%u, HT%s%s\n",
		device_get_nameunit(sc->dev),
		ieee80211_chan2ieee(ic, c),
		!IEEE80211_IS_CHAN_HT(c) ? " disabled" :
			IEEE80211_IS_CHAN_HT20(c) ? "20":
				IEEE80211_IS_CHAN_HT40U(c) ? "40U" : "40D",
		(ic->ic_flags & IEEE80211_F_SCAN) ? ", scanning" : "");

	if (chan == 0 || chan == IEEE80211_CHAN_ANY)
		return;

	for (i = 0; prog[i].chan != chan; i++);

	r1 = prog[i].r1;
	r2 = prog[i].r2;
	r3 = prog[i].r3;
	r4 = prog[i].r4;

	txpow1 = sc->txpow1[i];
	txpow2 = sc->txpow2[i];

	if (sc->ntxpath == 1)
		r2 |= (1 << 14);

	if (sc->nrxpath == 2)
		r2 |= (1 << 6);
	else if (sc->nrxpath == 1)
		r2 |= (1 << 17) | (1 << 6);

	if (IEEE80211_IS_CHAN_2GHZ(c))
	{
		r3 = (r3 & 0xffffc1ff) | (txpow1 << 9);
		r4 = (r4 & ~0x001f87c0) | (sc->rf_freq_off << 15) | (txpow2 << 6);
	}
	else
	{
		r3 = r3 & 0xffffc1ff;
		r4 = (r4 & ~0x001f87c0) | (sc->rf_freq_off << 15);

		if (txpow1 >= RT2860_EEPROM_TXPOW_5GHZ_MIN && txpow1 < 0)
		{
			txpow1 = (-RT2860_EEPROM_TXPOW_5GHZ_MIN + txpow1);
			if (txpow1 > RT2860_EEPROM_TXPOW_5GHZ_MAX)
				txpow1 = RT2860_EEPROM_TXPOW_5GHZ_MAX;

			r3 |= (txpow1 << 10);
		}
		else
		{
			if (txpow1 > RT2860_EEPROM_TXPOW_5GHZ_MAX)
				txpow1 = RT2860_EEPROM_TXPOW_5GHZ_MAX;

			r3 |= (txpow1 << 10) | (1 << 9);
		}

		if (txpow2 >= RT2860_EEPROM_TXPOW_5GHZ_MIN && txpow2 < 0)
		{
			txpow2 = (-RT2860_EEPROM_TXPOW_5GHZ_MIN + txpow2);
			if (txpow2 > RT2860_EEPROM_TXPOW_5GHZ_MAX)
				txpow2 = RT2860_EEPROM_TXPOW_5GHZ_MAX;

			r4 |= (txpow2 << 7);
		}
		else
		{
			if (txpow2 > RT2860_EEPROM_TXPOW_5GHZ_MAX)
				txpow2 = RT2860_EEPROM_TXPOW_5GHZ_MAX;

			r4 |= (txpow2 << 7) | (1 << 6);
		}
	}

	if (!(ic->ic_flags & IEEE80211_F_SCAN) && IEEE80211_IS_CHAN_HT40(c))
		r4 |= (1 << 21);

	rt2860_io_rf_write(sc, RT2860_REG_RF_R1, r1);
	rt2860_io_rf_write(sc, RT2860_REG_RF_R2, r2);
	rt2860_io_rf_write(sc, RT2860_REG_RF_R3, r3 & ~(1 << 2));
	rt2860_io_rf_write(sc, RT2860_REG_RF_R4, r4);

	DELAY(200);

	rt2860_io_rf_write(sc, RT2860_REG_RF_R1, r1);
	rt2860_io_rf_write(sc, RT2860_REG_RF_R2, r2);
	rt2860_io_rf_write(sc, RT2860_REG_RF_R3, r3 | (1 << 2));
	rt2860_io_rf_write(sc, RT2860_REG_RF_R4, r4);

	DELAY(200);

	rt2860_io_rf_write(sc, RT2860_REG_RF_R1, r1);
	rt2860_io_rf_write(sc, RT2860_REG_RF_R2, r2);
	rt2860_io_rf_write(sc, RT2860_REG_RF_R3, r3 & ~(1 << 2));
	rt2860_io_rf_write(sc, RT2860_REG_RF_R4, r4);

	rt2860_rf_select_chan_group(sc, c);

	DELAY(1000);
}
