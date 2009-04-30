/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "chart/DefaultChartAxisLegendSource.h"

#include <math.h>
#include <stdio.h>

#include "chart/ChartDataRange.h"
#include "chart/StringChartLegend.h"


int32
DefaultChartAxisLegendSource::GetAxisLegends(const ChartDataRange& range,
	ChartLegend** legends, double* values, int32 maxLegends)
{
// TODO: Also support scientific notation! Otherwise the numbers can get really
// long.
	double start = range.min;
	double end = range.max;
	double rangeSpan = end - start;

	if (rangeSpan >= maxLegends / 2) {
		// We only need to consider the integer part.

		// find an interval so that we get maxLegends / 2 to maxLegends legends
		double baseInterval = 1;
		double relativeFactor = 1;
		while (rangeSpan / baseInterval / relativeFactor >= maxLegends) {
			if (relativeFactor == 1) {
				relativeFactor = 2;
			} else if (relativeFactor == 2) {
				relativeFactor = 5;
			} else if (relativeFactor == 5) {
				baseInterval *= 10;
				relativeFactor = 1;
			}
		}

		// generate the legends
		int32 count = 0;
		double interval = baseInterval * relativeFactor;
		double value = ceil(start / interval) * interval;
		for (; value <= end; value += interval) {
			char buffer[128];
			snprintf(buffer, sizeof(buffer), "%.0f", value);
			StringChartLegend* legend
				= new(std::nothrow) StringChartLegend(buffer, 1);
			if (legend == NULL)
				return count;
	
			legends[count] = legend;
			values[count++] = value;
		}

		return count;
	}

	// The range is so small that we need a fraction interval.

	// First find out how many places after the decimal point we need.
	int positions = 0;
	double factor = 1;
	while (rangeSpan * factor < maxLegends / 2) {
		factor *= 10;
		positions++;
	}

	double relativeFactor = 1;
	if (rangeSpan * factor / relativeFactor >= maxLegends) {
		relativeFactor = 2;
		if (rangeSpan * factor / relativeFactor >= maxLegends)
			relativeFactor = 5;
	}

	// generate the legends
	int32 count = 0;
	double interval = relativeFactor / factor;
	double shiftedValue = ceil(start / interval);
	for (; shiftedValue * interval <= end; shiftedValue++) {
		double value = shiftedValue * interval;
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "%.*f", positions, value);
		StringChartLegend* legend
			= new(std::nothrow) StringChartLegend(buffer, 1);
		if (legend == NULL)
			return count;

		legends[count] = legend;
		values[count++] = value;
	}

	return count;
}
