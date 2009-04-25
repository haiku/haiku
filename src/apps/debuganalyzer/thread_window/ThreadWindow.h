/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_WINDOW_H
#define THREAD_WINDOW_H

#include "Model.h"
#include "SubWindow.h"


class BTabView;


class ThreadWindow : public SubWindow {
public:
								ThreadWindow(SubWindowManager* manager,
									Model* model, Model::Thread* thread);
	virtual						~ThreadWindow();

//	virtual	void				MessageReceived(BMessage* message);

//	virtual	void				Quit();

//	virtual	void				Show();

private:
			class GeneralPage;
			class WaitObjectsPage;

private:
			void				_SetModel(Model* model);

private:
			BTabView*			fMainTabView;
			GeneralPage*		fGeneralPage;
			WaitObjectsPage*	fWaitObjectsPage;
			Model*				fModel;
			Model::Thread*		fThread;
};



#endif	// THREAD_WINDOW_H
