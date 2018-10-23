/*
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


// This is deprecated API that is not even implemented - no need to export
// it on a GCC4 build (BeIDE needs it to run, though, so it's worthwhile for
// GCC2)
#if __GNUC__ < 3


#include "OldBufferStreamManager.h"

#include <MediaDebug.h>


/*************************************************************
 * public BBufferStreamManager
 *************************************************************/

BBufferStreamManager::BBufferStreamManager(char *name)
{
	UNIMPLEMENTED();
}


BBufferStreamManager::~BBufferStreamManager()
{
	UNIMPLEMENTED();
}


char *
BBufferStreamManager::Name() const
{
	UNIMPLEMENTED();
	return NULL;
}


BBufferStream *
BBufferStreamManager::Stream() const
{
	UNIMPLEMENTED();
	return NULL;
}


int32
BBufferStreamManager::BufferCount() const
{
	UNIMPLEMENTED();

	return 0;
}


void
BBufferStreamManager::SetBufferCount(int32 count)
{
	UNIMPLEMENTED();
}


int32
BBufferStreamManager::BufferSize() const
{
	UNIMPLEMENTED();

	return 0;
}


void
BBufferStreamManager::SetBufferSize(int32 bytes)
{
	UNIMPLEMENTED();
}


bigtime_t
BBufferStreamManager::BufferDelay() const
{
	UNIMPLEMENTED();

	return 0;
}


void
BBufferStreamManager::SetBufferDelay(bigtime_t usecs)
{
	UNIMPLEMENTED();
}


bigtime_t
BBufferStreamManager::Timeout() const
{
	UNIMPLEMENTED();

	return 0;
}


void
BBufferStreamManager::SetTimeout(bigtime_t usecs)
{
	UNIMPLEMENTED();
}


stream_state
BBufferStreamManager::Start()
{
	UNIMPLEMENTED();

	return B_IDLE;
}


stream_state
BBufferStreamManager::Stop()
{
	UNIMPLEMENTED();

	return B_IDLE;
}


stream_state
BBufferStreamManager::Abort()
{
	UNIMPLEMENTED();

	return B_IDLE;
}


stream_state
BBufferStreamManager::State() const
{
	UNIMPLEMENTED();

	return B_IDLE;
}


port_id
BBufferStreamManager::NotificationPort() const
{
	UNIMPLEMENTED();

	return 0;
}


void
BBufferStreamManager::SetNotificationPort(port_id port)
{
	UNIMPLEMENTED();
}


bool
BBufferStreamManager::Lock()
{
	UNIMPLEMENTED();

	return false;
}


void
BBufferStreamManager::Unlock()
{
	UNIMPLEMENTED();
}


status_t
BBufferStreamManager::Subscribe(BBufferStream *stream)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BBufferStreamManager::Unsubscribe()
{
	UNIMPLEMENTED();

	return B_ERROR;
}


subscriber_id
BBufferStreamManager::ID() const
{
	UNIMPLEMENTED();

	return 0;
}

/*************************************************************
 * protected BBufferStreamManager
 *************************************************************/

void
BBufferStreamManager::StartProcessing()
{
	UNIMPLEMENTED();
}


void
BBufferStreamManager::StopProcessing()
{
	UNIMPLEMENTED();
}


status_t
BBufferStreamManager::_ProcessingThread(void *arg)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


void
BBufferStreamManager::ProcessingThread()
{
	UNIMPLEMENTED();
}


void
BBufferStreamManager::SetState(stream_state newState)
{
	UNIMPLEMENTED();
}


bigtime_t
BBufferStreamManager::SnoozeUntil(bigtime_t sys_time)
{
	UNIMPLEMENTED();

	return 0;
}

/*************************************************************
 * private BBufferStreamManager
 *************************************************************/

void
BBufferStreamManager::_ReservedBufferStreamManager1()
{
	UNIMPLEMENTED();
}


void
BBufferStreamManager::_ReservedBufferStreamManager2()
{
	UNIMPLEMENTED();
}


void
BBufferStreamManager::_ReservedBufferStreamManager3()
{
	UNIMPLEMENTED();
}


#endif	// __GNUC__ < 3
