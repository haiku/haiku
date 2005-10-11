/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <MediaDefs.h>
#include <Autolock.h>
#include "BufferManager.h"
#include "SharedBufferList.h"
#include "debug.h"

BufferManager::BufferManager()
 :	fSharedBufferList(_shared_buffer_list::Clone()),
	fNextBufferId(1),
	fLocker(new BLocker("buffer manager locker")),
	fBufferInfoMap(new Map<media_buffer_id, buffer_info>)
{
	fSharedBufferListId = area_for(fSharedBufferList);
	ASSERT(fSharedBufferList!=NULL);
	ASSERT(fSharedBufferListId > 0);
}

BufferManager::~BufferManager()
{
	fSharedBufferList->Unmap();
	delete fLocker;
	delete fBufferInfoMap;
}

area_id
BufferManager::SharedBufferListID()
{
	return fSharedBufferListId;
}

status_t	
BufferManager::RegisterBuffer(team_id teamid, media_buffer_id bufferid,
							  size_t *size, int32 *flags, size_t *offset, area_id *area)
{
	BAutolock lock(fLocker);

	TRACE("RegisterBuffer team = %ld, bufferid = %ld\n", teamid, bufferid);
	
	buffer_info *info;
	if (!fBufferInfoMap->Get(bufferid, &info)) {
		ERROR("failed to register buffer! team = %ld, bufferid = %ld\n", teamid, bufferid);
		return B_ERROR;
	}
	
	info->teams.Insert(teamid);
	
	*area 	= info->area;
	*offset	= info->offset;
	*size 	= info->size, 
	*flags 	= info->flags;
	return B_OK;
}

status_t
BufferManager::RegisterBuffer(team_id teamid, size_t size, int32 flags, size_t offset, area_id area,
							  media_buffer_id *bufferid)
{
	BAutolock lock(fLocker);
	TRACE("RegisterBuffer team = %ld, areaid = %ld, offset = %ld, size = %ld\n", teamid, area, offset, size);
	
	void *adr;
	area_id newarea;

	newarea = clone_area("media_server cloned buffer", &adr, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, area);
	if (newarea <= B_OK) {
		ERROR("RegisterBuffer: failed to clone buffer! error = %#lx, team = %ld, areaid = %ld, offset = %ld, size = %ld\n", newarea, teamid, area, offset, size);
		return B_ERROR;
	}

	buffer_info info;

	*bufferid	= fNextBufferId;
	info.id 	= fNextBufferId;
	info.area 	= newarea;
	info.offset = offset;
	info.size 	= size;
	info.flags	= flags;
	info.teams.Insert(teamid);
	fBufferInfoMap->Insert(fNextBufferId, info);

	TRACE("RegisterBuffer: done, bufferid = %ld\n", fNextBufferId);

	fNextBufferId += 1;
	
	return B_OK;
}

status_t
BufferManager::UnregisterBuffer(team_id teamid, media_buffer_id bufferid)
{
	BAutolock lock(fLocker);
	TRACE("UnregisterBuffer: team = %ld, bufferid = %ld\n", teamid, bufferid);

	buffer_info *info;
	int index;
	
	if (!fBufferInfoMap->Get(bufferid, &info)) {
		ERROR("UnregisterBuffer: failed to unregister buffer! team = %ld, bufferid = %ld\n", teamid, bufferid);
		return B_ERROR;
	}

	index = info->teams.Find(teamid);
	if (index < 0) {
		ERROR("UnregisterBuffer: failed to find team = %ld belonging to bufferid = %ld\n", teamid, bufferid);
		return B_ERROR;
	}
	
	if (!info->teams.Remove(index)) {
		ERROR("UnregisterBuffer: failed to remove team = %ld from bufferid = %ld\n", teamid, bufferid);
		return B_ERROR;
	}
	TRACE("UnregisterBuffer: team = %ld removed from bufferid = %ld\n", teamid, bufferid);

	if (info->teams.IsEmpty()) {
	
		if (!fBufferInfoMap->Remove(bufferid)) {
			ERROR("UnregisterBuffer: failed to remove bufferid = %ld\n", bufferid);
			return B_ERROR;
		}
	
		TRACE("UnregisterBuffer: bufferid = %ld removed\n", bufferid);
	}

	return B_OK;
}

void
BufferManager::CleanupTeam(team_id team)
{
	BAutolock lock(fLocker);
	buffer_info *info;
	
	TRACE("BufferManager::CleanupTeam: team %ld\n", team);
	
	for (fBufferInfoMap->Rewind(); fBufferInfoMap->GetNext(&info); ) {
		team_id *otherteam;
		for (info->teams.Rewind(); info->teams.GetNext(&otherteam); ) {
			if (team == *otherteam) {
				PRINT(1, "BufferManager::CleanupTeam: removing team %ld from buffer id %ld\n", team, info->id);
				info->teams.RemoveCurrent();
			}
		}
		if (info->teams.IsEmpty()) {
			PRINT(1, "BufferManager::CleanupTeam: removing buffer id %ld that has no teams\n", info->id);
			fBufferInfoMap->RemoveCurrent(); 
		}
	}
}

void
BufferManager::Dump()
{
	BAutolock lock(fLocker);
	buffer_info *info;
	printf("\n");
	printf("BufferManager: list of buffers follows:\n");
	for (fBufferInfoMap->Rewind(); fBufferInfoMap->GetNext(&info); ) {
		printf(" buffer-id %ld, area-id %ld, offset %ld, size %ld, flags %#08lx\n",
			 info->id, info->area, info->offset, info->size, info->flags);
		printf("   assigned teams: ");
		team_id *team;
		for (info->teams.Rewind(); info->teams.GetNext(&team); ) {
			printf("%ld, ", *team);
		}
		printf("\n");
	}
	printf("BufferManager: list end\n");
}
