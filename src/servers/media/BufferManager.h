/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Locker.h>
#include <MediaDefs.h>

#include <TMap.h>
#include <TList.h>


struct _shared_buffer_list;


class BufferManager {
public:
							BufferManager();
							~BufferManager();
	
			area_id			SharedBufferListID();
	
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
	struct buffer_info {
		media_buffer_id	id;
		area_id			area;
		size_t			offset;
		size_t			size;
		int32			flags;
		List<team_id>	teams;
	};

			_shared_buffer_list* fSharedBufferList;
			area_id			fSharedBufferListID;
			media_buffer_id	fNextBufferID;
			BLocker			fLocker;
			Map<media_buffer_id, buffer_info> fBufferInfoMap;
};

