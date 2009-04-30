/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHART_DATA_SOURCE_H
#define CHART_DATA_SOURCE_H

#include <SupportDefs.h>

#include "chart/ChartDataRange.h"


class ChartDataSource {
public:
	virtual						~ChartDataSource();

	virtual	ChartDataRange		Domain() const = 0;
	virtual	ChartDataRange		Range() const = 0;

	virtual	void				GetSamples(double start, double end,
									double* samples, int32 count) = 0;
};

#endif	// CHART_DATA_SOURCE_H
