/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ThreadsPage.h"

#include <ColumnTypes.h>


ThreadsPage::ThreadsPage()
	:
	BGroupView(B_VERTICAL),
	fThreadsListView(NULL)
{
	SetName("Threads");

	fThreadsListView = new BColumnListView("threads list", 0);
	AddChild(fThreadsListView);

	fThreadsListView->AddColumn(new BStringColumn("Thread", 40, 20, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT), 0);
	fThreadsListView->AddColumn(new BStringColumn("Name", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT), 1);
	fThreadsListView->AddColumn(new BStringColumn("Run Time", 80, 20, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT), 2);
	fThreadsListView->AddColumn(new BStringColumn("Wait Time", 80, 20, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT), 3);
	fThreadsListView->AddColumn(new BStringColumn("Latencies", 80, 20, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT), 4);
	fThreadsListView->AddColumn(new BStringColumn("Preemptions", 80, 20, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT), 5);
}


ThreadsPage::~ThreadsPage()
{
}


void
ThreadsPage::SetModel(Model* model)
{
	fModel = model;
}
