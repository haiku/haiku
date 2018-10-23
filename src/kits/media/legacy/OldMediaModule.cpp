/*
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


// This is deprecated API that is not even implemented - no need to export
// it on a GCC4 build (BeIDE needs it to run, though, so it's worthwhile for
// GCC2)
#if __GNUC__ < 3


#include "OldMediaModule.h"

#include <MediaDebug.h>


/*************************************************************
 * public BMediaEvent
 *************************************************************/

mk_time
BMediaEvent::Duration()
{
	UNIMPLEMENTED();

	return 0;
}


bigtime_t
BMediaEvent::CaptureTime()
{
	UNIMPLEMENTED();

	return 0;
}

/*************************************************************
 * public BEventStream
 *************************************************************/

BEventStream::BEventStream()
{
	UNIMPLEMENTED();
}


BEventStream::~BEventStream()
{
	UNIMPLEMENTED();
}


mk_time
BEventStream::Start()
{
	UNIMPLEMENTED();

	return 0;
}


void
BEventStream::SetStart(mk_time)
{
	UNIMPLEMENTED();
}


mk_time
BEventStream::Duration()
{
	UNIMPLEMENTED();

	return 0;
}


void
BEventStream::SetDuration(mk_time)
{
	UNIMPLEMENTED();
}


status_t
BEventStream::SeekToTime(BMediaChannel *channel,
						 mk_time time)
{
	UNIMPLEMENTED();

	return B_ERROR;
}

/*************************************************************
 * public BMediaRenderer
 *************************************************************/


BMediaRenderer::BMediaRenderer(const char *name,
							   int32 priority)
{
	UNIMPLEMENTED();
}


BMediaRenderer::~BMediaRenderer()
{
	UNIMPLEMENTED();
}


char *
BMediaRenderer::Name()
{
	UNIMPLEMENTED();
	return NULL;
}


mk_time
BMediaRenderer::Latency()
{
	UNIMPLEMENTED();

	return 0;
}


BTransport *
BMediaRenderer::Transport()
{
	UNIMPLEMENTED();
	return NULL;
}


void
BMediaRenderer::SetTransport(BTransport *)
{
	UNIMPLEMENTED();
}


mk_time
BMediaRenderer::Start()
{
	UNIMPLEMENTED();

	return 0;
}


mk_time
BMediaRenderer::Duration()
{
	UNIMPLEMENTED();

	return 0;
}


BTimeBase *
BMediaRenderer::TimeBase()
{
	UNIMPLEMENTED();
	return NULL;
}


void
BMediaRenderer::Open()
{
	UNIMPLEMENTED();
}


void
BMediaRenderer::Close()
{
	UNIMPLEMENTED();
}


void
BMediaRenderer::WakeUp()
{
	UNIMPLEMENTED();
}


void
BMediaRenderer::TransportChanged(mk_time time,
								 mk_rate rate,
								 transport_status status)
{
	UNIMPLEMENTED();
}


void
BMediaRenderer::StreamChanged()
{
	UNIMPLEMENTED();
}


void
BMediaRenderer::OpenReceived()
{
	UNIMPLEMENTED();
}


void
BMediaRenderer::CloseReceived()
{
	UNIMPLEMENTED();
}


void
BMediaRenderer::WakeUpReceived()
{
	UNIMPLEMENTED();
}


void
BMediaRenderer::TransportChangedReceived(mk_time time,
										 mk_rate rate,
										 transport_status status)
{
	UNIMPLEMENTED();
}


void
BMediaRenderer::StreamChangedReceived()
{
	UNIMPLEMENTED();
}

/*************************************************************
 * private BMediaRenderer
 *************************************************************/


int32
BMediaRenderer::_LoopThread(void *arg)
{
	UNIMPLEMENTED();

	return 0;
}


void
BMediaRenderer::LoopThread()
{
	UNIMPLEMENTED();
}

/*************************************************************
 * public BTransport
 *************************************************************/


BTransport::BTransport()
{
	UNIMPLEMENTED();
}


BTransport::~BTransport()
{
	UNIMPLEMENTED();
}


BTimeBase *
BTransport::TimeBase()
{
	UNIMPLEMENTED();
	return NULL;
}


void
BTransport::SetTimeBase(BTimeBase *)
{
	UNIMPLEMENTED();
}


BList *
BTransport::Renderers()
{
	UNIMPLEMENTED();
	return NULL;
}


void
BTransport::AddRenderer(BMediaRenderer *)
{
	UNIMPLEMENTED();
}


bool
BTransport::RemoveRenderer(BMediaRenderer *)
{
	UNIMPLEMENTED();

	return false;
}


transport_status
BTransport::Status()
{
	UNIMPLEMENTED();

	return B_TRANSPORT_STOPPED;
}


