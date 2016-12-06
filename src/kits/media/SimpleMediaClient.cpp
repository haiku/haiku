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
BSimpleMediaClient::SetNotificationHook(notify_hook notifyHook, void* cookie)
{
	CALLED();

	fNotifyHook = notifyHook;
	fNotifyCookie = cookie;
}


void
BSimpleMediaClient::HandleStart(bigtime_t performanceTime)
{
	if (fNotifyHook != NULL) {
		(*fNotifyHook)(fNotifyCookie,
			BSimpleMediaClient::B_WILL_START,
			performanceTime);
	}
}


void
BSimpleMediaClient::HandleStop(bigtime_t performanceTime)
{
	if (fNotifyHook != NULL) {
		(*fNotifyHook)(fNotifyCookie,
			BSimpleMediaClient::B_WILL_STOP,
			performanceTime);
	}
}


void
BSimpleMediaClient::HandleSeek(bigtime_t mediaTime, bigtime_t performanceTime)
{
	if (fNotifyHook != NULL) {
		(*fNotifyHook)(fNotifyCookie,
			BSimpleMediaClient::B_WILL_SEEK,
			performanceTime, mediaTime);
	}
}


void
BSimpleMediaClient::HandleTimeWarp(bigtime_t realTime, bigtime_t performanceTime)
{
	if (fNotifyHook != NULL) {
		(*fNotifyHook)(fNotifyCookie,
			BSimpleMediaClient::B_WILL_TIMEWARP,
			realTime, performanceTime);
	}
}


status_t
BSimpleMediaClient::HandleFormatSuggestion(media_type type, int32 quality,
	media_format* format)
{
	if (fNotifyHook != NULL) {
		status_t result = B_ERROR;
		(*fNotifyHook)(fNotifyCookie,
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


BSimpleMediaInput::BSimpleMediaInput()
	:
	BMediaInput()
{
}


void
BSimpleMediaInput::Connected(const media_format& format)
{
	if (fNotifyHook != NULL)
		(*fNotifyHook)(B_CONNECTED, this);

	BMediaInput::Connected(format);
}


void
BSimpleMediaInput::Disconnected()
{
	if (fNotifyHook != NULL)
		(*fNotifyHook)(B_DISCONNECTED, this);

	BMediaConnection::Disconnected();
}


void
BSimpleMediaInput::SetHooks(process_hook processHook,
	notify_hook notifyHook, void* cookie)
{
	CALLED();

	fProcessHook = processHook;
	fNotifyHook = notifyHook;
	fBufferCookie = cookie;
}


void*
BSimpleMediaInput::Cookie() const
{
	CALLED();

	return fBufferCookie;
}


void
BSimpleMediaInput::BufferReceived(BBuffer* buffer)
{
	CALLED();

	if (fProcessHook != NULL)
		(*fProcessHook)((BMediaConnection*)this, buffer);
}


BSimpleMediaOutput::BSimpleMediaOutput()
	:
	BMediaOutput()
{
}


status_t
BSimpleMediaOutput::FormatProposal(media_format* format)
{
	if (fNotifyHook != NULL) {
		return (*fNotifyHook)(BSimpleMediaOutput::B_FORMAT_PROPOSAL,
			this, format);
	}

	return BMediaOutput::FormatProposal(format);
}


void
BSimpleMediaOutput::Connected(const media_format& format)
{
	if (fNotifyHook != NULL)
		(*fNotifyHook)(B_CONNECTED, this);

	BMediaConnection::Connected(format);
}


void
BSimpleMediaOutput::Disconnected()
{
	if (fNotifyHook != NULL)
		(*fNotifyHook)(B_DISCONNECTED, this);

	BMediaConnection::Disconnected();
}


void
BSimpleMediaOutput::SetHooks(process_hook processHook,
	notify_hook notifyHook, void* cookie)
{
	CALLED();

	fProcessHook = processHook;
	fNotifyHook = notifyHook;
	fBufferCookie = cookie;
}


void*
BSimpleMediaOutput::Cookie() const
{
	CALLED();

	return fBufferCookie;
}
