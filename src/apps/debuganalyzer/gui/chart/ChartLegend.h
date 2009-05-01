/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHART_LEGEND_H
#define CHART_LEGEND_H

#include <Point.h>
#include <Size.h>


class BView;
class ChartDataRange;


class ChartLegend {
public:
								ChartLegend(int32 level = 0);
									// A lower level means more likely to be
									// shown. <= 0 is mandatory.
	virtual						~ChartLegend();

			int32				Level()		{ return fLevel; }

private:
			int32				fLevel;
};


class ChartLegendRenderer {
public:
	virtual						~ChartLegendRenderer();

	virtual	void				GetMinimumLegendSpacing(BView* view,
									float* horizontal, float* vertical) = 0;

	virtual	BSize				LegendSize(ChartLegend* legend,
									BView* view) = 0;
	virtual	void				RenderLegend(ChartLegend* legend, BView* view,
									BPoint point) = 0;
};


#endif	// CHART_LEGEND_H
