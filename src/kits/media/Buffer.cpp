/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: Buffer.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaDefs.h>
#include <Buffer.h>
#include "SharedBufferList.h"
#include "debug.h"
#include "DataExchange.h"

namespace BPrivate { namespace media {
	extern team_id team;
} } // BPrivate::media
using namespace BPrivate::media;

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
	fMediaHeader.size_used = min_c(size_used, fSize);
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
	server_get_shared_buffer_area_request area_request;
	server_get_shared_buffer_area_reply area_reply;
	if (QueryServer(SERVER_GET_SHARED_BUFFER_AREA, &area_request, sizeof(area_request), &area_reply, sizeof(area_reply)) != B_OK) {
		FATAL("BBuffer::BBuffer: SERVER_GET_SHARED_BUFFER_AREA failed\n");
		return;
	}
	
	ASSERT(area_reply.area > 0);
	
	fBufferList = _shared_buffer_list::Clone(area_reply.area);
	if (fBufferList == NULL) {
		FATAL("BBuffer::BBuffer: _shared_buffer_list::Clone() failed\n");
		return;
	}

	server_register_buffer_request request;
	server_register_buffer_reply reply;
	
	request.team = team;
	request.info = info;

	// ask media_server to register this buffer, 
	// either identified by "buffer" or by area information.
	// media_server either has a copy of the area identified
	// by "buffer", or creates a new area.
	// the information and the area is cached by the media_server
	// until the last buffer has been unregistered
	// the area_id of the cached area is passed back to us, and we clone it.

	if (QueryServer(SERVER_REGISTER_BUFFER, &request, sizeof(request), &reply, sizeof(reply)) != B_OK) {
		FATAL("BBuffer::BBuffer: failed to register buffer with media_server\n");
		return;
	}

	ASSERT(reply.info.buffer > 0);
	ASSERT(reply.info.area > 0);
	ASSERT(reply.info.size > 0);

	// the response from media server contains enough information
	// to clone the memory for this buffer
	fBufferID = reply.info.buffer;
	fSize = reply.info.size;
	fFlags = reply.info.flags;
	fOffset = reply.info.offset;
	
	fArea = clone_area("a cloned BBuffer", &fData, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, reply.info.area);
	if (fArea <= B_OK) {
		// XXX should unregister buffer here
		FATAL("BBuffer::BBuffer: buffer cloning failed\n");
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

		delete_area(fArea);

		// ask media_server to unregister the buffer
		// when the last clone of this buffer is gone,
		// media_server will also remove it's cached area
		server_unregister_buffer_command cmd;
		cmd.team = team;
		cmd.bufferid = fBufferID;
		SendToServer(SERVER_UNREGISTER_BUFFER, &cmd, sizeof(cmd));
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


