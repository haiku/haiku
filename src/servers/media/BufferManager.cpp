/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "BufferManager.h"

#include <Autolock.h>

#include "debug.h"
#include "SharedBufferList.h"


BufferManager::BufferManager()
	:
	fSharedBufferList(_shared_buffer_list::Clone()),
	fNextBufferID(1),
	fLocker("buffer manager locker")
{
	fSharedBufferListID = area_for(fSharedBufferList);
}


BufferManager::~BufferManager()
{
	fSharedBufferList->Unmap();
}


area_id
BufferManager::SharedBufferListID()
{
	return fSharedBufferListID;
}

status_t	
BufferManager::RegisterBuffer(team_id team, media_buffer_id bufferID,
	size_t* _size, int32* _flags, size_t* _offset, area_id* _area)
{
	BAutolock lock(fLocker);

	TRACE("RegisterBuffer team = %ld, bufferid = %ld\n", team, bufferID);

	buffer_info* info;
	if (!fBufferInfoMap.Get(bufferID, &info)) {
		ERROR("failed to register buffer! team = %ld, bufferid = %ld\n", team,
			bufferID);
		return B_ERROR;
	}

	info->teams.Insert(team);

	*_area = info->area;
	*_offset = info->offset;
	*_size = info->size, 
	*_flags = info->flags;

	return B_OK;
}


status_t
BufferManager::RegisterBuffer(team_id team, size_t size, int32 flags,
	size_t offset, area_id area, media_buffer_id* _bufferID)
{
	BAutolock lock(fLocker);
	TRACE("RegisterBuffer team = %ld, area = %ld, offset = %ld, size = %ld\n",
		team, area, offset, size);
	
	void* address;
	area_id clonedArea = clone_area("media_server cloned buffer", &address,
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, area);
	if (clonedArea < 0) {
		ERROR("RegisterBuffer: failed to clone buffer! error = %#lx, team = "
			"%ld, areaid = %ld, offset = %ld, size = %ld\n", clonedArea, team,
			area, offset, size);
		return B_ERROR;
	}

	buffer_info info;
	info.id = fNextBufferID++;
	info.area = clonedArea;
	info.offset = offset;
	info.size = size;
	info.flags = flags;
	info.teams.Insert(team);

	*_bufferID = info.id;

	fBufferInfoMap.Insert(info.id, info);

	TRACE("RegisterBuffer: done, bufferID = %ld\n", info.id);

	return B_OK;
}


status_t
BufferManager::UnregisterBuffer(team_id team, media_buffer_id bufferID)
{
	BAutolock lock(fLocker);
	TRACE("UnregisterBuffer: team = %ld, bufferid = %ld\n", team, bufferID);

	buffer_info* info;
	if (!fBufferInfoMap.Get(bufferID, &info)) {
		ERROR("UnregisterBuffer: failed to unregister buffer! team = %ld, "
			"bufferid = %ld\n", team, bufferID);
		return B_ERROR;
	}

	int index = info->teams.Find(team);
	if (index < 0) {
		ERROR("UnregisterBuffer: failed to find team = %ld belonging to "
			"bufferID = %ld\n", team, bufferID);
		return B_ERROR;
	}
	
	if (!info->teams.Remove(index)) {
		ERROR("UnregisterBuffer: failed to remove team = %ld from bufferID "
			"= %ld\n", team, bufferID);
		return B_ERROR;
	}

	TRACE("UnregisterBuffer: team = %ld removed from bufferID = %ld\n", team,
		bufferID);

	if (info->teams.IsEmpty()) {
		if (!fBufferInfoMap.Remove(bufferID)) {
			ERROR("UnregisterBuffer: failed to remove bufferID = %ld\n",
				bufferID);
			return B_ERROR;
		}

		TRACE("UnregisterBuffer: bufferID = %ld removed\n", bufferID);
	}

	return B_OK;
}


void
BufferManager::CleanupTeam(team_id team)
{
	BAutolock lock(fLocker);

	TRACE("BufferManager::CleanupTeam: team %ld\n", team);

	buffer_info* info;
	for (fBufferInfoMap.Rewind(); fBufferInfoMap.GetNext(&info); ) {
		team_id* currentTeam;
		for (info->teams.Rewind(); info->teams.GetNext(&currentTeam); ) {
			if (team == *currentTeam) {
				PRINT(1, "BufferManager::CleanupTeam: removing team %ld from "
					"buffer id %ld\n", team, info->id);
				info->teams.RemoveCurrent();
			}
		}
		if (info->teams.IsEmpty()) {
			PRINT(1, "BufferManager::CleanupTeam: removing buffer id %ld that "
				"has no teams\n", info->id);
			fBufferInfoMap.RemoveCurrent(); 
		}
	}
}


void
BufferManager::Dump()
{
	BAutolock lock(fLocker);

	printf("\n");
	printf("BufferManager: list of buffers follows:\n");

	buffer_info *info;
	for (fBufferInfoMap.Rewind(); fBufferInfoMap.GetNext(&info); ) {
		printf(" buffer-id %ld, area-id %ld, offset %ld, size %ld, flags "
			"%#08lx\n", info->id, info->area, info->offset, info->size,
			info->flags);
		printf("   assigned teams: ");

		team_id* team;
		for (info->teams.Rewind(); info->teams.GetNext(&team); ) {
			printf("%ld, ", *team);
		}
		printf("\n");
	}
	printf("BufferManager: list end\n");
}
