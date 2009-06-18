/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_WINDOW_H
#define TEAM_WINDOW_H

#include <String.h>
#include <Window.h>

#include "ThreadListView.h"


class BButton;
class BTabView;
class ImageListView;
class Team;
class TeamDebugModel;


class TeamWindow : public BWindow, private ThreadListView::Listener {
public:
	class Listener;

public:
								TeamWindow(TeamDebugModel* debugModel,
									Listener* listener);
								~TeamWindow();

	static	TeamWindow*			Create(TeamDebugModel* debugModel,
									Listener* listener);
									// throws

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
	// ThreadListView::Listener
	virtual	void				ThreadSelectionChanged(::Thread* thread);

			void				_Init();

			void				_SetActiveThread(::Thread* thread);
			void				_UpdateRunButtons();

private:
			TeamDebugModel*		fDebugModel;
			::Thread*			fActiveThread;
			Listener*			fListener;
			BTabView*			fTabView;
			ThreadListView*		fThreadListView;
			ImageListView*		fImageListView;
			BButton*			fRunButton;
			BButton*			fStepOverButton;
			BButton*			fStepIntoButton;
			BButton*			fStepOutButton;
};


class TeamWindow::Listener {
public:
	virtual						~Listener();

	virtual	bool				TeamWindowQuitRequested(TeamWindow* window);
};


#endif	// TEAM_WINDOW_H
