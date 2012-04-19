
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

#ifndef _RT2860_AMRR_H_
#define _RT2860_AMRR_H_

#define RT2860_AMRR_MIN_SUCCESS_THRESHOLD	 				1
#define RT2860_AMRR_MAX_SUCCESS_THRESHOLD					15

struct rt2860_amrr
{
	int ntxpath;

	unsigned int min_success_threshold;
	unsigned int max_success_threshold;

	int	interval;
};

struct rt2860_amrr_node
{
	struct rt2860_amrr *amrr;

	int	rate_index;

	int	ticks;

	unsigned int txcnt;
	unsigned int success;
	unsigned int success_threshold;
	unsigned int recovery;
	unsigned int retrycnt;
};

void rt2860_amrr_init(struct rt2860_amrr *amrr, struct ieee80211vap *vap,
	int ntxpath, int min_success_threshold, int max_success_threshold, int msecs);

void rt2860_amrr_cleanup(struct rt2860_amrr *amrr);

void rt2860_amrr_node_init(struct rt2860_amrr *amrr,
    struct rt2860_amrr_node *amrr_node, struct ieee80211_node *ni);

int rt2860_amrr_choose(struct ieee80211_node *ni,
	struct rt2860_amrr_node *amrr_node);

static __inline void rt2860_amrr_tx_complete(struct rt2860_amrr_node *amrr_node,
	int ok, int retries)
{
	amrr_node->txcnt++;

	if (ok)
		amrr_node->success++;

	amrr_node->retrycnt += retries;
}

static __inline void rt2860_amrr_tx_update(struct rt2860_amrr_node *amrr_node,
	int txcnt, int success, int retrycnt)
{
	amrr_node->txcnt = txcnt;
	amrr_node->success = success;
	amrr_node->retrycnt = retrycnt;
}

#endif /* #ifndef _RT2860_AMRR_H_ */
