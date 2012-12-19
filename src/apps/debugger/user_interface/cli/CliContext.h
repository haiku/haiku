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
#include "TeamMemoryBlock.h"
#include "ValueNodeContainer.h"


class StackFrame;
class StackTrace;
class Team;
class TeamMemoryBlock;
class UserInterfaceListener;
class ValueNodeManager;


class CliContext : private Team::Listener,
	public TeamMemoryBlock::Listener,
	private ValueNodeContainer::Listener {
public:
			enum {
				EVENT_QUIT							= 0x01,
				EVENT_USER_INTERRUPT				= 0x02,
				EVENT_THREAD_ADDED					= 0x04,
				EVENT_THREAD_REMOVED				= 0x08,
				EVENT_THREAD_STOPPED				= 0x10,
				EVENT_THREAD_STACK_TRACE_CHANGED	= 0x20,
				EVENT_VALUE_NODE_CHANGED			= 0x40,
				EVENT_TEAM_MEMORY_BLOCK_RETRIEVED	= 0x80
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
			ValueNodeManager*	GetValueNodeManager() const
									{ return fNodeManager; }
			StackTrace*			GetStackTrace() const
									{ return fCurrentStackTrace; }

			Thread*				CurrentThread() const { return fCurrentThread; }
			thread_id			CurrentThreadID() const;
			void				SetCurrentThread(Thread* thread);
			void				PrintCurrentThread();

			int32				CurrentStackFrameIndex() const
									{ return fCurrentStackFrameIndex; }
			void				SetCurrentStackFrameIndex(int32 index);

			TeamMemoryBlock*	CurrentBlock() const { return fCurrentBlock; }

			const char*			PromptUser(const char* prompt);
			void				AddLineToInputHistory(const char* line);

			void				QuitSession(bool killTeam);

			void				WaitForThreadOrUser();
			void				WaitForEvents(int32 eventMask);
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
	virtual	void				ThreadStackTraceChanged(
									const Team::ThreadEvent& event);

	// TeamMemoryBlock::Listener
	virtual void				MemoryBlockRetrieved(TeamMemoryBlock* block);

	// ValueNodeContainer::Listener
	virtual	void				ValueNodeChanged(ValueNodeChild* nodeChild,
									ValueNode* oldNode, ValueNode* newNode);
	virtual	void				ValueNodeChildrenCreated(ValueNode* node);
	virtual	void				ValueNodeChildrenDeleted(ValueNode* node);
	virtual	void				ValueNodeValueChanged(ValueNode* node);

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
			ValueNodeManager*	fNodeManager;
			EditLine*			fEditLine;
			History*			fHistory;
			const char*			fPrompt;
			sem_id				fBlockingSemaphore;
			uint32				fInputLoopWaitingForEvents;
			uint32				fEventsOccurred;
			bool				fInputLoopWaiting;
	volatile bool				fTerminating;

			Thread*				fCurrentThread;
			StackTrace*			fCurrentStackTrace;
			int32				fCurrentStackFrameIndex;
			TeamMemoryBlock*	fCurrentBlock;

			EventList			fPendingEvents;
};


#endif	// CLI_CONTEXT_H
