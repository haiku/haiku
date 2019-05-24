/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TARGET_ADDRESS_RANGE_H
#define TARGET_ADDRESS_RANGE_H

#include <algorithm>

#include "Types.h"


class TargetAddressRange {
public:
	TargetAddressRange()
		:
		fStart(0),
		fSize(0)
	{
	}

	TargetAddressRange(target_addr_t start, target_size_t size)
		:
		fStart(start),
		fSize(size)
	{
	}

	bool operator==(const TargetAddressRange& other) const
	{
		return fStart == other.fStart && fSize == other.fSize;
	}

	bool operator!=(const TargetAddressRange& other) const
	{
		return !(*this == other);
	}

	target_addr_t Start() const
	{
		return fStart;
	}

	target_size_t Size() const
	{
		return fSize;
	}

	target_addr_t End() const
	{
		return fStart + fSize;
	}

	bool Contains(target_addr_t address) const
	{
		return address >= Start() && address < End();
	}

	bool Contains(const TargetAddressRange& other) const
	{
		return Start() <= other.Start() && End() >= other.End();
	}

	bool IntersectsWith(const TargetAddressRange& other) const
	{
		return Contains(other.Start()) || other.Contains(Start());
	}

	TargetAddressRange& operator|=(const TargetAddressRange& other)
	{
		if (fSize == 0)
			return *this = other;

		if (other.fSize > 0) {
			target_addr_t end = std::max(End(), other.End());
			fStart = std::min(fStart, other.fStart);
			fSize = end - fStart;
		}

		return *this;
	}

private:
	target_addr_t	fStart;
	target_size_t	fSize;
};


#endif	// TARGET_ADDRESS_RANGE_H
