/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "DefaultTypes.h"

#include <ByteOrder.h>


BString
toStringBOOL(const void* data)
{
	if (*(bool*)data)
		return "✔ true";
	else
		return "✖ false";
}


BString
toStringBYTE(const void* data)
{
	return (BString() << *(int8*)data);
}


BString
toStringSHRT(const void* data)
{
	return (BString() << *(int16*)data);
}


BString
toStringLONG(const  void* data)
{
	return (BString() << *(int32*)data);
}


BString
toStringLLNG(const void* data)
{
	return (BString() << *(int64*)data);
}


BString
toStringUBYT(const void* data)
{
	return (BString() << *(uint8*)data);
}


BString
toStringUSHT(const void* data)
{
	return (BString() << *(uint16*)data);
}


BString
toStringULNG(const void* data)
{
	return (BString() << *(uint32*)data);
}


BString
toStringULLG(const void* data)
{
	return (BString() << *(uint64*)data);
}


BString
toStringRAWT(const void* data)
{
	return "[Raw Data]";
}


int32
FindTypeCodeIndex(type_code code)
{
	for (int32 i = 0; kDefaultTypes[i].type != NULL; i++)
		if (kDefaultTypes[i].typeCode == code)
			return i;

	return -1;
}


void
TypeCodeToString(type_code code, char* str)
{
	*(type_code*)str = B_HOST_TO_BENDIAN_INT32(code);
	str[4] = '\0';
}
