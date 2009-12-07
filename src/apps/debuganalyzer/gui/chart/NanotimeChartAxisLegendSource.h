/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NANOTIME_CHART_AXIS_LEGEND_SOURCE_H
#define NANOTIME_CHART_AXIS_LEGEND_SOURCE_H


#include "chart/ChartAxisLegendSource.h"


class NanotimeChartAxisLegendSource : public ChartAxisLegendSource {
public:
	virtual	int32				GetAxisLegends(const ChartDataRange& range,
									ChartLegend** legends, double* values,
									int32 maxLegends);
};


#endif	// NANOTIME_CHART_AXIS_LEGEND_SOURCE_H
