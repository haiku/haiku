/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: OldAudioStream.cpp
 *  DESCR: 
 ***********************************************************************/
#include <OldAudioStream.h>
#include "debug.h"

/*************************************************************
 * public BADCStream
 *************************************************************/

BADCStream::BADCStream()
{
	UNIMPLEMENTED();
}


BADCStream::~BADCStream()
{
	UNIMPLEMENTED();
}


status_t
BADCStream::SetADCInput(int32 device)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BADCStream::ADCInput(int32 *device) const
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BADCStream::SetSamplingRate(float sRate)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BADCStream::SamplingRate(float *sRate) const
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BADCStream::BoostMic(bool boost)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


bool
BADCStream::IsMicBoosted() const
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}


status_t
BADCStream::SetStreamBuffers(size_t bufferSize,
							 int32 bufferCount)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}

/*************************************************************
 * protected BADCStream
 *************************************************************/


BMessenger *
BADCStream::Server() const
{
	UNIMPLEMENTED();
	return NULL;
}


stream_id
BADCStream::StreamID() const
{
	UNIMPLEMENTED();
	stream_id dummy;

	return dummy;
}

/*************************************************************
 * private BADCStream
 *************************************************************/


void
BADCStream::_ReservedADCStream1()
{
	UNIMPLEMENTED();
}


void
BADCStream::_ReservedADCStream2()
{
	UNIMPLEMENTED();
}


void
BADCStream::_ReservedADCStream3()
{
	UNIMPLEMENTED();
}

/*************************************************************
 * public BDACStream
 *************************************************************/

BDACStream::BDACStream()
{
	UNIMPLEMENTED();
}


BDACStream::~BDACStream()
{
	UNIMPLEMENTED();
}


status_t
BDACStream::SetSamplingRate(float sRate)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BDACStream::SamplingRate(float *sRate) const
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BDACStream::SetVolume(int32 device,
					  float l_volume,
					  float r_volume)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BDACStream::GetVolume(int32 device,
					  float *l_volume,
					  float *r_volume,
					  bool *enabled) const
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BDACStream::EnableDevice(int32 device,
						 bool enable)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


bool
BDACStream::IsDeviceEnabled(int32 device) const
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}


status_t
BDACStream::SetStreamBuffers(size_t bufferSize,
							 int32 bufferCount)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}

/*************************************************************
 * protected BDACStream
 *************************************************************/

BMessenger *
BDACStream::Server() const
{
	UNIMPLEMENTED();
	return NULL;
}


stream_id
BDACStream::StreamID() const
{
	UNIMPLEMENTED();
	stream_id dummy;

	return dummy;
}

/*************************************************************
 * private BDACStream
 *************************************************************/

void
BDACStream::_ReservedDACStream1()
{
	UNIMPLEMENTED();
}


void
BDACStream::_ReservedDACStream2()
{
	UNIMPLEMENTED();
}


void
BDACStream::_ReservedDACStream3()
{
	UNIMPLEMENTED();
}


