/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "DefaultTypes.h"

#include <ByteOrder.h>


int32
ResourceType::FindIndex(type_code code)
{
	for (int32 i = 0; kDefaultTypes[i].type != NULL; i++)
		if (StringToCode(kDefaultTypes[i].code) == code)
			return i;

	return -1;
}


int32
ResourceType::FindIndex(const char* type)
{
	for (int32 i = 0; kDefaultTypes[i].type != NULL; i++)
		if (strcmp(kDefaultTypes[i].type, type) == 0)
			return i;

	return -1;
}


void
ResourceType::CodeToString(type_code code, char* str)
{
	*(type_code*)str = B_HOST_TO_BENDIAN_INT32(code);
	str[4] = '\0';
}


type_code
ResourceType::StringToCode(const char* code)
{
	// TODO: Code may be in other formats, too! Ex.: 0x4C4F4E47
	return B_BENDIAN_TO_HOST_INT32(*(int32*)code);
}
