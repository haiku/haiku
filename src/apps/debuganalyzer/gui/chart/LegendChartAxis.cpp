/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "chart/LegendChartAxis.h"

#include <limits.h>
#include <stdio.h>

#include <algorithm>
#include <new>

#include <Font.h>
#include <View.h>

#include "chart/ChartLegend.h"
#include "chart/ChartAxisLegendSource.h"


static const int32 kChartRulerDistance = 2;
static const int32 kRulerSize = 3;
static const int32 kRulerMarkSize = 3;
static const int32 kRulerLegendDistance = 2;
static const int32 kChartLegendDistance
	= kChartRulerDistance + kRulerSize + kRulerMarkSize + kRulerLegendDistance;



struct LegendChartAxis::LegendInfo {
	ChartLegend*	legend;
	double			value;
	BSize			size;

	LegendInfo(ChartLegend* legend, double value, BSize size)
		:
		legend(legend),
		value(value),
		size(size)
	{
	}

	~LegendInfo()
	{
		delete legend;
	}
};


float
LegendChartAxis::_LegendPosition(double value, float legendSize,
	float totalSize, double scale)
{
	float position = (value - fRange.min) * scale - legendSize / 2;
	if (position + legendSize > totalSize)
		position = totalSize - legendSize;
	if (position < 0)
		position = 0;
	return position;
}


void
LegendChartAxis::_FilterLegends(int32 totalSize, int32 spacing,
	float BSize::* sizeField)
{
	// compute the min/max legend levels
	int32 legendCount = fLegends.CountItems();
	int32 minLevel = INT_MAX;
	int32 maxLevel = 0;

	for (int32 i = 0; i < legendCount; i++) {
		LegendInfo* info = fLegends.ItemAt(i);
		int32 level = info->legend->Level();
		if (level < minLevel)
			minLevel = level;
		if (level > maxLevel)
			maxLevel = level;
	}

	if (maxLevel <= 0)
		return;

	double rangeSize = fRange.max - fRange.min;
	if (rangeSize == 0)
		rangeSize = 1.0;
	double scale = (double)totalSize / rangeSize;

	// Filter out all higher level legends colliding with lower level or
	// preceeding same-level legends. We iterate backwards from the lower to
	// the higher levels
	for (int32 level = std::max(minLevel, 0L); level <= maxLevel;) {
		legendCount = fLegends.CountItems();

		// get the first legend position/end
		LegendInfo* info = fLegends.ItemAt(0);
		float position = _LegendPosition(info->value, info->size.*sizeField,
			(float)totalSize, scale);;

		int32 previousEnd = (int32)ceilf(position + info->size.*sizeField);
		int32 previousLevel = info->legend->Level();

		for (int32 i = 1; (info = fLegends.ItemAt(i)) != NULL; i++) {
			float position = _LegendPosition(info->value, info->size.*sizeField,
				(float)totalSize, scale);;

			if (position - spacing < previousEnd
				&& (previousLevel <= level
					|| info->legend->Level() <= level)
				&& std::max(previousLevel, info->legend->Level()) > 0) {
				// The item intersects with the previous one -- remove the
				// one at the higher level.
				if (info->legend->Level() >= previousLevel) {
					// This item is at the higher level -- remove it.
					delete fLegends.RemoveItemAt(i);
					i--;
					continue;
				}

				// The previous item is at the higher level -- remove it.
				delete fLegends.RemoveItemAt(i - 1);
				i--;
			}

			if (i == 0 && position < 0)
				position = 0;
			previousEnd = (int32)ceilf(position + info->size.*sizeField);
			previousLevel = info->legend->Level();
		}

		// repeat with the level as long as we've removed something
		if (legendCount == fLegends.CountItems())
			level++;
	}
}


LegendChartAxis::LegendChartAxis(ChartAxisLegendSource* legendSource,
	ChartLegendRenderer* legendRenderer)
	:
	fLegendSource(legendSource),
	fLegendRenderer(legendRenderer),
	fLocation(CHART_AXIS_BOTTOM),
	fRange(),
	fFrame(),
	fLegends(20, true),
	fHorizontalSpacing(20),
	fVerticalSpacing(10),
	fLayoutValid(false)
{
}


