/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _RANGE_ARRAY_H
#define _RANGE_ARRAY_H


#include <algorithm>

#include <Array.h>


namespace BPrivate {


template<typename Value>
struct Range {
	Value	offset;
	Value	size;

	Range(const Value& offset, const Value& size)
		:
		offset(offset),
		size(size)
	{
	}

	Value EndOffset() const
	{
		return offset + size;
	}
};


template<typename Value>
class RangeArray {
public:
			typedef Range<Value> RangeType;

public:
	inline						RangeArray();
	inline						RangeArray(const RangeArray<Value>& other);

	inline	int32				CountRanges() const
									{ return fRanges.Count(); }
	inline	bool				IsEmpty() const
									{ return fRanges.IsEmpty(); }
	inline	const RangeType*	Ranges() const
									{ return fRanges.Elements(); }

	inline	bool				AddRange(const RangeType& range);
			bool				AddRange(const Value& offset,
									const Value& size);
	inline	bool				RemoveRange(const RangeType& range);
			bool				RemoveRange(const Value& offset,
									const Value& size);
	inline	bool				RemoveRanges(int32 index, int32 count = 1);

	inline	void				Clear()			{ fRanges.Clear(); }
	inline	void				MakeEmpty()		{ fRanges.MakeEmpty(); }

	inline	bool				IntersectsWith(const RangeType& range) const;
			bool				IntersectsWith(const Value& offset,
									const Value& size) const;

			int32				InsertionIndex(const Value& offset) const;

	inline	const RangeType&	RangeAt(int32 index) const
									{ return fRanges.ElementAt(index); }

	inline	const RangeType&	operator[](int32 index) const
									{ return fRanges[index]; }

	inline	RangeArray<Value>&	operator=(const RangeArray<Value>& other);

private:
	inline	 RangeType&			_RangeAt(int32 index)
									{ return fRanges.ElementAt(index); }

private:
			Array<RangeType>	fRanges;
};


template<typename Value>
inline
RangeArray<Value>::RangeArray()
{
}


template<typename Value>
inline
RangeArray<Value>::RangeArray(const RangeArray<Value>& other)
	:
	fRanges(other.fRanges)
{
}


template<typename Value>
inline bool
RangeArray<Value>::AddRange(const RangeType& range)
{
	return AddRange(range.offset, range.size);
}


/*!	Adds the range starting at \a offset with size \a size.

	The range is automatically joined with ranges it adjoins or overlaps with.

	\return \c true, if the range was added successfully, \c false, if a memory
		allocation failed.
*/
template<typename Value>
bool
RangeArray<Value>::AddRange(const Value& offset, const Value& size)
{
	if (size == 0)
		return true;

	int32 index = InsertionIndex(offset);

	// determine the last range the range intersects with/adjoins
	Value endOffset = offset + size;
	int32 endIndex = index;
		// index after the last affected range
	int32 count = CountRanges();
	while (endIndex < count && RangeAt(endIndex).offset <= endOffset)
		endIndex++;

	// determine whether the range adjoins the previous range
	if (index > 0 && offset == RangeAt(index - 1).EndOffset())
		index--;

	if (index == endIndex) {
		// no joining possible -- just insert the new range
		return fRanges.Insert(RangeType(offset, size), index);
	}

	// Joining is possible. We'll adjust the first affected range and remove the
	// others (if any).
	RangeType& firstRange = _RangeAt(index);
	firstRange.offset = std::min(firstRange.offset, offset);
	firstRange.size = std::max(endOffset, RangeAt(endIndex - 1).EndOffset())
		- firstRange.offset;;

	if (index + 1 < endIndex)
		RemoveRanges(index + 1, endIndex - index - 1);

	return true;
}


template<typename Value>
inline bool
RangeArray<Value>::RemoveRange(const RangeType& range)
{
	return RemoveRange(range.offset, range.size);
}


/*!	Removes the range starting at \a offset with size \a size.

	Ranges that are completely covered by the given range are removed. Ranges
	that partially intersect with it are cut accordingly.

	If a range is split into two ranges by the operation, a memory allocation
	might be necessary and the method can fail, if the memory allocation fails.

	\return \c true, if the range was removed successfully, \c false, if a
		memory allocation failed.
*/
template<typename Value>
bool
RangeArray<Value>::RemoveRange(const Value& offset, const Value& size)
{
	if (size == 0)
		return true;

	int32 index = InsertionIndex(offset);

	// determine the last range the range intersects with
	Value endOffset = offset + size;
	int32 endIndex = index;
		// index after the last affected range
	int32 count = CountRanges();
	while (endIndex < count && RangeAt(endIndex).offset < endOffset)
		endIndex++;

	if (index == endIndex) {
		// the given range doesn't intersect with any exiting range
		return true;
	}

	// determine what ranges to remove completely
	RangeType& firstRange = _RangeAt(index);
	RangeType& lastRange = _RangeAt(endIndex - 1);

	int32 firstRemoveIndex = firstRange.offset >= offset ? index : index + 1;
	int32 endRemoveIndex = lastRange.EndOffset() <= endOffset
		? endIndex : endIndex - 1;

	if (firstRemoveIndex > endRemoveIndex) {
		// The range lies in the middle of an existing range. We need to split.
		RangeType newRange(endOffset, firstRange.EndOffset() - endOffset);
		if (!fRanges.Insert(newRange, index + 1))
			return false;

		firstRange.size = offset - firstRange.offset;
		return true;
	}

	// cut first and last range
	if (index < firstRemoveIndex)
		firstRange.size = offset - firstRange.offset;

	if (endOffset > endRemoveIndex) {
		lastRange.size = lastRange.EndOffset() - endOffset;
		lastRange.offset = endOffset;
	}

	// remove ranges
	if (firstRemoveIndex < endRemoveIndex)
		RemoveRanges(firstRemoveIndex, endRemoveIndex - firstRemoveIndex);

	return true;
}


template<typename Value>
inline bool
RangeArray<Value>::RemoveRanges(int32 index, int32 count)
{
	return fRanges.Remove(index, count);
}


template<typename Value>
inline bool
RangeArray<Value>::IntersectsWith(const RangeType& range) const
{
	return IntersectsWith(range.offset, range.size);
}


template<typename Value>
bool
RangeArray<Value>::IntersectsWith(const Value& offset, const Value& size) const
{
	int32 index = InsertionIndex(offset);
	return index < CountRanges() && RangeAt(index).offset < offset + size;
}


/*!	Returns the insertion index of a range starting at \a offset.

	If the array contains a range that starts at, includes (but doesn't end at)
	\a offset, the index of that range is returned. If \a offset lies in between
	two ranges or before the first range, the index of the range following
	\a offset is returned. Otherwise \c CountRanges() is returned.

	\return The insertion index for a range starting at \a offset.
*/
template<typename Value>
int32
RangeArray<Value>::InsertionIndex(const Value& offset) const
{
	// binary search the index
	int32 lower = 0;
	int32 upper = CountRanges();

	while (lower < upper) {
		int32 mid = (lower + upper) / 2;
		const RangeType& range = RangeAt(mid);
		if (offset >= range.EndOffset())
			lower = mid + 1;
		else
			upper = mid;
	}

	return lower;
}


template<typename Value>
inline RangeArray<Value>&
RangeArray<Value>::operator=(const RangeArray<Value>& other)
{
	fRanges = other.fRanges;
	return *this;
}


}	// namespace BPrivate


using BPrivate::Range;
using BPrivate::RangeArray;


#endif	// _RANGE_ARRAY_H
