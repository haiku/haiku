/*
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


// This is deprecated API that is not even implemented - no need to export
// it on a GCC4 build (BeIDE needs it to run, though, so it's worthwhile for
// GCC2)
#if __GNUC__ < 3


#include "OldAudioStream.h"

#include <MediaDebug.h>


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

	return B_ERROR;
}


status_t
BADCStream::ADCInput(int32 *device) const
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BADCStream::SetSamplingRate(float sRate)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BADCStream::SamplingRate(float *sRate) const
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BADCStream::BoostMic(bool boost)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


bool
BADCStream::IsMicBoosted() const
{
	UNIMPLEMENTED();

	return false;
}


status_t
BADCStream::SetStreamBuffers(size_t bufferSize,
							 int32 bufferCount)
{
	UNIMPLEMENTED();

	return B_ERROR;
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

	return 0;
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

	return B_ERROR;
}


status_t
BDACStream::SamplingRate(float *sRate) const
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BDACStream::SetVolume(int32 device,
					  float l_volume,
					  float r_volume)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BDACStream::GetVolume(int32 device,
					  float *l_volume,
					  float *r_volume,
					  bool *enabled) const
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BDACStream::EnableDevice(int32 device,
						 bool enable)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


bool
BDACStream::IsDeviceEnabled(int32 device) const
{
	UNIMPLEMENTED();

	return false;
}


status_t
BDACStream::SetStreamBuffers(size_t bufferSize,
							 int32 bufferCount)
{
	UNIMPLEMENTED();

	return B_ERROR;
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

	return 0;
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


#endif	// __GNUC__ < 3
