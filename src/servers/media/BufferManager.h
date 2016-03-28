/*
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BUFFER_MANAGER_H
#define _BUFFER_MANAGER_H

#include <set>

#include <Locker.h>
#include <MediaDefs.h>

#include <HashMap.h>


namespace BPrivate {
	class SharedBufferList;
}


class BufferManager {
public:
							BufferManager();
							~BufferManager();

			area_id			SharedBufferListArea();

			status_t		RegisterBuffer(team_id team,
								media_buffer_id bufferID, size_t* _size,
								int32* _flags, size_t* _offset, area_id* _area);

			status_t		RegisterBuffer(team_id team, size_t size,
								int32 flags, size_t offset, area_id area,
								media_buffer_id* _bufferID);

			status_t		UnregisterBuffer(team_id team,
								media_buffer_id bufferID);

			void			CleanupTeam(team_id team);

			void			Dump();

private:
			area_id			_CloneArea(area_id area);
			void			_ReleaseClonedArea(area_id clone);

private:
	struct clone_info {
		area_id				clone;
		vint32				ref_count;
	};

	struct buffer_info {
		media_buffer_id		id;
		area_id				area;
		size_t				offset;
		size_t				size;
		int32				flags;
		std::set<team_id>	teams;
	};

	typedef HashMap<HashKey32<media_buffer_id>, buffer_info> BufferInfoMap;
	typedef HashMap<HashKey32<area_id>, clone_info> CloneInfoMap;
	typedef HashMap<HashKey32<area_id>, area_id> SourceInfoMap;

			BPrivate::SharedBufferList* fSharedBufferList;
			area_id			fSharedBufferListArea;
			media_buffer_id	fNextBufferID;
			BLocker			fLocker;
			BufferInfoMap	fBufferInfoMap;
			CloneInfoMap	fCloneInfoMap;
			SourceInfoMap	fSourceInfoMap;
};

#endif // _BUFFER_MANAGER_H
