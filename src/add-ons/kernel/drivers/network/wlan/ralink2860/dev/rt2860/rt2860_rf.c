
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

#include <dev/rt2860/rt2860_rf.h>
#include <dev/rt2860/rt2860_reg.h>
#include <dev/rt2860/rt2860_eeprom.h>
#include <dev/rt2860/rt2860_io.h>
#include <dev/rt2860/rt2860_debug.h>

static void rt2872_rf_set_chan(struct rt2860_softc *sc,	struct ieee80211_channel *c);

extern uint8_t rt3052_rf_default[];

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

static const struct rfprog {
	uint8_t		chan;
	uint32_t	r1, r2, r3, r4;
} rt2860_rf2850[] = {
	{   1, 0x100bb3, 0x1301e1, 0x05a014, 0x001402 },
	{   2, 0x100bb3, 0x1301e1, 0x05a014, 0x001407 },
	{   3, 0x100bb3, 0x1301e2, 0x05a014, 0x001402 },
	{   4, 0x100bb3, 0x1301e2, 0x05a014, 0x001407 },
	{   5, 0x100bb3, 0x1301e3, 0x05a014, 0x001402 },
	{   6, 0x100bb3, 0x1301e3, 0x05a014, 0x001407 },
	{   7, 0x100bb3, 0x1301e4, 0x05a014, 0x001402 },
	{   8, 0x100bb3, 0x1301e4, 0x05a014, 0x001407 },
	{   9, 0x100bb3, 0x1301e5, 0x05a014, 0x001402 },
	{  10, 0x100bb3, 0x1301e5, 0x05a014, 0x001407 },
	{  11, 0x100bb3, 0x1301e6, 0x05a014, 0x001402 },
	{  12, 0x100bb3, 0x1301e6, 0x05a014, 0x001407 },
	{  13, 0x100bb3, 0x1301e7, 0x05a014, 0x001402 },
	{  14, 0x100bb3, 0x1301e8, 0x05a014, 0x001404 },
	{  36, 0x100bb3, 0x130266, 0x056014, 0x001408 },
	{  38, 0x100bb3, 0x130267, 0x056014, 0x001404 },
	{  40, 0x100bb2, 0x1301a0, 0x056014, 0x001400 },
	{  44, 0x100bb2, 0x1301a0, 0x056014, 0x001408 },
	{  46, 0x100bb2, 0x1301a1, 0x056014, 0x001402 },
	{  48, 0x100bb2, 0x1301a1, 0x056014, 0x001406 },
	{  52, 0x100bb2, 0x1301a2, 0x056014, 0x001404 },
	{  54, 0x100bb2, 0x1301a2, 0x056014, 0x001408 },
	{  56, 0x100bb2, 0x1301a3, 0x056014, 0x001402 },
	{  60, 0x100bb2, 0x1301a4, 0x056014, 0x001400 },
	{  62, 0x100bb2, 0x1301a4, 0x056014, 0x001404 },
	{  64, 0x100bb2, 0x1301a4, 0x056014, 0x001408 },
	{ 100, 0x100bb2, 0x1301ac, 0x05e014, 0x001400 },
	{ 102, 0x100bb2, 0x1701ac, 0x15e014, 0x001404 },
	{ 104, 0x100bb2, 0x1701ac, 0x15e014, 0x001408 },
	{ 108, 0x100bb3, 0x17028c, 0x15e014, 0x001404 },
	{ 110, 0x100bb3, 0x13028d, 0x05e014, 0x001400 },
	{ 112, 0x100bb3, 0x13028d, 0x05e014, 0x001406 },
	{ 116, 0x100bb3, 0x13028e, 0x05e014, 0x001408 },
	{ 118, 0x100bb3, 0x13028f, 0x05e014, 0x001404 },
	{ 120, 0x100bb1, 0x1300e0, 0x05e014, 0x001400 },
	{ 124, 0x100bb1, 0x1300e0, 0x05e014, 0x001404 },
	{ 126, 0x100bb1, 0x1300e0, 0x05e014, 0x001406 },
	{ 128, 0x100bb1, 0x1300e0, 0x05e014, 0x001408 },
	{ 132, 0x100bb1, 0x1300e1, 0x05e014, 0x001402 },
	{ 134, 0x100bb1, 0x1300e1, 0x05e014, 0x001404 },
	{ 136, 0x100bb1, 0x1300e1, 0x05e014, 0x001406 },
	{ 140, 0x100bb1, 0x1300e2, 0x05e014, 0x001400 },
	{ 149, 0x100bb1, 0x1300e2, 0x05e014, 0x001409 },
	{ 151, 0x100bb1, 0x1300e3, 0x05e014, 0x001401 },
	{ 153, 0x100bb1, 0x1300e3, 0x05e014, 0x001403 },
	{ 157, 0x100bb1, 0x1300e3, 0x05e014, 0x001407 },
	{ 159, 0x100bb1, 0x1300e3, 0x05e014, 0x001409 },
	{ 161, 0x100bb1, 0x1300e4, 0x05e014, 0x001401 },
	{ 165, 0x100bb1, 0x1300e4, 0x05e014, 0x001405 },
	{ 167, 0x100bb1, 0x1300f4, 0x05e014, 0x001407 },
	{ 169, 0x100bb1, 0x1300f4, 0x05e014, 0x001409 },
	{ 171, 0x100bb1, 0x1300f5, 0x05e014, 0x001401 },
	{ 173, 0x100bb1, 0x1300f5, 0x05e014, 0x001403 }
};

