/*******************************************************************************
/
/	File:			BufferGroup.h
/
/   Description:   A BBufferGroup organizes sets of BBuffers so that you can request
/	and reclaim them.
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_BUFFER_GROUP_H)
#define _BUFFER_GROUP_H

#include <MediaDefs.h>

struct _shared_buffer_list;

class BBufferGroup
{
public:

		BBufferGroup(
				size_t size,
				int32 count = 3,
				uint32 placement = B_ANY_ADDRESS,
				uint32 lock = B_FULL_LOCK);
explicit	BBufferGroup();
		BBufferGroup(
				int32 count,
				const media_buffer_id * buffers);
		~BBufferGroup();	/* BBufferGroup is NOT a virtual class!!! */

		status_t InitCheck();

			/* use this function to add buffers you created on your own */
		status_t AddBuffer(
				const buffer_clone_info & info,
				BBuffer ** out_buffer = NULL);

		BBuffer * RequestBuffer(
				size_t size,
				bigtime_t timeout = B_INFINITE_TIMEOUT);
		status_t RequestBuffer(
				BBuffer * buffer,
				bigtime_t timeout = B_INFINITE_TIMEOUT);
		status_t RequestError();	/* return last RequestBuffer error, useful if NULL is returned */

		status_t CountBuffers(
				int32 * out_count);
		status_t GetBufferList(
				int32 buf_count,
				BBuffer ** out_buffers);

		status_t WaitForBuffers();
		status_t ReclaimAllBuffers();

private:
		/* in BeOS R5 this is a deprecated api, from BeOS R4 times */
		status_t 				AddBuffersTo(BMessage * message, const char * name, bool needLock=true);
		
		status_t				InitBufferGroup(); 		/* used internally */
		BBufferGroup(const BBufferGroup &);				/* not implemented */
		BBufferGroup& operator=(const BBufferGroup&);	/* not implemented */

		friend struct 			_shared_buffer_list;

		status_t 				fInitError;
		status_t 				fRequestError;
		int32					fBufferCount;
		_shared_buffer_list *	fBufferList;
		
		// this is a BBufferGroup specific semaphore used for reclaiming BBuffers of this group
		// is also is a system wide unique identifier of this group
		sem_id					fReclaimSem;

		uint32 					_reserved_buffer_group_[9];
};

#endif /* _BUFFER_GROUP_H */

