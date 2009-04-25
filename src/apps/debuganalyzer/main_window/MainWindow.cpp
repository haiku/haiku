/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "MainWindow.h"

#include <stdio.h>

#include <new>

#include <Application.h>
#include <GroupLayoutBuilder.h>
#include <TabView.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "DataSource.h"
#include "MessageCodes.h"
#include "ModelLoader.h"
#include "SubWindowManager.h"

#include "main_window/ThreadsPage.h"
#include "thread_window/ThreadWindow.h"


MainWindow::MainWindow(DataSource* dataSource)
	:
	BWindow(BRect(50, 50, 599, 499), "DebugAnalyzer", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fMainTabView(NULL),
	fThreadsPage(NULL),
	fModel(NULL),
	fModelLoader(NULL),
	fSubWindowManager(NULL)
{
	fSubWindowManager = new SubWindowManager(this);

	BGroupLayout* rootLayout = new BGroupLayout(B_VERTICAL);
	SetLayout(rootLayout);

	fMainTabView = new BTabView("main tab view");

	BGroupLayoutBuilder(rootLayout)
		.Add(fMainTabView);

	fMainTabView->AddTab(new BView("Teams", 0));
	fMainTabView->AddTab(fThreadsPage = new ThreadsPage(this));

	// create a model loader, if we have a data source
	if (dataSource != NULL)
		fModelLoader = new ModelLoader(dataSource, BMessenger(this), NULL);
}


MainWindow::~MainWindow()
{
	delete fModelLoader;

	if (fModel != NULL)
		fModel->RemoveReference();

	fSubWindowManager->RemoveReference();
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_MODEL_LOADED_SUCCESSFULLY:
		{
printf("MSG_MODEL_LOADED_SUCCESSFULLY\n");
			Model* model = fModelLoader->DetachModel();
			delete fModelLoader;
			fModelLoader = NULL;
			_SetModel(model);
			model->RemoveReference();
			break;
		}

		case MSG_MODEL_LOADED_FAILED:
		case MSG_MODEL_LOADED_ABORTED:
		{
printf("MSG_MODEL_LOADED_FAILED/MSG_MODEL_LOADED_ABORTED\n");
			delete fModelLoader;
			fModelLoader = NULL;
			// TODO: User feedback (in failed case)!
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
MainWindow::Quit()
{
	fSubWindowManager->Broadcast(B_QUIT_REQUESTED);
	be_app->PostMessage(MSG_WINDOW_QUIT);

	BWindow::Quit();
}


void
MainWindow::Show()
{
	BWindow::Show();

	AutoLocker<MainWindow> locker;

	if (fModelLoader == NULL)
		return;

	status_t error = fModelLoader->StartLoading();
	if (error != B_OK) {
		delete fModelLoader;
		fModelLoader = NULL;
		// TODO: User feedback!
	}
}


void
MainWindow::OpenThreadWindow(Model::Thread* thread)
{
	// create a sub window key
	ObjectSubWindowKey* key = new(std::nothrow) ObjectSubWindowKey(thread);
	if (key == NULL) {
		// TODO: Report error!
		return;
	}
	ObjectDeleter<ObjectSubWindowKey> keyDeleter(key);

	AutoLocker<SubWindowManager> locker(fSubWindowManager);

	// check whether the window already exists
	ThreadWindow* window = dynamic_cast<ThreadWindow*>(
		fSubWindowManager->LookupSubWindow(*key));
	if (window != NULL) {
		// window exists -- just bring it to front
		locker.Unlock();
		window->Lock();
		window->Activate();
		return;
	}

	// window doesn't exist yet -- create it
	try {
		window = new ThreadWindow(fSubWindowManager, fModel, thread);
	} catch (std::bad_alloc) {
		// TODO: Report error!
	}

	if (!window->AddToSubWindowManager(key)) {
		// TODO: Report error!
		delete window;
	}

	keyDeleter.Detach();

	window->Show();
}


void
MainWindow::_SetModel(Model* model)
{
	if (fModel != NULL)
		fModel->RemoveReference();

	fModel = model;

	if (fModel != NULL)
		fModel->AddReference();

	fThreadsPage->SetModel(fModel);
}
