/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
struct _shared_buffer_list;

#include <TMap.h>
#include <TList.h>

class BufferManager
{
public:
	BufferManager();
	~BufferManager();
	
	area_id		SharedBufferListID();
	
	status_t	RegisterBuffer(team_id teamid, media_buffer_id bufferid,
							   size_t *size, int32 *flags, size_t *offset, area_id *area);

	status_t	RegisterBuffer(team_id teamid, size_t size, int32 flags, size_t offset, area_id area,
							   media_buffer_id *bufferid);

	status_t	UnregisterBuffer(team_id teamid, media_buffer_id bufferid);
	
	void		CleanupTeam(team_id teamid);

	void 		Dump();

private:
	struct buffer_info
	{
		media_buffer_id id;
		area_id area;
		size_t offset;
		size_t size;
		int32 flags;
		List<team_id> teams;
	};
	
	_shared_buffer_list *	fSharedBufferList;
	area_id					fSharedBufferListId;
	media_buffer_id			fNextBufferId;
	BLocker *				fLocker;
	Map<media_buffer_id, buffer_info> *fBufferInfoMap;
};

