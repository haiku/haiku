/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */

#include <DateFormat.h>


// default constructor
BDateFormat::BDateFormat()
	: BDateTimeFormat()
{
}

// copy constructor
BDateFormat::BDateFormat(const BDateFormat &other)
	: BDateTimeFormat(other)
{
}

// destructor
BDateFormat::~BDateFormat()
{
}

// Format
status_t
BDateFormat::Format(bigtime_t value, BString* buffer) const
{
	return B_ERROR;
}
