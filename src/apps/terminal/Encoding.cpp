/*
 * Copyright 2003-2009 Haiku, Inc.
 * Distributed under the terms of the MIT license.
 */

#include "Encoding.h"
#include "TermConst.h"

#include <string.h>


struct etable {
	const char *name;     	// long name for menu item.
	const char *shortname;	// short name (use for command-line etc.)
	const int32 id;      	// encoding id 
};

/*
 * encoding_table ... use encoding menu, message, and preference keys.
 */
const static etable kEncodingTable[] = {
	{"UTF-8", "UTF8", M_UTF8},
	{"ISO-8859-1", "8859-1", B_ISO1_CONVERSION},
	{"ISO-8859-2", "8859-2", B_ISO2_CONVERSION},
	{"ISO-8859-3", "8859-3", B_ISO3_CONVERSION},
	{"ISO-8859-4", "8859-4", B_ISO4_CONVERSION},
	{"ISO-8859-5", "8859-5", B_ISO5_CONVERSION},
	{"ISO-8859-6", "8859-6", B_ISO6_CONVERSION},
	{"ISO-8859-7", "8859-7", B_ISO7_CONVERSION},
	{"ISO-8859-8", "8859-8", B_ISO8_CONVERSION},
	{"ISO-8859-9", "8859-9", B_ISO9_CONVERSION},
	{"ISO-8859-10", "8859-10", B_ISO10_CONVERSION},
	{"MacRoman", "MacRoman", B_MAC_ROMAN_CONVERSION},
	{"JIS", "JIS", B_JIS_CONVERSION},
	{"Shift-JIS", "SJIS", B_SJIS_CONVERSION},
	{"EUC-jp", "EUCJ", B_EUC_CONVERSION},
	{"EUC-kr", "EUCK", B_EUC_KR_CONVERSION},
	{"GB18030", "GB18030", B_GBK_CONVERSION},
	{"Big5", "Big5", B_BIG5_CONVERSION},

	/* Not Implemented.
	{"EUC-tw", "EUCT", M_EUC_TW},
	{"ISO-2022-cn", "ISOC", M_ISO_2022_CN},
	{"ISO-2022-kr", "ISOK", M_ISO_2022_KR},
	*/  

	{NULL, NULL, 0},
};



status_t
get_next_encoding(int i, int *id)
{
	if (id == NULL)
		return B_BAD_VALUE;
	
	if (i < 0 || i >= (int)(sizeof(kEncodingTable) / sizeof(etable)) - 1)
		return B_BAD_INDEX;

	*id = kEncodingTable[i].id;
	
	return B_OK;
}


int
EncodingID(const char *longname)
{
	int id = M_UTF8;
	const etable *s = kEncodingTable;
	
	for (int i = 0; s->name; s++, i++) {
		if (!strcmp(s->name, longname)) {
			id = s->id;
			break;
		}
	}
	return id;
}


const char *
EncodingAsShortString(int id)
{
	const etable *p = kEncodingTable;
	while (p->name) {
		if (id == p->id)
			return p->shortname;
		p++;
	}

	return kEncodingTable[0].shortname;
}


const char *
EncodingAsString(int id)
{
	const etable *p = kEncodingTable;
	while (p->name) {
		if (id == p->id)
			return p->name;
		p++;
	}

	return kEncodingTable[0].name;
}
