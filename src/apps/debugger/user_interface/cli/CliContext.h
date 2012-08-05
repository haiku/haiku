/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_CONTEXT_H
#define CLI_CONTEXT_H


#include <sys/cdefs.h>
	// Needed in histedit.h.
#include <histedit.h>

#include <Locker.h>

#include "Team.h"


class Team;
class UserInterfaceListener;


class CliContext : private Team::Listener {
public:
			enum {
				EVENT_QUIT					= 0x01,
				EVENT_USER_INTERRUPT		= 0x02,
				EVENT_THREAD_ADDED			= 0x04,
				EVENT_THREAD_REMOVED		= 0x08,
				EVENT_THREAD_STOPPED		= 0x10,
			};

public:
								CliContext();
								~CliContext();

			status_t			Init(Team* team,
									UserInterfaceListener* listener);
			void				Cleanup();

			void				Terminating();

			bool				IsTerminating() const	{ return fTerminating; }

			// service methods for the input loop thread follow

			Team*				GetTeam() const	{ return fTeam; }
			UserInterfaceListener* GetUserInterfaceListener() const
									{ return fListener; }

			Thread*				CurrentThread() const { return fCurrentThread; }
			thread_id			CurrentThreadID() const;
			void				SetCurrentThread(Thread* thread);
			void				PrintCurrentThread();

			const char*			PromptUser(const char* prompt);
			void				AddLineToInputHistory(const char* line);

			void				QuitSession(bool killTeam);

			void				WaitForThreadOrUser();
			void				ProcessPendingEvents();

private:
			struct Event;

			typedef DoublyLinkedList<Event> EventList;

private:
	// Team::Listener
	virtual	void				ThreadAdded(const Team::ThreadEvent& event);
	virtual	void				ThreadRemoved(const Team::ThreadEvent& event);

	virtual	void				ThreadStateChanged(
									const Team::ThreadEvent& event);

private:
			void				_QueueEvent(Event* event);

			void				_PrepareToWaitForEvents(uint32 eventMask);
			uint32				_WaitForEvents();
			void				_SignalInputLoop(uint32 events);

	static	const char*			_GetPrompt(EditLine* editLine);

private:
			BLocker				fLock;
			Team*				fTeam;
			UserInterfaceListener* fListener;
			EditLine*			fEditLine;
			History*			fHistory;
			const char*			fPrompt;
			sem_id				fBlockingSemaphore;
			uint32				fInputLoopWaitingForEvents;
			uint32				fEventsOccurred;
			bool				fInputLoopWaiting;
	volatile bool				fTerminating;

			Thread*				fCurrentThread;

			EventList			fPendingEvents;
};


#endif	// CLI_CONTEXT_H
