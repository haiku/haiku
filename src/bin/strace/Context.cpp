/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Hugo Santos <hugosantos@gmail.com>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include "Context.h"


string
Context::FormatSigned(int64 value, const char *type) const
{
	char modifier[16], tmp[32];

	if (fDecimal)
		snprintf(modifier, sizeof(modifier), "%%%si", type);
	else
		snprintf(modifier, sizeof(modifier), "0x%%%sx", type);

	snprintf(tmp, sizeof(tmp), modifier, value);
	return tmp;
}

string
Context::FormatUnsigned(uint64 value) const
{
	char tmp[32];
	snprintf(tmp, sizeof(tmp), fDecimal ? "%llu" : "0x%llx", value);
	return tmp;
}

string
Context::FormatFlags(uint64 value) const
{
	char tmp[32];
	snprintf(tmp, sizeof(tmp), "0x%llx", value);
	return tmp;
}
