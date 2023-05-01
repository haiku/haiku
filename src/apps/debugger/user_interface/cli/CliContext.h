/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_CONTEXT_H
#define CLI_CONTEXT_H


#include <sys/cdefs.h>
	// Needed in histedit.h.
#include <histedit.h>

#include <Locker.h>
#include <Looper.h>

#include "ExpressionInfo.h"
#include "Team.h"
#include "TeamMemoryBlock.h"
#include "ValueNodeContainer.h"


class SourceLanguage;
class StackFrame;
class StackTrace;
class Team;
class TeamMemoryBlock;
class UserInterfaceListener;
class ValueNodeManager;


class CliContext : private Team::Listener,
	public TeamMemoryBlock::Listener,
	public ExpressionInfo::Listener,
	private ValueNodeContainer::Listener,
	public BLooper {
public:
			enum {
				MSG_QUIT							= 'quit',
				MSG_USER_INTERRUPT					= 'uint',
				MSG_THREAD_ADDED					= 'thad',
				MSG_THREAD_REMOVED					= 'thar',
				MSG_THREAD_STATE_CHANGED			= 'tsch',
				MSG_THREAD_STACK_TRACE_CHANGED		= 'tstc',
				MSG_VALUE_NODE_CHANGED				= 'vnch',
				MSG_TEAM_MEMORY_BLOCK_RETRIEVED		= 'tmbr',
				MSG_EXPRESSION_EVALUATED			= 'exev',
				MSG_DEBUG_REPORT_CHANGED			= 'drch',
				MSG_CORE_FILE_CHANGED				= 'cfch'
			};

public:
								CliContext();
								~CliContext();

			status_t			Init(::Team* team,
									UserInterfaceListener* listener);
			void				Cleanup();

			void				Terminating();

			bool				IsTerminating() const	{ return fTerminating; }

	virtual	void				MessageReceived(BMessage* message);

			// service methods for the input loop thread follow

			::Team*				GetTeam() const	{ return fTeam; }
			UserInterfaceListener* GetUserInterfaceListener() const
									{ return fListener; }
			ValueNodeManager*	GetValueNodeManager() const
									{ return fNodeManager; }
			StackTrace*			GetStackTrace() const
									{ return fCurrentStackTrace; }

			::Thread*			CurrentThread() const { return fCurrentThread; }
			thread_id			CurrentThreadID() const;
			void				SetCurrentThread(::Thread* thread);
			void				PrintCurrentThread();

			int32				CurrentStackFrameIndex() const
									{ return fCurrentStackFrameIndex; }
			void				SetCurrentStackFrameIndex(int32 index);

			status_t			EvaluateExpression(const char * expression,
									SourceLanguage* language, target_addr_t& address);

			status_t			GetMemoryBlock(target_addr_t address,
									TeamMemoryBlock*& block);

			const char*			PromptUser(const char* prompt);
			void				AddLineToInputHistory(const char* line);

			void				QuitSession(bool killTeam);

			void				WaitForThreadOrUser();
			void				WaitForEvent(uint32 event);

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

	virtual	void				DebugReportChanged(
									const Team::DebugReportEvent& event);

	virtual	void				CoreFileChanged(
									const Team::CoreFileChangedEvent& event);

	// TeamMemoryBlock::Listener
	virtual void				MemoryBlockRetrieved(TeamMemoryBlock* block);

	// ExpressionInfo::Listener
	virtual	void				ExpressionEvaluated(ExpressionInfo* info,
									status_t result, ExpressionResult* value);

	// ValueNodeContainer::Listener
	virtual	void				ValueNodeChanged(ValueNodeChild* nodeChild,
									ValueNode* oldNode, ValueNode* newNode);
	virtual	void				ValueNodeChildrenCreated(ValueNode* node);
	virtual	void				ValueNodeChildrenDeleted(ValueNode* node);
	virtual	void				ValueNodeValueChanged(ValueNode* node);

private:
	static	const char*			_GetPrompt(EditLine* editLine);
			void				_WaitForEvent(uint32 event);

private:
	mutable	BLocker				fLock;
			::Team*				fTeam;
			UserInterfaceListener* fListener;
			ValueNodeManager*	fNodeManager;
			EditLine*			fEditLine;
			History*			fHistory;
			const char*			fPrompt;
			sem_id				fWaitForEventSemaphore;
			uint32				fEventOccurred;
	volatile bool				fTerminating;

			BReference< ::Thread>	fStoppedThread;
			::Thread*			fCurrentThread;
			StackTrace*			fCurrentStackTrace;
			int32				fCurrentStackFrameIndex;
			TeamMemoryBlock*	fCurrentBlock;

			ExpressionInfo*		fExpressionInfo;
			status_t			fExpressionResult;
			ExpressionResult*	fExpressionValue;
};


#endif	// CLI_CONTEXT_H
