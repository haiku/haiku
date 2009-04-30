/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHART_AXIS_H
#define CHART_AXIS_H

#include <Rect.h>

#include "chart/ChartDefs.h"


class BView;
class ChartDataRange;


class ChartAxis {
public:
	virtual						~ChartAxis();

	virtual	void				SetLocation(ChartAxisLocation location) = 0;
	virtual	void				SetRange(const ChartDataRange& range) = 0;
	virtual	void				SetFrame(BRect frame) = 0;
	virtual	BSize				PreferredSize(BView* view, BSize maxSize) = 0;
	virtual	void				Render(BView* view, BRect updateRect) = 0;
};


#endif	// CHART_AXIS_H