LegendChartAxis::~LegendChartAxis()
{
}


void
LegendChartAxis::SetLocation(ChartAxisLocation location)
{
	if (location != fLocation) {
		fLocation = location;
		_InvalidateLayout();
	}
}


void
LegendChartAxis::SetRange(const ChartDataRange& range)
{
	if (range != fRange) {
		fRange = range;
		_InvalidateLayout();
	}
}


void
LegendChartAxis::SetFrame(BRect frame)
{
	if (frame != fFrame) {
		fFrame = frame;
		_InvalidateLayout();
	}
}


BSize
LegendChartAxis::PreferredSize(BView* view, BSize maxSize)
{
	// estimate the maximum legend count we might need
	float hSpacing, vSpacing;
	int32 maxLegends = _EstimateMaxLegendCount(view, maxSize, &hSpacing,
		&vSpacing);
	BSize spacing(hSpacing, vSpacing);
	if (maxLegends < 4)
		maxLegends = 4;

	// get the legends
	ChartLegend* legends[maxLegends];
	double values[maxLegends];

	int32 legendCount = fLegendSource->GetAxisLegends(fRange, legends, values,
		maxLegends);

	// get the sizes, delete the legends, and compute the preferred size
	float BSize::* sizeField;
	float BSize::* otherSizeField;
	if (fLocation == CHART_AXIS_LEFT || fLocation == CHART_AXIS_RIGHT) {
		sizeField = &BSize::height;
		otherSizeField = &BSize::width;
	} else {
		sizeField = &BSize::width;
		otherSizeField = &BSize::height;
	}

	BSize preferredSize;

	for (int32 i = 0; i < legendCount; i++) {
		ChartLegend* legend = legends[i];
		BSize size = fLegendRenderer->LegendSize(legend, view);
		delete legend;

		if (size.*sizeField > preferredSize.*sizeField)
			preferredSize.*sizeField = size.*sizeField;
		if (size.*otherSizeField > preferredSize.*otherSizeField)
			preferredSize.*otherSizeField = size.*otherSizeField;
	}

	// Suppose we want to have at least 2 legends.
	preferredSize.*sizeField
		= ceilf(preferredSize.*sizeField * 2 + spacing.*sizeField);
	preferredSize.*otherSizeField += kChartLegendDistance;

	return preferredSize;
}


