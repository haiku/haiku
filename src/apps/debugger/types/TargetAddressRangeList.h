/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TARGET_ADDRESS_RANGE_LIST_H
#define TARGET_ADDRESS_RANGE_LIST_H


#include <Array.h>
#include <Referenceable.h>

#include "TargetAddressRange.h"


class TargetAddressRangeList : public BReferenceable {
public:
								TargetAddressRangeList();
								TargetAddressRangeList(
									const TargetAddressRange& range);
								TargetAddressRangeList(
									const TargetAddressRangeList& other);

			void				Clear();
			bool				AddRange(const TargetAddressRange& range);
	inline	bool				AddRange(target_addr_t start,
									target_size_t size);

			int32				CountRanges() const;
			TargetAddressRange	RangeAt(int32 index) const;

			target_addr_t		LowestAddress() const;
			TargetAddressRange	CoveringRange() const;

			bool				Contains(target_addr_t address) const;

			TargetAddressRangeList& operator=(
									const TargetAddressRangeList& other);

private:
	typedef Array<TargetAddressRange> RangeArray;

private:
			RangeArray			fRanges;
};


bool
TargetAddressRangeList::AddRange(target_addr_t start, target_size_t size)
{
	return AddRange(TargetAddressRange(start, size));
}


#endif	// TARGET_ADDRESS_RANGE_LIST_H
