/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: OldBufferStreamManager.cpp
 *  DESCR: 
 ***********************************************************************/
#include <OldBufferStreamManager.h>
#include "debug.h"

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
	int32 dummy;

	return dummy;
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
	int32 dummy;

	return dummy;
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
	bigtime_t dummy;

	return dummy;
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
	bigtime_t dummy;

	return dummy;
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
	stream_state dummy;

	return dummy;
}


stream_state
BBufferStreamManager::Stop()
{
	UNIMPLEMENTED();
	stream_state dummy;

	return dummy;
}


stream_state
BBufferStreamManager::Abort()
{
	UNIMPLEMENTED();
	stream_state dummy;

	return dummy;
}


stream_state
BBufferStreamManager::State() const
{
	UNIMPLEMENTED();
	stream_state dummy;

	return dummy;
}


port_id
BBufferStreamManager::NotificationPort() const
{
	UNIMPLEMENTED();
	port_id dummy;

	return dummy;
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
	bool dummy;

	return dummy;
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
	status_t dummy;

	return dummy;
}


status_t
BBufferStreamManager::Unsubscribe()
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


subscriber_id
BBufferStreamManager::ID() const
{
	UNIMPLEMENTED();
	subscriber_id dummy;

	return dummy;
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
	status_t dummy;

	return dummy;
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
	bigtime_t dummy;

	return dummy;
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


