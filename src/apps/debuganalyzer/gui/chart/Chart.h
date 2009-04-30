/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHART_H
#define CHART_H

#include <View.h>

#include <ObjectList.h>

#include "chart/ChartDataRange.h"
#include "chart/ChartDefs.h"


class ChartAxis;
class ChartDataSource;
class ChartRenderer;
class ChartRendererDataSourceConfig;


class Chart : public BView {
public:
								Chart(ChartRenderer* renderer,
									const char* name = NULL);
	virtual						~Chart();

			bool				AddDataSource(ChartDataSource* dataSource,
									int32 index,
									ChartRendererDataSourceConfig* config
										= NULL);
			bool				AddDataSource(ChartDataSource* dataSource,
									ChartRendererDataSourceConfig* config
										= NULL);
			bool				RemoveDataSource(
									ChartDataSource* dataSource);
			ChartDataSource*	RemoveDataSource(int32 index);
			void				RemoveAllDataSources();

			void				SetAxis(ChartAxisLocation location,
									ChartAxis* axis);

#if 0
			ChartDataRange		Domain() const;

			void				SetDisplayDomain(const ChartDataRange& domain);
			void				SetDisplayRange(const ChartDataRange& range);
#endif

	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual	void				Draw(BRect updateRect);

#if 0
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
#endif

	virtual	void				DoLayout();

private:
			typedef BObjectList<ChartDataSource> DataSourceList;

			struct AxisInfo {
				ChartAxis*		axis;
				BRect			frame;

								AxisInfo();
				void			SetFrame(float left, float top, float right,
									float bottom);
				void			SetRange(const ChartDataRange& range);
				void			Render(BView* view, const BRect& updateRect);
			};

private:
			void				_UpdateDomainAndRange();

private:
			ChartRenderer*		fRenderer;
			DataSourceList		fDataSources;
			AxisInfo			fLeftAxis;
			AxisInfo			fTopAxis;
			AxisInfo			fRightAxis;
			AxisInfo			fBottomAxis;
			ChartDataRange		fDomain;
			ChartDataRange		fRange;
#if 0
			ChartDataRange		fDisplayDomain;
			ChartDataRange		fDisplayRange;
#endif
			BRect				fChartFrame;
};


#endif	// CHART_H
