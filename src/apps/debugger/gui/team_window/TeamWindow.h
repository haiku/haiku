/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_WINDOW_H
#define TEAM_WINDOW_H


#include <String.h>
#include <Window.h>

#include "SourceView.h"
#include "Function.h"
#include "ImageFunctionsView.h"
#include "ImageListView.h"
#include "StackFrame.h"
#include "StackTraceView.h"
#include "Team.h"
#include "TeamDebugModel.h"
#include "ThreadListView.h"
#include "VariablesView.h"


class BButton;
class BMenuBar;
class BTabView;
class Image;
class RegistersView;
class SourceCode;
class StackFrame;
class VariablesView;


class TeamWindow : public BWindow, ThreadListView::Listener,
	ImageListView::Listener, StackTraceView::Listener,
	ImageFunctionsView::Listener, SourceView::Listener, VariablesView::Listener,
	Team::Listener, TeamDebugModel::Listener, Function::Listener,
	StackFrame::Listener {
public:
	class Listener;

public:
								TeamWindow(TeamDebugModel* debugModel,
									Listener* listener);
								~TeamWindow();

	static	TeamWindow*			Create(TeamDebugModel* debugModel,
									Listener* listener);
									// throws

	virtual	void				DispatchMessage(BMessage* message,
									BHandler* handler);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
	// ThreadListView::Listener
	virtual	void				ThreadSelectionChanged(::Thread* thread);

	// ImageListView::Listener
	virtual	void				ImageSelectionChanged(Image* image);

	// StackTraceView::Listener
	virtual	void				StackFrameSelectionChanged(StackFrame* frame);

	// ImageFunctionsView::Listener
	virtual	void				FunctionSelectionChanged(
									FunctionInstance* function);

	// SourceView::Listener
	virtual	void				SetBreakpointRequested(target_addr_t address,
									bool enabled);
	virtual	void				ClearBreakpointRequested(target_addr_t address);

	// VariablesView::Listener
	virtual	void				StackFrameValueRequested(::Thread* thread,
									StackFrame* stackFrame, Variable* variable,
									TypeComponentPath* path);

	// Team::Listener
	virtual	void				ThreadStateChanged(
									const Team::ThreadEvent& event);
	virtual	void				ThreadCpuStateChanged(
									const Team::ThreadEvent& event);
	virtual	void				ThreadStackTraceChanged(
									const Team::ThreadEvent& event);
	virtual	void				ImageDebugInfoChanged(
									const Team::ImageEvent& event);

	// TeamDebugModel::Listener
	virtual	void				UserBreakpointChanged(
									const TeamDebugModel::BreakpointEvent&
										event);

	// Function::Listener
	virtual	void				FunctionSourceCodeChanged(Function* function);

	// StackFrame::Listener
	virtual	void				StackFrameValueRetrieved(StackFrame* stackFrame,
									Variable* variable,
									TypeComponentPath* path);

			void				_Init();

			void				_SetActiveThread(::Thread* thread);
			void				_SetActiveImage(Image* image);
			void				_SetActiveStackTrace(StackTrace* stackTrace);
			void				_SetActiveStackFrame(StackFrame* frame);
			void				_SetActiveFunction(FunctionInstance* function);
			void				_SetActiveSourceCode(SourceCode* sourceCode);
			void				_UpdateCpuState();
			void				_UpdateRunButtons();
			void				_ScrollToActiveFunction();

			void				_HandleThreadStateChanged(thread_id threadID);
			void				_HandleCpuStateChanged(thread_id threadID);
			void				_HandleStackTraceChanged(thread_id threadID);
			void				_HandleStackFrameValueRetrieved(
									StackFrame* stackFrame, Variable* variable,
									TypeComponentPath* path);
			void				_HandleImageDebugInfoChanged(image_id imageID);
			void				_HandleSourceCodeChanged();
			void				_HandleUserBreakpointChanged(
									target_addr_t address);

private:
			TeamDebugModel*		fDebugModel;
			::Thread*			fActiveThread;
			Image*				fActiveImage;
			StackTrace*			fActiveStackTrace;
			StackFrame*			fActiveStackFrame;
			FunctionInstance*	fActiveFunction;
			SourceCode*			fActiveSourceCode;
			Listener*			fListener;
			BTabView*			fTabView;
			BTabView*			fLocalsTabView;
			ThreadListView*		fThreadListView;
			ImageListView*		fImageListView;
			ImageFunctionsView*	fImageFunctionsView;
			VariablesView*		fVariablesView;
			RegistersView*		fRegistersView;
			StackTraceView*		fStackTraceView;
			SourceView*			fSourceView;
			BButton*			fRunButton;
			BButton*			fStepOverButton;
			BButton*			fStepIntoButton;
			BButton*			fStepOutButton;
			BMenuBar*			fMenuBar;
};


class TeamWindow::Listener {
public:
	virtual						~Listener();

	virtual	void				FunctionSourceCodeRequested(
									FunctionInstance* function) = 0;
	virtual	void				ImageDebugInfoRequested(Image* image) = 0;
	virtual	void				StackFrameValueRequested(::Thread* thread,
									StackFrame* stackFrame, Variable* variable,
									TypeComponentPath* path) = 0;
									// called with team locked
	virtual	void				ThreadActionRequested(thread_id threadID,
									uint32 action) = 0;
	virtual	void				SetBreakpointRequested(target_addr_t address,
									bool enabled) = 0;
	virtual	void				ClearBreakpointRequested(
									target_addr_t address) = 0;
	virtual	bool				TeamWindowQuitRequested() = 0;
};


#endif	// TEAM_WINDOW_H
