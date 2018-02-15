/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BULK_LOAD_STATE_MACHINE_H
#define BULK_LOAD_STATE_MACHINE_H

#include <String.h>
#include <File.h>
#include <Locker.h>
#include <Path.h>

#include "AbstractServerProcess.h"
#include "Model.h"


class BulkLoadStateMachine : public AbstractServerProcessListener {
public:
								BulkLoadStateMachine(Model* model);
	virtual						~BulkLoadStateMachine();

			bool				IsRunning();

			void				Start();
			void				Stop();

			void				ServerProcessExited();

private:
	static	status_t			StartProcess(void* cookie);
			void				ContextPoll();
			void				SetContextState(bulk_load_state state);
			void				StopAllProcesses();

			bool				CanTransitionTo(
									bulk_load_state targetState);

	static	void				InitiatePopulatePackagesForDepotCallback(
									const DepotInfo& depotInfo,
									void* context);

			status_t			InitiateServerProcess(
									AbstractServerProcess* process);
			status_t			InitiateBulkRepositories();
			status_t			InitiateBulkPopulateIcons();
			status_t			InitiateBulkPopulatePackagesForDepot(
									const DepotInfo& depotInfo);
			void				InitiateBulkPopulatePackagesForAllDepots();


private:
			BLocker				fLock;
			BulkLoadContext*	fBulkLoadContext;
			Model*				fModel;

};


#endif // BULK_LOAD_STATE_MACHINE_H
