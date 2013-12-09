/*
 * Copyright 2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <UrlResult.h>


BUrlResult::~BUrlResult()
{
}


void
BUrlResult::SetContentType(BString contentType)
{
	fContentType = contentType;
}


void
BUrlResult::SetLength(size_t length)
{
	fLength = length;
}


BString
BUrlResult::ContentType() const
{
	return fContentType;
}


size_t
BUrlResult::Length() const
{
	return fLength;
}
