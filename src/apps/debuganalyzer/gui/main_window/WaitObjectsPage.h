/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WAIT_OBJECTS_PAGE_H
#define MAIN_WAIT_OBJECTS_PAGE_H


#include "AbstractWaitObjectsPage.h"
#include "Model.h"
#include "main_window/MainWindow.h"


class MainWindow::WaitObjectsPage
	: public AbstractWaitObjectsPage<Model, Model::WaitObjectGroup,
		Model::WaitObject> {
public:
								WaitObjectsPage(MainWindow* parent);

private:
			MainWindow*			fParent;
};


#endif	// MAIN_WAIT_OBJECTS_PAGE_H
