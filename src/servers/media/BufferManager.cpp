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
	fSharedBufferListId(-1),
	fNextBufferId(1),
	fLocker(new BLocker("buffer manager locker")),
	fBufferInfoMap(new Map<media_buffer_id, buffer_info>)
{
	fSharedBufferListId = area_for(fSharedBufferList);
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

	TRACE("RegisterBuffer team = 0x%08x, bufferid = 0x%08x\n",(int)teamid,(int)bufferid);
	
	buffer_info *info;
	if (!fBufferInfoMap->GetPointer(bufferid, &info)) {
		FATAL("failed to register buffer! team = 0x%08x, bufferid = 0x%08x\n",(int)teamid,(int)bufferid);
		PrintToStream();
		return B_ERROR;
	}
	
	info->teams.Insert(teamid);
	
	*area 	= info->area;
	*offset	= info->offset;
	*size 	= info->size, 
	*flags 	= info->flags;

	PrintToStream();
	return B_OK;
}

status_t
BufferManager::RegisterBuffer(team_id teamid, size_t size, int32 flags, size_t offset, area_id area,
							  media_buffer_id *bufferid)
{
	BAutolock lock(fLocker);
	TRACE("RegisterBuffer team = 0x%08x, areaid = 0x%08x, offset = 0x%08x, size = 0x%08x\n",(int)teamid,(int)area,(int)offset,(int)size);
	
	void *adr;
	area_id newarea;

	newarea = clone_area("media_server cloned buffer", &adr, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, area);
	if (newarea <= B_OK) {
		FATAL("RegisterBuffer: failed to clone buffer! team = 0x%08x, areaid = 0x%08x, offset = 0x%08x, size = 0x%08x\n",(int)teamid,(int)area,(int)offset,(int)size);
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

	TRACE("RegisterBuffer: done, bufferid = 0x%08x\n", fNextBufferId);

	fNextBufferId += 1;
	
	PrintToStream();
	return B_OK;
}

status_t
BufferManager::UnregisterBuffer(team_id teamid, media_buffer_id bufferid)
{
	BAutolock lock(fLocker);
	TRACE("UnregisterBuffer: team = 0x%08x bufferid = 0x%08x\n",(int)teamid,(int)bufferid);

	buffer_info *info;
	int index;
	
	if (!fBufferInfoMap->GetPointer(bufferid, &info)) {
		FATAL("UnregisterBuffer: failed to unregister buffer! team = 0x%08x, bufferid = 0x%08x\n",(int)teamid,(int)bufferid);
		PrintToStream();
		return B_ERROR;
	}

	index = info->teams.Find(teamid);
	if (index < 0) {
		FATAL("UnregisterBuffer: failed to find team = 0x%08x from bufferid = 0x%08x\n",(int)teamid,(int)bufferid);
		PrintToStream();
		return B_ERROR;
	}
	
	if (!info->teams.Remove(index)) {
		FATAL("UnregisterBuffer: failed to remove team = 0x%08x from bufferid = 0x%08x\n",(int)teamid,(int)bufferid);
		PrintToStream();
		return B_ERROR;
	}
	TRACE("UnregisterBuffer: team = 0x%08x removed from bufferid = 0x%08x\n",(int)teamid,(int)bufferid);

	if (info->teams.IsEmpty()) {
	
		if (!fBufferInfoMap->Remove(bufferid)) {
			FATAL("UnregisterBuffer: failed to remove bufferid = 0x%08x\n",(int)bufferid);
			PrintToStream();
			return B_ERROR;
		}
	
		TRACE("UnregisterBuffer: bufferid = 0x%08x removed\n",(int)bufferid);
	}

	return B_OK;
}

void
BufferManager::CleanupTeam(team_id team)
{
	FATAL("BufferManager::CleanupTeam: should cleanup team %ld\n", team);
}


void
BufferManager::PrintToStream()
{
	return;
/*
	BAutolock lock(fLocker);
	_buffer_list *list;
	_team_list *team;
	
	printf("Buffer list:\n");
	for (list = fBufferList; list; list = list->next) {
		printf(" bufferid = 0x%08x, areaid = 0x%08x, offset = 0x%08x, size = 0x%08x, flags = 0x%08x, next = 0x%08x\n",
			(int)list->id,(int)list->area,(int)list->offset,(int)list->size,(int)list->flags,(int)list->next);
		for (team = list->teams; team; team = team->next)
			printf("   team = 0x%08x, next = 0x%08x =>",(int)team->team,(int)team->next);
		printf("\n");
	}
	*/
}
