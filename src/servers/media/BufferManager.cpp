/*
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "BufferManager.h"

#include <Autolock.h>

#include "MediaDebug.h"
#include "SharedBufferList.h"


BufferManager::BufferManager()
	:
	fSharedBufferList(NULL),
	fSharedBufferListArea(-1),
	fNextBufferID(1),
	fLocker("buffer manager locker")
{
	fSharedBufferListArea
		= BPrivate::SharedBufferList::Create(&fSharedBufferList);
}


BufferManager::~BufferManager()
{
	fSharedBufferList->Put();
}


area_id
BufferManager::SharedBufferListArea()
{
	return fSharedBufferListArea;
}


status_t
BufferManager::RegisterBuffer(team_id team, media_buffer_id bufferID,
	size_t* _size, int32* _flags, size_t* _offset, area_id* _area)
{
	BAutolock lock(fLocker);

	TRACE("RegisterBuffer team = %" B_PRId32 ", bufferid = %" B_PRId32 "\n",
		team, bufferID);

	buffer_info* info;
	if (!fBufferInfoMap.Get(bufferID, info)) {
		ERROR("failed to register buffer! team = %" B_PRId32 ", bufferid = %"
			B_PRId32 "\n", team, bufferID);
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
	TRACE("RegisterBuffer team = %" B_PRId32 ", area = %"
		B_PRId32 ", offset = %" B_PRIuSIZE ", size = %" B_PRIuSIZE "\n",
		team, area, offset, size);

	area_id clonedArea = _CloneArea(area);
	if (clonedArea < 0) {
		ERROR("RegisterBuffer: failed to clone buffer! error = %#" B_PRIx32
			", team = %" B_PRId32 ", area = %" B_PRId32 ", offset = %"
			B_PRIuSIZE ", size = %" B_PRIuSIZE "\n", clonedArea, team,
			area, offset, size);
		return clonedArea;
	}

	buffer_info info;
	info.id = fNextBufferID++;
	info.area = clonedArea;
	info.offset = offset;
	info.size = size;
	info.flags = flags;

	try {
		info.teams.insert(team);
		if (fBufferInfoMap.Put(info.id, info) != B_OK)
			throw std::bad_alloc();
	} catch (std::bad_alloc& exception) {
		_ReleaseClonedArea(clonedArea);
		return B_NO_MEMORY;
	}

	TRACE("RegisterBuffer: done, bufferID = %" B_PRId32 "\n", info.id);

	*_bufferID = info.id;
	return B_OK;
}


status_t
BufferManager::UnregisterBuffer(team_id team, media_buffer_id bufferID)
{
	BAutolock lock(fLocker);
	TRACE("UnregisterBuffer: team = %" B_PRId32 ", bufferID = %" B_PRId32 "\n",
		team, bufferID);

	buffer_info* info;
	if (!fBufferInfoMap.Get(bufferID, info)) {
		ERROR("UnregisterBuffer: failed to unregister buffer! team = %"
			B_PRId32 ", bufferID = %" B_PRId32 "\n", team, bufferID);
		return B_ERROR;
	}

	if (info->teams.find(team) == info->teams.end()) {
		ERROR("UnregisterBuffer: failed to find team = %" B_PRId32 " belonging"
			" to bufferID = %" B_PRId32 "\n", team, bufferID);
		return B_ERROR;
	}

	info->teams.erase(team);

	TRACE("UnregisterBuffer: team = %" B_PRId32 " removed from bufferID = %"
		B_PRId32 "\n", team, bufferID);

	if (info->teams.empty()) {
		_ReleaseClonedArea(info->area);
		fBufferInfoMap.Remove(bufferID);

		TRACE("UnregisterBuffer: bufferID = %" B_PRId32 " removed\n", bufferID);
	}

	return B_OK;
}


void
BufferManager::CleanupTeam(team_id team)
{
	BAutolock lock(fLocker);

	TRACE("BufferManager::CleanupTeam: team %" B_PRId32 "\n", team);

	BufferInfoMap::Iterator iterator = fBufferInfoMap.GetIterator();
	while (iterator.HasNext()) {
		BufferInfoMap::Entry entry = iterator.Next();

		entry.value.teams.erase(team);

		if (entry.value.teams.empty()) {
			PRINT(1, "BufferManager::CleanupTeam: removing buffer id %"
				B_PRId32 " that has no teams\n", entry.key.GetHashCode());
			_ReleaseClonedArea(entry.value.area);
			fBufferInfoMap.Remove(iterator);
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
		buffer_info info = iterator.Next().value;
		printf(" buffer-id %" B_PRId32 ", area-id %" B_PRId32 ", offset %ld, "
			"size %ld, flags %#08" B_PRIx32 "\n", info.id, info.area,
			info.offset, info.size, info.flags);
		printf("   assigned teams: ");

		std::set<team_id>::iterator teamIterator = info.teams.begin();
		for (; teamIterator != info.teams.end(); teamIterator++) {
			printf("%" B_PRId32 ", ", *teamIterator);
		}
		printf("\n");
	}
	printf("BufferManager: list end\n");
}


area_id
BufferManager::_CloneArea(area_id area)
{
	{
		clone_info* info;
		if (fCloneInfoMap.Get(area, info)) {
			// we have already cloned this particular area
			TRACE("BufferManager::_CloneArea() area %" B_PRId32 " has already"
				" been cloned (id %" B_PRId32 ")\n", area, info->clone);

			info->ref_count++;
			return info->clone;
		}
	}

	void* address;
	area_id clonedArea = clone_area("media_server cloned buffer", &address,
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, area);

	TRACE("BufferManager::_CloneArea() cloned area %" B_PRId32 ", clone id %"
		B_PRId32 "\n", area, clonedArea);

	if (clonedArea < 0)
		return clonedArea;

	clone_info info;
	info.clone = clonedArea;
	info.ref_count = 1;

	if (fCloneInfoMap.Put(area, info) == B_OK) {
		if (fSourceInfoMap.Put(clonedArea, area) == B_OK)
			return clonedArea;

		fCloneInfoMap.Remove(area);
	}

	delete_area(clonedArea);
	return B_NO_MEMORY;
}


void
BufferManager::_ReleaseClonedArea(area_id clone)
{
	area_id source = fSourceInfoMap.Get(clone);

	clone_info* info;
	if (!fCloneInfoMap.Get(source, info)) {
		ERROR("BufferManager::_ReleaseClonedArea(): could not find clone info "
			"for id %" B_PRId32 " (clone %" B_PRId32 ")\n", source, clone);
		return;
	}

	if (--info->ref_count == 0) {
		TRACE("BufferManager::_ReleaseClonedArea(): delete cloned area %"
			B_PRId32 " (source %" B_PRId32 ")\n", clone, source);

		fSourceInfoMap.Remove(clone);
		fCloneInfoMap.Remove(source);
		delete_area(clone);
	} else {
		TRACE("BufferManager::_ReleaseClonedArea(): released cloned area %"
			B_PRId32 " (source %" B_PRId32 ")\n", clone, source);
	}
}
