/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_WAIT_OBJECTS_PAGE_H
#define THREAD_WAIT_OBJECTS_PAGE_H


#include "AbstractWaitObjectsPage.h"
#include "ThreadModel.h"
#include "thread_window/ThreadWindow.h"


class ThreadWindow::WaitObjectsPage
	: public AbstractWaitObjectsPage<ThreadModel, ThreadModel::WaitObjectGroup,
		Model::ThreadWaitObject> {
public:
								WaitObjectsPage();
};



#endif	// THREAD_WAIT_OBJECTS_PAGE_H