void
BTransport::SetStatus(transport_status)
{
	UNIMPLEMENTED();
}


mk_time
BTransport::PerformanceTime()
{
	UNIMPLEMENTED();

	return 0;
}


mk_rate
BTransport::PerformanceRate()
{
	UNIMPLEMENTED();

	return 0;
}


mk_time
BTransport::TimeOffset()
{
	UNIMPLEMENTED();

	return 0;
}


void
BTransport::SetTimeOffset(mk_time)
{
	UNIMPLEMENTED();
}


mk_time
BTransport::MaximumLatency()
{
	UNIMPLEMENTED();

	return 0;
}


mk_time
BTransport::PerformanceStart()
{
	UNIMPLEMENTED();

	return 0;
}


mk_time
BTransport::PerformanceEnd()
{
	UNIMPLEMENTED();

	return 0;
}


void
BTransport::Open()
{
	UNIMPLEMENTED();
}


void
BTransport::Close()
{
	UNIMPLEMENTED();
}


void
BTransport::TransportChanged()
{
	UNIMPLEMENTED();
}


void
BTransport::TimeSkipped()
{
	UNIMPLEMENTED();
}


void
BTransport::RequestWakeUp(mk_time,
						  BMediaRenderer *)
{
	UNIMPLEMENTED();
}


void
BTransport::SeekToTime(mk_time)
{
	UNIMPLEMENTED();
}


BMediaChannel *
BTransport::GetChannel(int32 selector)
{
	UNIMPLEMENTED();
	return NULL;
}

/*************************************************************
 * public BTimeBase
 *************************************************************/


BTimeBase::BTimeBase(mk_rate rate)
{
	UNIMPLEMENTED();
}


BTimeBase::~BTimeBase()
{
	UNIMPLEMENTED();
}


BList *
BTimeBase::Transports()
{
	UNIMPLEMENTED();
	return NULL;
}


void
BTimeBase::AddTransport(BTransport *)
{
	UNIMPLEMENTED();
}


bool
BTimeBase::RemoveTransport(BTransport *)
{
	UNIMPLEMENTED();

	return false;
}


void
BTimeBase::TimeSkipped()
{
	UNIMPLEMENTED();
}


status_t
BTimeBase::CallAt(mk_time time,
				  mk_deferred_call function,
				  void *arg)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


mk_time
BTimeBase::Time()
{
	UNIMPLEMENTED();

	return 0;
}


mk_rate
BTimeBase::Rate()
{
	UNIMPLEMENTED();

	return 0;
}


mk_time
BTimeBase::TimeAt(bigtime_t system_time)
{
	UNIMPLEMENTED();

	return 0;
}


bigtime_t
BTimeBase::SystemTimeAt(mk_time time)
{
	UNIMPLEMENTED();

	return 0;
}


void
BTimeBase::Sync(mk_time time,
				bigtime_t system_time)
{
	UNIMPLEMENTED();
}


bool
BTimeBase::IsAbsolute()
{
	UNIMPLEMENTED();

	return false;
}

/*************************************************************
 * private BTimeBase
 *************************************************************/

int32
BTimeBase::_SnoozeThread(void *arg)
{
	UNIMPLEMENTED();

	return 0;
}


void
BTimeBase::SnoozeThread()
{
	UNIMPLEMENTED();
}

/*************************************************************
 * public BMediaChannel
 *************************************************************/

BMediaChannel::BMediaChannel(mk_rate rate,
							 BMediaRenderer *renderer,
							 BEventStream *source)
{
	UNIMPLEMENTED();
}


BMediaChannel::~BMediaChannel()
{
	UNIMPLEMENTED();
}


BMediaRenderer *
BMediaChannel::Renderer()
{
	UNIMPLEMENTED();
	return NULL;
}


void
BMediaChannel::SetRenderer(BMediaRenderer *)
{
	UNIMPLEMENTED();
}


BEventStream *
BMediaChannel::Source()
{
	UNIMPLEMENTED();
	return NULL;
}


void
BMediaChannel::SetSource(BEventStream *)
{
	UNIMPLEMENTED();
}


mk_rate
BMediaChannel::Rate()
{
	UNIMPLEMENTED();

	return 0;
}


void
BMediaChannel::SetRate(mk_rate)
{
	UNIMPLEMENTED();
}


bool
BMediaChannel::LockChannel()
{
	UNIMPLEMENTED();

	return false;
}


status_t
BMediaChannel::LockWithTimeout(bigtime_t)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


void
BMediaChannel::UnlockChannel()
{
	UNIMPLEMENTED();
}


void
BMediaChannel::StreamChanged()
{
	UNIMPLEMENTED();
}


#endif	// __GNUC__ < 3
