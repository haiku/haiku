/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "RangeList.h"

#include <AutoDeleter.h>


RangeList::RangeList()
	: BObjectList<Range>(20, true)
{
}


RangeList::~RangeList()
{
}


status_t
RangeList::AddRange(int32 lowValue, int32 highValue)
{
	if (lowValue > highValue)
		return B_BAD_VALUE;

	int32 i = 0;
	if (CountItems() != 0) {
		for (; i < CountItems(); i++) {
			Range* range = ItemAt(i);
			if (lowValue < range->lowerBound) {
				if (highValue < range->lowerBound) {
					// the new range is completely below the bounds
					// of the ranges we currently contain,
					// insert it here.
					break;
				} else if (highValue <= range->upperBound) {
					// the new range partly overlaps the lower
					// current range
					range->lowerBound = lowValue;
					return B_OK;
				} else {
					// the new range completely encompasses
					// the current range
					range->lowerBound = lowValue;
					range->upperBound = highValue;
					_CollapseOverlappingRanges(i +1, highValue);
				}

			} else if (lowValue < range->upperBound) {
				if (highValue <= range->upperBound) {
					// the requested range is already completely contained
					// within our existing range list
					return B_OK;
				} else {
					range->upperBound = highValue;
					_CollapseOverlappingRanges(i + 1, highValue);
					return B_OK;
				}
			}
		}
	}

	Range* range = new(std::nothrow) Range(lowValue, highValue);
	if (range == NULL)
		return B_NO_MEMORY;

	BPrivate::ObjectDeleter<Range> rangeDeleter(range);
	if (!AddItem(range, i))
		return B_NO_MEMORY;

	rangeDeleter.Detach();
	return B_OK;
}


status_t
RangeList::AddRange(const Range& range)
{
	return AddRange(range.lowerBound, range.upperBound);
}


void
RangeList::RemoveRangeAt(int32 index)
{
	if (index < 0 || index >= CountItems())
		return;

	RemoveItem(ItemAt(index));
}


bool
RangeList::Contains(int32 value) const
{
	for (int32 i = 0; i < CountItems(); i++) {
		const Range* range = ItemAt(i);
		if (value < range->lowerBound || value > range->upperBound)
			break;
		else if (value >= range->lowerBound && value <= range->upperBound)
			return true;
	}

	return false;
}


int32
RangeList::CountRanges() const
{
	return CountItems();
}


const Range*
RangeList::RangeAt(int32 index) const
{
	return ItemAt(index);
}


void
RangeList::_CollapseOverlappingRanges(int32 startIndex, int32 highValue)
{
	for (int32 i = startIndex; i < CountItems();) {
		// check if it also overlaps any of the following
		// ranges.
		Range* nextRange = ItemAt(i);
		if (nextRange->lowerBound > highValue)
			return;
		else if (nextRange->upperBound < highValue) {
			RemoveItem(nextRange);
			continue;
		} else {
			nextRange->lowerBound = highValue + 1;
			return;
		}
	}
}
