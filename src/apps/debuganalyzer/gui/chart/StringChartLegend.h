/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_CHART_LEGEND_H
#define STRING_CHART_LEGEND_H

#include <Font.h>
#include <String.h>

#include "chart/ChartLegend.h"


class StringChartLegend : public ChartLegend {
public:
								StringChartLegend(const char* string,
									int32 level = 0);

			const char*			String() const	{ return fString.String(); }

private:
			BString				fString;
};


class StringChartLegendRenderer : public ChartLegendRenderer {
public:
								StringChartLegendRenderer();
								StringChartLegendRenderer(const BFont& font);

	virtual	void				GetMinimumLegendSpacing(BView* view,
									float* horizontal, float* vertical);

	virtual	BSize				LegendSize(ChartLegend* legend,
									BView* view);
	virtual	void				RenderLegend(ChartLegend* legend, BView* view,
									BPoint point);

private:
			void				_Init();

private:
			BFont				fFont;
			float				fFontAscent;
			float				fFontHeight;
			float				fEmWidth;
};


#endif	// STRING_CHART_LEGEND_H
