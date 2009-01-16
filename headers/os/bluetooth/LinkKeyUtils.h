/*
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _LINKKEY_UTILS_H
#define _LINKKEY_UTILS_H

#include <stdio.h>

#include <bluetooth/bluetooth.h>


namespace Bluetooth {

class LinkKeyUtils {
public:
	static bool Compare(linkkey_t* lk1, linkkey_t* lk2)
	{
		return memcmp(lk1, lk2, sizeof(linkkey_t)) == 0;
	}

	static linkkey_t NullKey()
	{
		return (linkkey_t){{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
	}
	
	static char* ToString(const linkkey_t lk) 
	{
		// TODO: not safe
		static char str[50];

		sprintf(str, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:"
				"%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", 
				lk.l[0], lk.l[1], lk.l[2], lk.l[3], lk.l[4], lk.l[5],
				lk.l[6], lk.l[7], lk.l[8], lk.l[9], lk.l[10], lk.l[11],
				lk.l[12], lk.l[13], lk.l[14], lk.l[15]);

		return str;
	}

	static linkkey_t FromString(const char *lkstr)
	{
		if (lkstr != NULL) {
			int l0, l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12, l13, l14, l15;
			size_t count = sscanf(lkstr, "%2X:%2X:%2X:%2X:%2X:%2X:%2X:%2X:"
							"%2X:%2X:%2X:%2X:%2X:%2X:%2X:%2X", &l0, &l1, &l2, &l3, 
							&l4, &l5, &l6, &l7, &l8, &l9, &l10, &l11, &l12, &l13,
							&l14, &l15);

			if (count == 16) {
				return (linkkey_t){{l0, l1, l2, l3, l4, l5, l6, l7, l8,
					l9, l10, l11, l12, l13, l14, l15}};
			}
		}

		return NullKey();
	} 
};

}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::LinkKeyUtils;
#endif

#endif // _LINKKEY_UTILS_H