static const struct rt2860_rf_fi3020
{
	uint8_t channel, n, r, k;
} rt2860_rf_fi3020[] =
{
	/* 802.11g  */
	{1,	241,	2,	2},
	{2,	241,	2,	7},
	{3,	242,	2,	2},
	{4,	242,	2,	7},
	{5,	243,	2,	2},
	{6,	243,	2,	7},
	{7,	244,	2,	2},
	{8,	244,	2,	7},
	{9,	245,	2,	2},
	{10,	245,	2,	7},
	{11,	246,	2,	2},
	{12,	246,	2,	7},
	{13,	247,	2,	2},
	{14,	248,	2,	4},

	/* 802.11 UNI / HyperLan 2 */
	{36,	0x56,	0,	4},
	{38,	0x56,	0,	6},
	{40,	0x56,	0,	8},
	{44,	0x57,	0,	0},
	{46,	0x57,	0,	2},
	{48,	0x57,	0,	4},
	{52,	0x57,	0,	8},
	{54,	0x57,	0,	10},
	{56,	0x58,	0,	0},
	{60,	0x58,	0,	4},
	{62,	0x58,	0,	6},
	{64,	0x58,	0,	8},

	/* 802.11 HyperLan 2 */
	{100,	0x5b,	0,	8},
	{102,	0x5b,	0,	10},
	{104,	0x5c,	0,	0},
	{108,	0x5c,	0,	4},
	{110,	0x5c,	0,	6},
	{112,	0x5c,	0,	8},
	{116,	0x5d,	0,	0},
	{118,	0x5d,	0,	2},
	{120,	0x5d,	0,	4},
	{124,	0x5d,	0,	8},
	{126,	0x5d,	0,	10},
	{128,	0x5e,	0,	0},
	{132,	0x5e,	0,	4},
	{134,	0x5e,	0,	6},
	{136,	0x5e,	0,	8},
	{140,	0x5f,	0,	0},

	/* 802.11 UNII */
	{149,	0x5f,	0,	9},
	{151,	0x5f,	0,	11},
	{153,	0x60,	0,	1},
	{157,	0x60,	0,	5},
	{159,	0x60,	0,	7},
	{161,	0x60,	0,	9},
	{165,	0x61,	0,	1},
	{167,	0x61,	0,	3},
	{169,	0x61,	0,	5},
	{171,	0x61,	0,	7},
	{173,	0x61,	0,	9},
};

