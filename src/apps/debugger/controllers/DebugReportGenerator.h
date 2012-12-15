/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUG_REPORT_GENERATOR_H
#define DEBUG_REPORT_GENERATOR_H


#include <Looper.h>

#include "Team.h"
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
	private ValueNodeContainer::Listener {
public:
								DebugReportGenerator(::Team* team,
									UserInterfaceListener* listener);
								~DebugReportGenerator();

			status_t			Init();

	static	DebugReportGenerator* Create(::Team* team,
									UserInterfaceListener* listener);

	virtual void				MessageReceived(BMessage* message);

	virtual	void				ThreadStackTraceChanged(
									const Team::ThreadEvent& event);

	virtual	void				ValueNodeValueChanged(ValueNode* node);

private:
			status_t			_GenerateReport(const entry_ref& outputPath);
			status_t			_GenerateReportHeader(BString& output);
			status_t			_DumpLoadedImages(BString& output);
			status_t			_DumpRunningThreads(BString& output);
			status_t			_DumpDebuggedThreadInfo(BString& output,
									::Thread* thread);

			status_t			_ResolveLocationIfNeeded(ValueNodeChild* child,
									StackFrame* frame);
			status_t			_ResolveValueIfNeeded(ValueNode* node,
									StackFrame* frame, int32 maxDepth);

private:
			::Team*				fTeam;
			Architecture*		fArchitecture;
			sem_id				fTeamDataSem;
			ValueNodeManager*	fNodeManager;
			UserInterfaceListener* fListener;
			ValueNode*			fWaitingNode;
			::Thread*			fTraceWaitingThread;
};

#endif // DEBUG_REPORT_GENERATOR_H
