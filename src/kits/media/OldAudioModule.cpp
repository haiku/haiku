/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: OldAudioModule.cpp
 *  DESCR: 
 ***********************************************************************/
#include <OldAudioModule.h>
#include "debug.h"

/*************************************************************
 * public BAudioEvent
 *************************************************************/

BAudioEvent::BAudioEvent(int32 frames,
						 bool stereo,
						 float *samples)
{
	UNIMPLEMENTED();
}


BAudioEvent::~BAudioEvent()
{
	UNIMPLEMENTED();
}


mk_time
BAudioEvent::Start()
{
	UNIMPLEMENTED();
	mk_time dummy;

	return dummy;
}


void
BAudioEvent::SetStart(mk_time)
{
	UNIMPLEMENTED();
}


mk_time
BAudioEvent::Duration()
{
	UNIMPLEMENTED();
	mk_time dummy;

	return dummy;
}


int32
BAudioEvent::Frames()
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


float *
BAudioEvent::Samples()
{
	UNIMPLEMENTED();
	return NULL;
}


int32
BAudioEvent::ChannelCount()
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


float
BAudioEvent::Gain()
{
	UNIMPLEMENTED();
	float dummy;

	return dummy;
}


void
BAudioEvent::SetGain(float)
{
	UNIMPLEMENTED();
}


int32
BAudioEvent::Destination()
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


void
BAudioEvent::SetDestination(int32)
{
	UNIMPLEMENTED();
}


bool
BAudioEvent::MixIn(float *dst,
				   int32 frames,
				   mk_time time)
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}


BMediaEvent *
BAudioEvent::Clone()
{
	UNIMPLEMENTED();
	return NULL;
}


bigtime_t
BAudioEvent::CaptureTime()
{
	UNIMPLEMENTED();
	bigtime_t dummy;

	return dummy;
}


void
BAudioEvent::SetCaptureTime(bigtime_t)
{
	UNIMPLEMENTED();
}


/*************************************************************
 * public BDACRenderer
 *************************************************************/

BDACRenderer::BDACRenderer(const char *name)
{
	UNIMPLEMENTED();
}


BDACRenderer::~BDACRenderer()
{
	UNIMPLEMENTED();
}


mk_rate
BDACRenderer::Units()
{
	UNIMPLEMENTED();
	mk_rate dummy;

	return dummy;
}


mk_time
BDACRenderer::Latency()
{
	UNIMPLEMENTED();
	mk_time dummy;

	return dummy;
}


mk_time
BDACRenderer::Start()
{
	UNIMPLEMENTED();
	mk_time dummy;

	return dummy;
}


mk_time
BDACRenderer::Duration()
{
	UNIMPLEMENTED();
	mk_time dummy;

	return dummy;
}


BTimeBase *
BDACRenderer::TimeBase()
{
	UNIMPLEMENTED();
	return NULL;
}


void
BDACRenderer::Open()
{
	UNIMPLEMENTED();
}


void
BDACRenderer::Close()
{
	UNIMPLEMENTED();
}


void
BDACRenderer::Wakeup()
{
	UNIMPLEMENTED();
}


void
BDACRenderer::TransportChanged(mk_time time,
							   mk_rate rate,
							   transport_status status)
{
	UNIMPLEMENTED();
}


void
BDACRenderer::StreamChanged()
{
	UNIMPLEMENTED();
}


BMediaChannel *
BDACRenderer::Channel()
{
	UNIMPLEMENTED();
	return NULL;
}

/*************************************************************
 * private BDACRenderer
 *************************************************************/


bool
BDACRenderer::_WriteDAC(void *arg,
						char *buf,
						uint32 bytes,
						void *header)
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}


bool
BDACRenderer::WriteDAC(short *buf,
					   int32 frames,
					   audio_buffer_header *header)
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}


bool
BDACRenderer::MixActiveSegments(mk_time start)
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}


void
BDACRenderer::MixOutput(short *dst)
{
	UNIMPLEMENTED();
}


/*************************************************************
 * public BAudioFileStream
 *************************************************************/

BAudioFileStream::BAudioFileStream(BMediaChannel *channel,
								   BFile *file,
								   mk_time start)
{
	UNIMPLEMENTED();
}


BAudioFileStream::~BAudioFileStream()
{
	UNIMPLEMENTED();
}


BMediaEvent *
BAudioFileStream::GetEvent(BMediaChannel *channel)
{
	UNIMPLEMENTED();
	return NULL;
}


BMediaEvent *
BAudioFileStream::PeekEvent(BMediaChannel *channel,
							mk_time asap)
{
	UNIMPLEMENTED();
	return NULL;
}


status_t
BAudioFileStream::SeekToTime(BMediaChannel *channel,
							 mk_time time)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


void
BAudioFileStream::SetStart(mk_time start)
{
	UNIMPLEMENTED();
}


bigtime_t
BAudioFileStream::CaptureTime()
{
	UNIMPLEMENTED();
	bigtime_t dummy;

	return dummy;
}


BMediaChannel *
BAudioFileStream::Channel()
{
	UNIMPLEMENTED();
	return NULL;
}

/*************************************************************
 * public BADCSource
 *************************************************************/

BADCSource::BADCSource(BMediaChannel *channel,
					   mk_time start)
{
	UNIMPLEMENTED();
}


BADCSource::~BADCSource()
{
	UNIMPLEMENTED();
}


BMediaEvent *
BADCSource::GetEvent(BMediaChannel *channel)
{
	UNIMPLEMENTED();
	return NULL;
}


BMediaEvent *
BADCSource::PeekEvent(BMediaChannel *channel,
					  mk_time asap)
{
	UNIMPLEMENTED();
	return NULL;
}


status_t
BADCSource::SeekToTime(BMediaChannel *channel,
					   mk_time time)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


void
BADCSource::SetStart(mk_time start)
{
	UNIMPLEMENTED();
}


BMediaChannel *
BADCSource::Channel()
{
	UNIMPLEMENTED();
	return NULL;
}

/*************************************************************
 * private BADCSource
 *************************************************************/

bool
BADCSource::_ReadADC(void *arg,
					 char *buf,
					 uint32 bytes,
					 void *header)
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}


void
BADCSource::ReadADC(short *buf,
					int32 frames,
					audio_buffer_header *header)
{
	UNIMPLEMENTED();
}