void
LegendChartAxis::Render(BView* view, BRect updateRect)
{
	if (!_ValidateLayout(view))
		return;

	float valueDirection;
	float rulerDirection;
	float BSize::* sizeField;
	float BSize::* otherSizeField;
	float BPoint::* pointField;
	float BPoint::* otherPointField;

	switch (fLocation) {
		case CHART_AXIS_LEFT:
		case CHART_AXIS_RIGHT:
			valueDirection = -1;
			rulerDirection = fLocation == CHART_AXIS_LEFT ? -1 : 1;
			sizeField = &BSize::height;
			otherSizeField = &BSize::width;
			pointField = &BPoint::y;
			otherPointField = &BPoint::x;
			break;
		case CHART_AXIS_TOP:
		case CHART_AXIS_BOTTOM:
			valueDirection = 1;
			rulerDirection = fLocation == CHART_AXIS_TOP ? -1 : 1;
			sizeField = &BSize::width;
			otherSizeField = &BSize::height;
			pointField = &BPoint::x;
			otherPointField = &BPoint::y;
			break;
		default:
			return;
	}

	float totalSize = floorf(fFrame.Size().*sizeField) + 1;
	double rangeSize = fRange.max - fRange.min;
	if (rangeSize == 0)
		rangeSize = 1.0;
	double scale = (double)totalSize / rangeSize;

	// draw the ruler
	float rulerStart = fFrame.LeftBottom().*pointField;
	float rulerChartClosest = rulerDirection == 1
		? fFrame.LeftTop().*otherPointField + kChartRulerDistance
		: fFrame.RightBottom().*otherPointField - kChartRulerDistance;
	float rulerEnd = fFrame.RightTop().*pointField;
	float rulerChartDistant = rulerChartClosest + rulerDirection * kRulerSize;

	rgb_color black = { 0, 0, 0, 255 };
	view->BeginLineArray(3 + fLegends.CountItems());
	BPoint first;
	first.*pointField = rulerStart;
	first.*otherPointField = rulerChartClosest;
	BPoint second = first;
	second.*otherPointField = rulerChartDistant;
	BPoint third = second;
	third.*pointField = rulerEnd;
	BPoint fourth = third;
	fourth.*otherPointField = rulerChartClosest;
	view->AddLine(first, second, black);
	view->AddLine(second, third, black);
	view->AddLine(third, fourth, black);

	// marks
	for (int32 i = 0; LegendInfo* info = fLegends.ItemAt(i); i++) {
		float position = (info->value - fRange.min) * scale;
		position = rulerStart + valueDirection * position;
		first.*pointField = position;
		first.*otherPointField = rulerChartDistant;
		second.*pointField = position;
		second.*otherPointField = rulerChartDistant
			+ rulerDirection * kRulerMarkSize;
		view->AddLine(first, second, black);
	}
	view->EndLineArray();

	// draw the legends
	float legendRulerClosest = rulerChartDistant
		+ rulerDirection * (kRulerMarkSize + kRulerLegendDistance);

	for (int32 i = 0; LegendInfo* info = fLegends.ItemAt(i); i++) {
		float position = _LegendPosition(info->value, info->size.*sizeField,
			(float)totalSize, scale);;

		first.*pointField = rulerStart
			+ (valueDirection == 1
				? position : -position - info->size.*sizeField);
		first.*otherPointField = rulerDirection == 1
			? legendRulerClosest
			: legendRulerClosest - info->size.*otherSizeField;

		fLegendRenderer->RenderLegend(info->legend, view, first);
	}
}


void
LegendChartAxis::_InvalidateLayout()
{
	fLayoutValid = false;
}


bool
LegendChartAxis::_ValidateLayout(BView* view)
{
	if (fLayoutValid)
		return true;

	fLegends.MakeEmpty();

	int32 width = fFrame.IntegerWidth() + 1;
	int32 height = fFrame.IntegerHeight() + 1;

	// estimate the maximum legend count we might need
	int32 maxLegends = _EstimateMaxLegendCount(view, fFrame.Size(),
		&fHorizontalSpacing, &fVerticalSpacing);

	if (maxLegends == 0)
		return false;

	// get the legends
	ChartLegend* legends[maxLegends];
	double values[maxLegends];

	int32 legendCount = fLegendSource->GetAxisLegends(fRange, legends, values,
		maxLegends);
	if (legendCount == 0)
		return false;

	// create legend infos
	for (int32 i = 0; i < legendCount; i++) {
		ChartLegend* legend = legends[i];
		BSize size = fLegendRenderer->LegendSize(legend, view);
		LegendInfo* info = new(std::nothrow) LegendInfo(legend, values[i],
			size);
		if (info == NULL || !fLegends.AddItem(info)) {
			// TODO: Report error!
			delete info;
			for (int32 k = i; k < legendCount; k++)
				delete legends[k];
			return false;
		}
	}

	if (fLocation == CHART_AXIS_LEFT || fLocation == CHART_AXIS_RIGHT)
		_FilterLegends(height, fVerticalSpacing, &BSize::height);
	else
		_FilterLegends(width, fHorizontalSpacing, &BSize::width);

	fLayoutValid = true;
	return true;
}


int32
LegendChartAxis::_EstimateMaxLegendCount(BView* view, BSize size,
	float* _hSpacing, float* _vSpacing)
{
	// get the legend spacing
	fLegendRenderer->GetMinimumLegendSpacing(view, _hSpacing, _vSpacing);

	// estimate the maximum legend count we might need
	if (fLocation == CHART_AXIS_LEFT || fLocation == CHART_AXIS_RIGHT)
		return (int32)((size.IntegerHeight() + 1) / (10 + *_vSpacing));
	return (int32)((size.IntegerWidth() + 1) / (20 + *_hSpacing));
}
