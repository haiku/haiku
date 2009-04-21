/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "MainWindow.h"

#include <stdio.h>

#include <Application.h>
#include <GroupLayoutBuilder.h>
#include <TabView.h>

#include <AutoLocker.h>

#include "DataSource.h"
#include "MessageCodes.h"
#include "Model.h"
#include "ModelLoader.h"
#include "ThreadsPage.h"


MainWindow::MainWindow(DataSource* dataSource)
	:
	BWindow(BRect(50, 50, 599, 499), "DebugAnalyzer", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fMainTabView(NULL),
	fThreadsPage(new ThreadsPage),
	fModel(NULL),
	fModelLoader(NULL)
{
	BGroupLayout* rootLayout = new BGroupLayout(B_VERTICAL);
	SetLayout(rootLayout);

	fMainTabView = new BTabView("main tab view");

	BGroupLayoutBuilder(rootLayout)
		.Add(fMainTabView);

	fMainTabView->AddTab(new BView("Teams", 0));
	fMainTabView->AddTab(new ThreadsPage);

	// create a model loader, if we have a data source
	if (dataSource != NULL)
		fModelLoader = new ModelLoader(dataSource, BMessenger(this), NULL);
}


MainWindow::~MainWindow()
{
	delete fModelLoader;
	delete fModel;
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
MainWindow::_SetModel(Model* model)
{
	delete fModel;
	fModel = model;

	fThreadsPage->SetModel(fModel);
}
