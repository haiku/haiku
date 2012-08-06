/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TargetAddressTableColumn.h"

#include <stdio.h>


TargetAddressTableColumn::TargetAddressTableColumn(int32 modelIndex,
	const char* title, float width, float minWidth, float maxWidth,
	uint32 truncate, alignment align)
	:
	StringTableColumn(modelIndex, title, width, minWidth, maxWidth, truncate,
		align)
{
}


BField*
TargetAddressTableColumn::PrepareField(const BVariant& value) const
{
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%#" B_PRIx64, value.ToUInt64());

	return StringTableColumn::PrepareField(
		BVariant(buffer, B_VARIANT_DONT_COPY_DATA));
}


int
TargetAddressTableColumn::CompareValues(const BVariant& a, const BVariant& b)
{
	uint64 valueA = a.ToUInt64();
	uint64 valueB = b.ToUInt64();
	return valueA < valueB ? -1 : (valueA == valueB ? 0 : 1);
}
