/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef DEFAULT_TYPES_H
#define DEFAULT_TYPES_H


#include "AppFlagsEdit.h"
#include "BooleanEdit.h"
#include "EditView.h"
#include "NormalEdit.h"
#include "ResourceRow.h"

#include <String.h>


struct ResourceType {
			const char* 	type;
			const char*		code;
			const char* 	data;
			uint32			size;
			EditView*		edit;

	static	int32			FindIndex(const char* type);
	static	int32			FindIndex(type_code code);

	static	void			CodeToString(type_code code, char* str);
	static	type_code		StringToCode(const char* code);
};


#define LINE	"", "", "", ~0, NULL
#define END		NULL, NULL, NULL, 0, NULL

const ResourceType kDefaultTypes[] = {
	{ "app_signature",			"",		"application/MyApp",		18, new NormalEdit() 	},
	{ "app_name_catalog_entry", "",		"MyApp:System name:MyApp",	24, new NormalEdit()	},
	//{ "app_flags",				"",		"None",						4,	NULL },
	{ LINE },
	{ "bool",					"BOOL", "false",					1, 	new BooleanEdit()	},
	{ LINE },
	{ "int8",					"BYTE", "0", 						1, 	new NormalEdit() 	},
	{ "int16",					"SHRT", "0", 						2, 	new NormalEdit() 	},
	{ "int32",					"LONG", "0", 						4, 	new NormalEdit() 	},
	{ "int64",					"LLNG", "0", 						8, 	new NormalEdit() 	},
	{ LINE },
	{ "uint8",					"UBYT", "0", 						1, 	new NormalEdit() 	},
	{ "uint16",					"USHT", "0",						2, 	new NormalEdit() 	},
	{ "uint32",					"ULNG", "0",						4, 	new NormalEdit()	},
	{ "uint64",					"ULLG", "0",						8, 	new NormalEdit()	},
	{ LINE },
	{ "string",					"CSTR", "\"\"",						0,	NULL },
	{ "raw",					"RAWT", "",							0, 	NULL },
	{ LINE },
	{ "array",					"",		"", 						0, 	NULL },
	{ "message",				"",		"", 						0, 	NULL },
	{ "import", 				"",		"", 						0, 	NULL },
	{ END }
};

const int32 kDefaultTypeSelected = 8;
	// int32

#undef LINE
#undef END


#endif
