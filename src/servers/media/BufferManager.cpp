/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
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
	if (!fBufferInfoMap.Get(bufferID, info)) {
		ERROR("failed to register buffer! team = %ld, bufferid = %ld\n", team,
			bufferID);
		return B_ERROR;
	}

	info->teams.insert(team);

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
	
	area_id clonedArea = _CloneArea(area);
	if (clonedArea < 0) {
		ERROR("RegisterBuffer: failed to clone buffer! error = %#lx, team = "
			"%ld, area = %ld, offset = %ld, size = %ld\n", clonedArea, team,
			area, offset, size);
		return clonedArea;
	}

	buffer_info info;
	info.id = fNextBufferID++;
	info.area = clonedArea;
	info.offset = offset;
	info.size = size;
	info.flags = flags;
	info.teams.insert(team);

	*_bufferID = info.id;

	fBufferInfoMap.Put(info.id, info);

	TRACE("RegisterBuffer: done, bufferID = %ld\n", info.id);

	return B_OK;
}


status_t
BufferManager::UnregisterBuffer(team_id team, media_buffer_id bufferID)
{
	BAutolock lock(fLocker);
	TRACE("UnregisterBuffer: team = %ld, bufferid = %ld\n", team, bufferID);

	buffer_info* info;
	if (!fBufferInfoMap.Get(bufferID, info)) {
		ERROR("UnregisterBuffer: failed to unregister buffer! team = %ld, "
			"bufferid = %ld\n", team, bufferID);
		return B_ERROR;
	}

	if (info->teams.find(team) == info->teams.end()) {
		ERROR("UnregisterBuffer: failed to find team = %ld belonging to "
			"bufferID = %ld\n", team, bufferID);
		return B_ERROR;
	}
	
	info->teams.erase(team);

	TRACE("UnregisterBuffer: team = %ld removed from bufferID = %ld\n", team,
		bufferID);

	if (info->teams.empty()) {
		_ReleaseClonedArea(info->area);
		fBufferInfoMap.Remove(bufferID);

		TRACE("UnregisterBuffer: bufferID = %ld removed\n", bufferID);
	}

	return B_OK;
}


void
BufferManager::CleanupTeam(team_id team)
{
	BAutolock lock(fLocker);

	TRACE("BufferManager::CleanupTeam: team %ld\n", team);

	BufferInfoMap::Iterator iterator = fBufferInfoMap.GetIterator();
	while (iterator.HasNext()) {
		BufferInfoMap::Entry entry = iterator.Next();

		entry.value.teams.erase(team);

		if (entry.value.teams.empty()) {
			PRINT(1, "BufferManager::CleanupTeam: removing buffer id %ld that "
				"has no teams\n", entry.key);
			_ReleaseClonedArea(entry.value.area);
			iterator.Remove();
		}
	}
}


void
BufferManager::Dump()
{
	BAutolock lock(fLocker);

	printf("\n");
	printf("BufferManager: list of buffers follows:\n");

	BufferInfoMap::Iterator iterator = fBufferInfoMap.GetIterator();
	while (iterator.HasNext()) {
		buffer_info& info = *iterator.NextValue();
		printf(" buffer-id %ld, area-id %ld, offset %ld, size %ld, flags "
			"%#08lx\n", info.id, info.area, info.offset, info.size, info.flags);
		printf("   assigned teams: ");

		std::set<team_id>::iterator teamIterator = info.teams.begin();
		for (; teamIterator != info.teams.end(); teamIterator++) {
			printf("%ld, ", *teamIterator);
		}
		printf("\n");
	}
	printf("BufferManager: list end\n");
}


area_id
BufferManager::_CloneArea(area_id area)
{
	void* address;
	area_id clonedArea = clone_area("media_server cloned buffer", &address,
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, area);
	
	return clonedArea;
}


void
BufferManager::_ReleaseClonedArea(area_id clone)
{
	delete_area(clone);
}
