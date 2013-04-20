/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "SemaphoreInfo.h"


SemaphoreInfo::SemaphoreInfo()
	:
	fTeam(-1),
	fSemaphore(-1),
	fName(),
	fCount(0),
	fLatestHolder(-1)
{
}


SemaphoreInfo::SemaphoreInfo(const SemaphoreInfo &other)
	:
	fTeam(other.fTeam),
	fSemaphore(other.fSemaphore),
	fName(other.fName),
	fCount(other.fCount),
	fLatestHolder(other.fLatestHolder)
{
}


SemaphoreInfo::SemaphoreInfo(team_id team, sem_id semaphore,
	const BString& name, int32 count, thread_id latestHolder)
	:
	fTeam(team),
	fSemaphore(semaphore),
	fName(name),
	fCount(count),
	fLatestHolder(latestHolder)
{
}


void
SemaphoreInfo::SetTo(team_id team, sem_id semaphore, const BString& name,
	int32 count, thread_id latestHolder)
{
	fTeam = team;
	fSemaphore = semaphore;
	fName = name;
	fCount = count;
	fLatestHolder = latestHolder;
}
