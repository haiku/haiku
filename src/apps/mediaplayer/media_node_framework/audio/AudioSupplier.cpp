/*	Copyright 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 *	All Rights Reserved. Distributed under the terms of the MIT license.
 */


#include "AudioSupplier.h"

#include "AudioProducer.h"


AudioSupplier::AudioSupplier()
{
}


AudioSupplier::~AudioSupplier()
{
}


void
AudioSupplier::SetAudioProducer(AudioProducer* producer)
{
	fAudioProducer = producer;
}


status_t
AudioSupplier::InitCheck() const
{
	return fAudioProducer ? B_OK : B_NO_INIT;
}

