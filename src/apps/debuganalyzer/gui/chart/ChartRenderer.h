/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHART_RENDERER_H
#define CHART_RENDERER_H

#include <Rect.h>


class BView;
class ChartDataRange;
class ChartDataSource;


class ChartRendererDataSourceConfig {
public:
	virtual						~ChartRendererDataSourceConfig();
};


class ChartRenderer {
public:
	virtual						~ChartRenderer();

	virtual	bool				AddDataSource(
									ChartDataSource* dataSource,
									int32 index,
									ChartRendererDataSourceConfig* config) = 0;
	virtual	void				RemoveDataSource(
									ChartDataSource* dataSource) = 0;

	virtual	void				SetFrame(const BRect& frame) = 0;
	virtual	void				SetDomain(const ChartDataRange& domain) = 0;
	virtual	void				SetRange(const ChartDataRange& range) = 0;

	virtual	void				Render(BView* view, BRect updateRect) = 0;
};


#endif	// CHART_RENDERER_H
