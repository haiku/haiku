/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <MediaConnection.h>

#include <string.h>

#include "MediaClientNode.h"

#include "MediaDebug.h"


BMediaConnection::BMediaConnection(media_connection_kinds kinds,
	const char* name)
	:
	fOwner(NULL),
	fBind(NULL)
{
	CALLED();

	fConnection.kinds = kinds;
	fConnection.id = -1;
	//fConnection.client = media_client::null;
	if (name != NULL)
		strcpy(fConnection.name, name);
}


BMediaConnection::~BMediaConnection()
{
	CALLED();

}


const media_connection&
BMediaConnection::Connection() const
{
	return fConnection;
}


BMediaClient*
BMediaConnection::Client() const
{
	return fOwner;
}


const char*
BMediaConnection::Name() const
{
	return fConnection.name;
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


bool
BMediaConnection::IsConnected() const
{
	CALLED();

	return fConnected;
}


status_t
BMediaConnection::Disconnect()
{
	CALLED();

	return fOwner->_DisconnectConnection(this);
}


status_t
BMediaConnection::Release()
{
	CALLED();

	status_t ret = fOwner->_ReleaseConnection(this);
	if (ret != B_OK)
		return ret;

	delete this;
	return ret;
}


void
BMediaConnection::Connected(const media_format& format)
{
	// Update the status of our connection format.
	fConnection.format = format;

	fConnected = true;
}


void
BMediaConnection::Disconnected()
{
	CALLED();

	fConnected = false;
}


void
BMediaConnection::_ConnectionRegistered(BMediaClient* owner,
	media_connection_id id)
{
	fOwner = owner;
	fConnection.id = id;
	fConnection.client = fOwner->Client();

	if (fConnection.IsOutput()) {
		fConnection.source.port = fOwner->fNode->ControlPort();
		fConnection.source.id = fConnection.id;

		fConnection.destination = media_destination::null;
	} else {
		fConnection.destination.port = fOwner->fNode->ControlPort();
		fConnection.destination.id = fConnection.id;

		fConnection.source = media_source::null;
	}
}


const media_source&
BMediaConnection::_Source() const
{
	return fConnection.source;
}


const media_destination&
BMediaConnection::_Destination() const
{
	return fConnection.destination;
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


BMediaInput::BMediaInput(const char* name)
	:
	BMediaConnection(B_MEDIA_INPUT, name)
{
}


BMediaInput::~BMediaInput()
{
	CALLED();
}


void
BMediaInput::HandleBuffer(BBuffer* buffer)
{
	CALLED();
}


void
BMediaInput::Connected(const media_format& format)
{
	BMediaConnection::Connected(format);
}


void
BMediaInput::Disconnected()
{
	BMediaConnection::Disconnected();
}


void BMediaInput::_ReservedMediaInput0() {}
void BMediaInput::_ReservedMediaInput1() {}
void BMediaInput::_ReservedMediaInput2() {}
void BMediaInput::_ReservedMediaInput3() {}
void BMediaInput::_ReservedMediaInput4() {}
void BMediaInput::_ReservedMediaInput5() {}
void BMediaInput::_ReservedMediaInput6() {}
void BMediaInput::_ReservedMediaInput7() {}
void BMediaInput::_ReservedMediaInput8() {}
void BMediaInput::_ReservedMediaInput9() {}
void BMediaInput::_ReservedMediaInput10() {}


BMediaOutput::BMediaOutput(const char* name)
	:
	BMediaConnection(B_MEDIA_OUTPUT, name),
	fBufferGroup(NULL)
{
}


BMediaOutput::~BMediaOutput()
{
	CALLED();
}


status_t
BMediaOutput::SendBuffer(BBuffer* buffer)
{
	CALLED();

	if (!IsConnected())
		return B_ERROR;

	return fOwner->fNode->SendBuffer(buffer, this);
}


void
BMediaOutput::Connected(const media_format& format)
{
	BMediaConnection::Connected(format);
}


void
BMediaOutput::Disconnected()
{
	BMediaConnection::Disconnected();
}


bool
BMediaOutput::_IsEnabled() const
{
	CALLED();

	return fEnabled;
}


void
BMediaOutput::_SetEnabled(bool enabled)
{
	fEnabled = enabled;
}


void BMediaOutput::_ReservedMediaOutput0() {}
void BMediaOutput::_ReservedMediaOutput1() {}
void BMediaOutput::_ReservedMediaOutput2() {}
void BMediaOutput::_ReservedMediaOutput3() {}
void BMediaOutput::_ReservedMediaOutput4() {}
void BMediaOutput::_ReservedMediaOutput5() {}
void BMediaOutput::_ReservedMediaOutput6() {}
void BMediaOutput::_ReservedMediaOutput7() {}
void BMediaOutput::_ReservedMediaOutput8() {}
void BMediaOutput::_ReservedMediaOutput9() {}
void BMediaOutput::_ReservedMediaOutput10() {}
