/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef RANGE_LIST_H
#define RANGE_LIST_H


#include <ObjectList.h>
#include <SupportDefs.h>


struct Range {
	int32 lowerBound;
	int32 upperBound;

	Range()
		:
		lowerBound(-1),
		upperBound(-1)
	{
	}

	Range(int32 lowValue, int32 highValue)
		:
		lowerBound(lowValue),
		upperBound(highValue)
	{
	}
};


class RangeList : private BObjectList<Range>
{
public:
							RangeList();
	virtual					~RangeList();


			status_t		AddRange(int32 lowValue, int32 highValue);
			status_t		AddRange(const Range& range);

			void			RemoveRangeAt(int32 index);

			int32			CountRanges() const;
			const Range*	RangeAt(int32 index) const;

			bool			Contains(int32 value) const;

private:
			void			_CollapseOverlappingRanges(int32 startIndex,
								int32 highValue);
};

#endif // RANGE_LIST_H
