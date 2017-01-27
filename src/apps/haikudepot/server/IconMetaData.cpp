/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "IconMetaData.h"


uint64_t
IconMetaData::GetCreateTimestamp()
{
	return fCreateTimestamp;
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
