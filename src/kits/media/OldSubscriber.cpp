/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: OldSubscriber.cpp
 *  DESCR: 
 ***********************************************************************/
#include <OldSubscriber.h>
#include "debug.h"

/*************************************************************
 * public BSubscriber
 *************************************************************/

BSubscriber::BSubscriber(const char *name)
{
	UNIMPLEMENTED();
}


BSubscriber::~BSubscriber()
{
	UNIMPLEMENTED();
}


status_t
BSubscriber::Subscribe(BAbstractBufferStream *stream)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BSubscriber::Unsubscribe()
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


subscriber_id
BSubscriber::ID() const
{
	UNIMPLEMENTED();
	subscriber_id dummy;

	return dummy;
}


const char *
BSubscriber::Name() const
{
	UNIMPLEMENTED();
	return NULL;
}


void
BSubscriber::SetTimeout(bigtime_t microseconds)
{
	UNIMPLEMENTED();
}


bigtime_t
BSubscriber::Timeout() const
{
	UNIMPLEMENTED();
	bigtime_t dummy;

	return dummy;
}


status_t
BSubscriber::EnterStream(subscriber_id neighbor,
						 bool before,
						 void *userData,
						 enter_stream_hook entryFunction,
						 exit_stream_hook exitFunction,
						 bool background)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BSubscriber::ExitStream(bool synch)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


bool
BSubscriber::IsInStream() const
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}

/*************************************************************
 * protected BSubscriber
 *************************************************************/

status_t
BSubscriber::_ProcessLoop(void *arg)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BSubscriber::ProcessLoop()
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


BAbstractBufferStream *
BSubscriber::Stream() const
{
	UNIMPLEMENTED();
	return NULL;
}

/*************************************************************
 * private BSubscriber
 *************************************************************/

void
BSubscriber::_ReservedSubscriber1()
{
	UNIMPLEMENTED();
}


void
BSubscriber::_ReservedSubscriber2()
{
	UNIMPLEMENTED();
}


void
BSubscriber::_ReservedSubscriber3()
{
	UNIMPLEMENTED();
}


