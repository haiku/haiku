/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEFAULT_CHART_AXIS_LEGEND_SOURCE_H
#define DEFAULT_CHART_AXIS_LEGEND_SOURCE_H

#include "chart/ChartAxisLegendSource.h"


class DefaultChartAxisLegendSource : public ChartAxisLegendSource {
public:
	virtual	int32				GetAxisLegends(const ChartDataRange& range,
									ChartLegend** legends, double* values,
									int32 maxLegends);
};


#endif	// DEFAULT_CHART_AXIS_LEGEND_SOURCE_H
