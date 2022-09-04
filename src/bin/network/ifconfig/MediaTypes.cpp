/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 *		Atis Elsts, the.kfx@gmail.com
 */


#include "MediaTypes.h"

#include <stdint.h>
#include <string.h>

extern "C" {
#	include <freebsd_network/compat/net/if_media.h>
}


struct media_type {
	int			type;
	const char*	name;
	const char* pretty;
	int 		subtype_mask;
	struct {
		int subtype;
		const char* name;
		const char* pretty;
	} subtypes [10];
	struct {
		int option;
		bool read_only;
		const char* name;
		const char* pretty;
	} options [6];
};


const media_type kMediaTypes[] = {
	{
		IFM_ETHER,
		"ether",
		"Ethernet",
		IFM_TMASK,
		{
			//{ IFM_AUTO, "auto", "Auto-select" },
			//{ IFM_AUI, "AUI", "10 MBit, AUI" },
			//{ IFM_10_2, "10base2", "10 MBit, 10BASE-2" },
			{ IFM_10_T, "10baseT", "10 MBit, 10BASE-T" },
			{ IFM_100_TX, "100baseTX", "100 MBit, 100BASE-TX" },
			{ IFM_1000_T, "1000baseT", "1 GBit, 1000BASE-T" },
			{ IFM_1000_SX, "1000baseSX", "1 GBit, 1000BASE-SX" },
			{ IFM_10G_T, "10GbaseT", "10 GBit, 10GBASE-T" },
			{ IFM_10G_SR, "10GbaseSR", "10 Gbit, 850 nm Fibre"},
			{ IFM_10G_LR, "10GbaseLR", "10 Gbit, 1310 nm Fibre"},
			{ IFM_10G_LRM, "10GbaseLRM", "10 Gbit, 1300 nm Fibre"},
			{ IFM_10G_TWINAX, "10GbaseCR", "10 Gbit, Direct Attach"},
			{ -1, NULL, NULL }
		},
		{
			{ -1, false, NULL, NULL }
		}
	},
	{
		IFM_IEEE80211,
		"80211",
		"Wireless Ethernet",
		IFM_MMASK,
		{
			{ IFM_IEEE80211_11A, "802.11a", "802.11a" },
			{ IFM_IEEE80211_11B, "802.11b", "802.11b" },
			{ IFM_IEEE80211_11G, "802.11g", "802.11g" },
			{ IFM_IEEE80211_FH, "802.11 FH", "802.11 FH" },
			{ IFM_IEEE80211_11NA, "802.11n(a)", "802.11n(a)" },
			{ IFM_IEEE80211_11NG, "802.11n(g)", "802.11n(g)" },
			{ IFM_IEEE80211_VHT5G, "802.11ac", "802.11ac" },
			{ -1, NULL, NULL }
		},
		{
			{ -1, false, NULL, NULL }
		}
	},
	{
		0, // for generic options
		"all",
		"All",
		IFM_TMASK,
		{
			{ IFM_AUTO, "auto", "Auto-select" },
			{ -1, NULL, NULL }
		},
		{
			{ IFM_FDX, true, "fullduplex", "Full Duplex" },
			{ IFM_HDX, true, "halfduplex", "Half Duplex" },
			{ IFM_LOOP, true, "loop", "Loop" },
			//{ IFM_ACTIVE, false, "active", "Active" },
			{ -1, false, NULL, NULL }
		}
	},
	{ -1, NULL, NULL, -1, {{ -1, NULL, NULL }}, {{ -1, false, NULL, NULL }} }
};


const char*
get_media_type_name(size_t index)
{
	if (index < sizeof(kMediaTypes) / sizeof(kMediaTypes[0]))
		return kMediaTypes[index].pretty;

	return NULL;
}


const char*
get_media_subtype_name(size_t typeIndex, size_t subIndex)
{
	if (typeIndex < sizeof(kMediaTypes) / sizeof(kMediaTypes[0])) {
		if (kMediaTypes[typeIndex].subtypes[subIndex].subtype >= 0)
			return kMediaTypes[typeIndex].subtypes[subIndex].name;
	}

	return NULL;
}


bool
media_parse_subtype(const char* string, int media, int* type)
{
	for (size_t i = 0; kMediaTypes[i].type >= 0; i++) {
		// only check for generic or correct subtypes
		if (kMediaTypes[i].type &&
			kMediaTypes[i].type != media)
			continue;
		for (size_t j = 0; kMediaTypes[i].subtypes[j].subtype >= 0; j++) {
			if (strcmp(kMediaTypes[i].subtypes[j].name, string) == 0) {
				// found a match
				*type = kMediaTypes[i].subtypes[j].subtype;
				return true;
			}
		}
	}
	return false;
}


const char*
media_type_to_string(int media)
{
	for (size_t i = 0; kMediaTypes[i].type >= 0; i++) {
		// loopback doesn't really have a media anyway
		if (IFM_TYPE(media) == 0)
			break;

		// only check for generic or correct subtypes
		if (kMediaTypes[i].type
			&& kMediaTypes[i].type != IFM_TYPE(media))
			continue;

		const int subtype = (media & kMediaTypes[i].subtype_mask);
		for (size_t j = 0; kMediaTypes[i].subtypes[j].subtype >= 0; j++) {
			if (kMediaTypes[i].subtypes[j].subtype == subtype) {
				// found a match
				return kMediaTypes[i].subtypes[j].pretty;
			}
		}
	}

	return NULL;
}
