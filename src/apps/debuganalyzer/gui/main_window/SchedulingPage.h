/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_SCHEDULING_PAGE_H
#define MAIN_SCHEDULING_PAGE_H

#include <GroupView.h>

#include "table/Table.h"

#include "main_window/MainWindow.h"


class MainWindow::SchedulingPage : public BGroupView, private TableListener {
public:
								SchedulingPage(MainWindow* parent);
	virtual						~SchedulingPage();

			void				SetModel(Model* model);

private:
	// TableListener
	virtual	void				TableRowInvoked(Table* table, int32 rowIndex);

private:
			MainWindow*			fParent;
			Model*				fModel;
};



#endif	// MAIN_SCHEDULING_PAGE_H
