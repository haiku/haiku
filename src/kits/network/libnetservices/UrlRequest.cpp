/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <UrlRequest.h>
#include <Debug.h>
#include <stdio.h>

using namespace BPrivate::Network;


static BReference<BUrlContext> gDefaultContext = new(std::nothrow) BUrlContext();


BUrlRequest::BUrlRequest(const BUrl& url, BDataIO* output,
	BUrlProtocolListener* listener, BUrlContext* context,
	const char* threadName, const char* protocolName)
	:
	fUrl(url),
	fContext(context),
	fListener(listener),
	fOutput(output),
	fQuit(false),
	fRunning(false),
	fThreadStatus(B_NO_INIT),
	fThreadId(0),
	fThreadName(threadName),
	fProtocol(protocolName)
{
	if (fContext == NULL)
		fContext = gDefaultContext;
}


BUrlRequest::~BUrlRequest()
{
	Stop();
}


// #pragma mark URL protocol thread management


thread_id
BUrlRequest::Run()
{
	// Thread already running
	if (fRunning) {
		PRINT(("BUrlRequest::Run() : Oops, already running ! "
			"[urlProtocol=%p]!\n", this));
		return fThreadId;
	}

	fThreadId = spawn_thread(BUrlRequest::_ThreadEntry, fThreadName,
		B_NORMAL_PRIORITY, this);

	if (fThreadId < B_OK)
		return fThreadId;

	fRunning = true;

	status_t launchErr = resume_thread(fThreadId);
	if (launchErr < B_OK) {
		PRINT(("BUrlRequest::Run() : Failed to resume thread %" B_PRId32 "\n",
			fThreadId));
		return launchErr;
	}

	return fThreadId;
}


status_t
BUrlRequest::Stop()
{
	if (!fRunning)
		return B_ERROR;

	fQuit = true;
	return B_OK;
}


// #pragma mark URL protocol parameters modification


status_t
BUrlRequest::SetUrl(const BUrl& url)
{
	// We should avoid to change URL while the thread is running ...
	if (IsRunning())
		return B_ERROR;

	fUrl = url;
	return B_OK;
}


status_t
BUrlRequest::SetContext(BUrlContext* context)
{
	if (IsRunning())
		return B_ERROR;

	if (context == NULL)
		fContext = gDefaultContext;
	else
		fContext = context;

	return B_OK;
}


status_t
BUrlRequest::SetListener(BUrlProtocolListener* listener)
{
	if (IsRunning())
		return B_ERROR;

	fListener = listener;
	return B_OK;
}


status_t
BUrlRequest::SetOutput(BDataIO* output)
{
	if (IsRunning())
		return B_ERROR;

	fOutput = output;
	return B_OK;
}


// #pragma mark URL protocol parameters access


const BUrl&
BUrlRequest::Url() const
{
	return fUrl;
}


BUrlContext*
BUrlRequest::Context() const
{
	return fContext;
}


BUrlProtocolListener*
BUrlRequest::Listener() const
{
	return fListener;
}


const BString&
BUrlRequest::Protocol() const
{
	return fProtocol;
}


#ifndef LIBNETAPI_DEPRECATED
BDataIO*
BUrlRequest::Output() const
{
	return fOutput;
}
#endif

// #pragma mark URL protocol informations


bool
BUrlRequest::IsRunning() const
{
	return fRunning;
}


status_t
BUrlRequest::Status() const
{
	return fThreadStatus;
}


// #pragma mark Thread management


/*static*/ int32
BUrlRequest::_ThreadEntry(void* arg)
{
	BUrlRequest* request = reinterpret_cast<BUrlRequest*>(arg);
	request->fThreadStatus = B_BUSY;
	request->_ProtocolSetup();

	status_t protocolLoopExitStatus = request->_ProtocolLoop();

	request->fRunning = false;
	request->fThreadStatus = protocolLoopExitStatus;

	if (request->fListener != NULL) {
		request->fListener->RequestCompleted(request,
			protocolLoopExitStatus == B_OK);
	}

	return B_OK;
}


void
BUrlRequest::_EmitDebug(BUrlProtocolDebugMessage type,
	const char* format, ...)
{
	if (fListener == NULL)
		return;

	va_list arguments;
	va_start(arguments, format);

	char debugMsg[1024];
	vsnprintf(debugMsg, sizeof(debugMsg), format, arguments);
	fListener->DebugMessage(this, type, debugMsg);
	va_end(arguments);
}
