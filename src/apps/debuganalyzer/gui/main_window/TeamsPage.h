/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_TEAMS_PAGE_H
#define MAIN_TEAMS_PAGE_H

#include <GroupView.h>

#include "table/Table.h"

#include "main_window/MainWindow.h"


class MainWindow::TeamsPage : public BGroupView, private TableListener {
public:
								TeamsPage(MainWindow* parent);
	virtual						~TeamsPage();

			void				SetModel(Model* model);

private:
			class TeamsTableModel;

private:
	// TableListener
	virtual	void				TableRowInvoked(Table* table, int32 rowIndex);

private:
			MainWindow*			fParent;
			Table*				fTeamsTable;
			TeamsTableModel*	fTeamsTableModel;
			Model*				fModel;
};



#endif	// MAIN_TEAMS_PAGE_H
