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
class ColorCheckBox;


class ThreadWindow::ActivityPage : public BGroupView {
public:
								ActivityPage();
	virtual						~ActivityPage();

			void				SetModel(ThreadModel* model);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

private:
			class ThreadActivityData;

private:
			void				_UpdateChartDataEnabled(int timeType);

private:
			ThreadModel*		fThreadModel;
			Chart*				fActivityChart;
			ChartRenderer*		fActivityChartRenderer;
			ThreadActivityData*	fRunTimeData;
			ThreadActivityData*	fWaitTimeData;
			ThreadActivityData*	fPreemptionTimeData;
			ThreadActivityData*	fLatencyTimeData;
			ColorCheckBox*		fRunTimeCheckBox;
			ColorCheckBox*		fWaitTimeCheckBox;
			ColorCheckBox*		fPreemptionTimeCheckBox;
			ColorCheckBox*		fLatencyTimeCheckBox;
};


#endif	// THREAD_ACTIVITY_PAGE_H
