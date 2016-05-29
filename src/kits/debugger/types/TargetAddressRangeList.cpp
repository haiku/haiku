/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TargetAddressRangeList.h"

#include <algorithm>


TargetAddressRangeList::TargetAddressRangeList()
{
}


TargetAddressRangeList::TargetAddressRangeList(const TargetAddressRange& range)
{
	AddRange(range);
}


TargetAddressRangeList::TargetAddressRangeList(
	const TargetAddressRangeList& other)
	:
	fRanges(other.fRanges)
{
}


void
TargetAddressRangeList::Clear()
{
	fRanges.Clear();
}


bool
TargetAddressRangeList::AddRange(const TargetAddressRange& range)
{
	return fRanges.Add(range);
}


int32
TargetAddressRangeList::CountRanges() const
{
	return fRanges.Size();
}


TargetAddressRange
TargetAddressRangeList::RangeAt(int32 index) const
{
	return index >= 0 && index < fRanges.Size()
		? fRanges[index] : TargetAddressRange();
}


target_addr_t
TargetAddressRangeList::LowestAddress() const
{
	int32 count = fRanges.Size();
	if (count == 0)
		return 0;

	target_addr_t lowest = fRanges[0].Start();
	for (int32 i = 0; i < count; i++)
		lowest = std::min(lowest, fRanges[i].Start());

	return lowest;
}


TargetAddressRange
TargetAddressRangeList::CoveringRange() const
{
	TargetAddressRange range;
	int32 count = fRanges.Size();
	for (int32 i = 0; i < count; i++)
		range |= fRanges[i];

	return range;
}


bool
TargetAddressRangeList::Contains(target_addr_t address) const
{
	int32 count = fRanges.Size();
	for (int32 i = 0; i < count; i++) {
		if (fRanges[i].Contains(address))
			return true;
	}

	return false;
}


TargetAddressRangeList&
TargetAddressRangeList::operator=(const TargetAddressRangeList& other)
{
	fRanges = other.fRanges;
	return *this;
}
