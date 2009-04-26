/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_WAIT_OBJECTS_PAGE_H
#define THREAD_WAIT_OBJECTS_PAGE_H

#include <GroupView.h>

#include "table/TreeTable.h"

#include "thread_window/ThreadWindow.h"


class ThreadWindow::WaitObjectsPage : public BGroupView,
	private TreeTableListener {
public:
								WaitObjectsPage();
	virtual						~WaitObjectsPage();

			void				SetModel(Model* model, Model::Thread* thread);

private:
			class WaitObjectsTreeModel;

private:
	// TableListener
	virtual	void				TreeTableNodeInvoked(TreeTable* table,
									void* node);

private:
			TreeTable*			fWaitObjectsTree;
			WaitObjectsTreeModel* fWaitObjectsTreeModel;
			Model*				fModel;
			Model::Thread*		fThread;
};



#endif	// THREAD_WAIT_OBJECTS_PAGE_H
