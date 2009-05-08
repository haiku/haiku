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

	inline	ChartDataRange		Domain() const;
	inline	ChartDataRange		Range() const;

	inline	ChartDataRange		DisplayDomain() const;
	inline	ChartDataRange		DisplayRange() const;

			void				SetDisplayDomain(ChartDataRange domain);
			void				SetDisplayRange(ChartDataRange range);

			double				DomainZoomLimit() const;
			void				SetDomainZoomLimit(double limit);

	virtual	void				DomainChanged();
	virtual	void				RangeChanged();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code,
									const BMessage* dragMessage);
	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual	void				Draw(BRect updateRect);
	virtual	void				ScrollTo(BPoint where);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

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
			void				_UpdateScrollBar(bool horizontal);
			void				_ScrollTo(float value, bool horizontal);
			void				_Zoom(float x, float steps);

private:
			ChartRenderer*		fRenderer;
			DataSourceList		fDataSources;
			AxisInfo			fLeftAxis;
			AxisInfo			fTopAxis;
			AxisInfo			fRightAxis;
			AxisInfo			fBottomAxis;
			ChartDataRange		fDomain;
			ChartDataRange		fRange;
			ChartDataRange		fDisplayDomain;
			ChartDataRange		fDisplayRange;
			BRect				fChartFrame;
			float				fHScrollSize;
			float				fVScrollSize;
			float				fHScrollValue;
			float				fVScrollValue;
			int32				fIgnoreScrollEvent;
			double				fDomainZoomLimit;
			BPoint				fLastMousePos;
			BPoint				fDraggingStartPos;
			float				fDraggingStartScrollValue;
};


ChartDataRange
Chart::Domain() const
{
	return fDomain;
}


ChartDataRange
Chart::Range() const
{
	return fRange;
}


ChartDataRange
Chart::DisplayDomain() const
{
	return fDisplayDomain;
}


ChartDataRange
Chart::DisplayRange() const
{
	return fDisplayRange;
}


#endif	// CHART_H
