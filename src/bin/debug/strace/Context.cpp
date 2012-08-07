/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Hugo Santos <hugosantos@gmail.com>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include "Context.h"

#include <stdio.h>
#include <string.h>

string
Context::FormatSigned(int64 value, int bytes) const
{
	char tmp[32];

	// decimal

	if (fDecimal) {
		snprintf(tmp, sizeof(tmp), "%" B_PRId64, value);
		return tmp;
	}

	// hex

	snprintf(tmp, sizeof(tmp), "0x%" B_PRIx64, value);

	// Negative numbers are expanded when being converted to int64. Hence
	// we skip all but the last 2 * bytes hex digits to retain the original
	// type's width.
	int len = strlen(tmp);
	int offset = len - min_c(len, bytes * 2);

	// use the existing "0x" prefix or prepend it again
	if (offset <= 2) {
		offset = 0;
	} else {
		tmp[--offset] = 'x';
		tmp[--offset] = '0';
	}

	return tmp + offset;
}

string
Context::FormatUnsigned(uint64 value) const
{
	char tmp[32];
	snprintf(tmp, sizeof(tmp), fDecimal ? "%" B_PRIu64 : "0x%" B_PRIx64, value);
	return tmp;
}

string
Context::FormatFlags(uint64 value) const
{
	char tmp[32];
	snprintf(tmp, sizeof(tmp), "0x%" B_PRIx64, value);
	return tmp;
}

string
Context::FormatPointer(const void *address) const
{
	char buffer[32];
	sprintf(buffer, "%p", address);
	return buffer;
}
