/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <Path.h>

#include <AutoLocker.h>

#include "DebuggerInterface.h"
#include "Team.h"



WriteCoreFileJob::WriteCoreFileJob(Team* team,
	DebuggerInterface* interface, const entry_ref& path)
	:
	fKey(&path, JOB_TYPE_WRITE_CORE_FILE),
	fTeam(team),
	fDebuggerInterface(interface),
	fTargetPath(path)
{
	fDebuggerInterface->AcquireReference();
}


WriteCoreFileJob::~WriteCoreFileJob()
{
	fDebuggerInterface->AcquireReference();
}


const JobKey&
WriteCoreFileJob::Key() const
{
	return fKey;
}


status_t
WriteCoreFileJob::Do()
{
	BPath path(&fTargetPath);
	status_t error = path.InitCheck();
	if (error != B_OK)
		return error;

	error = fDebuggerInterface->WriteCoreFile(path.Path());
	if (error != B_OK)
		return error;

	AutoLocker< ::Team> teamLocker(fTeam);
	fTeam->NotifyCoreFileChanged(path.Path());

	return B_OK;
}
