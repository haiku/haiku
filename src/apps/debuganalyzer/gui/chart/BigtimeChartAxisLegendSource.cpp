/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "chart/BigtimeChartAxisLegendSource.h"

#include <stdio.h>

#include "chart/ChartDataRange.h"
#include "chart/StringChartLegend.h"


struct decomposed_time {
	bigtime_t	hours;
	int			minutes;
	int			seconds;
	int			micros;
};


static void
decompose_time(bigtime_t time, decomposed_time& decomposed)
{
	// microsecs
	decomposed.micros = time % 1000000;
	time /= 1000000;
	// secs
	decomposed.seconds = time % 60;
	time /= 60;
	// mins
	decomposed.minutes = time % 60;
	time /= 60;
	// hours
	decomposed.hours = time;
}


int32
BigtimeChartAxisLegendSource::GetAxisLegends(const ChartDataRange& range,
	ChartLegend** legends, double* values, int32 maxLegends)
{
	// interpret range as time range
	bigtime_t startTime = (bigtime_t)range.min;
	bigtime_t endTime = (bigtime_t)range.max;
printf("BigtimeChartAxisLegendSource::GetAxisLegends(): range: %lld - %lld\n", startTime, endTime);
// TODO: Handle sub-microsecs ranges!
	if (startTime >= endTime)
		return 0;

	bigtime_t positionFactors[4];
	positionFactors[3] = 1;
	positionFactors[2] = 1000000;
	positionFactors[1] = positionFactors[2] * 60;
	positionFactors[0] = positionFactors[1] * 60;

	// find the main position (h, m, s, us) we want to play with
	int32 position = 0;
	bigtime_t rangeTime = endTime - startTime;
	while (rangeTime / positionFactors[position] + 1 < maxLegends / 2
			&& position < 3) {
		position++;
	}

	// adjust the factor so that we get maxLegends / 2 to maxLegends legends
	bigtime_t baseInterval = positionFactors[position];
	bigtime_t relativeFactor = 1;
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
	bigtime_t interval = baseInterval * relativeFactor;
	bigtime_t time = (startTime + interval - 1) / interval * interval;
	for (; time <= endTime; time += interval) {
		decomposed_time decomposed;
		decompose_time(time, decomposed);
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "%02lld:%02d:%02d.%06d",
			decomposed.hours, decomposed.minutes, decomposed.seconds,
			decomposed.micros);
// TODO: Drop superfluous micro seconds digits, or even microseconds and seconds
// completely.

		StringChartLegend* legend = new(std::nothrow) StringChartLegend(buffer,
			1);
		if (legend == 0)
			return count;

		legends[count] = legend;
		values[count++] = (double)time;
	}

	return count;
}
