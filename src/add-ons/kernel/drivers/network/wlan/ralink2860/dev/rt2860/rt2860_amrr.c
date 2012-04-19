
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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>

#include <dev/rt2860/rt2860_amrr.h>

/*
 * Defines and macros
 */

#define RT2860_AMRR_IS_SUCCESS(amrr_node)				((amrr_node)->retrycnt < (amrr_node)->txcnt / 10)

#define RT2860_AMRR_IS_FAILURE(amrr_node)				((amrr_node)->retrycnt > (amrr_node)->txcnt / 3)

#define RT2860_AMRR_IS_ENOUGH(amrr_node)				((amrr_node)->txcnt > 10)

/*
 * Static function prototypes
 */

static int rt2860_amrr_update(struct rt2860_amrr *amrr,
	struct rt2860_amrr_node *amrr_node, struct ieee80211_node *ni);

/*
 * rt2860_amrr_init
 */
void rt2860_amrr_init(struct rt2860_amrr *amrr, struct ieee80211vap *vap,
	int ntxpath, int min_success_threshold, int max_success_threshold, int msecs)
{
	int t;

	amrr->ntxpath = ntxpath;

	amrr->min_success_threshold = min_success_threshold;
	amrr->max_success_threshold = max_success_threshold;

	if (msecs < 100)
		msecs = 100;

	t = msecs_to_ticks(msecs);

	amrr->interval = (t < 1) ? 1 : t;
}

/*
 * rt2860_amrr_cleanup
 */
void rt2860_amrr_cleanup(struct rt2860_amrr *amrr)
{
}

/*
 * rt2860_amrr_node_init
 */
void rt2860_amrr_node_init(struct rt2860_amrr *amrr,
    struct rt2860_amrr_node *amrr_node, struct ieee80211_node *ni)
{
	const struct ieee80211_rateset *rs;

	amrr_node->amrr = amrr;
	amrr_node->success = 0;
	amrr_node->recovery = 0;
	amrr_node->txcnt = 0;
	amrr_node->retrycnt = 0;
	amrr_node->success_threshold = amrr->min_success_threshold;

	if (ni->ni_flags & IEEE80211_NODE_HT)
	{
		rs = (const struct ieee80211_rateset *) &ni->ni_htrates;

		for (amrr_node->rate_index = rs->rs_nrates - 1;
			amrr_node->rate_index > 0 && (rs->rs_rates[amrr_node->rate_index] & IEEE80211_RATE_VAL) > 4;
			amrr_node->rate_index--) ;

		ni->ni_txrate = rs->rs_rates[amrr_node->rate_index] | IEEE80211_RATE_MCS;
	}
	else
	{
		rs = &ni->ni_rates;

		for (amrr_node->rate_index = rs->rs_nrates - 1;
			amrr_node->rate_index > 0 && (rs->rs_rates[amrr_node->rate_index] & IEEE80211_RATE_VAL) > 72;
			amrr_node->rate_index--) ;

		ni->ni_txrate = rs->rs_rates[amrr_node->rate_index] & IEEE80211_RATE_VAL;
	}

	amrr_node->ticks = ticks;
}

/*
 * rt2860_amrr_choose
 */
int rt2860_amrr_choose(struct ieee80211_node *ni,
	struct rt2860_amrr_node *amrr_node)
{
	struct rt2860_amrr *amrr;
	int rate_index;

	amrr = amrr_node->amrr;

	if (RT2860_AMRR_IS_ENOUGH(amrr_node) &&
		(ticks - amrr_node->ticks) > amrr->interval)
	{
		rate_index = rt2860_amrr_update(amrr, amrr_node, ni);
		if (rate_index != amrr_node->rate_index)
		{
			if (ni->ni_flags & IEEE80211_NODE_HT)
				ni->ni_txrate = ni->ni_htrates.rs_rates[rate_index] | IEEE80211_RATE_MCS;
			else
				ni->ni_txrate = ni->ni_rates.rs_rates[rate_index] & IEEE80211_RATE_VAL;

			amrr_node->rate_index = rate_index;
		}

		amrr_node->ticks = ticks;
	}
	else
	{
		rate_index = amrr_node->rate_index;
	}

	return rate_index;
}

/*
 * rt2860_amrr_update
 */
static int rt2860_amrr_update(struct rt2860_amrr *amrr,
	struct rt2860_amrr_node *amrr_node, struct ieee80211_node *ni)
{
	const struct ieee80211_rateset *rs;
	int rate_index;

	KASSERT(RT2860_AMRR_IS_ENOUGH(amrr_node),
		("not enough Tx count: txcnt=%d",
		 amrr_node->txcnt));

	if (ni->ni_flags & IEEE80211_NODE_HT)
		rs = (const struct ieee80211_rateset *) &ni->ni_htrates;
	else
		rs = &ni->ni_rates;

	rate_index = amrr_node->rate_index;

	if (RT2860_AMRR_IS_SUCCESS(amrr_node))
	{
		amrr_node->success++;
		if ((amrr_node->success >= amrr_node->success_threshold) &&
		    (rate_index + 1 < rs->rs_nrates) &&
			(!(ni->ni_flags & IEEE80211_NODE_HT) || (rs->rs_rates[rate_index + 1] & IEEE80211_RATE_VAL) < (amrr->ntxpath * 8)))
		{
			amrr_node->recovery = 1;
			amrr_node->success = 0;

			rate_index++;
		}
		else
		{
			amrr_node->recovery = 0;
		}
	}
	else if (RT2860_AMRR_IS_FAILURE(amrr_node))
	{
		amrr_node->success = 0;

		if (rate_index > 0)
		{
			if (amrr_node->recovery)
			{
				amrr_node->success_threshold *= 2;
				if (amrr_node->success_threshold > amrr->max_success_threshold)
					amrr_node->success_threshold = amrr->max_success_threshold;
			}
			else
			{
				amrr_node->success_threshold = amrr->min_success_threshold;
			}

			rate_index--;
		}

		amrr_node->recovery = 0;
	}

	amrr_node->txcnt = 0;
	amrr_node->retrycnt = 0;

	return rate_index;
}
