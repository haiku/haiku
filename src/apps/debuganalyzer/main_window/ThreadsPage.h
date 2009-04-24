/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREADS_PAGE_H
#define THREADS_PAGE_H

#include <GroupView.h>


class Model;
class Table;


class ThreadsPage : public BGroupView {
public:
								ThreadsPage();
	virtual						~ThreadsPage();

			void				SetModel(Model* model);

private:
			class ThreadsTableModel;

private:
			Table*				fThreadsTable;
			ThreadsTableModel*	fThreadsTableModel;
			Model*				fModel;
};



#endif	// THREADS_PAGE_H
