/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <SimpleMediaClient.h>

#include <debug.h>


BSimpleMediaClient::BSimpleMediaClient(const char* name,
	media_type type, media_client_kinds kinds)
	:
	BMediaClient(name, type, kinds),
	fNotifyHook(NULL),
	fNotifyCookie(NULL)
{
	CALLED();
}


BSimpleMediaClient::~BSimpleMediaClient()
{
	CALLED();
}


BSimpleMediaInput*
BSimpleMediaClient::BeginInput()
{
	CALLED();

	BSimpleMediaInput* input = new BSimpleMediaInput();
	RegisterInput(input);
	return input;
}


BSimpleMediaOutput*
BSimpleMediaClient::BeginOutput()
{
	CALLED();

	BSimpleMediaOutput* output = new BSimpleMediaOutput();
	RegisterOutput(output);
	return output;
}


void
BSimpleMediaClient::SetHook(notify_hook notifyHook, void* cookie)
{
	CALLED();

	fNotifyHook = notifyHook;
	fNotifyCookie = cookie;
}


void
BSimpleMediaClient::HandleStart(bigtime_t performanceTime)
{
	if (fNotifyHook != NULL) {
		(*fNotifyHook)(BSimpleMediaClient::fNotifyCookie,
			BSimpleMediaClient::B_WILL_START,
			performanceTime);
	}
}


void
BSimpleMediaClient::HandleStop(bigtime_t performanceTime)
{
	if (fNotifyHook != NULL) {
		(*fNotifyHook)(BSimpleMediaClient::fNotifyCookie,
			BSimpleMediaClient::B_WILL_STOP,
			performanceTime);
	}
}


void
BSimpleMediaClient::HandleSeek(bigtime_t mediaTime, bigtime_t performanceTime)
{
	if (fNotifyHook != NULL) {
		(*fNotifyHook)(BSimpleMediaClient::fNotifyCookie,
			BSimpleMediaClient::B_WILL_SEEK,
			performanceTime, mediaTime);
	}
}


status_t
BSimpleMediaClient::FormatSuggestion(media_type type, int32 quality,
	media_format* format)
{
	if (fNotifyHook != NULL) {
		status_t result = B_ERROR;
		(*fNotifyHook)(BSimpleMediaClient::fNotifyCookie,
			BSimpleMediaClient::B_FORMAT_SUGGESTION,
			type, quality, format, &result);
		return result;
	}
	return B_ERROR;
}


void BSimpleMediaClient::_ReservedSimpleMediaClient0() {}
void BSimpleMediaClient::_ReservedSimpleMediaClient1() {}
void BSimpleMediaClient::_ReservedSimpleMediaClient2() {}
void BSimpleMediaClient::_ReservedSimpleMediaClient3() {}
void BSimpleMediaClient::_ReservedSimpleMediaClient4() {}
void BSimpleMediaClient::_ReservedSimpleMediaClient5() {}


BSimpleMediaConnection::BSimpleMediaConnection(media_connection_kinds kinds)
	:
	BMediaConnection(kinds),
	fProcessHook(NULL),
	fNotifyHook(NULL),
	fBufferCookie(NULL)
{
}


BSimpleMediaConnection::~BSimpleMediaConnection()
{
	CALLED();
}


void
BSimpleMediaConnection::SetHooks(process_hook processHook,
	notify_hook notifyHook, void* cookie)
{
	CALLED();

	fProcessHook = processHook;
	fNotifyHook = notifyHook;
	fBufferCookie = cookie;
}


void*
BSimpleMediaConnection::Cookie() const
{
	CALLED();

	return fBufferCookie;
}


BSimpleMediaInput::BSimpleMediaInput()
	:
	BMediaConnection(B_MEDIA_INPUT),
	BSimpleMediaConnection(B_MEDIA_INPUT),
	BMediaInput()
{
}


BSimpleMediaInput::~BSimpleMediaInput()
{
	CALLED();
}


void
BSimpleMediaInput::Connected(const media_format& format)
{
	if (fNotifyHook != NULL)
		(*fNotifyHook)(this, BSimpleMediaConnection::B_INPUT_CONNECTED);

	BMediaInput::Connected(format);
}


void
BSimpleMediaInput::Disconnected()
{
	if (fNotifyHook != NULL)
		(*fNotifyHook)(this, BSimpleMediaConnection::B_INPUT_DISCONNECTED);

	BMediaConnection::Disconnected();
}


void
BSimpleMediaInput::HandleBuffer(BBuffer* buffer)
{
	CALLED();

	if (fProcessHook != NULL)
		(*fProcessHook)(this, buffer);
}


BSimpleMediaOutput::BSimpleMediaOutput()
	:
	BMediaConnection(B_MEDIA_OUTPUT),
	BSimpleMediaConnection(B_MEDIA_OUTPUT),
	BMediaOutput()
{
}


BSimpleMediaOutput::~BSimpleMediaOutput()
{
	CALLED();
}


status_t
BSimpleMediaOutput::FormatProposal(media_format* format)
{
	if (fNotifyHook != NULL) {
		return (*fNotifyHook)(this,
			BSimpleMediaConnection::B_FORMAT_PROPOSAL, format);
	}

	return BMediaOutput::FormatProposal(format);
}


void
BSimpleMediaOutput::Connected(const media_format& format)
{
	if (fNotifyHook != NULL)
		(*fNotifyHook)(this, BSimpleMediaConnection::B_OUTPUT_CONNECTED);

	BSimpleMediaConnection::Connected(format);
}


void
BSimpleMediaOutput::Disconnected()
{
	if (fNotifyHook != NULL)
		(*fNotifyHook)(this, BSimpleMediaConnection::B_OUTPUT_DISCONNECTED);

	BSimpleMediaConnection::Disconnected();
}
