/*
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


// This is deprecated API that is not even implemented - no need to export
// it on a GCC4 build (BeIDE needs it to run, though, so it's worthwhile for
// GCC2)
#if __GNUC__ < 3


#include "OldBufferStream.h"

#include <MediaDebug.h>
#include <new>


/*************************************************************
 * public BAbstractBufferStream
 *************************************************************/
status_t
BAbstractBufferStream::GetStreamParameters(size_t *bufferSize,
										   int32 *bufferCount,
										   bool *isRunning,
										   int32 *subscriberCount) const
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BAbstractBufferStream::SetStreamBuffers(size_t bufferSize,
										int32 bufferCount)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BAbstractBufferStream::StartStreaming()
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BAbstractBufferStream::StopStreaming()
{
	UNIMPLEMENTED();

	return B_ERROR;
}

/*************************************************************
 * protected BAbstractBufferStream
 *************************************************************/

void
BAbstractBufferStream::_ReservedAbstractBufferStream1()
{
	UNIMPLEMENTED();
}


void
BAbstractBufferStream::_ReservedAbstractBufferStream2()
{
	UNIMPLEMENTED();
}


void
BAbstractBufferStream::_ReservedAbstractBufferStream3()
{
	UNIMPLEMENTED();
}


void
BAbstractBufferStream::_ReservedAbstractBufferStream4()
{
	UNIMPLEMENTED();
}


stream_id
BAbstractBufferStream::StreamID() const
{
	UNIMPLEMENTED();

	return 0;
}


status_t
BAbstractBufferStream::Subscribe(char *name,
								 subscriber_id *subID,
								 sem_id semID)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BAbstractBufferStream::Unsubscribe(subscriber_id subID)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BAbstractBufferStream::EnterStream(subscriber_id subID,
								   subscriber_id neighbor,
								   bool before)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BAbstractBufferStream::ExitStream(subscriber_id subID)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


BMessenger *
BAbstractBufferStream::Server() const
{
	UNIMPLEMENTED();
	return NULL;
}


status_t
BAbstractBufferStream::SendRPC(BMessage *msg,
							   BMessage *reply) const
{
	UNIMPLEMENTED();

	return B_ERROR;
}

/*************************************************************
 * public BBufferStream
 *************************************************************/

BBufferStream::BBufferStream(size_t headerSize,
							 BBufferStreamManager *controller,
							 BSubscriber *headFeeder,
							 BSubscriber *tailFeeder)
{
	UNIMPLEMENTED();
}


BBufferStream::~BBufferStream()
{
	UNIMPLEMENTED();
}


void *
BBufferStream::operator new(size_t size)
{
	UNIMPLEMENTED();

	void *dummy = NULL;
		// just to circumvent a warning that operator new should not return NULL
	return dummy;
}


void
BBufferStream::operator delete(void *stream, size_t size)
{
	UNIMPLEMENTED();
}


size_t
BBufferStream::HeaderSize() const
{
	UNIMPLEMENTED();

	return 0;
}


status_t
BBufferStream::GetStreamParameters(size_t *bufferSize,
								   int32 *bufferCount,
								   bool *isRunning,
								   int32 *subscriberCount) const
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BBufferStream::SetStreamBuffers(size_t bufferSize,
								int32 bufferCount)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BBufferStream::StartStreaming()
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BBufferStream::StopStreaming()
{
	UNIMPLEMENTED();

	return B_ERROR;
}


BBufferStreamManager *
BBufferStream::StreamManager() const
{
	UNIMPLEMENTED();
	return NULL;
}


int32
BBufferStream::CountBuffers() const
{
	UNIMPLEMENTED();

	return 0;
}


status_t
BBufferStream::Subscribe(char *name,
						 subscriber_id *subID,
						 sem_id semID)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BBufferStream::Unsubscribe(subscriber_id subID)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BBufferStream::EnterStream(subscriber_id subID,
						   subscriber_id neighbor,
						   bool before)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BBufferStream::ExitStream(subscriber_id subID)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


bool
BBufferStream::IsSubscribed(subscriber_id subID)
{
	UNIMPLEMENTED();

	return false;
}


bool
BBufferStream::IsEntered(subscriber_id subID)
{
	UNIMPLEMENTED();

	return false;
}


