/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHART_AXIS_LEGEND_SOURCE_H
#define CHART_AXIS_LEGEND_SOURCE_H

#include <SupportDefs.h>


class ChartDataRange;
class ChartLegend;


class ChartAxisLegendSource {
public:
	virtual						~ChartAxisLegendSource();

	virtual	int32				GetAxisLegends(const ChartDataRange& range,
									ChartLegend** legends, double* values,
									int32 maxLegends) = 0;
};


#endif	// CHART_AXIS_LEGEND_SOURCE_H
