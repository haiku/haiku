/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LEGEND_CHART_AXIS_H
#define LEGEND_CHART_AXIS_H

#include <ObjectList.h>

#include "chart/ChartAxis.h"
#include "chart/ChartDataRange.h"


class ChartAxisLegendSource;
class ChartLegendRenderer;


class LegendChartAxis : public ChartAxis {
public:
								LegendChartAxis(
									ChartAxisLegendSource* legendSource,
									ChartLegendRenderer* legendRenderer);
	virtual						~LegendChartAxis();

	virtual	void				SetLocation(ChartAxisLocation location);
	virtual	void				SetRange(const ChartDataRange& range);
	virtual	void				SetFrame(BRect frame);
	virtual	BSize				PreferredSize(BView* view, BSize maxSize);
	virtual	void				Render(BView* view, BRect updateRect);

private:
			struct LegendInfo;
			typedef BObjectList<LegendInfo> LegendList;
private:
			void				_InvalidateLayout();
			bool				_ValidateLayout(BView* view);
			int32				_EstimateMaxLegendCount(BView* view, BSize size,
									float* _hSpacing, float* _vSpacing);
	inline	float				_LegendPosition(double value, float legendSize,
									float totalSize, double scale);
	inline	void				_FilterLegends(int32 totalSize, int32 spacing,
									float BSize::* sizeField);

private:
			ChartAxisLegendSource* fLegendSource;
			ChartLegendRenderer* fLegendRenderer;
			ChartAxisLocation	fLocation;
			ChartDataRange		fRange;
			BRect				fFrame;
			LegendList			fLegends;
			float				fHorizontalSpacing;
			float				fVerticalSpacing;
			bool				fLayoutValid;
};


#endif	// LEGEND_CHART_AXIS_H
