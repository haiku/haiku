/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: OldMediaModule.cpp
 *  DESCR: 
 ***********************************************************************/
#include <OldMediaModule.h>
#include "debug.h"

/*************************************************************
 * public BMediaEvent
 *************************************************************/

mk_time
BMediaEvent::Duration()
{
	UNIMPLEMENTED();
	mk_time dummy;

	return dummy;
}


bigtime_t
BMediaEvent::CaptureTime()
{
	UNIMPLEMENTED();
	bigtime_t dummy;

	return dummy;
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
	mk_time dummy;

	return dummy;
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
	mk_time dummy;

	return dummy;
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
	status_t dummy;

	return dummy;
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
	mk_time dummy;

	return dummy;
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
	mk_time dummy;

	return dummy;
}


mk_time
BMediaRenderer::Duration()
{
	UNIMPLEMENTED();
	mk_time dummy;

	return dummy;
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
	int32 dummy;

	return dummy;
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
	bool dummy;

	return dummy;
}


transport_status
BTransport::Status()
{
	UNIMPLEMENTED();
	transport_status dummy;

	return dummy;
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
	mk_time dummy;

	return dummy;
}


mk_rate
BTransport::PerformanceRate()
{
	UNIMPLEMENTED();
	mk_rate dummy;

	return dummy;
}


mk_time
BTransport::TimeOffset()
{
	UNIMPLEMENTED();
	mk_time dummy;

	return dummy;
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
	mk_time dummy;

	return dummy;
}


mk_time
BTransport::PerformanceStart()
{
	UNIMPLEMENTED();
	mk_time dummy;

	return dummy;
}


mk_time
BTransport::PerformanceEnd()
{
	UNIMPLEMENTED();
	mk_time dummy;

	return dummy;
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
	bool dummy;

	return dummy;
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
	status_t dummy;

	return dummy;
}


mk_time
BTimeBase::Time()
{
	UNIMPLEMENTED();
	mk_time dummy;

	return dummy;
}


mk_rate
BTimeBase::Rate()
{
	UNIMPLEMENTED();
	mk_rate dummy;

	return dummy;
}


mk_time
BTimeBase::TimeAt(bigtime_t system_time)
{
	UNIMPLEMENTED();
	mk_time dummy;

	return dummy;
}


bigtime_t
BTimeBase::SystemTimeAt(mk_time time)
{
	UNIMPLEMENTED();
	bigtime_t dummy;

	return dummy;
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
	bool dummy;

	return dummy;
}

/*************************************************************
 * private BTimeBase
 *************************************************************/

int32
BTimeBase::_SnoozeThread(void *arg)
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
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
	mk_rate dummy;

	return dummy;
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
	bool dummy;

	return dummy;
}


status_t
BMediaChannel::LockWithTimeout(bigtime_t)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
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


