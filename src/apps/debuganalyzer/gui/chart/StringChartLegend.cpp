/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "chart/StringChartLegend.h"

#include <Font.h>
#include <View.h>


// #pragma mark - StringChartLegend


StringChartLegend::StringChartLegend(const char* string, int32 level)
	:
	ChartLegend(level),
	fString(string)
{
}


// #pragma mark - StringChartLegendRenderer


void
StringChartLegendRenderer::GetMinimumLegendSpacing(BView* view,
	float* horizontal, float* vertical)
{
// TODO: Implement for real!
	*horizontal = 40;
	*vertical = 20;
}


BSize
StringChartLegendRenderer::MaximumLegendSize(BView* view)
{
// TODO: Implement for real!
	return BSize(100, 20);
}


BSize
StringChartLegendRenderer::LegendSize(ChartLegend* _legend, BView* view)
{
	StringChartLegend* legend = dynamic_cast<StringChartLegend*>(_legend);
	if (legend == NULL)
		return BSize();

// TODO: It's a bit expensive to do that for each legend!
// TODO: Have a font per renderer?
	BFont font;
	view->GetFont(&font);
	font_height fh;
	font.GetHeight(&fh);
	float fontHeight = ceilf(fh.ascent) + ceilf(fh.descent);
	float width = ceilf(font.StringWidth(legend->String()));

	return BSize(width, fontHeight);
}


void
StringChartLegendRenderer::RenderLegend(ChartLegend* _legend, BView* view,
	BPoint point)
{
	StringChartLegend* legend = dynamic_cast<StringChartLegend*>(_legend);
	if (legend == NULL)
		return;

	BFont font;
	view->GetFont(&font);
	font_height fh;
	font.GetHeight(&fh);
	point.y += fh.ascent;

	view->SetHighColor(rgb_color().set_to(0, 0, 0, 255));
	view->DrawString(legend->String(), point);
}
