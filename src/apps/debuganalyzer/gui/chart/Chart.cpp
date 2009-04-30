/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "chart/Chart.h"

#include <stdio.h>

#include <new>

#include <ControlLook.h>
#include <Region.h>

#include "chart/ChartAxis.h"
#include "chart/ChartDataSource.h"
#include "chart/ChartRenderer.h"


// #pragma mark - Chart::AxisInfo


Chart::AxisInfo::AxisInfo()
	:
	axis(NULL)
{
}


void
Chart::AxisInfo::SetFrame(float left, float top, float right, float bottom)
{
	frame.Set(left, top, right, bottom);
	if (axis != NULL)
		axis->SetFrame(frame);
}


void
Chart::AxisInfo::SetRange(const ChartDataRange& range)
{
	if (axis != NULL)
		axis->SetRange(range);
}


void
Chart::AxisInfo::Render(BView* view, const BRect& updateRect)
{
	if (axis != NULL)
		axis->Render(view, updateRect);
}


// #pragma mark - Chart


Chart::Chart(ChartRenderer* renderer, const char* name)
	:
	BView(name, B_FRAME_EVENTS | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fRenderer(renderer)
{
	SetViewColor(B_TRANSPARENT_32_BIT);

//	fRenderer->SetFrame(Bounds());
}


Chart::~Chart()
{
}


bool
Chart::AddDataSource(ChartDataSource* dataSource, int32 index,
	ChartRendererDataSourceConfig* config)
{
printf("Chart::AddDataSource(%p)\n", dataSource);
	if (dataSource == NULL)
		return false;

	if (index < 0 || index > fDataSources.CountItems())
		index = fDataSources.CountItems();

	if (!fDataSources.AddItem(dataSource, index))
		return false;

	if (!fRenderer->AddDataSource(dataSource, index, config)) {
		fDataSources.RemoveItemAt(index);
		return false;
	}

	_UpdateDomainAndRange();

	InvalidateLayout();
	Invalidate();

	return true;
}


bool
Chart::AddDataSource(ChartDataSource* dataSource,
	ChartRendererDataSourceConfig* config)
{
	return AddDataSource(dataSource, -1, config);
}


bool
Chart::RemoveDataSource(ChartDataSource* dataSource)
{
	if (dataSource == NULL)
		return false;

	return RemoveDataSource(fDataSources.IndexOf(dataSource));
}


ChartDataSource*
Chart::RemoveDataSource(int32 index)
{
printf("Chart::RemoveDataSource(%ld)\n", index);
	if (index < 0 || index >= fDataSources.CountItems())
		return NULL;

	ChartDataSource* dataSource = fDataSources.RemoveItemAt(index);

	fRenderer->RemoveDataSource(dataSource);

	_UpdateDomainAndRange();
	
	return dataSource;
}


void
Chart::RemoveAllDataSources()
{
printf("Chart::RemoveAllDataSources()\n");
	int32 count = fDataSources.CountItems();
	for (int32 i = count - 1; i >= 0; i--)
		fRenderer->RemoveDataSource(fDataSources.ItemAt(i));

	fDataSources.MakeEmpty();

	_UpdateDomainAndRange();

	InvalidateLayout();
	Invalidate();
}


void
Chart::SetAxis(ChartAxisLocation location, ChartAxis* axis)
{
	switch (location) {
		case CHART_AXIS_LEFT:
			fLeftAxis.axis = axis;
			break;
		case CHART_AXIS_TOP:
			fTopAxis.axis = axis;
			break;
		case CHART_AXIS_RIGHT:
			fRightAxis.axis = axis;
			break;
		case CHART_AXIS_BOTTOM:
			fBottomAxis.axis = axis;
			break;
		default:
			return;
	}

	axis->SetLocation(location);

	InvalidateLayout();
	Invalidate();
}


#if 0
ChartDataRange
Chart::Domain() const
{
	return fDataSource != NULL ? fDataSource->Domain() : ChartDataRange();
}


void
Chart::SetDisplayDomain(const ChartDataRange& domain)
{
	fRenderer->SetDomain(domain);
	Invalidate();
}


void
Chart::SetDisplayDomain(const ChartDataRange& range)
{
	fRenderer->SetRange(range);
	Invalidate();
}
#endif


void
Chart::FrameResized(float newWidth, float newHeight)
{
//printf("Chart::FrameResized(%f, %f)\n", newWidth, newHeight);
//	fRenderer->SetFrame(Bounds());

	Invalidate();
}


void
Chart::Draw(BRect updateRect)
{
printf("Chart::Draw((%f, %f) - (%f, %f))\n", updateRect.left, updateRect.top, updateRect.right, updateRect.bottom);
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color color;

	// clear the axes background
	if (fLeftAxis.axis != NULL || fTopAxis.axis != NULL
		|| fRightAxis.axis != NULL || fBottomAxis.axis != NULL) {
		SetHighColor(background);
		BRegion clippingRegion(Bounds());
		clippingRegion.Exclude(fChartFrame);
		ConstrainClippingRegion(&clippingRegion);
		FillRect(Bounds());
		ConstrainClippingRegion(NULL);
	}

	// render the axes
	fLeftAxis.Render(this, updateRect);
	fTopAxis.Render(this, updateRect);
	fRightAxis.Render(this, updateRect);
	fBottomAxis.Render(this, updateRect);

	// draw the frame around the chart area and clear the background
	BRect chartFrame(fChartFrame);
	be_control_look->DrawBorder(this, chartFrame, updateRect, background,
		B_PLAIN_BORDER);
		// DrawBorder() insets the supplied rect
	SetHighColor(color.set_to(255, 255, 255, 255));
	FillRect(chartFrame);

	// render the chart
	BRegion clippingRegion(chartFrame);
	ConstrainClippingRegion(&clippingRegion);
	fRenderer->Render(this, updateRect);
}


#if 0
BSize
Chart::MinSize()
{
}


BSize
Chart::MaxSize()
{
}


BSize
Chart::PreferredSize()
{
}
#endif


void
Chart::DoLayout()
{
	// get size in pixels
	BSize size = Bounds().Size();
printf("Chart::DoLayout(%f, %f)\n", size.width, size.height);
	int32 width = size.IntegerWidth() + 1;
	int32 height = size.IntegerHeight() + 1;

	// compute the axis insets
	int32 left = 0;
	int32 right = 0;
	int32 top = 0;
	int32 bottom = 0;

	if (fLeftAxis.axis != NULL)
		left = fLeftAxis.axis->PreferredSize(this).IntegerWidth() + 1;
	if (fRightAxis.axis != NULL)
		right = fRightAxis.axis->PreferredSize(this).IntegerWidth() + 1;
	if (fTopAxis.axis != NULL)
		top = fTopAxis.axis->PreferredSize(this).IntegerHeight() + 1;
	if (fBottomAxis.axis != NULL)
		bottom = fBottomAxis.axis->PreferredSize(this).IntegerHeight() + 1;

	fChartFrame = BRect(left, top, width - right - 1, height - bottom - 1);
	fRenderer->SetFrame(fChartFrame.InsetByCopy(1, 1));
printf("  fChartFrame: (%f, %f) - (%f, %f)\n", fChartFrame.left, fChartFrame.top, fChartFrame.right, fChartFrame.bottom);

	fLeftAxis.SetFrame(0, fChartFrame.top, fChartFrame.left - 1,
		fChartFrame.bottom);
	fRightAxis.SetFrame(fChartFrame.right + 1, fChartFrame.top, width - 1,
		fChartFrame.bottom);
	fTopAxis.SetFrame(fChartFrame.left, 0, fChartFrame.right,
		fChartFrame.top - 1);
	fBottomAxis.SetFrame(fChartFrame.left, fChartFrame.bottom + 1,
		fChartFrame.right, height - 1);
}


void
Chart::_UpdateDomainAndRange()
{
	if (fDataSources.IsEmpty()) {
		fDomain = ChartDataRange();
		fRange = ChartDataRange();
		return;
	} else {
		ChartDataSource* firstSource = fDataSources.ItemAt(0);
		fDomain = firstSource->Domain();
		fRange = firstSource->Range();
	
		for (int32 i = 1; ChartDataSource* source = fDataSources.ItemAt(i);
				i++) {
			fDomain.Extend(source->Domain());
			fRange.Extend(source->Range());
		}
	}

	fRenderer->SetDomain(fDomain);
	fRenderer->SetRange(fRange);

	fLeftAxis.SetRange(fRange);
	fTopAxis.SetRange(fDomain);
	fRightAxis.SetRange(fRange);
	fBottomAxis.SetRange(fDomain);
}
