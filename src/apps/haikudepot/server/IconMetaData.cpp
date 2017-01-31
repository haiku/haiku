/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "IconMetaData.h"


BDateTime
IconMetaData::_CreateDateTime(uint64_t millisSinceEpoc)
{
	time_t secondsSinceEpoc = (millisSinceEpoc / 1000);
	BDateTime result;
	result.SetTime_t(secondsSinceEpoc);
	return result;
}


uint64_t
IconMetaData::GetCreateTimestamp()
{
	return fCreateTimestamp;
}


BDateTime
IconMetaData::GetCreateTimestampAsDateTime()
{
	return _CreateDateTime(GetCreateTimestamp());
}


void
IconMetaData::SetCreateTimestamp(uint64_t value)
{
	fCreateTimestamp = value;
}


uint64_t
IconMetaData::GetDataModifiedTimestamp()
{
	return fDataModifiedTimestamp;
}


void
IconMetaData::SetDataModifiedTimestamp(uint64_t value)
{
	fDataModifiedTimestamp = value;
}

BDateTime
IconMetaData::GetDataModifiedTimestampAsDateTime()
{
	return _CreateDateTime(GetDataModifiedTimestamp());
}

