/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "chart/LineChartRenderer.h"

#include <stdio.h>

#include <Shape.h>
#include <View.h>

#include "chart/ChartDataSource.h"


// #pragma mark - DataSourceInfo


struct LineChartRenderer::DataSourceInfo {
public:
			ChartDataSource*	source;
			double*				samples;
			int32				sampleCount;
			bool				samplesValid;
			LineChartRendererDataSourceConfig config;

public:
								DataSourceInfo(ChartDataSource* source,
									LineChartRendererDataSourceConfig* config);
								~DataSourceInfo();

			bool				UpdateSamples(const ChartDataRange& domain,
									int32 sampleCount);
};


LineChartRenderer::DataSourceInfo::DataSourceInfo(ChartDataSource* source,
	LineChartRendererDataSourceConfig* config)
	:
	source(source),
	samples(NULL),
	sampleCount(0),
	samplesValid(false)
{
	if (config != NULL)
		this->config = *config;
}


LineChartRenderer::DataSourceInfo::~DataSourceInfo()
{
	delete[] samples;
}


bool
LineChartRenderer::DataSourceInfo::UpdateSamples(const ChartDataRange& domain,
	int32 sampleCount)
{
	if (samplesValid)
		return true;

	// check the sample count -- we might need to realloc the sample array
	if (sampleCount != this->sampleCount) {
		delete[] samples;
		this->sampleCount = 0;

		samples = new(std::nothrow) double[sampleCount];
		if (samples == NULL)
			return false;

		this->sampleCount = sampleCount;
	}

	// get the new samples
	source->GetSamples(domain.min, domain.max, samples, sampleCount);

	samplesValid = true;
	return true;
}


// #pragma mark - LineChartRenderer


LineChartRenderer::LineChartRenderer()
	:
	fFrame(),
	fDomain(),
	fRange()
{
}


LineChartRenderer::~LineChartRenderer()
{
}


bool
LineChartRenderer::AddDataSource(ChartDataSource* dataSource, int32 index,
	ChartRendererDataSourceConfig* config)
{
	DataSourceInfo* info = new(std::nothrow) DataSourceInfo(dataSource,
		dynamic_cast<LineChartRendererDataSourceConfig*>(config));
	if (info == NULL)
		return false;

	if (!fDataSources.AddItem(info, index)) {
		delete info;
		return false;
	}

	return true;
}


void
LineChartRenderer::RemoveDataSource(ChartDataSource* dataSource)
{
	for (int32 i = 0; DataSourceInfo* info = fDataSources.ItemAt(i); i++) {
		info->samplesValid = false;
		if (info->source == dataSource) {
			delete fDataSources.RemoveItemAt(i);
			return;
		}
	}
}


void
LineChartRenderer::SetFrame(const BRect& frame)
{
	fFrame = frame;
	_InvalidateSamples();
}


void
LineChartRenderer::SetDomain(const ChartDataRange& domain)
{
	if (domain != fDomain) {
		fDomain = domain;
		_InvalidateSamples();
	}
}


void
LineChartRenderer::SetRange(const ChartDataRange& range)
{
	if (range != fRange) {
		fRange = range;
		_InvalidateSamples();
	}
}


void
LineChartRenderer::Render(BView* view, BRect updateRect)
{
	if (!updateRect.IsValid() || updateRect.left > fFrame.right
		|| fFrame.left > updateRect.right) {
		return;
	}

	if (fDomain.min >= fDomain.max || fRange.min >= fRange.max)
		return;

	if (!_UpdateSamples())
		return;

	// get the range to draw (draw one more sample on each side)
	int32 left = (int32)fFrame.left;
	int32 first = (int32)updateRect.left - left - 1;
	int32 last = (int32)updateRect.right - left + 1;
	if (first < 0)
		first = 0;
	if (last > fFrame.IntegerWidth())
		last = fFrame.IntegerWidth();
	if (first > last)
		return;

	double minRange = fRange.min;
	double sampleRange = fRange.max - minRange;
	if (sampleRange == 0) {
		minRange = fRange.min - 0.5;
		sampleRange = 1;
	}
	double scale = (double)fFrame.IntegerHeight() / sampleRange;

	// draw
	view->SetLineMode(B_ROUND_CAP, B_ROUND_JOIN);
	for (int32 i = 0; DataSourceInfo* info = fDataSources.ItemAt(i); i++) {

		float bottom = fFrame.bottom;
		BShape shape;
		shape.MoveTo(BPoint(left + first,
			bottom - ((info->samples[first] - minRange) * scale)));

		for (int32 i = first; i <= last; i++) {
			shape.LineTo(BPoint(float(left + i),
				float(bottom - ((info->samples[i] - minRange) * scale))));
		}
		view->SetHighColor(info->config.Color());
		view->MovePenTo(B_ORIGIN);
		view->StrokeShape(&shape);
	}
}


void
LineChartRenderer::_InvalidateSamples()
{
	for (int32 i = 0; DataSourceInfo* info = fDataSources.ItemAt(i); i++)
		info->samplesValid = false;
}


bool
LineChartRenderer::_UpdateSamples()
{
	int32 width = fFrame.IntegerWidth() + 1;
	if (width <= 0)
		return false;

	for (int32 i = 0; DataSourceInfo* info = fDataSources.ItemAt(i); i++) {
		if (!info->UpdateSamples(fDomain, width))
			return false;
	}

	return true;
}


// #pragma mark - LineChartRendererDataSourceConfig


LineChartRendererDataSourceConfig::LineChartRendererDataSourceConfig()
{
	fColor.red = 0;
	fColor.green = 0;
	fColor.blue = 0;
	fColor.alpha = 255;
}


LineChartRendererDataSourceConfig::LineChartRendererDataSourceConfig(
	const rgb_color& color)
{
	fColor = color;
}


LineChartRendererDataSourceConfig::~LineChartRendererDataSourceConfig()
{
}


const rgb_color&
LineChartRendererDataSourceConfig::Color() const
{
	return fColor;
}


void
LineChartRendererDataSourceConfig::SetColor(const rgb_color& color)
{
	fColor = color;
}
