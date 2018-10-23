/*
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


// This is deprecated API that is not even implemented - no need to export
// it on a GCC4 build (BeIDE needs it to run, though, so it's worthwhile for
// GCC2)
#if __GNUC__ < 3


#include "OldAudioModule.h"

#include <MediaDebug.h>


/*************************************************************
 * public BAudioEvent
 *************************************************************/

BAudioEvent::BAudioEvent(int32 frames, bool stereo, float *samples)
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

	return 0;
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

	return 0;
}


int32
BAudioEvent::Frames()
{
	UNIMPLEMENTED();

	return 0;
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

	return 0;
}


float
BAudioEvent::Gain()
{
	UNIMPLEMENTED();

	return 0.0f;
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

	return 0;
}


void
BAudioEvent::SetDestination(int32)
{
	UNIMPLEMENTED();
}


bool
BAudioEvent::MixIn(float *dst, int32 frames, mk_time time)
{
	UNIMPLEMENTED();

	return false;
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

	return 0;
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

	return 0;
}


mk_time
BDACRenderer::Latency()
{
	UNIMPLEMENTED();

	return 0;
}


mk_time
BDACRenderer::Start()
{
	UNIMPLEMENTED();

	return 0;
}


mk_time
BDACRenderer::Duration()
{
	UNIMPLEMENTED();

	return 0;
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
BDACRenderer::TransportChanged(mk_time time, mk_rate rate,
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
BDACRenderer::_WriteDAC(void *arg, char *buf, uint32 bytes, void *header)
{
	UNIMPLEMENTED();

	return false;
}


bool
BDACRenderer::WriteDAC(short *buf, int32 frames, audio_buffer_header *header)
{
	UNIMPLEMENTED();

	return false;
}


bool
BDACRenderer::MixActiveSegments(mk_time start)
{
	UNIMPLEMENTED();

	return false;
}


void
BDACRenderer::MixOutput(short *dst)
{
	UNIMPLEMENTED();
}


/*************************************************************
 * public BAudioFileStream
 *************************************************************/

BAudioFileStream::BAudioFileStream(BMediaChannel *channel, BFile *file,
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
BAudioFileStream::PeekEvent(BMediaChannel *channel, mk_time asap)
{
	UNIMPLEMENTED();
	return NULL;
}


status_t
BAudioFileStream::SeekToTime(BMediaChannel *channel, mk_time time)
{
	UNIMPLEMENTED();

	return B_ERROR;
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

	return 0;
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

BADCSource::BADCSource(BMediaChannel *channel, mk_time start)
	:
	fEventLock("BADCSource lock")
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
BADCSource::PeekEvent(BMediaChannel *channel, mk_time asap)
{
	UNIMPLEMENTED();
	return NULL;
}


status_t
BADCSource::SeekToTime(BMediaChannel *channel, mk_time time)
{
	UNIMPLEMENTED();

	return B_ERROR;
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
BADCSource::_ReadADC(void *arg, char *buf, uint32 bytes, void *header)
{
	UNIMPLEMENTED();

	return false;
}


void
BADCSource::ReadADC(short *buf, int32 frames, audio_buffer_header *header)
{
	UNIMPLEMENTED();
}


#endif	// __GNUC__ < 3
