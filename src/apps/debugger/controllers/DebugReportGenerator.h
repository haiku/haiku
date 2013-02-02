/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUG_REPORT_GENERATOR_H
#define DEBUG_REPORT_GENERATOR_H


#include <Looper.h>

#include "Team.h"
#include "TeamMemoryBlock.h"
#include "ValueNodeContainer.h"


class entry_ref;
class Architecture;
class BString;
class StackFrame;
class Team;
class Thread;
class UserInterfaceListener;
class Value;
class ValueNode;
class ValueNodeChild;
class ValueNodeManager;


class DebugReportGenerator : public BLooper, private Team::Listener,
	private TeamMemoryBlock::Listener, private ValueNodeContainer::Listener {
public:
								DebugReportGenerator(::Team* team,
									UserInterfaceListener* listener);
								~DebugReportGenerator();

			status_t			Init();

	static	DebugReportGenerator* Create(::Team* team,
									UserInterfaceListener* listener);

	virtual void				MessageReceived(BMessage* message);

private:
	// Team::Listener
	virtual	void				ThreadStackTraceChanged(
									const Team::ThreadEvent& event);

	// TeamMemoryBlock::Listener
	virtual	void				MemoryBlockRetrieved(TeamMemoryBlock* block);

	// ValueNodeContainer::Listener
	virtual	void				ValueNodeValueChanged(ValueNode* node);


private:
			status_t			_GenerateReport(const entry_ref& outputPath);
			status_t			_GenerateReportHeader(BString& _output);
			status_t			_DumpLoadedImages(BString& _output);
			status_t			_DumpRunningThreads(BString& _output);
			status_t			_DumpDebuggedThreadInfo(BString& _output,
									::Thread* thread);
			void				_DumpStackFrameMemory(BString& _output,
									CpuState* state,
									target_addr_t framePointer,
									uint8 stackDirection);

			status_t			_ResolveValueIfNeeded(ValueNode* node,
									StackFrame* frame, int32 maxDepth);

private:
			::Team*				fTeam;
			Architecture*		fArchitecture;
			sem_id				fTeamDataSem;
			ValueNodeManager*	fNodeManager;
			UserInterfaceListener* fListener;
			ValueNode*			fWaitingNode;
			TeamMemoryBlock*	fCurrentBlock;
			::Thread*			fTraceWaitingThread;
};

#endif // DEBUG_REPORT_GENERATOR_H
