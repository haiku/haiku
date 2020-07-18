/*
 * Copyright 2013-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <UrlResult.h>


using namespace BPrivate::Network;


BUrlResult::BUrlResult()
	:
	fContentType(),
	fLength(0)
{
}


BUrlResult::~BUrlResult()
{
}


void
BUrlResult::SetContentType(BString contentType)
{
	fContentType = contentType;
}


void
BUrlResult::SetLength(off_t length)
{
	fLength = length;
}


BString
BUrlResult::ContentType() const
{
	return fContentType;
}


off_t
BUrlResult::Length() const
{
	return fLength;
}