static const struct {
	uint8_t	reg;
	uint8_t	val;
}  rt3090_def_rf[] = {
	{  4, 0x40 },
	{  5, 0x03 },
	{  6, 0x02 },
	{  7, 0x70 },
	{  9, 0x0f },
	{ 10, 0x41 },
	{ 11, 0x21 },
	{ 12, 0x7b },
	{ 14, 0x90 },
	{ 15, 0x58 },
	{ 16, 0xb3 },
	{ 17, 0x92 },
	{ 18, 0x2c },
	{ 19, 0x02 },
	{ 20, 0xba },
	{ 21, 0xdb },
	{ 24, 0x16 },
	{ 25, 0x01 },
	{ 29, 0x1f }
};

struct {
	uint8_t	n, r, k;
} rt3090_freqs[] = {
	{ 0xf1, 2,  2 },
	{ 0xf1, 2,  7 },
	{ 0xf2, 2,  2 },
	{ 0xf2, 2,  7 },
	{ 0xf3, 2,  2 },
	{ 0xf3, 2,  7 },
	{ 0xf4, 2,  2 },
	{ 0xf4, 2,  7 },
	{ 0xf5, 2,  2 },
	{ 0xf5, 2,  7 },
	{ 0xf6, 2,  2 },
	{ 0xf6, 2,  7 },
	{ 0xf7, 2,  2 },
	{ 0xf8, 2,  4 },
	{ 0x56, 0,  4 },
	{ 0x56, 0,  6 },
	{ 0x56, 0,  8 },
	{ 0x57, 0,  0 },
	{ 0x57, 0,  2 },
	{ 0x57, 0,  4 },
	{ 0x57, 0,  8 },
	{ 0x57, 0, 10 },
	{ 0x58, 0,  0 },
	{ 0x58, 0,  4 },
	{ 0x58, 0,  6 },
	{ 0x58, 0,  8 },
	{ 0x5b, 0,  8 },
	{ 0x5b, 0, 10 },
	{ 0x5c, 0,  0 },
	{ 0x5c, 0,  4 },
	{ 0x5c, 0,  6 },
	{ 0x5c, 0,  8 },
	{ 0x5d, 0,  0 },
	{ 0x5d, 0,  2 },
	{ 0x5d, 0,  4 },
	{ 0x5d, 0,  8 },
	{ 0x5d, 0, 10 },
	{ 0x5e, 0,  0 },
	{ 0x5e, 0,  4 },
	{ 0x5e, 0,  6 },
	{ 0x5e, 0,  8 },
	{ 0x5f, 0,  0 },
	{ 0x5f, 0,  9 },
	{ 0x5f, 0, 11 },
	{ 0x60, 0,  1 },
	{ 0x60, 0,  5 },
	{ 0x60, 0,  7 },
	{ 0x60, 0,  9 },
	{ 0x61, 0,  1 },
	{ 0x61, 0,  3 },
	{ 0x61, 0,  5 },
	{ 0x61, 0,  7 },
	{ 0x61, 0,  9 }
};



uint8_t
rt3090_rf_read(struct rt2860_softc *sc, uint8_t reg)
{
	uint32_t tmp;
	int ntries;

	for (ntries = 0; ntries < 100; ntries++) {
		if (!(rt2860_io_mac_read(sc, RT3070_RF_CSR_CFG) & RT3070_RF_KICK))
			break;
		DELAY(1);
	}
	if (ntries == 100) {
		device_printf(sc->dev, "could not read RF register\n");
		return 0xff;
	}
	tmp = RT3070_RF_KICK | reg << 8;
	rt2860_io_mac_write(sc, RT3070_RF_CSR_CFG, tmp);

	for (ntries = 0; ntries < 100; ntries++) {
		tmp = rt2860_io_mac_read(sc, RT3070_RF_CSR_CFG);
		if (!(tmp & RT3070_RF_KICK))
			break;
		DELAY(1);
	}
	if (ntries == 100) {
		device_printf(sc->dev, "could not read RF register\n");
		return 0xff;
	}
	return tmp & 0xff;
}

