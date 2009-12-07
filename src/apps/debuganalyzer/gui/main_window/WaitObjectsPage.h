/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WAIT_OBJECTS_PAGE_H
#define MAIN_WAIT_OBJECTS_PAGE_H


#include <GroupView.h>

#include "table/Table.h"

#include "main_window/MainWindow.h"


class MainWindow::WaitObjectsPage : public BGroupView {
public:
								WaitObjectsPage(MainWindow* parent);
	virtual						~WaitObjectsPage();

			void				SetModel(Model* model);

private:
	class WaitObjectsTableModel;

			MainWindow*			fParent;
			Table*				fWaitObjectsTable;
			WaitObjectsTableModel* fWaitObjectsTableModel;
			Model*				fModel;
};


#endif	// MAIN_WAIT_OBJECTS_PAGE_H
