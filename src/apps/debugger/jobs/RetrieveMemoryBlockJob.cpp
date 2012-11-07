/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <AutoLocker.h>
#include <memory_private.h>

#include "Team.h"
#include "TeamMemory.h"
#include "TeamMemoryBlock.h"


RetrieveMemoryBlockJob::RetrieveMemoryBlockJob(Team* team,
	TeamMemory* teamMemory, TeamMemoryBlock* memoryBlock)
	:
	fKey(memoryBlock, JOB_TYPE_GET_MEMORY_BLOCK),
	fTeam(team),
	fTeamMemory(teamMemory),
	fMemoryBlock(memoryBlock)
{
	fTeamMemory->AcquireReference();
	fMemoryBlock->AcquireReference();
}


RetrieveMemoryBlockJob::~RetrieveMemoryBlockJob()
{
	fTeamMemory->ReleaseReference();
	fMemoryBlock->ReleaseReference();
}


const JobKey&
RetrieveMemoryBlockJob::Key() const
{
	return fKey;
}


status_t
RetrieveMemoryBlockJob::Do()
{
	ssize_t result = fTeamMemory->ReadMemory(fMemoryBlock->BaseAddress(),
		fMemoryBlock->Data(), fMemoryBlock->Size());
	if (result < 0)
		return result;

	uint32 protection = 0;
	uint32 locking = 0;
	status_t error = get_memory_properties(fTeam->ID(),
		(const void *)fMemoryBlock->BaseAddress(), &protection, &locking);
	if (error != B_OK)
		return error;

	fMemoryBlock->SetWritable((protection & B_WRITE_AREA) != 0);
	fMemoryBlock->MarkValid();
	return B_OK;
}
