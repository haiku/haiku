/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "main_window/SchedulingPage.h"

#include <stdio.h>

#include <new>


MainWindow::SchedulingPage::SchedulingPage(MainWindow* parent)
	:
	BGroupView(B_VERTICAL),
	fParent(parent),
	fModel(NULL)
{
	SetName("Scheduling");
}


MainWindow::SchedulingPage::~SchedulingPage()
{
}


void
MainWindow::SchedulingPage::SetModel(Model* model)
{
	if (model == fModel)
		return;

	if (fModel != NULL) {
	}

	fModel = model;

	if (fModel != NULL) {
	}
}


void
MainWindow::SchedulingPage::TableRowInvoked(Table* table, int32 rowIndex)
{
	// TODO: ...
}

