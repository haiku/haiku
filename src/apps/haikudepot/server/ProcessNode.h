/*
 * Copyright 2018-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef PROCESS_NODE_H
#define PROCESS_NODE_H

#include <ObjectList.h>
#include <OS.h>


class AbstractProcess;


/*! This class is designed to be used by the ProcessCoordinator class.  The
    purpose of the class is to hold a process and also any dependent processes
    of this one.  This effectively creates a dependency tree of processes.  This
    class is also able to start and stop threads that run the process.
*/

class ProcessNode {
public:
								ProcessNode(AbstractProcess* process);
	virtual						~ProcessNode();

			AbstractProcess*	Process() const;
			status_t			StartProcess();
			status_t			StopProcess();

			void				AddPredecessor(ProcessNode* node);
			int32				CountPredecessors() const;
			ProcessNode*		PredecessorAt(int32 index) const;
			bool				AllPredecessorsComplete() const;

			int32				CountSuccessors() const;
			ProcessNode*		SuccessorAt(int32 index) const;

private:
	static	status_t			_StartProcess(void* cookie);
			status_t			_SpinUntilProcessState(
									uint32 desiredStatesMask,
									uint32 timeoutSeconds);
			void				_AddSuccessor(ProcessNode* node);

			thread_id			fWorker;
			AbstractProcess*	fProcess;
			BObjectList<ProcessNode>
								fPredecessorNodes;
			BObjectList<ProcessNode>
								fSuccessorNodes;
};


#endif // PROCESS_TREE_NODE_H
