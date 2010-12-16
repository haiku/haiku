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

#include "MessageCodes.h"
#include "ThreadModel.h"
#include "ThreadModelLoader.h"

#include "thread_window/ActivityPage.h"
#include "thread_window/GeneralPage.h"
#include "thread_window/WaitObjectsPage.h"


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
	fActivityPage(NULL),
	fModel(model),
	fThread(thread),
	fThreadModel(NULL),
	fThreadModelLoader(NULL)
{
	BGroupLayout* rootLayout = new BGroupLayout(B_VERTICAL);
	SetLayout(rootLayout);

	fMainTabView = new BTabView("main tab view");

	BGroupLayoutBuilder(rootLayout)
		.Add(fMainTabView);

	fMainTabView->AddTab(fGeneralPage = new GeneralPage);
	fMainTabView->AddTab(fWaitObjectsPage = new WaitObjectsPage);
	fMainTabView->AddTab(fActivityPage = new ActivityPage);

	fGeneralPage->SetModel(fModel, fThread);

	fModel->AcquireReference();

	// create a thread model loader
	fThreadModelLoader = new ThreadModelLoader(fModel, fThread,
		BMessenger(this), NULL);
}


ThreadWindow::~ThreadWindow()
{
	if (fThreadModelLoader != NULL)
		fThreadModelLoader->Delete();

	delete fThreadModel;

	fModel->ReleaseReference();
}


void
ThreadWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_MODEL_LOADED_SUCCESSFULLY:
		{
printf("MSG_MODEL_LOADED_SUCCESSFULLY\n");
			ThreadModel* model = fThreadModelLoader->DetachModel();
			fThreadModelLoader->Delete();
			fThreadModelLoader = NULL;
			_SetModel(model);
			break;
		}

		case MSG_MODEL_LOADED_FAILED:
		case MSG_MODEL_LOADED_ABORTED:
		{
printf("MSG_MODEL_LOADED_FAILED/MSG_MODEL_LOADED_ABORTED\n");
			fThreadModelLoader->Delete();
			fThreadModelLoader = NULL;
			// TODO: User feedback (in failed case)!
			break;
		}

		default:
			SubWindow::MessageReceived(message);
			break;
	}
}


void
ThreadWindow::Quit()
{
	if (fThreadModelLoader != NULL)
		fThreadModelLoader->Abort(true);

	SubWindow::Quit();
}


void
ThreadWindow::Show()
{
	SubWindow::Show();

	AutoLocker<ThreadWindow> locker;

	if (fThreadModelLoader == NULL)
		return;

	status_t error = fThreadModelLoader->StartLoading();
	if (error != B_OK) {
		fThreadModelLoader->Delete();
		fThreadModelLoader = NULL;
		// TODO: User feedback!
	}
}


void
ThreadWindow::_SetModel(ThreadModel* model)
{
	delete fThreadModel;

	fThreadModel = model;

	fWaitObjectsPage->SetModel(fThreadModel);
	fActivityPage->SetModel(fThreadModel);
}
