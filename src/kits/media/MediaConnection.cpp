/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <MediaConnection.h>

#include "debug.h"


BMediaConnection::BMediaConnection(media_connection_kinds kinds)
	:
	fOwner(NULL),
	fBind(NULL),
	fBufferGroup(NULL)
{
	CALLED();

	fConnection.kinds = kinds;
	fConnection.id = 0;
	//fConnection.client = media_client::null;
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


bool
BMediaConnection::IsOutput() const
{
	CALLED();

	return fConnection.IsOutput();
}


bool
BMediaConnection::IsInput() const
{
	CALLED();

	return fConnection.IsInput();
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

	fConnection.format = format;
}


const media_format&
BMediaConnection::AcceptedFormat() const
{
	CALLED();

	return fConnection.format;
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

	delete fBufferGroup;
	fBufferGroup = NULL;

	return fOwner->ConnectionDisconnected(this);
}


status_t
BMediaConnection::Release()
{
	CALLED();

	return fOwner->ConnectionReleased(this);
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
BMediaConnection::Connected(const media_format& format)
{
	fConnected = true;
}


void
BMediaConnection::Disconnected()
{
	fConnected = false;
}


void
BMediaConnection::ConnectionRegistered(BMediaClient* owner,
	media_connection_id id)
{
	fOwner = owner;
	fConnection.id = id;
	fConnection.client = fOwner->Client();

	if (IsOutput()) {
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
BMediaConnection::Source() const
{
	return fConnection.Source();
}


const media_destination&
BMediaConnection::Destination() const
{
	return fConnection.Destination();
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


BMediaInput::BMediaInput()
	:
	BMediaConnection(B_MEDIA_INPUT)
{
}


media_input
BMediaInput::MediaInput() const
{
	return Connection().MediaInput();
}


status_t
BMediaInput::FormatChanged(const media_format& format)
{
	if (!format_is_compatible(format, AcceptedFormat()))
		return B_MEDIA_BAD_FORMAT;

	SetAcceptedFormat(format);

	return B_OK;
}


void
BMediaInput::BufferReceived(BBuffer* buffer)
{
	CALLED();

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


BMediaOutput::BMediaOutput()
	:
	BMediaConnection(B_MEDIA_OUTPUT)
{
}


bool
BMediaOutput::IsOutputEnabled() const
{
	CALLED();

	return fOutputEnabled;
}


status_t
BMediaOutput::PrepareToConnect(media_format* format)
{
	SetAcceptedFormat(*format);

	return B_OK;
}


status_t
BMediaOutput::FormatProposal(media_format* format)
{
	*format = AcceptedFormat();
	return B_OK;
}


status_t
BMediaOutput::FormatChangeRequested(media_format* format)
{
	return B_ERROR;
}


status_t
BMediaOutput::SendBuffer(BBuffer* buffer)
{
	CALLED();

	return fOwner->fNode->SendBuffer(buffer, this);
}


media_output
BMediaOutput::MediaOutput() const
{
	return Connection().MediaOutput();
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
