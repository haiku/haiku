/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "chart/NanotimeChartAxisLegendSource.h"

#include <stdio.h>

#include "chart/ChartDataRange.h"
#include "chart/StringChartLegend.h"
#include "util/TimeUtils.h"


int32
NanotimeChartAxisLegendSource::GetAxisLegends(const ChartDataRange& range,
	ChartLegend** legends, double* values, int32 maxLegends)
{
	// interpret range as time range
	nanotime_t startTime = (nanotime_t)range.min;
	nanotime_t endTime = (nanotime_t)range.max;
// TODO: Handle sub-nanosecs ranges!
	if (startTime >= endTime)
		return 0;

	nanotime_t positionFactors[4];
	positionFactors[3] = 1;
	positionFactors[2] = 1000000000;
	positionFactors[1] = positionFactors[2] * 60;
	positionFactors[0] = positionFactors[1] * 60;

	// find the main position (h, m, s, us) we want to play with
	int32 position = 0;
	nanotime_t rangeTime = endTime - startTime;
	while (rangeTime / positionFactors[position] + 1 < maxLegends / 2
			&& position < 3) {
		position++;
	}

	// adjust the factor so that we get maxLegends / 2 to maxLegends legends
	nanotime_t baseInterval = positionFactors[position];
	nanotime_t relativeFactor = 1;
	while (rangeTime / (baseInterval * relativeFactor) >= maxLegends) {
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
	nanotime_t interval = baseInterval * relativeFactor;
	nanotime_t time = (startTime + interval - 1) / interval * interval;
	for (; time <= endTime; time += interval) {
		decomposed_nanotime decomposed;
		decompose_time(time, decomposed);
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "%02lld:%02d:%02d.%09d",
			decomposed.hours, decomposed.minutes, decomposed.seconds,
			decomposed.nanos);
// TODO: Drop superfluous nanoseconds digits, or even nanoseconds and seconds
// completely.

		StringChartLegend* legend
			= new(std::nothrow) StringChartLegend(buffer, 1);
		if (legend == NULL)
			return count;

		legends[count] = legend;
		values[count++] = (double)time;
	}

	return count;
}
