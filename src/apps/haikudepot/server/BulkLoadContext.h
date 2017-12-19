/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BULK_LOAD_CONTEXT_H
#define BULK_LOAD_CONTEXT_H

#include <String.h>
#include <File.h>
#include <Path.h>

#include "AbstractServerProcess.h"
#include "List.h"


typedef enum bulk_load_state {
	BULK_LOAD_INITIAL					= 1,
	BULK_LOAD_REPOSITORY_AND_REFERENCE	= 2,
	BULK_LOAD_PKGS_AND_ICONS			= 3,
	BULK_LOAD_COMPLETE					= 4
} bulk_load_state;


class BulkLoadContext {
public:
								BulkLoadContext();
	virtual						~BulkLoadContext();

			void				StopAllProcesses();

			bulk_load_state		State();
			void				SetState(bulk_load_state value);

			AbstractServerProcess*
								IconProcess();
			void				SetIconProcess(AbstractServerProcess* value);

			AbstractServerProcess*
								RepositoryProcess();
			void				SetRepositoryProcess(
									AbstractServerProcess* value);

			int32				CountPkgProcesses();
			AbstractServerProcess*
								PkgProcessAt(int32 index);
			void				AddPkgProcess(AbstractServerProcess *value);

			void				AddProcessOption(uint32 flag);
			uint32				ProcessOptions();

private:
			bulk_load_state
								fState;

			AbstractServerProcess*
								fIconProcess;
			AbstractServerProcess*
								fRepositoryProcess;
			List<AbstractServerProcess*, true>*
								fPkgProcesses;
			uint32				fProcessOptions;

};


#endif // BULK_LOAD_CONTEXT_H
