/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LINE_CHART_RENDERER_H
#define LINE_CHART_RENDERER_H

#include <InterfaceDefs.h>
#include <Rect.h>

#include <ObjectList.h>

#include "chart/ChartDataRange.h"
#include "chart/ChartRenderer.h"


class BView;


class LineChartRenderer : public ChartRenderer {
public:
	class DataSourceConfig;

public:
								LineChartRenderer();
	virtual						~LineChartRenderer();

	virtual	bool				AddDataSource(ChartDataSource* dataSource,
									int32 index,
									ChartRendererDataSourceConfig* config);
	virtual	void				RemoveDataSource(ChartDataSource* dataSource);

	virtual	void				SetFrame(const BRect& frame);
	virtual	void				SetDomain(const ChartDataRange& domain);
	virtual	void				SetRange(const ChartDataRange& range);

	virtual	void				Render(BView* view, BRect updateRect);

private:
			struct DataSourceInfo;

			typedef BObjectList<DataSourceInfo> DataSourceList;

private:
			void				_InvalidateSamples();
			bool				_UpdateSamples();

private:
			DataSourceList		fDataSources;
			BRect				fFrame;
			ChartDataRange		fDomain;
			ChartDataRange		fRange;
};


class LineChartRendererDataSourceConfig : public ChartRendererDataSourceConfig {
public:
								LineChartRendererDataSourceConfig();
								LineChartRendererDataSourceConfig(
									const rgb_color& color);
	virtual						~LineChartRendererDataSourceConfig();

			const rgb_color&	Color() const;
			void				SetColor(const rgb_color& color);

private:
			rgb_color			fColor;
};


#endif	// LINE_CHART_RENDERER_H
