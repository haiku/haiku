/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "StandardMetaData.h"


StandardMetaData::StandardMetaData()
{
	fCreateTimestamp = 0;
	fDataModifiedTimestamp = 0;
}


/*static*/ BDateTime
StandardMetaData::_CreateDateTime(uint64_t millisSinceEpoc)
{
	time_t secondsSinceEpoc = (millisSinceEpoc / 1000);
	BDateTime result;
	result.SetTime_t(secondsSinceEpoc);
	return result;
}


uint64_t
StandardMetaData::GetCreateTimestamp() const
{
	return fCreateTimestamp;
}


BDateTime
StandardMetaData::GetCreateTimestampAsDateTime() const
{
	return _CreateDateTime(GetCreateTimestamp());
}


void
StandardMetaData::SetCreateTimestamp(uint64_t value)
{
	fCreateTimestamp = value;
}


uint64_t
StandardMetaData::GetDataModifiedTimestamp() const
{
	return fDataModifiedTimestamp;
}


void
StandardMetaData::SetDataModifiedTimestamp(uint64_t value)
{
	fDataModifiedTimestamp = value;
}


BDateTime
StandardMetaData::GetDataModifiedTimestampAsDateTime() const
{
	return _CreateDateTime(GetDataModifiedTimestamp());
}


bool
StandardMetaData::IsPopulated() const
{
	return fCreateTimestamp != 0 && fDataModifiedTimestamp != 0;
}
