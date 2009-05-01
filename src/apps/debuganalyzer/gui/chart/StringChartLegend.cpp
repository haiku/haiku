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


StringChartLegendRenderer::StringChartLegendRenderer()
	:
	fFont(*be_plain_font)
{
	_Init();
}


StringChartLegendRenderer::StringChartLegendRenderer(const BFont& font)
	:
	fFont(font)
{
	_Init();
}


void
StringChartLegendRenderer::GetMinimumLegendSpacing(BView* view,
	float* horizontal, float* vertical)
{
	*horizontal = 2 * fEmWidth;
	*vertical = fFontHeight / 2;
}


BSize
StringChartLegendRenderer::LegendSize(ChartLegend* _legend, BView* view)
{
	StringChartLegend* legend = dynamic_cast<StringChartLegend*>(_legend);
	if (legend == NULL)
		return BSize();

	float width = ceilf(fFont.StringWidth(legend->String()));
	return BSize(width, fFontHeight);
}


void
StringChartLegendRenderer::RenderLegend(ChartLegend* _legend, BView* view,
	BPoint point)
{
	StringChartLegend* legend = dynamic_cast<StringChartLegend*>(_legend);
	if (legend == NULL)
		return;

	point.y += ceil(fFontAscent);

	view->SetFont(&fFont);
	view->SetHighColor(rgb_color().set_to(0, 0, 0, 255));
	view->DrawString(legend->String(), point);
}


void
StringChartLegendRenderer::_Init()
{
	font_height fh;
	fFont.GetHeight(&fh);
	fFontAscent = ceilf(fh.ascent);
	fFontHeight = fFontAscent + ceilf(fh.descent);
	fEmWidth = ceilf(fFont.StringWidth("m", 1));
}