void
rt3090_rf_write(struct rt2860_softc *sc, uint8_t reg, uint8_t val)
{
	uint32_t tmp;
	int ntries;

	for (ntries = 0; ntries < 10; ntries++) {
		if (!(rt2860_io_mac_read(sc, RT3070_RF_CSR_CFG) & RT3070_RF_KICK))
			break;
		DELAY(10);
	}
	if (ntries == 10) {
		device_printf(sc->dev, "could not write to RF\n");
		return;
	}

	tmp = RT3070_RF_WRITE | RT3070_RF_KICK | reg << 8 | val;
	rt2860_io_mac_write(sc, RT3070_RF_CSR_CFG, tmp);
}

/*
 * rt2860_rf_name
 */
const char *rt2860_rf_name(int rf_rev)
{
	switch (rf_rev)
	{
		case RT2860_EEPROM_RF_2820:
			return "RT2820 2.4G 2T3R";

		case RT2860_EEPROM_RF_2850:
			return "RT2850 2.4G/5G 2T3R";

		case RT2860_EEPROM_RF_2720:
			return "RT2720 2.4G 1T2R";

		case RT2860_EEPROM_RF_2750:
			return "RT2750 2.4G/5G 1T2R";

		case RT2860_EEPROM_RF_3020:
			return "RT3020 2.4G 1T1R";

		case RT2860_EEPROM_RF_2020:
			return "RT2020 2.4G B/G";

		case RT2860_EEPROM_RF_3021:
			return "RT3021 2.4G 1T2R";

		case RT2860_EEPROM_RF_3022:
			return "RT3022 2.4G 2T2R";

		case RT2860_EEPROM_RF_3052:
			return "RT3052 2.4G/5G 2T2R";

		case RT2860_EEPROM_RF_2853:
			return "RT2853 2.4G.5G 3T3R";

		case RT2860_EEPROM_RF_3320:
			return "RT3320 2.4G 1T1R with PA";

		case RT2860_EEPROM_RF_3322:
			return "RT3322 2.4G 2T2R with PA";

		case RT2860_EEPROM_RF_3053:
			return "RT3053 2.4G/5G 3T3R";

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

void
rt3090_set_chan(struct rt2860_softc *sc, u_int chan)
{
	int8_t txpow1, txpow2;
	uint8_t rf;
	int i;

	KASSERT((chan >= 1 && chan <= 14), "RT3090 is 2GHz only");	/* RT3090 is 2GHz only */

	/* find the settings for this channel (we know it exists) */
	for (i = 0; rt2860_rf2850[i].chan != chan; i++);

	/* use Tx power values from EEPROM */
	txpow1 = sc->txpow1[i];
	txpow2 = sc->txpow2[i];

	rt3090_rf_write(sc, 2, rt3090_freqs[i].n);
	rf = rt3090_rf_read(sc, 3);
	rf = (rf & ~0x0f) | rt3090_freqs[i].k;
	rt3090_rf_write(sc, 3, rf);
	rf = rt3090_rf_read(sc, 6);
	rf = (rf & ~0x03) | rt3090_freqs[i].r;
	rt3090_rf_write(sc, 6, rf);

	/* set Tx0 power */
	rf = rt3090_rf_read(sc, 12);
	rf = (rf & ~0x1f) | txpow1;
	rt3090_rf_write(sc, 12, rf);

	/* set Tx1 power */
	rf = rt3090_rf_read(sc, 13);
	rf = (rf & ~0x1f) | txpow2;
	rt3090_rf_write(sc, 13, rf);

	rf = rt3090_rf_read(sc, 1);
	rf &= ~0xfc;
	if (sc->ntxpath == 1)
		rf |= RT3070_TX1_PD | RT3070_TX2_PD;
	else if (sc->ntxpath == 2)
		rf |= RT3070_TX2_PD;
	if (sc->nrxpath == 1)
		rf |= RT3070_RX1_PD | RT3070_RX2_PD;
	else if (sc->nrxpath == 2)
		rf |= RT3070_RX2_PD;
	rt3090_rf_write(sc, 1, rf);

	/* set RF offset */
	rf = rt3090_rf_read(sc, 23);
	rf = (rf & ~0x7f) | sc->rf_freq_off;
	rt3090_rf_write(sc, 23, rf);

	/* program RF filter */
	rf = rt3090_rf_read(sc, 24);	/* Tx */
	rf = (rf & ~0x3f) | sc->rf24_20mhz;
	rt3090_rf_write(sc, 24, rf);
	rf = rt3090_rf_read(sc, 31);	/* Rx */
	rf = (rf & ~0x3f) | sc->rf24_20mhz;
	rt3090_rf_write(sc, 31, rf);

	/* enable RF tuning */
	rf = rt3090_rf_read(sc, 7);
	rt3090_rf_write(sc, 7, rf | RT3070_TUNE);
}

int
rt3090_rf_init(struct rt2860_softc *sc)
{
	uint32_t tmp;
	uint8_t rf, bbp;
	int i;

	rf = rt3090_rf_read(sc, 30);
	/* toggle RF R30 bit 7 */
	rt3090_rf_write(sc, 30, rf | 0x80);
	DELAY(1000);
	rt3090_rf_write(sc, 30, rf & ~0x80);

	tmp = rt2860_io_mac_read(sc, RT3070_LDO_CFG0);
	tmp &= ~0x1f000000;
	if (sc->patch_dac && (sc->mac_rev & 0x0000ffff) < 0x0211)
		tmp |= 0x0d000000;	/* 1.35V */
	else
		tmp |= 0x01000000;	/* 1.2V */
	rt2860_io_mac_write(sc, RT3070_LDO_CFG0, tmp);

	/* patch LNA_PE_G1 */
	tmp = rt2860_io_mac_read(sc, RT3070_GPIO_SWITCH);
	rt2860_io_mac_write(sc, RT3070_GPIO_SWITCH, tmp & ~0x20);

	/* initialize RF registers to default value */
	for (i = 0; i < (sizeof(rt3090_def_rf)/2); i++) {
		rt3090_rf_write(sc, rt3090_def_rf[i].reg,
		    rt3090_def_rf[i].val);
	}

	/* select 20MHz bandwidth */
	rt3090_rf_write(sc, 31, 0x14);

	rf = rt3090_rf_read(sc, 6);
	rt3090_rf_write(sc, 6, rf | 0x40);

	if ((sc->mac_rev & 0xffff0000) != 0x35930000) {
		/* calibrate filter for 20MHz bandwidth */
		sc->rf24_20mhz = 0x1f;	/* default value */
		rt3090_filter_calib(sc, 0x07, 0x16, &sc->rf24_20mhz);

		/* select 40MHz bandwidth */
		bbp = rt2860_io_bbp_read(sc, 4);
		rt2860_io_bbp_write(sc, 4, (bbp & ~0x08) | 0x10);
		rf = rt3090_rf_read(sc, 31);
		rt3090_rf_write(sc, 31, rf | 0x20);

		/* calibrate filter for 40MHz bandwidth */
		sc->rf24_40mhz = 0x2f;	/* default value */
		rt3090_filter_calib(sc, 0x27, 0x19, &sc->rf24_40mhz);

		/* go back to 20MHz bandwidth */
		bbp = rt2860_io_bbp_read(sc, 4);
		rt2860_io_bbp_write(sc, 4, bbp & ~0x18);
	}
	if ((sc->mac_rev & 0x0000ffff) < 0x0211)
		rt3090_rf_write(sc, 27, 0x03);

	tmp = rt2860_io_mac_read(sc, RT3070_OPT_14);
	rt2860_io_mac_write(sc, RT3070_OPT_14, tmp | 1);

	if (sc->rf_rev == RT2860_EEPROM_RF_3020)
		rt3090_set_rx_antenna(sc, 0);

	bbp = rt2860_io_bbp_read(sc, 138);
	if ((sc->mac_rev & 0xffff0000) == 0x35930000) {
		if (sc->ntxpath == 1)
			bbp |= 0x60;	/* turn off DAC1 and DAC2 */
		else if (sc->ntxpath == 2)
			bbp |= 0x40;	/* turn off DAC2 */
		if (sc->nrxpath == 1)
			bbp &= ~0x06;	/* turn off ADC1 and ADC2 */
		else if (sc->nrxpath == 2)
			bbp &= ~0x04;	/* turn off ADC2 */
	} else {
		if (sc->ntxpath == 1)
			bbp |= 0x20;	/* turn off DAC1 */
		if (sc->nrxpath == 1)
			bbp &= ~0x02;	/* turn off ADC1 */
	}
	rt2860_io_bbp_write(sc, 138, bbp);

	rf = rt3090_rf_read(sc, 1);
	rf &= ~(RT3070_RX0_PD | RT3070_TX0_PD);
	rf |= RT3070_RF_BLOCK | RT3070_RX1_PD | RT3070_TX1_PD;
	rt3090_rf_write(sc, 1, rf);

	rf = rt3090_rf_read(sc, 15);
	rt3090_rf_write(sc, 15, rf & ~RT3070_TX_LO2);

	rf = rt3090_rf_read(sc, 17);
	rf &= ~RT3070_TX_LO1;
	if ((sc->mac_rev & 0x0000ffff) >= 0x0211 && !sc->ext_lna_2ghz)
		rf |= 0x20;	/* fix for long range Rx issue */
	if (sc->txmixgain_2ghz >= 2)
		rf = (rf & ~0x7) | sc->txmixgain_2ghz;
	rt3090_rf_write(sc, 17, rf);

	rf = rt3090_rf_read(sc, 20);
	rt3090_rf_write(sc, 20, rf & ~RT3070_RX_LO1);

	rf = rt3090_rf_read(sc, 21);
	rt3090_rf_write(sc, 21, rf & ~RT3070_RX_LO2);

	return 0;
}

void
rt3090_set_rx_antenna(struct rt2860_softc *sc, int aux)
{
	uint32_t tmp;

	if (aux) {
		tmp = rt2860_io_mac_read(sc, RT2860_REG_EEPROM_CSR);
		rt2860_io_mac_write(sc, RT2860_REG_EEPROM_CSR, tmp & ~RT2860_REG_EESK);
		tmp = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_GPIO_CTRL_CFG);
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_GPIO_CTRL_CFG, (tmp & ~0x0808) | 0x08);
	} else {
		tmp = rt2860_io_mac_read(sc, RT2860_REG_EEPROM_CSR);
		rt2860_io_mac_write(sc, RT2860_REG_EEPROM_CSR, tmp | RT2860_REG_EESK);
		tmp = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_GPIO_CTRL_CFG);
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_GPIO_CTRL_CFG, tmp & ~0x0808);
	}
}

