struct _shared_buffer_list;

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

	void 		PrintToStream();

private:
	struct _team_list 
	{
		struct _team_list *next;
		team_id team;
	};
	struct _buffer_list
	{
		struct _buffer_list *next;
		media_buffer_id id;
		area_id area;
		size_t offset;
		size_t size;
		int32 flags;
		_team_list *teams;
	};
	_shared_buffer_list *	fSharedBufferList;
	area_id					fAreaId;
	_buffer_list *			fBufferList;
	BLocker *				fLocker;
	media_buffer_id			fNextBufferId;
};

