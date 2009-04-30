/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_ACTIVITY_PAGE_H
#define THREAD_ACTIVITY_PAGE_H

#include <GroupView.h>

#include "thread_window/ThreadWindow.h"


class Chart;
class ChartRenderer;


class ThreadWindow::ActivityPage : public BGroupView {
public:
								ActivityPage();
	virtual						~ActivityPage();

			void				SetModel(ThreadModel* model);

private:
			class ThreadActivityData;

private:
			ThreadModel*		fThreadModel;
			Chart*				fActivityChart;
			ChartRenderer*		fActivityChartRenderer;
			ThreadActivityData*	fRunTimeData;
			ThreadActivityData*	fWaitTimeData;
			ThreadActivityData*	fLatencyTimeData;
			ThreadActivityData*	fPreemptionTimeData;
};



#endif	// THREAD_ACTIVITY_PAGE_H