void
rt3090_rf_wakeup(struct rt2860_softc *sc)
{
	uint32_t tmp;
	uint8_t rf;

	if ((sc->mac_rev & 0xffff0000) == 0x35930000) {
		/* enable VCO */
		rf = rt3090_rf_read(sc, 1);
		rt3090_rf_write(sc, 1, rf | RT3593_VCO);

		/* initiate VCO calibration */
		rf = rt3090_rf_read(sc, 3);
		rt3090_rf_write(sc, 3, rf | RT3593_VCOCAL);

		/* enable VCO bias current control */
		rf = rt3090_rf_read(sc, 6);
		rt3090_rf_write(sc, 6, rf | RT3593_VCO_IC);

		/* initiate res calibration */
		rf = rt3090_rf_read(sc, 2);
		rt3090_rf_write(sc, 2, rf | RT3593_RESCAL);

		/* set reference current control to 0.33 mA */
		rf = rt3090_rf_read(sc, 22);
		rf &= ~RT3593_CP_IC_MASK;
		rf |= 1 << RT3593_CP_IC_SHIFT;
		rt3090_rf_write(sc, 22, rf);

		/* enable RX CTB */
		rf = rt3090_rf_read(sc, 46);
		rt3090_rf_write(sc, 46, rf | RT3593_RX_CTB);

		rf = rt3090_rf_read(sc, 20);
		rf &= ~(RT3593_LDO_RF_VC_MASK | RT3593_LDO_PLL_VC_MASK);
		rt3090_rf_write(sc, 20, rf);
	} else {
		/* enable RF block */
		rf = rt3090_rf_read(sc, 1);
		rt3090_rf_write(sc, 1, rf | RT3070_RF_BLOCK);

		/* enable VCO bias current control */
		rf = rt3090_rf_read(sc, 7);
		rt3090_rf_write(sc, 7, rf | 0x30);

		rf = rt3090_rf_read(sc, 9);
		rt3090_rf_write(sc, 9, rf | 0x0e);

		/* enable RX CTB */
		rf = rt3090_rf_read(sc, 21);
		rt3090_rf_write(sc, 21, rf | RT3070_RX_CTB);

		/* fix Tx to Rx IQ glitch by raising RF voltage */
		rf = rt3090_rf_read(sc, 27);
		rf &= ~0x77;
		if ((sc->mac_rev & 0x0000ffff) < 0x0211)
			rf |= 0x03;
		rt3090_rf_write(sc, 27, rf);
	}
	if (sc->patch_dac && (sc->mac_rev & 0x0000ffff) < 0x0211) {
		tmp = rt2860_io_mac_read(sc, RT3070_LDO_CFG0);
		tmp = (tmp & ~0x1f000000) | 0x0d000000;
		rt2860_io_mac_write(sc, RT3070_LDO_CFG0, tmp);
	}
}

