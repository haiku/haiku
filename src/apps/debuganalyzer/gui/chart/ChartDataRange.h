/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHART_DATA_RANGE_H
#define CHART_DATA_RANGE_H

#include <algorithm>


class ChartDataRange {
public:
	double	min;
	double	max;

	ChartDataRange()
		:
		min(0),
		max(0)
	{
	}

	ChartDataRange(double min, double max)
		:
		min(min),
		max(max)
	{
	}

	ChartDataRange(const ChartDataRange& other)
		:
		min(other.min),
		max(other.max)
	{
	}

	bool IsValid() const
	{
		
		return min < max;
	}

	double Size() const
	{
		return max - min;
	}

	ChartDataRange& Extend(const ChartDataRange& other)
	{
		min = std::min(min, other.min);
		max = std::max(max, other.max);
		return *this;
	}

	ChartDataRange& OffsetBy(double offset)
	{
		min += offset;
		max += offset;
		return *this;
	}

	ChartDataRange& OffsetTo(double newMin)
	{
		max += newMin - min;
		min = newMin;
		return *this;
	}

	ChartDataRange& operator=(const ChartDataRange& other)
	{
		min = other.min;
		max = other.max;
		return *this;
	}

	bool operator==(const ChartDataRange& other) const
	{
		return min == other.min && max == other.max;
	}

	bool operator!=(const ChartDataRange& other) const
	{
		return !(*this == other);
	}
};


#endif	// CHART_DATA_RANGE_H
