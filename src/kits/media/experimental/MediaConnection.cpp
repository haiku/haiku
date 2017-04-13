/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <MediaConnection.h>

#include "MediaClientNode.h"

#include "debug.h"


BMediaConnection::BMediaConnection(media_connection_kinds kinds)
	:
	fOwner(NULL),
	fBind(NULL),
	fBufferGroup(NULL),
	fMinLatency(0),
	fMaxLatency(0)
{
	CALLED();

	fConnection.kinds = kinds;
	fConnection.id = -1;
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


BMediaClient*
BMediaConnection::Client() const
{
	return fOwner;
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

	status_t ret = fOwner->_DisconnectConnection(this);
	if (ret != B_OK)
		return ret;

	delete fBufferGroup;
	fBufferGroup = NULL;

	return ret;
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


size_t
BMediaConnection::BufferSize() const
{
	CALLED();

	switch (fConnection.format.type) {
		case B_MEDIA_RAW_AUDIO:
			return fConnection.format.u.raw_audio.buffer_size;

		case B_MEDIA_RAW_VIDEO:
			return fConnection.format.u.raw_video.display.bytes_per_row *
				fConnection.format.u.raw_video.display.line_count;

		default:
			return 0;
	}
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
BMediaConnection::GetLatencyRange(bigtime_t* min, bigtime_t* max) const
{
	CALLED();

	*min = fMinLatency;
	*max = fMaxLatency;
}


void
BMediaConnection::SetLatencyRange(bigtime_t min, bigtime_t max)
{
	CALLED();

	fMinLatency = min;
	fMaxLatency = max;
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
	return fConnection._Source();
}


const media_destination&
BMediaConnection::_Destination() const
{
	return fConnection._Destination();
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


BMediaInput::~BMediaInput()
{
	CALLED();
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
BMediaInput::HandleBuffer(BBuffer* buffer)
{
	CALLED();

}


media_input
BMediaInput::_MediaInput() const
{
	return Connection()._MediaInput();
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


BMediaOutput::~BMediaOutput()
{
	CALLED();
}


bool
BMediaOutput::IsEnabled() const
{
	CALLED();

	return fEnabled;
}


void
BMediaOutput::SetEnabled(bool enabled)
{
	fEnabled = enabled;
}


status_t
BMediaOutput::PrepareToConnect(media_format* format)
{
	if (!format_is_compatible(AcceptedFormat(), *format))
		return B_ERROR;

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

	if (!IsConnected())
		return B_ERROR;

	return fOwner->fNode->SendBuffer(buffer, this);
}


media_output
BMediaOutput::_MediaOutput() const
{
	return Connection()._MediaOutput();
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