int
rt3090_filter_calib(struct rt2860_softc *sc, uint8_t init, uint8_t target,
    uint8_t *val)
{
	uint8_t rf22, rf24;
	uint8_t bbp55_pb, bbp55_sb, delta;
	int ntries;

	/* program filter */
	rf24 = rt3090_rf_read(sc, 24);
	rf24 = (rf24 & 0xc0) | init;	/* initial filter value */
	rt3090_rf_write(sc, 24, rf24);

	/* enable baseband loopback mode */
	rf22 = rt3090_rf_read(sc, 22);
	rt3090_rf_write(sc, 22, rf22 | RT3070_BB_LOOPBACK);

	/* set power and frequency of passband test tone */
	rt2860_io_bbp_write(sc, 24, 0x00);
	for (ntries = 0; ntries < 100; ntries++) {
		/* transmit test tone */
		rt2860_io_bbp_write(sc, 25, 0x90);
		DELAY(1000);
		/* read received power */
		bbp55_pb = rt2860_io_bbp_read(sc, 55);
		if (bbp55_pb != 0)
			break;
	}
	if (ntries == 100)
		return ETIMEDOUT;

	/* set power and frequency of stopband test tone */
	rt2860_io_bbp_write(sc, 24, 0x06);
	for (ntries = 0; ntries < 100; ntries++) {
		/* transmit test tone */
		rt2860_io_bbp_write(sc, 25, 0x90);
		DELAY(1000);
		/* read received power */
		bbp55_sb = rt2860_io_bbp_read(sc, 55);

		delta = bbp55_pb - bbp55_sb;
		if (delta > target)
			break;

		/* reprogram filter */
		rf24++;
		rt3090_rf_write(sc, 24, rf24);
	}
	if (ntries < 100) {
		if (rf24 != init)
			rf24--;	/* backtrack */
		*val = rf24;
		rt3090_rf_write(sc, 24, rf24);
	}

	/* restore initial state */
	rt2860_io_bbp_write(sc, 24, 0x00);

	/* disable baseband loopback mode */
	rf22 = rt3090_rf_read(sc, 22);
	rt3090_rf_write(sc, 22, rf22 & ~RT3070_BB_LOOPBACK);

	return 0;
}

