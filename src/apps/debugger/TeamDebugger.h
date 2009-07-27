/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_DEBUGGER_H
#define TEAM_DEBUGGER_H


#include <debugger.h>
#include <Looper.h>

#include <debug_support.h>

#include "DebugEvent.h"
#include "Team.h"
#include "TeamWindow.h"
#include "ThreadHandler.h"
#include "Worker.h"


class DebuggerInterface;
class FileManager;
class SettingsManager;
class TeamDebugInfo;


class TeamDebugger : public BLooper, private TeamWindow::Listener,
	private JobListener, private Team::Listener {
public:
	class Listener;

public:
								TeamDebugger(Listener* listener,
									SettingsManager* settingsManager);
								~TeamDebugger();

			status_t			Init(team_id teamID, thread_id threadID,
									bool stopInMain);

			team_id				TeamID() const	{ return fTeamID; }

	virtual	void				MessageReceived(BMessage* message);

private:
	// TeamWindow::Listener
	virtual	void				FunctionSourceCodeRequested(
									FunctionInstance* function);
	virtual	void				ImageDebugInfoRequested(Image* image);
	virtual	void				StackFrameValueRequested(::Thread* thread,
									StackFrame* stackFrame, Variable* variable,
									TypeComponentPath* path);
	virtual	void				ThreadActionRequested(thread_id threadID,
									uint32 action);
	virtual	void				SetBreakpointRequested(target_addr_t address,
									bool enabled);
	virtual	void				ClearBreakpointRequested(target_addr_t address);
	virtual	bool				TeamWindowQuitRequested();

	// JobListener
	virtual	void				JobDone(Job* job);
	virtual	void				JobFailed(Job* job);
	virtual	void				JobAborted(Job* job);

	// Team::Listener
	virtual	void				ThreadStateChanged(
									const ::Team::ThreadEvent& event);
	virtual	void				ThreadCpuStateChanged(
									const ::Team::ThreadEvent& event);
	virtual	void				ThreadStackTraceChanged(
									const ::Team::ThreadEvent& event);
	virtual	void				ImageDebugInfoChanged(
									const ::Team::ImageEvent& event);

private:
	struct ImageHandler;
	struct ImageHandlerHashDefinition;
	typedef BOpenHashTable<ImageHandlerHashDefinition> ImageHandlerTable;

private:
	static	status_t			_DebugEventListenerEntry(void* data);
			status_t			_DebugEventListener();

			void				_HandleDebuggerMessage(DebugEvent* event);

			bool				_HandleThreadCreated(
									ThreadCreatedEvent* event);
			bool				_HandleThreadDeleted(
									ThreadDeletedEvent* event);
			bool				_HandleImageCreated(
									ImageCreatedEvent* event);
			bool				_HandleImageDeleted(
									ImageDeletedEvent* event);

			void				_HandleImageDebugInfoChanged(image_id imageID);
			void				_HandleImageFileChanged(image_id imageID);

			void				_HandleSetUserBreakpoint(target_addr_t address,
									bool enabled);
			void				_HandleClearUserBreakpoint(
									target_addr_t address);

			ThreadHandler*		_GetThreadHandler(thread_id threadID);

			status_t			_AddImage(const ImageInfo& imageInfo,
									Image** _image = NULL);

			void				_LoadSettings();
			void				_SaveSettings();

			void				_NotifyUser(const char* title,
									const char* text,...);

private:
			Listener*			fListener;
			SettingsManager*	fSettingsManager;
			::Team*				fTeam;
			team_id				fTeamID;
			ThreadHandlerTable	fThreadHandlers;
									// protected by the team lock
			ImageHandlerTable*	fImageHandlers;
			DebuggerInterface*	fDebuggerInterface;
			TeamDebugInfo*		fTeamDebugInfo;
			FileManager*		fFileManager;
			Worker*				fWorker;
			BreakpointManager*	fBreakpointManager;
			thread_id			fDebugEventListener;
			TeamWindow*			fTeamWindow;
	volatile bool				fTerminating;
			bool				fKillTeamOnQuit;
};


class TeamDebugger::Listener {
public:
	virtual						~Listener();

	virtual	void				TeamDebuggerQuit(TeamDebugger* debugger) = 0;
};


#endif	// TEAM_DEBUGGER_H
