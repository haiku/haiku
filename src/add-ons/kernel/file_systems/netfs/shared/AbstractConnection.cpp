// AbstractConnection.cpp

#include "AbstractConnection.h"

#include <AutoLocker.h>

#include "Channel.h"
#include "DebugSupport.h"

// constructor
AbstractConnection::AbstractConnection()
	: Connection(),
	  fInitStatus(B_NO_INIT),
	  fDownStreamChannels(),
	  fUpStreamChannelSemaphore(-1),
	  fUpStreamChannelLock(),
	  fUpStreamChannels(),
	  fFreeUpStreamChannels(0)
{
}

// destructor
AbstractConnection::~AbstractConnection()
{
	// Danger: If derived classes implement Close(), this call will not reach
	// them. So, the caller should rather call Close() explicitely before
	// destroying the connection.
	Close();
	// delete all channels
	for (ChannelVector::Iterator it = fDownStreamChannels.Begin();
		 it != fDownStreamChannels.End();
		 it++) {
		delete *it;
	}
	fDownStreamChannels.MakeEmpty();
	AutoLocker<Locker> _(fUpStreamChannelLock);
	for (ChannelVector::Iterator it = fUpStreamChannels.Begin();
		 it != fUpStreamChannels.End();
		 it++) {
		delete *it;
	}
	fUpStreamChannels.MakeEmpty();
}

// Init
status_t
AbstractConnection::Init()
{
	fInitStatus = B_OK;
	// create upstream channel semaphore
	fUpStreamChannelSemaphore = create_sem(0, "upstream channels");
	if (fUpStreamChannelSemaphore < 0)
		return (fInitStatus = fUpStreamChannelSemaphore);
	return fInitStatus;
}

// Close
void
AbstractConnection::Close()
{
	// close all channels
	for (ChannelVector::Iterator it = fDownStreamChannels.Begin();
		 it != fDownStreamChannels.End();
		 it++) {
		(*it)->Close();
	}
	AutoLocker<Locker> _(fUpStreamChannelLock);
	for (ChannelVector::Iterator it = fUpStreamChannels.Begin();
		 it != fUpStreamChannels.End();
		 it++) {
		(*it)->Close();
	}
}

//// GetAuthorizedUser
//User*
//AbstractConnection::GetAuthorizedUser()
//{
//	return NULL;
//}

// AddDownStreamChannel
status_t
AbstractConnection::AddDownStreamChannel(Channel* channel)
{
	if (!channel)
		return B_BAD_VALUE;
	return fDownStreamChannels.PushBack(channel);
}

// AddUpStreamChannel
status_t
AbstractConnection::AddUpStreamChannel(Channel* channel)
{
	if (!channel)
		return B_BAD_VALUE;
	AutoLocker<Locker> _(fUpStreamChannelLock);
	status_t error = fUpStreamChannels.PushBack(channel);
	if (error != B_OK)
		return error;
	PutUpStreamChannel(channel);
	return B_OK;
}

// CountDownStreamChannels
int32
AbstractConnection::CountDownStreamChannels() const
{
	return fDownStreamChannels.Count();
}

// GetDownStreamChannel
Channel*
AbstractConnection::DownStreamChannelAt(int32 index) const
{
	if (index < 0 || index >= fDownStreamChannels.Count())
		return NULL;
	return fDownStreamChannels.ElementAt(index);
}

// GetUpStreamChannel
status_t
AbstractConnection::GetUpStreamChannel(Channel** channel, bigtime_t timeout)
{
	if (!channel)
		return B_BAD_VALUE;
	if (timeout < 0)
		timeout = 0;
	// acquire the semaphore
	status_t error = acquire_sem_etc(fUpStreamChannelSemaphore, 1,
		B_RELATIVE_TIMEOUT, timeout);
	if (error != B_OK)
		return error;
	// we've acquire the semaphore successfully, so a free channel must be
	// waiting for us
	AutoLocker<Locker> _(fUpStreamChannelLock);
	if (fFreeUpStreamChannels < 1) {
		ERROR(("AbstractConnection::GetUpStreamChannel(): Acquired the "
			"upstream semaphore successfully, but there's no free channel!\n"));
		return B_ERROR;
	}
	fFreeUpStreamChannels--;
	*channel = fUpStreamChannels.ElementAt(fFreeUpStreamChannels);
	return B_OK;
}

// PutUpStreamChannel
status_t
AbstractConnection::PutUpStreamChannel(Channel* channel)
{
	if (!channel)
		return B_BAD_VALUE;
	// find the channel
	AutoLocker<Locker> _(fUpStreamChannelLock);
	int32 index = fUpStreamChannels.IndexOf(channel, fFreeUpStreamChannels);
	if (index < 0)
		return B_BAD_VALUE;
	// swap it with the first non-free channel, release the semaphore,
	// and bump the free channel count
	if (index != fFreeUpStreamChannels) {
		Channel*& target = fUpStreamChannels.ElementAt(fFreeUpStreamChannels);
		fUpStreamChannels.ElementAt(index) = target;
		target = channel;
	}
	status_t error = release_sem(fUpStreamChannelSemaphore);
	if (error == B_OK)
		fFreeUpStreamChannels++;
	return error;
}

