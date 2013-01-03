/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef DEFAULT_TYPES_H
#define DEFAULT_TYPES_H


#include <String.h>


struct ResourceDataType	{
	const char* type;
	type_code	typeCode;
	uint32		size;
	BString (*toString)(const void*);
};

// TODO: Rework design of this. This one sucks.

BString toStringBOOL(const void* data);
BString toStringBYTE(const void* data);
BString toStringSHRT(const void* data);
BString toStringLONG(const void* data);
BString toStringLLNG(const void* data);
BString toStringUBYT(const void* data);
BString toStringUSHT(const void* data);
BString toStringULNG(const void* data);
BString toStringULLG(const void* data);
BString toStringRAWT(const void* data);

char * const kDefaultData[] = {
	0, 0, 0, 0, 0, 0, 0, 0
};

#define LINE	"", 0, ~0
#define END		NULL, 0, 0

const ResourceDataType kDefaultTypes[] = {
	{ "bool",	'BOOL', 1, toStringBOOL },
	{ LINE },
	{ "int8",	'BYTE', 1, toStringBYTE },
	{ "int16",	'SHRT', 2, toStringSHRT },
	{ "int32",	'LONG', 4, toStringLONG },
	{ "int64",	'LLNG', 8, toStringLLNG },
	{ LINE },
	{ "uint8",	'UBYT', 1, toStringUBYT },
	{ "uint16",	'USHT', 2, toStringUSHT },
	{ "uint32",	'ULNG', 4, toStringULNG },
	{ "uint64",	'ULLG', 8, toStringULLG },
	{ LINE },
	{ "raw",	'RAWT', 0, toStringRAWT },
	{ END }
};

const int32 kDefaultTypeSelected = 4;
	// int32

#undef LINE
#undef END

int32 FindTypeCodeIndex(type_code code);
void TypeCodeToString(type_code code, char* str);


#endif
