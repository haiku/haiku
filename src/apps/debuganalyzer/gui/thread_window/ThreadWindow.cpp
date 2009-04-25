/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ThreadWindow.h"

#include <stdio.h>

#include <Application.h>
#include <GroupLayoutBuilder.h>
#include <String.h>
#include <TabView.h>

#include <AutoLocker.h>

//#include "MessageCodes.h"

#include "thread_window/GeneralPage.h"


static BString
get_window_name(Model::Thread* thread)
{
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "Thread: %s (%ld)", thread->Name(),
		thread->ID());
	return BString(buffer);
}


ThreadWindow::ThreadWindow(SubWindowManager* manager, Model* model,
	Model::Thread* thread)
	:
	SubWindow(manager, BRect(50, 50, 599, 499),
		get_window_name(thread).String(), B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fMainTabView(NULL),
	fGeneralPage(NULL),
	fWaitObjectsPage(NULL),
	fModel(model),
	fThread(thread)
{
	BGroupLayout* rootLayout = new BGroupLayout(B_VERTICAL);
	SetLayout(rootLayout);

	fMainTabView = new BTabView("main tab view");

	BGroupLayoutBuilder(rootLayout)
		.Add(fMainTabView);

	fMainTabView->AddTab(fGeneralPage = new GeneralPage);
//	fMainTabView->AddTab(fWaitObjectsPage = new WaitObjectsPage);
fMainTabView->AddTab(new BView("Dummy", 0));
//fMainTabView->Select(0);

	fGeneralPage->SetModel(fModel, fThread);
//	fWaitObjectsPage->SetModel(fModel, fThread);

	fModel->AddReference();
}


ThreadWindow::~ThreadWindow()
{
	fModel->RemoveReference();
}
