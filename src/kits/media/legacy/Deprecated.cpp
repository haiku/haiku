/*
 * Copyright 2002-2015, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <MediaDefs.h>
#include <MediaRoster.h>
#include <SupportDefs.h>

#include "MediaDebug.h"

// This file contains parts of the media_kit that can be removed
// as considered useless, deprecated and/or not worth to be
// implemented.

// BMediaRoster

status_t
BMediaRoster::SetRealtimeFlags(uint32 enabled)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BMediaRoster::GetRealtimeFlags(uint32* _enabled)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


/*static*/ status_t
BMediaRoster::ParseCommand(BMessage& reply)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BMediaRoster::GetDefaultInfo(media_node_id forDefault, BMessage& config)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BMediaRoster::SetRunningDefault(media_node_id forDefault,
	const media_node& node)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


//! Deprecated call.
status_t
BMediaRoster::SetOutputBuffersFor(const media_source& output,
	BBufferGroup* group, bool willReclaim)
{
	UNIMPLEMENTED();
	debugger("BMediaRoster::SetOutputBuffersFor missing\n");
	return B_ERROR;
}

// MediaDefs.h

status_t launch_media_server(uint32 flags);

status_t media_realtime_init_image(image_id image, uint32 flags);

status_t media_realtime_init_thread(thread_id thread, size_t stack_used,
	uint32 flags);


status_t
launch_media_server(uint32 flags)
{
	return launch_media_server(0, NULL, NULL, flags);
}


//	Given an image_id, prepare that image_id for realtime media
//	If the kind of media indicated by "flags" is not enabled for real-time,
//	B_MEDIA_REALTIME_DISABLED is returned.
//	If there are not enough system resources to enable real-time performance,
//	B_MEDIA_REALTIME_UNAVAILABLE is returned.
status_t
media_realtime_init_image(image_id image, uint32 flags)
{
	UNIMPLEMENTED();
	return B_OK;
}


//	Given a thread ID, and an optional indication of what the thread is
//	doing in "flags", prepare the thread for real-time media performance.
//	Currently, this means locking the thread stack, up to size_used bytes,
//	or all of it if 0 is passed. Typically, you will not be using all
//	256 kB of the stack, so you should pass some smaller value you determine
//	from profiling the thread; typically in the 32-64kB range.
//	Return values are the same as for media_prepare_realtime_image().
status_t
media_realtime_init_thread(thread_id thread, size_t stack_used, uint32 flags)
{
	UNIMPLEMENTED();
	return B_OK;
}
