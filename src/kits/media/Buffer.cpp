/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: Buffer.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaDefs.h>
#include <Buffer.h>
#include <Message.h>
#include "SharedBufferList.h"
#include "debug.h"
#include "../server/headers/ServerInterface.h"

static team_id CurrentTeam();
team_id CurrentTeam()
{
	thread_info info;
	get_thread_info(find_thread(NULL),&info);
	return info.team;
}

/*************************************************************
 * public struct buffer_clone_info
 *************************************************************/

buffer_clone_info::buffer_clone_info()
{
	CALLED();
	buffer	= 0;
	area 	= 0;
	offset 	= 0;
	size 	= 0;
	flags 	= 0;
}


buffer_clone_info::~buffer_clone_info()
{
	CALLED();
}

/*************************************************************
 * public BBuffer
 *************************************************************/

void *
BBuffer::Data()
{
	CALLED();
	return fData;
}


size_t
BBuffer::SizeAvailable()
{
	CALLED();
	return fSize;
}


size_t
BBuffer::SizeUsed()
{
	CALLED();
	return fMediaHeader.size_used;
}


void
BBuffer::SetSizeUsed(size_t size_used)
{
	CALLED();
	fMediaHeader.size_used = min_c(size_used,fSize);
}


uint32
BBuffer::Flags()
{
	CALLED();
	return fFlags;
}


void
BBuffer::Recycle()
{
	CALLED();
	if (fBufferList == NULL)
		return;
	fBufferList->RecycleBuffer(this);
}


buffer_clone_info
BBuffer::CloneInfo() const
{
	CALLED();
	buffer_clone_info info;

	info.buffer	= fBufferID;
	info.area	= fArea;
	info.offset	= fOffset;
	info.size	= fSize;
	info.flags	= fFlags;

	return info;
}


media_buffer_id
BBuffer::ID()
{
	CALLED();
	return fBufferID;
}


media_type
BBuffer::Type()
{
	CALLED();
	return fMediaHeader.type;
}


media_header *
BBuffer::Header()
{
	CALLED();
	return &fMediaHeader;
}


media_audio_header *
BBuffer::AudioHeader()
{
	CALLED();
	return &fMediaHeader.u.raw_audio;
}


media_video_header *
BBuffer::VideoHeader()
{
	CALLED();
	return &fMediaHeader.u.raw_video;
}


size_t
BBuffer::Size()
{
	CALLED();
	return SizeAvailable();
}

/*************************************************************
 * private BBuffer
 *************************************************************/

/* explicit */
BBuffer::BBuffer(const buffer_clone_info & info) : 
	fBufferList(0), // must be 0 if not correct initialized
	fData(0), // must be 0 if not correct initialized
	fSize(0), // should be 0 if not correct initialized
	fBufferID(0) // must be 0 if not registered
{
	CALLED();
	
	// special case for BSmallBuffer
	if (info.area == 0 && info.buffer == 0)
		return;

	// ask media_server to get the area_id of the shared buffer list
	area_id id;
	BMessage request(MEDIA_SERVER_GET_SHARED_BUFFER_AREA);
	BMessage reply;

	if (MediaKitPrivate::QueryServer(&request, &reply) != B_OK)
		return;

	id = reply.FindInt32("area");

	fBufferList = _shared_buffer_list::Clone(id);
	if (fBufferList == NULL)
		return;


	BMessage response;
	BMessage create(MEDIA_SERVER_REGISTER_BUFFER);
	create.AddInt32("team",CurrentTeam());
	create.AddInt32("area",info.area);
	create.AddInt32("offset",info.offset);
	create.AddInt32("size",info.size);
	create.AddInt32("flags",info.flags);
	create.AddInt32("buffer",info.buffer);

	// ask media_server to register this buffer, 
	// either identified by "buffer" or by area information.
	// media_server either has a copy of the area identified
	// by "buffer", or creates a new area.
	// the information and the area is cached by the media_server
	// until the last buffer has been unregistered
	// the area_id of the cached area is passed back to us, and we clone it.

	if (MediaKitPrivate::QueryServer(&create, &response) != B_OK)
		return;

	// the response from media server contains enough information
	// to clone the memory for this buffer
	fBufferID = response.FindInt32("buffer");
	fSize = response.FindInt32("size");
	fFlags = response.FindInt32("flags");
	fOffset = response.FindInt32("offset");
	id = response.FindInt32("area");

	fArea = clone_area("a cloned BBuffer", &fData, B_ANY_ADDRESS,B_READ_AREA | B_WRITE_AREA,id);
	if (fArea <= B_OK) {
		TRACE("buffer cloning failed\n");
		fData = 0;
		return;
	}

	fData = (char *)fData + fOffset;
	fMediaHeader.size_used = 0;
}


BBuffer::~BBuffer()
{
	CALLED();
	// unmap the BufferList
	if (fBufferList != NULL) {
		fBufferList->Unmap();
	}
	// unmap the Data
	if (fData != NULL) {
		BMessage unregister(MEDIA_SERVER_UNREGISTER_BUFFER);
		BMessage response;
		unregister.AddInt32("team",(int32)CurrentTeam());
		unregister.AddInt32("buffer",fBufferID);

		// ask media_server to unregister the buffer
		// when the last clone of this buffer is gone,
		// media_server will also remove it's cached area

		MediaKitPrivate::QueryServer(&unregister, &response);
		
		delete_area(fArea);
	}
}


void
BBuffer::SetHeader(const media_header *header)
{
	CALLED();
	fMediaHeader = *header;
	fMediaHeader.buffer = fBufferID;
}


/*************************************************************
 * public BSmallBuffer
 *************************************************************/

static const buffer_clone_info info;
BSmallBuffer::BSmallBuffer()
	: BBuffer(info)
{
	UNIMPLEMENTED();
	debugger("BSmallBuffer::BSmallBuffer called\n");
}


size_t
BSmallBuffer::SmallBufferSizeLimit()
{
	CALLED();
	return 64;
}


