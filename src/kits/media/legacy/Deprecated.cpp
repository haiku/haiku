/*
 * Copyright 2002-2015, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <MediaRoster.h>
#include <SupportDefs.h>

#include "MediaDebug.h"

// This file contains parts of the media_kit that can be removed
// as considered useless, deprecated and/or not worth to be
// implemented.

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
