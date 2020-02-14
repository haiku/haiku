/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

/*
 * Copyright (c) 2002, 2003 Marcus Overhagen <Marcus@Overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include <Buffer.h>

#include <AppMisc.h>
#include <MediaDefs.h>

#include "MediaDebug.h"
#include "MediaMisc.h"
#include "DataExchange.h"
#include "SharedBufferList.h"


using namespace BPrivate::media;


//	#pragma mark - buffer_clone_info


buffer_clone_info::buffer_clone_info()
{
	CALLED();
	buffer = 0;
	area = 0;
	offset = 0;
	size = 0;
	flags = 0;
}


buffer_clone_info::~buffer_clone_info()
{
	CALLED();
}


//	#pragma mark - public BBuffer


void*
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
	fFlags &= ~BUFFER_TO_RECLAIM;
	if ((fFlags & BUFFER_MARKED_FOR_DELETION) != 0)
		delete this;
	else
		fBufferList->RecycleBuffer(this);
}


buffer_clone_info
BBuffer::CloneInfo() const
{
	CALLED();
	buffer_clone_info info;

	info.buffer = fMediaHeader.buffer;
	info.area = fArea;
	info.offset = fOffset;
	info.size = fSize;
	info.flags = fFlags;

	return info;
}


media_buffer_id
BBuffer::ID()
{
	CALLED();
	return fMediaHeader.buffer;
}


media_type
BBuffer::Type()
{
	CALLED();
	return fMediaHeader.type;
}


media_header*
BBuffer::Header()
{
	CALLED();
	return &fMediaHeader;
}


media_audio_header*
BBuffer::AudioHeader()
{
	CALLED();
	return &fMediaHeader.u.raw_audio;
}


media_video_header*
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


// #pragma mark - private BBuffer


BBuffer::BBuffer(const buffer_clone_info& info)
	:
	fBufferList(NULL),
	fArea(-1),
	fData(NULL),
	fOffset(0),
	fSize(0),
	fFlags(0)
{
	CALLED();

	// Ensure that the media_header is clean
	memset(&fMediaHeader, 0, sizeof(fMediaHeader));
	// special case for BSmallBuffer
	if (info.area == 0 && info.buffer == 0)
		return;

	// Must be -1 if registration fail
	fMediaHeader.buffer = -1;

	fBufferList = BPrivate::SharedBufferList::Get();
	if (fBufferList == NULL) {
		ERROR("BBuffer::BBuffer: BPrivate::SharedBufferList::Get() failed\n");
		return;
	}

	server_register_buffer_request request;
	server_register_buffer_reply reply;

	request.team = BPrivate::current_team();
	request.info = info;

	// ask media_server to register this buffer,
	// either identified by "buffer" or by area information.
	// media_server either has a copy of the area identified
	// by "buffer", or creates a new area.
	// the information and the area is cached by the media_server
	// until the last buffer has been unregistered
	// the area_id of the cached area is passed back to us, and we clone it.

	if (QueryServer(SERVER_REGISTER_BUFFER, &request, sizeof(request), &reply,
			sizeof(reply)) != B_OK) {
		ERROR("BBuffer::BBuffer: failed to register buffer with "
			"media_server\n");
		return;
	}

	ASSERT(reply.info.buffer > 0);
	ASSERT(reply.info.area > 0);
	ASSERT(reply.info.size > 0);

	fArea = clone_area("a cloned BBuffer", &fData, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, reply.info.area);
	if (fArea < 0) {
		ERROR("BBuffer::BBuffer: buffer cloning failed"
			", unregistering buffer\n");
		server_unregister_buffer_command cmd;
		cmd.team = BPrivate::current_team();
		cmd.buffer_id = reply.info.buffer;
		SendToServer(SERVER_UNREGISTER_BUFFER, &cmd, sizeof(cmd));
		return;
	}

	// the response from media server contains enough information
	// to clone the memory for this buffer
	fSize = reply.info.size;
	fFlags = reply.info.flags;
	fOffset = reply.info.offset;
	fMediaHeader.size_used = 0;
	fMediaHeader.buffer = reply.info.buffer;
	fData = (char*)fData + fOffset;
}


BBuffer::~BBuffer()
{
	CALLED();

	// unmap the BufferList
	if (fBufferList != NULL)
		fBufferList->Put();

	// unmap the Data
	if (fData != NULL) {
		delete_area(fArea);

		// Ask media_server to unregister the buffer when the last clone of
		// this buffer is gone, media_server will also remove its cached area.
		server_unregister_buffer_command cmd;
		cmd.team = BPrivate::current_team();
		cmd.buffer_id = fMediaHeader.buffer;
		SendToServer(SERVER_UNREGISTER_BUFFER, &cmd, sizeof(cmd));
	}
}


void
BBuffer::SetHeader(const media_header* header)
{
	CALLED();
	ASSERT(header->buffer == fMediaHeader.buffer);
	if (header->buffer != fMediaHeader.buffer)
		debugger("oops");
	fMediaHeader = *header;
}


// #pragma mark - public BSmallBuffer


static const buffer_clone_info sSmallBufferInfo;


BSmallBuffer::BSmallBuffer()
	:
	BBuffer(sSmallBufferInfo)
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


