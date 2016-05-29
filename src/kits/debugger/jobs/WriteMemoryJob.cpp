/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include "Team.h"
#include "TeamMemory.h"


WriteMemoryJob::WriteMemoryJob(Team* team,
	TeamMemory* teamMemory, target_addr_t address, void* data,
	target_size_t size)
	:
	fKey(data, JOB_TYPE_WRITE_MEMORY),
	fTeam(team),
	fTeamMemory(teamMemory),
	fTargetAddress(address),
	fData(data),
	fSize(size)
{
	fTeamMemory->AcquireReference();
}


WriteMemoryJob::~WriteMemoryJob()
{
	fTeamMemory->ReleaseReference();
}


const JobKey&
WriteMemoryJob::Key() const
{
	return fKey;
}


status_t
WriteMemoryJob::Do()
{
	ssize_t result = fTeamMemory->WriteMemory(fTargetAddress, fData, fSize);
	if (result < 0)
		return result;

	fTeam->NotifyMemoryChanged(fTargetAddress, fSize);

	return B_OK;
}
