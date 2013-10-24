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


static const char* kProtocolThreadStrStatus[B_PROT_THREAD_STATUS__END+1]
	=  {
		"Request successfully completed",
		"Request running",
		"Socket error",
		"Connection failed",
		"Hostname resolution failed",
		"Network write failed",
		"Network read failed",
		"Out of memory",
		"Protocol-specific error",
		"Unknown error"
		};


BUrlRequest::BUrlRequest(const BUrl& url, BUrlProtocolListener* listener,
	BUrlContext* context, const char* threadName, const char* protocolName)
	:
	fUrl(url),
	fContext(context),
	fListener(listener),
	fQuit(false),
	fRunning(false),
	fThreadStatus(B_PROT_PAUSED),
	fThreadId(0),
	fThreadName(threadName),
	fProtocol(protocolName)
{
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

	status_t launchErr = resume_thread(fThreadId);
	if (launchErr < B_OK) {
		PRINT(("BUrlRequest::Run() : Failed to resume thread %ld\n",
			fThreadId));
		return launchErr;
	}

	fRunning = true;
	return fThreadId;
}


status_t
BUrlRequest::Pause()
{
	// TODO
	return B_ERROR;
}


status_t
BUrlRequest::Resume()
{
	// TODO
	return B_ERROR;
}


status_t
BUrlRequest::Stop()
{
	if (!fRunning)
		return B_ERROR;

	status_t threadStatus = B_OK;
	fQuit = true;

	wait_for_thread(fThreadId, &threadStatus);
	return threadStatus;
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


const char*
BUrlRequest::StatusString(status_t threadStatus) const
{
	if (threadStatus < B_PROT_THREAD_STATUS__BASE)
		threadStatus = B_PROT_THREAD_STATUS__END;
	else if (threadStatus >= B_PROT_PROTOCOL_ERROR)
		threadStatus = B_PROT_PROTOCOL_ERROR;

	return kProtocolThreadStrStatus[threadStatus];
}


// #pragma mark Thread management


/*static*/ int32
BUrlRequest::_ThreadEntry(void* arg)
{
	BUrlRequest* urlProtocol = reinterpret_cast<BUrlRequest*>(arg);
	urlProtocol->fThreadStatus = B_PROT_RUNNING;

	status_t protocolLoopExitStatus = urlProtocol->_ProtocolLoop();

	urlProtocol->fRunning = false;
	urlProtocol->fThreadStatus = protocolLoopExitStatus;

	if (urlProtocol->fListener != NULL) {
		urlProtocol->fListener->RequestCompleted(urlProtocol,
			protocolLoopExitStatus == B_PROT_SUCCESS);
		printf("Notified to %p\n", urlProtocol->fListener);
	}

	return B_OK;
}


status_t
BUrlRequest::_ProtocolLoop()
{
	// Dummy _ProtocolLoop
	while (!fQuit)
		snooze(1000);

	return B_PROT_SUCCESS;
}


void
BUrlRequest::_EmitDebug(BUrlProtocolDebugMessage type,
	const char* format, ...)
{
	if (fListener == NULL)
		return;

	va_list arguments;
	va_start(arguments, format);

	char debugMsg[256];
	vsnprintf(debugMsg, 256, format, arguments);
	fListener->DebugMessage(this, type, debugMsg);
	va_end(arguments);
}
