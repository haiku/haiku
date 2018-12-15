/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "StandardMetaData.h"


StandardMetaData::StandardMetaData()
{
	fCreateTimestamp = 0;
	fDataModifiedTimestamp = 0;
}


BDateTime
StandardMetaData::_CreateDateTime(uint64_t millisSinceEpoc)
{
	time_t secondsSinceEpoc = (millisSinceEpoc / 1000);
	BDateTime result;
	result.SetTime_t(secondsSinceEpoc);
	return result;
}


uint64_t
StandardMetaData::GetCreateTimestamp()
{
	return fCreateTimestamp;
}


BDateTime
StandardMetaData::GetCreateTimestampAsDateTime()
{
	return _CreateDateTime(GetCreateTimestamp());
}


void
StandardMetaData::SetCreateTimestamp(uint64_t value)
{
	fCreateTimestamp = value;
}


uint64_t
StandardMetaData::GetDataModifiedTimestamp()
{
	return fDataModifiedTimestamp;
}


void
StandardMetaData::SetDataModifiedTimestamp(uint64_t value)
{
	fDataModifiedTimestamp = value;
}


BDateTime
StandardMetaData::GetDataModifiedTimestampAsDateTime()
{
	return _CreateDateTime(GetDataModifiedTimestamp());
}


bool
StandardMetaData::IsPopulated()
{
	return fCreateTimestamp != 0 && fDataModifiedTimestamp != 0;
}
