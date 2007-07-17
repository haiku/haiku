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
	{"ISO-8859-1", "8859-1", '1', M_ISO_8859_1},
	{"ISO-8859-2", "8859-2", '2', M_ISO_8859_2},
	{"ISO-8859-3", "8859-3", '3', M_ISO_8859_3},
	{"ISO-8859-4", "8859-4", '4', M_ISO_8859_4},
	{"ISO-8859-5", "8859-5", '5', M_ISO_8859_5},
	{"ISO-8859-6", "8859-6", '6', M_ISO_8859_6},
	{"ISO-8859-7", "8859-7", '7', M_ISO_8859_7},
	{"ISO-8859-8", "8859-8", '8', M_ISO_8859_8},
	{"ISO-8859-9", "8859-9", '9', M_ISO_8859_9},
	{"ISO-8859-10", "8859-10", '0', M_ISO_8859_10},
	{"MacRoman", "MacRoman",'M',  M_MAC_ROMAN},
	{"JIS", "JIS", 'J', M_ISO_2022_JP},
	{"Shift-JIS", "SJIS", 'S', M_SJIS},
	{"EUC-jp", "EUCJ", 'E', M_EUC_JP},
	{"EUC-kr", "EUCK", 'K', M_EUC_KR},

	/* Not Implement.
	{"EUC-tw", "EUCT", "T", M_EUC_TW},
	{"Big5", "Big5", 'B', M_BIG5},
	{"ISO-2022-cn", "ISOC", 'C', M_ISO_2022_CN},
	{"ISO-2022-kr", "ISOK", 'R', M_ISO_2022_KR},
	*/  

	{NULL, NULL, 0, 0},
};


static int sCurrentEncoding = M_UTF8;


status_t
get_nth_encoding(int i, int *id)
{
	if (id == NULL)
		return B_BAD_VALUE;
	
	if (i < 0 || i >= (int)(sizeof(kEncodingTable) / sizeof(etable)))
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
	return kEncodingTable[id].name;
}


const char
id2shortcut(int id)
{
	return kEncodingTable[id].shortcut;
}


void
SetEncoding(int encoding)
{
	sCurrentEncoding = encoding;
}


int
GetEncoding()
{
	return sCurrentEncoding;
}

