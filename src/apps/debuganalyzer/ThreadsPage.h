/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREADS_PAGE_H
#define THREADS_PAGE_H

#include <GroupView.h>


class BColumnListView;
class Model;


class ThreadsPage : public BGroupView {
public:
								ThreadsPage();
	virtual						~ThreadsPage();

			void				SetModel(Model* model);

private:
			BColumnListView*	fThreadsListView;
			Model*				fModel;
};



#endif	// THREADS_PAGE_H
