/**
 * @file MidiRoster.cpp
 *
 * @author Matthijs Hollemans
 */

#include "debug.h"
#include "MidiRoster.h"

//------------------------------------------------------------------------------

BMidiEndpoint* BMidiRoster::NextEndpoint(int32* id)
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

BMidiProducer* BMidiRoster::NextProducer(int32* id)
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

BMidiConsumer* BMidiRoster::NextConsumer(int32* id)
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

BMidiEndpoint* BMidiRoster::FindEndpoint(int32 id, bool local_only)
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

BMidiProducer* BMidiRoster::FindProducer(int32 id, bool local_only)
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

BMidiConsumer* BMidiRoster::FindConsumer(int32 id, bool local_only)
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

void BMidiRoster::StartWatching(const BMessenger* msngr)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiRoster::StopWatching()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

status_t BMidiRoster::Register(BMidiEndpoint* object)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiRoster::Unregister(BMidiEndpoint* object)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

BMidiRoster* BMidiRoster::MidiRoster()
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

BMidiRoster::BMidiRoster(BMessenger* remote)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

BMidiRoster::~BMidiRoster()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiRoster::_Reserved1() { }
void BMidiRoster::_Reserved2() { }
void BMidiRoster::_Reserved3() { }
void BMidiRoster::_Reserved4() { }
void BMidiRoster::_Reserved5() { }
void BMidiRoster::_Reserved6() { }
void BMidiRoster::_Reserved7() { }
void BMidiRoster::_Reserved8() { }

//------------------------------------------------------------------------------

status_t BMidiRoster::RemoteConnect(
	int32 producer, int32 consumer, int32 port)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiRoster::RemoteDisconnect(int32 producer, int32 consumer)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

void BMidiRoster::RemoteConnected(int32 producer, int32 consumer)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiRoster::RemoteDisconnected(int32 producer, int32 consumer)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

BMidiEndpoint* BMidiRoster::RemoteCreateProducer(
	int32 producer, const char* name)
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

BMidiEndpoint* BMidiRoster::RemoteCreateConsumer(
	int32 consumer, const char* name, int32 port, int32 latency)
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

void BMidiRoster::RemoteDelete(BMidiEndpoint* object)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiRoster::RemoteRename(
	BMidiEndpoint* object, const char* name)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiRoster::RemoteChangeLatency(
	BMidiEndpoint* object, bigtime_t latency)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

status_t BMidiRoster::Remote(int32 id, int32 op)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiRoster::DoRemote(BMessage* msg, BMessage* result)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

void BMidiRoster::Rename(BMidiEndpoint* midi, const char* name)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

status_t BMidiRoster::Release(BMidiEndpoint* midi)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiRoster::Acquire(BMidiEndpoint* midi)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

void BMidiRoster::Create(BMidiEndpoint* midi)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiRoster::SetLatency(BMidiConsumer* midi, bigtime_t latency)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

status_t BMidiRoster::SetProperties(
	BMidiEndpoint* midi, const BMessage* props)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiRoster::GetProperties(
	const BMidiEndpoint* midi, BMessage* props)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiRoster::Connect(
	BMidiProducer* source, BMidiConsumer* sink)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiRoster::Disconnect(
	BMidiProducer* source, BMidiConsumer* sink)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

BLooper* BMidiRoster::Looper()
{
	UNIMPLEMENTED
	return NULL;
}

//------------------------------------------------------------------------------

