/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_WINDOW_H
#define TEAM_WINDOW_H

#include <String.h>
#include <Window.h>

#include "StackTraceView.h"
#include "Team.h"
#include "ThreadListView.h"


class BButton;
class BTabView;
class ImageListView;
class RegisterView;
class TeamDebugModel;


class TeamWindow : public BWindow, private ThreadListView::Listener,
	StackTraceView::Listener, Team::Listener {
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

	// StackTraceView::Listener
	virtual	void				StackFrameSelectionChanged(StackFrame* frame);

	// Team::Listener
	virtual	void				ThreadStateChanged(
									const Team::ThreadEvent& event);
	virtual	void				ThreadCpuStateChanged(
									const Team::ThreadEvent& event);
	virtual	void				ThreadStackTraceChanged(
									const Team::ThreadEvent& event);

			void				_Init();

			void				_SetActiveThread(::Thread* thread);
			void				_SetActiveStackFrame(StackFrame* frame);
			void				_UpdateCpuState();
			void				_UpdateRunButtons();

			void				_HandleThreadStateChanged(thread_id threadID);
			void				_HandleCpuStateChanged(thread_id threadID);
			void				_HandleStackTraceChanged(thread_id threadID);

private:
			TeamDebugModel*		fDebugModel;
			::Thread*			fActiveThread;
			StackFrame*			fActiveStackFrame;
			Listener*			fListener;
			BTabView*			fTabView;
			BTabView*			fLocalsTabView;
			ThreadListView*		fThreadListView;
			ImageListView*		fImageListView;
			RegisterView*		fRegisterView;
			StackTraceView*		fStackTraceView;
			BButton*			fRunButton;
			BButton*			fStepOverButton;
			BButton*			fStepIntoButton;
			BButton*			fStepOutButton;
};


class TeamWindow::Listener {
public:
	virtual						~Listener();

	virtual	void				ThreadActionRequested(TeamWindow* window,
									thread_id threadID, uint32 action) = 0;
	virtual	bool				TeamWindowQuitRequested(TeamWindow* window) = 0;
};


#endif	// TEAM_WINDOW_H