void
rt3090_rf_setup(struct rt2860_softc *sc)
{
	uint8_t bbp;
	int i;

	if ((sc->mac_rev & 0x0000ffff) >= 0x0211) {
		/* enable DC filter */
		rt2860_io_bbp_write(sc, 103, 0xc0);

		/* improve power consumption */
		bbp = rt2860_io_bbp_read(sc, 31);
		rt2860_io_bbp_write(sc, 31, bbp & ~0x03);
	}

	rt2860_io_mac_write(sc, RT2860_REG_TX_SW_CFG1, 0);
	if ((sc->mac_rev & 0x0000ffff) < 0x0211) {
		rt2860_io_mac_write(sc, RT2860_REG_TX_SW_CFG2,
		    sc->patch_dac ? 0x2c : 0x0f);
	} else
		rt2860_io_mac_write(sc, RT2860_REG_TX_SW_CFG2, 0);

	/* initialize RF registers from ROM */
	for (i = 0; i < 10; i++) {
		if (sc->rf[i].reg == 0 || sc->rf[i].reg == 0xff)
			continue;
		rt3090_rf_write(sc, sc->rf[i].reg, sc->rf[i].val);
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

	if (sc->mac_rev == 0x28720200) {
		rt2872_rf_set_chan(sc, c);
		return;
	} 

	ifp = sc->ifp;
	ic = ifp->if_l2com;
	prog = rt2860_rf_2850;

	/* get central channel position */

	chan = ieee80211_chan2ieee(ic, c);

	if ((sc->mac_rev & 0xffff0000) >= 0x30710000) {
		rt3090_set_chan(sc, chan);
		return;
	}

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
		r4 = (r4 & ~0x001f87c0) | (sc->rf_freq_off << 15) |
		    (txpow2 << 6);
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

/*
 * rt2872_rf_set_chan
 */
static void 
rt2872_rf_set_chan(struct rt2860_softc *sc,
	struct ieee80211_channel *c)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	const struct rt2860_rf_prog *prog;
	uint32_t r1, r2, r3, r4;
	uint32_t r6, r7, r12, r13, r23, r24;
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

	for (i = 0; rt2860_rf_fi3020[i].channel != chan; i++);

	/* Programm channel parameters */
	r2 = rt2860_rf_fi3020[i].n;
	rt2860_io_rf_write(sc, 2 , r2 );
	r3 = rt2860_rf_fi3020[i].k;
	rt2860_io_rf_write(sc, 3 , r3 );

	r6 = (rt3052_rf_default[6] & 0xFC) | (rt2860_rf_fi3020[i].r & 0x03);
	rt2860_io_rf_write(sc, 6 , r6 );

	/* Set Tx Power */
	r12 = (rt3052_rf_default[12] & 0xE0) | (txpow1 & 0x1f);
	rt2860_io_rf_write(sc, 12, r12);

	/* Set Tx1 Power */
	r13 = (rt3052_rf_default[13] & 0xE0) | (txpow2 & 0x1f);
	rt2860_io_rf_write(sc, 13, r13);

	/* Set RF offset */
	r23 = (rt3052_rf_default[23] & 0x80) | (sc->rf_freq_off);
	rt2860_io_rf_write(sc, 23, r23);

	/* Set BW */
	r24 = (rt3052_rf_default[24] & 0xDF);
	if (!(ic->ic_flags & IEEE80211_F_SCAN) && IEEE80211_IS_CHAN_HT40(c))
	    r24 |= 0x20;
	rt2860_io_rf_write(sc, 24, r24);

	/* Enable RF tuning */
	r7 = (rt3052_rf_default[7]) | 1;
	rt2860_io_rf_write(sc, 7 , r7 );

	/* Antenna */
	r1 = (rt3052_rf_default[1] & 0xab) | ((sc->nrxpath == 1)?0x10:0) |
	    ((sc->ntxpath == 1)?0x20:0);
	rt2860_io_rf_write(sc, 1 , r1 );

	DELAY(200);

	rt2860_rf_select_chan_group(sc, c);

	DELAY(1000);
}

