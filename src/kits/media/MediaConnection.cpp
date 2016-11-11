/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <MediaConnection.h>

#include <MediaClient.h>

#include "debug.h"


bool
BMediaConnection::IsOutput() const
{
	CALLED();

	return fKind == B_MEDIA_OUTPUT;
}


bool
BMediaConnection::IsInput() const
{
	CALLED();

	return fKind == B_MEDIA_INPUT;
}


void
BMediaConnection::BuildMediaOutput(media_output* output) const
{
	CALLED();

	output->format = AcceptedFormat();
	output->node = fOwner->fNode->Node();
	output->source = fSource;
	output->source.port = fOwner->fNode->ControlPort();
}


void
BMediaConnection::BuildMediaInput(media_input* input) const
{
	CALLED();

	input->format = AcceptedFormat();
	input->node = fOwner->fNode->Node();
	input->destination = fDestination;
	input->destination.port = fOwner->fNode->ControlPort();
}


const media_destination&
BMediaConnection::Destination() const
{
	CALLED();

	return fDestination;
}


const media_source&
BMediaConnection::Source() const
{
	CALLED();

	return fSource;
}


bool
BMediaConnection::HasBinding() const
{
	CALLED();

	return fBind != NULL;
}


BMediaConnection*
BMediaConnection::Binding() const
{
	CALLED();

	return fBind;
}


void
BMediaConnection::SetAcceptedFormat(const media_format& format)
{
	CALLED();

	fFormat = format;
}


const media_format&
BMediaConnection::AcceptedFormat() const
{
	CALLED();

	return fFormat;
}


bool
BMediaConnection::IsConnected() const
{
	CALLED();

	return fConnected;
}


bool
BMediaConnection::IsOutputEnabled() const
{
	CALLED();

	return fOutputEnabled;
}


void*
BMediaConnection::Cookie() const
{
	CALLED();

	return fBufferCookie;
}


status_t
BMediaConnection::Disconnect()
{
	CALLED();

	return fOwner->DisconnectConnection(this);
}


status_t
BMediaConnection::Reset()
{
	CALLED();

	if (IsOutput())
		fDestination = media_destination::null;
	else
		fSource = media_source::null;

	delete fBufferGroup;
	fBufferGroup = NULL;

	return fOwner->ResetConnection(this);
}


status_t
BMediaConnection::Release()
{
	CALLED();

	return fOwner->ReleaseConnection(this);
}


void
BMediaConnection::SetHooks(process_hook processHook,
	notify_hook notifyHook, void* cookie)
{
	CALLED();

	fProcessHook = processHook;
	fNotifyHook = notifyHook;
	fBufferCookie = cookie;
}


BMediaConnection::BMediaConnection(BMediaClient* owner,
	media_connection_kind kind)
	:
	fKind(kind),
	fOwner(owner),
	fBind(NULL)
{
	CALLED();

	_Init();
}


BMediaConnection::BMediaConnection(BMediaClient* owner,
	const media_output& output)
	:
	fKind(B_MEDIA_OUTPUT),
	fOwner(owner),
	fBind(NULL)
{
	CALLED();

	_Init();
}


BMediaConnection::BMediaConnection(BMediaClient* owner,
	const media_input& input)
	:
	fKind(B_MEDIA_INPUT),
	fOwner(owner),
	fBind(NULL)
{
	CALLED();

	_Init();
}


BMediaConnection::~BMediaConnection()
{
	CALLED();

}


void
BMediaConnection::SetBufferSize(size_t size)
{
	CALLED();

	fBufferSize = size;
}


size_t
BMediaConnection::BufferSize() const
{
	CALLED();

	return fBufferSize;
}


void
BMediaConnection::SetBufferDuration(bigtime_t duration)
{
	CALLED();

	fBufferDuration = duration;
}


bigtime_t
BMediaConnection::BufferDuration() const
{
	CALLED();

	return fBufferDuration;
}


void
BMediaConnection::ConnectedCallback(const media_source& source,
	const media_format& format)
{
	fSource = source;
	SetAcceptedFormat(format);

	if (fNotifyHook != NULL)
		(*fNotifyHook)(B_CONNECTED, this);

	fConnected = true;
}


void
BMediaConnection::DisconnectedCallback(const media_source& source)
{
	fSource = media_source::null;

	if (fNotifyHook != NULL)
		(*fNotifyHook)(B_DISCONNECTED, this);

	fConnected = false;
}


void
BMediaConnection::ConnectCallback(const media_destination& destination)
{
	fDestination = destination;
}


void
BMediaConnection::DisconnectCallback(const media_destination& destination)
{
	fDestination = destination;
}


void
BMediaConnection::_Init()
{
	CALLED();

	fBufferGroup = NULL;
	fNotifyHook = NULL;
	fProcessHook = NULL;

	fSource = media_source::null;
	fDestination = media_destination::null;
}


void BMediaConnection::_ReservedMediaConnection0() {}
void BMediaConnection::_ReservedMediaConnection1() {}
void BMediaConnection::_ReservedMediaConnection2() {}
void BMediaConnection::_ReservedMediaConnection3() {}
void BMediaConnection::_ReservedMediaConnection4() {}
void BMediaConnection::_ReservedMediaConnection5() {}
void BMediaConnection::_ReservedMediaConnection6() {}
void BMediaConnection::_ReservedMediaConnection7() {}
void BMediaConnection::_ReservedMediaConnection8() {}
void BMediaConnection::_ReservedMediaConnection9() {}
void BMediaConnection::_ReservedMediaConnection10() {}
