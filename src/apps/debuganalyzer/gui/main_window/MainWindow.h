/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>

#include "Model.h"


class BTabView;
class DataSource;
class ModelLoader;
class SubWindowManager;


class MainWindow : public BWindow {
public:
								MainWindow(DataSource* dataSource);
	virtual						~MainWindow();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Quit();

	virtual	void				Show();

			void				OpenTeamWindow(Model::Team* team);
			void				OpenThreadWindow(Model::Thread* thread);

private:
			class GeneralPage;
			class TeamsPage;
			class ThreadsPage;
			class SchedulingPage;
			class WaitObjectsPage;

private:
			void				_SetModel(Model* model);

private:
			BTabView*			fMainTabView;
			GeneralPage*		fGeneralPage;
			TeamsPage*			fTeamsPage;
			ThreadsPage*		fThreadsPage;
			SchedulingPage*		fSchedulingPage;
			WaitObjectsPage*	fWaitObjectsPage;
			Model*				fModel;
			ModelLoader*		fModelLoader;
			SubWindowManager*	fSubWindowManager;
};


#endif	// MAIN_WINDOW_H
