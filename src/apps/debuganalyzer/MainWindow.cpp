/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "MainWindow.h"

#include <GroupLayoutBuilder.h>
#include <TabView.h>

#include "ThreadsPage.h"


MainWindow::MainWindow()
	:
	BWindow(BRect(50, 50, 599, 499), "DebugAnalyzer", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fMainTabView(NULL)
{
	BGroupLayout* rootLayout = new BGroupLayout(B_VERTICAL);
	SetLayout(rootLayout);

	fMainTabView = new BTabView("main tab view");

	BGroupLayoutBuilder(rootLayout)
		.Add(fMainTabView);

	fMainTabView->AddTab(new BView("Teams", 0));
	fMainTabView->AddTab(new ThreadsPage);
}


MainWindow::~MainWindow()
{
}
