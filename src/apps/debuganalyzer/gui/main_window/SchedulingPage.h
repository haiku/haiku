/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_SCHEDULING_PAGE_H
#define MAIN_SCHEDULING_PAGE_H


#include <GroupView.h>

#include "table/Table.h"

#include "ListSelectionModel.h"
#include "main_window/MainWindow.h"


class BScrollView;


class MainWindow::SchedulingPage : public BGroupView, private TableListener {
public:
								SchedulingPage(MainWindow* parent);
	virtual						~SchedulingPage();

			void				SetModel(Model* model);

private:
			struct SchedulingEvent;
			struct IOSchedulingEvent;
			class SchedulingData;
			struct TimeRange;
			class TimelineHeaderRenderer;
			class BaseView;
			class LineBaseView;
			class ThreadsView;
			class SchedulingView;
			class ViewPort;

			struct FontInfo {
				font_height	fontHeight;
				float		lineHeight;
			};

private:
			MainWindow*			fParent;
			Model*				fModel;
			BScrollView*		fScrollView;
			ViewPort*			fViewPort;
			ThreadsView*		fThreadsView;
			SchedulingView*		fSchedulingView;
			FontInfo			fFontInfo;
			ListSelectionModel	fFilterModel;
			ListSelectionModel	fSelectionModel;
};


#endif	// MAIN_SCHEDULING_PAGE_H