status_t
BBufferStream::SubscriberInfo(subscriber_id subID,
							  char **name,
							  stream_id *streamID,
							  int32 *position)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BBufferStream::UnblockSubscriber(subscriber_id subID)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BBufferStream::AcquireBuffer(subscriber_id subID,
							 buffer_id *bufID,
							 bigtime_t timeout)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BBufferStream::ReleaseBuffer(subscriber_id subID)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


size_t
BBufferStream::BufferSize(buffer_id bufID) const
{
	UNIMPLEMENTED();

	return 0;
}


char *
BBufferStream::BufferData(buffer_id bufID) const
{
	UNIMPLEMENTED();
	return NULL;
}


bool
BBufferStream::IsFinalBuffer(buffer_id bufID) const
{
	UNIMPLEMENTED();

	return false;
}


int32
BBufferStream::CountBuffersHeld(subscriber_id subID)
{
	UNIMPLEMENTED();

	return 0;
}


int32
BBufferStream::CountSubscribers() const
{
	UNIMPLEMENTED();

	return 0;
}


int32
BBufferStream::CountEnteredSubscribers() const
{
	UNIMPLEMENTED();

	return 0;
}


subscriber_id
BBufferStream::FirstSubscriber() const
{
	UNIMPLEMENTED();

	return 0;
}


subscriber_id
BBufferStream::LastSubscriber() const
{
	UNIMPLEMENTED();

	return 0;
}


subscriber_id
BBufferStream::NextSubscriber(subscriber_id subID)
{
	UNIMPLEMENTED();

	return 0;
}


subscriber_id
BBufferStream::PrevSubscriber(subscriber_id subID)
{
	UNIMPLEMENTED();

	return 0;
}


void
BBufferStream::PrintStream()
{
	UNIMPLEMENTED();
}


void
BBufferStream::PrintBuffers()
{
	UNIMPLEMENTED();
}


void
BBufferStream::PrintSubscribers()
{
	UNIMPLEMENTED();
}


bool
BBufferStream::Lock()
{
	UNIMPLEMENTED();

	return false;
}


void
BBufferStream::Unlock()
{
	UNIMPLEMENTED();
}


status_t
BBufferStream::AddBuffer(buffer_id bufID)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


buffer_id
BBufferStream::RemoveBuffer(bool force)
{
	UNIMPLEMENTED();

	return 0;
}


buffer_id
BBufferStream::CreateBuffer(size_t size,
							bool isFinal)
{
	UNIMPLEMENTED();

	return 0;
}


void
BBufferStream::DestroyBuffer(buffer_id bufID)
{
	UNIMPLEMENTED();
}


void
BBufferStream::RescindBuffers()
{
	UNIMPLEMENTED();
}

/*************************************************************
 * private BBufferStream
 *************************************************************/

void
BBufferStream::_ReservedBufferStream1()
{
	UNIMPLEMENTED();
}


void
BBufferStream::_ReservedBufferStream2()
{
	UNIMPLEMENTED();
}


void
BBufferStream::_ReservedBufferStream3()
{
	UNIMPLEMENTED();
}


void
BBufferStream::_ReservedBufferStream4()
{
	UNIMPLEMENTED();
}


void
BBufferStream::InitSubscribers()
{
	UNIMPLEMENTED();
}


bool
BBufferStream::IsSubscribedSafe(subscriber_id subID) const
{
	UNIMPLEMENTED();

	return false;
}


bool
BBufferStream::IsEnteredSafe(subscriber_id subID) const
{
	UNIMPLEMENTED();

	return false;
}


void
BBufferStream::InitBuffers()
{
	UNIMPLEMENTED();
}


status_t
BBufferStream::WakeSubscriber(subscriber_id subID)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


void
BBufferStream::InheritBuffers(subscriber_id subID)
{
	UNIMPLEMENTED();
}


void
BBufferStream::BequeathBuffers(subscriber_id subID)
{
	UNIMPLEMENTED();
}


status_t
BBufferStream::ReleaseBufferSafe(subscriber_id subID)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BBufferStream::ReleaseBufferTo(buffer_id bufID,
							   subscriber_id subID)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


void
BBufferStream::FreeAllBuffers()
{
	UNIMPLEMENTED();
}


void
BBufferStream::FreeAllSubscribers()
{
	UNIMPLEMENTED();
}


#endif	// __GNUC__ < 3

