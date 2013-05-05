
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

#ifndef _RT2860_RF_H_
#define _RT2860_RF_H_

#include <dev/rt2860/rt2860_softc.h>

const char *rt2860_rf_name(int rf_rev);

void rt2860_rf_select_chan_group(struct rt2860_softc *sc,
	struct ieee80211_channel *c);

void rt2860_rf_set_chan(struct rt2860_softc *sc,
	struct ieee80211_channel *c);

uint8_t rt3090_rf_read(struct rt2860_softc *sc, uint8_t reg);

void rt3090_rf_write(struct rt2860_softc *sc, uint8_t reg, uint8_t val);

void rt3090_set_chan(struct rt2860_softc *sc, u_int chan);

int rt3090_rf_init(struct rt2860_softc *sc);

void rt3090_set_rx_antenna(struct rt2860_softc *, int);

void rt3090_rf_wakeup(struct rt2860_softc *sc);

int rt3090_filter_calib(struct rt2860_softc *sc, uint8_t init, uint8_t target,
    uint8_t *val);

void rt3090_rf_setup(struct rt2860_softc *sc);

#endif /* #ifndef _RT2860_RF_H_ */
