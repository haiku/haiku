/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "MimeSnifferAddon.h"

// constructor
BMimeSnifferAddon::BMimeSnifferAddon()
{
}

// destructor
BMimeSnifferAddon::~BMimeSnifferAddon()
{
}

// MinimalBufferSize
size_t
BMimeSnifferAddon::MinimalBufferSize()
{
	return 0;
}

// GuessMimeType
float
BMimeSnifferAddon::GuessMimeType(const char* fileName, BMimeType* type)
{
	return -1;
}

// GuessMimeType
float
BMimeSnifferAddon::GuessMimeType(BFile* file, const void* buffer, int32 length,
	BMimeType* type)
{
	return -1;
}
