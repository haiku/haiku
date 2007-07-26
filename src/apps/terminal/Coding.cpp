#include "Coding.h"
#include <string.h>

struct etable {
	const char *name;     	// long name for menu item.
	const char *shortname;	// short name (use for command-line etc.)
	const char shortcut;	// short cut key code
	const uint32 id;      	// encoding id 
};


/*
 * encoding_table ... use encoding menu, message, and preference keys.
 */
const static etable kEncodingTable[] =
{
	{"UTF-8", "UTF8", 'U', M_UTF8},
	{"ISO-8859-1", "8859-1", '1', B_ISO1_CONVERSION},
	{"ISO-8859-2", "8859-2", '2', B_ISO2_CONVERSION},
	{"ISO-8859-3", "8859-3", '3', B_ISO3_CONVERSION},
	{"ISO-8859-4", "8859-4", '4', B_ISO4_CONVERSION},
	{"ISO-8859-5", "8859-5", '5', B_ISO5_CONVERSION},
	{"ISO-8859-6", "8859-6", '6', B_ISO6_CONVERSION},
	{"ISO-8859-7", "8859-7", '7', B_ISO7_CONVERSION},
	{"ISO-8859-8", "8859-8", '8', B_ISO8_CONVERSION},
	{"ISO-8859-9", "8859-9", '9', B_ISO9_CONVERSION},
	{"ISO-8859-10", "8859-10", '0', B_ISO10_CONVERSION},
	{"MacRoman", "MacRoman",'M',  B_MAC_ROMAN_CONVERSION},
	{"JIS", "JIS", 'J', B_JIS_CONVERSION},
	{"Shift-JIS", "SJIS", 'S', B_SJIS_CONVERSION},
	{"EUC-jp", "EUCJ", 'E', B_EUC_CONVERSION},
	{"EUC-kr", "EUCK", 'K', B_EUC_KR_CONVERSION},


	/* Not Implement.
	{"EUC-tw", "EUCT", "T", M_EUC_TW},
	{"Big5", "Big5", 'B', M_BIG5},
	{"ISO-2022-cn", "ISOC", 'C', M_ISO_2022_CN},
	{"ISO-2022-kr", "ISOK", 'R', M_ISO_2022_KR},
	*/  

	{NULL, NULL, 0, 0},
};



status_t
get_nth_encoding(int i, int *id)
{
	if (id == NULL)
		return B_BAD_VALUE;
	
	if (i < 0 || i >= (int)(sizeof(kEncodingTable) / sizeof(etable)) - 1)
		return B_BAD_INDEX;

	*id = kEncodingTable[i].id;
	
	return B_OK;
}


int
longname2id(const char *longname)
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
longname2shortname(const char *longname)
{
	const char *shortName = NULL;

	const etable *p = kEncodingTable;
	while (p->name) {
		if (!strcmp(p->name, longname)) {
			shortName = p->shortname;
			break;
		}
		p++;
	}
	
	return shortName;
}


const char *
id2longname(int id)
{
	const etable *p = kEncodingTable;
	while (p->name) {
		if (id == p->id)
			return p->name;
		p++;
	}
}


const char
id2shortcut(int id)
{
	const etable *p = kEncodingTable;
	while (p->name) {
		if (id == p->id)
			return p->shortcut;
		p++;
	}
}

