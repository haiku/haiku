/*
 * Copyright 2012-2013, Rene Gollent, rene@gollent.com.
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
class AreaInfo;
class BString;
class DebuggerInterface;
class SemaphoreInfo;
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
									UserInterfaceListener* listener,
									DebuggerInterface* interface);
								~DebugReportGenerator();

			status_t			Init();

	static	DebugReportGenerator* Create(::Team* team,
									UserInterfaceListener* listener,
									DebuggerInterface* interface);

	virtual void				MessageReceived(BMessage* message);

private:
	// Team::Listener
	virtual	void				ThreadStackTraceChanged(
									const Team::ThreadEvent& event);

	// TeamMemoryBlock::Listener
	virtual	void				MemoryBlockRetrieved(TeamMemoryBlock* block);
	virtual	void				MemoryBlockRetrievalFailed(
									TeamMemoryBlock* block, status_t result);

	// ValueNodeContainer::Listener
	virtual	void				ValueNodeValueChanged(ValueNode* node);


private:
			status_t			_GenerateReport(const entry_ref& outputPath);
			status_t			_GenerateReportHeader(BString& _output);
			status_t			_DumpLoadedImages(BString& _output);
			status_t			_DumpAreas(BString& _output);
			status_t			_DumpSemaphores(BString& _output);
			status_t			_DumpRunningThreads(BString& _output);
			status_t			_DumpDebuggedThreadInfo(BString& _output,
									::Thread* thread);
			void				_DumpStackFrameMemory(BString& _output,
									CpuState* state,
									target_addr_t framePointer,
									uint8 stackDirection);

			status_t			_ResolveValueIfNeeded(ValueNode* node,
									StackFrame* frame, int32 maxDepth);

			void				_HandleMemoryBlockRetrieved(
									TeamMemoryBlock* block, status_t result);

	static	int					_CompareAreas(const AreaInfo* a,
									const AreaInfo* b);
	static	int					_CompareImages(const Image* a, const Image* b);
	static	int					_CompareSemaphores(const SemaphoreInfo* a,
									const SemaphoreInfo* b);
	static	int					_CompareThreads(const ::Thread* a,
									const ::Thread* b);
private:
			::Team*				fTeam;
			Architecture*		fArchitecture;
			DebuggerInterface*	fDebuggerInterface;
			sem_id				fTeamDataSem;
			ValueNodeManager*	fNodeManager;
			UserInterfaceListener* fListener;
			ValueNode*			fWaitingNode;
			TeamMemoryBlock*	fCurrentBlock;
			status_t			fBlockRetrievalStatus;
			::Thread*			fTraceWaitingThread;
};

#endif // DEBUG_REPORT_GENERATOR_H
