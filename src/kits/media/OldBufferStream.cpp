/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: OldBufferStream.cpp
 *  DESCR: 
 ***********************************************************************/
#include <OldBufferStream.h>
#include "debug.h"

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
	status_t dummy;

	return dummy;
}


status_t
BAbstractBufferStream::SetStreamBuffers(size_t bufferSize,
										int32 bufferCount)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}
		

status_t
BAbstractBufferStream::StartStreaming()
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BAbstractBufferStream::StopStreaming()
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
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
	stream_id dummy;

	return dummy;
}


status_t
BAbstractBufferStream::Subscribe(char *name,
								 subscriber_id *subID,
								 sem_id semID)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BAbstractBufferStream::Unsubscribe(subscriber_id subID)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BAbstractBufferStream::EnterStream(subscriber_id subID,
								   subscriber_id neighbor,
								   bool before)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BAbstractBufferStream::ExitStream(subscriber_id subID)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
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
	status_t dummy;

	return dummy;
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
	return NULL;
}


void
BBufferStream::operator delete(void *stream,
							   size_t size)
{
	UNIMPLEMENTED();
}


size_t
BBufferStream::HeaderSize() const
{
	UNIMPLEMENTED();
	size_t dummy;

	return dummy;
}


status_t
BBufferStream::GetStreamParameters(size_t *bufferSize,
								   int32 *bufferCount,
								   bool *isRunning,
								   int32 *subscriberCount) const
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BBufferStream::SetStreamBuffers(size_t bufferSize,
								int32 bufferCount)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BBufferStream::StartStreaming()
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BBufferStream::StopStreaming()
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
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
	int32 dummy;

	return dummy;
}


status_t
BBufferStream::Subscribe(char *name,
						 subscriber_id *subID,
						 sem_id semID)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BBufferStream::Unsubscribe(subscriber_id subID)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BBufferStream::EnterStream(subscriber_id subID,
						   subscriber_id neighbor,
						   bool before)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BBufferStream::ExitStream(subscriber_id subID)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


bool
BBufferStream::IsSubscribed(subscriber_id subID)
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}


bool
BBufferStream::IsEntered(subscriber_id subID)
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}


status_t
BBufferStream::SubscriberInfo(subscriber_id subID,
							  char **name,
							  stream_id *streamID,
							  int32 *position)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BBufferStream::UnblockSubscriber(subscriber_id subID)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BBufferStream::AcquireBuffer(subscriber_id subID,
							 buffer_id *bufID,
							 bigtime_t timeout)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BBufferStream::ReleaseBuffer(subscriber_id subID)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


size_t
BBufferStream::BufferSize(buffer_id bufID) const
{
	UNIMPLEMENTED();
	size_t dummy;

	return dummy;
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
	bool dummy;

	return dummy;
}


int32
BBufferStream::CountBuffersHeld(subscriber_id subID)
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BBufferStream::CountSubscribers() const
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BBufferStream::CountEnteredSubscribers() const
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


subscriber_id
BBufferStream::FirstSubscriber() const
{
	UNIMPLEMENTED();
	subscriber_id dummy;

	return dummy;
}


subscriber_id
BBufferStream::LastSubscriber() const
{
	UNIMPLEMENTED();
	subscriber_id dummy;

	return dummy;
}


subscriber_id
BBufferStream::NextSubscriber(subscriber_id subID)
{
	UNIMPLEMENTED();
	subscriber_id dummy;

	return dummy;
}


subscriber_id
BBufferStream::PrevSubscriber(subscriber_id subID)
{
	UNIMPLEMENTED();
	subscriber_id dummy;

	return dummy;
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
	bool dummy;

	return dummy;
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
	status_t dummy;

	return dummy;
}


buffer_id
BBufferStream::RemoveBuffer(bool force)
{
	UNIMPLEMENTED();
	buffer_id dummy;

	return dummy;
}


buffer_id
BBufferStream::CreateBuffer(size_t size,
							bool isFinal)
{
	UNIMPLEMENTED();
	buffer_id dummy;

	return dummy;
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
	bool dummy;

	return dummy;
}


bool
BBufferStream::IsEnteredSafe(subscriber_id subID) const
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
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
	status_t dummy;

	return dummy;
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
	status_t dummy;

	return dummy;
}


status_t
BBufferStream::ReleaseBufferTo(buffer_id bufID,
							   subscriber_id subID)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
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


