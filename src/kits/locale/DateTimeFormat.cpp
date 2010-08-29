/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */

#include <DateTimeFormat.h>


// default constructor
BDateTimeFormat::BDateTimeFormat()
	: BFormat()
{
}


// copy constructor
BDateTimeFormat::BDateTimeFormat(const BDateTimeFormat &other)
	: BFormat(other)
{
}


// destructor
BDateTimeFormat::~BDateTimeFormat()
{
}


// Format
status_t
BDateTimeFormat::Format(bigtime_t value, BString* buffer) const
{
	return B_ERROR;
}
