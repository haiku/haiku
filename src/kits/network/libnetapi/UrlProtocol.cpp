/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <UrlProtocol.h>
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


BUrlProtocol::BUrlProtocol(const BUrl& url, BUrlProtocolListener* listener,
	BUrlContext* context, BUrlResult* result, const char* threadName,
	const char* protocolName)
	:
	fUrl(url),
	fResult(result),
	fContext(context),
	fListener(listener),
	fQuit(false),
	fRunning(false),
	fThreadId(0),
	fThreadName(threadName),
	fProtocol(protocolName)
{
}


BUrlProtocol::~BUrlProtocol()
{
}


// #pragma mark URL protocol thread management


thread_id
BUrlProtocol::Run()
{
	// Thread already running
	if (fRunning) {
		PRINT(("BUrlProtocol::Run() : Oops, already running ! "
			"[urlProtocol=%p]!\n", this));
		return fThreadId;
	}

	fThreadId = spawn_thread(BUrlProtocol::_ThreadEntry, fThreadName,
		B_NORMAL_PRIORITY, this);

	if (fThreadId < B_OK)
		return fThreadId;

	status_t launchErr = resume_thread(fThreadId);
	if (launchErr < B_OK) {
		PRINT(("BUrlProtocol::Run() : Failed to resume thread %ld\n",
			fThreadId));
		return launchErr;
	}

	fRunning = true;
	return fThreadId;
}


status_t
BUrlProtocol::Pause()
{
	// TODO
	return B_ERROR;
}


status_t
BUrlProtocol::Resume()
{
	// TODO
	return B_ERROR;
}


status_t
BUrlProtocol::Stop()
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
BUrlProtocol::SetUrl(const BUrl& url)
{
	// We should avoid to change URL while the thread is running ...
	if (IsRunning())
		return B_ERROR;

	fUrl = url;
	return B_OK;
}


status_t
BUrlProtocol::SetResult(BUrlResult* result)
{
	if (IsRunning())
		return B_ERROR;

	fResult = result;
	return B_OK;
}


status_t
BUrlProtocol::SetContext(BUrlContext* context)
{
	if (IsRunning())
		return B_ERROR;

	fContext = context;
	return B_OK;
}


status_t
BUrlProtocol::SetListener(BUrlProtocolListener* listener)
{
	if (IsRunning())
		return B_ERROR;

	fListener = listener;
	return B_OK;
}


// #pragma mark URL protocol parameters access


const BUrl&
BUrlProtocol::Url() const
{
	return fUrl;
}


BUrlResult*
BUrlProtocol::Result() const
{
	return fResult;
}


BUrlContext*
BUrlProtocol::Context() const
{
	return fContext;
}


BUrlProtocolListener*
BUrlProtocol::Listener() const
{
	return fListener;
}


const BString&
BUrlProtocol::Protocol() const
{
	return fProtocol;
}


// #pragma mark URL protocol informations


bool
BUrlProtocol::IsRunning() const
{
	return fRunning;
}


status_t
BUrlProtocol::Status() const
{
	return fThreadStatus;
}


const char*
BUrlProtocol::StatusString(status_t threadStatus) const
{
	if (threadStatus < B_PROT_THREAD_STATUS__BASE)
		threadStatus = B_PROT_THREAD_STATUS__END;
	else if (threadStatus >= B_PROT_PROTOCOL_ERROR)
		threadStatus = B_PROT_PROTOCOL_ERROR;

	return kProtocolThreadStrStatus[threadStatus];
}


// #pragma mark Thread management


/*static*/ int32
BUrlProtocol::_ThreadEntry(void* arg)
{
	BUrlProtocol* urlProtocol = reinterpret_cast<BUrlProtocol*>(arg);
	urlProtocol->fThreadStatus = B_PROT_RUNNING;

	status_t protocolLoopExitStatus = urlProtocol->_ProtocolLoop();

	urlProtocol->fRunning = false;
	urlProtocol->fThreadStatus = protocolLoopExitStatus;

	if (urlProtocol->fListener != NULL)
		urlProtocol->fListener->RequestCompleted(urlProtocol,
			protocolLoopExitStatus == B_PROT_SUCCESS);

	return B_OK;
}


status_t
BUrlProtocol::_ProtocolLoop()
{
	// Dummy _ProtocolLoop
	while (!fQuit)
		snooze(1000);

	return B_PROT_SUCCESS;
}


void
BUrlProtocol::_EmitDebug(BUrlProtocolDebugMessage type,
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


BMallocIO&
BUrlProtocol::_ResultRawData()
{
	return fResult->fRawData;
}


BHttpHeaders&
BUrlProtocol::_ResultHeaders()
{
	return fResult->fHeaders;
}


void
BUrlProtocol::_SetResultStatusCode(int32 statusCode)
{
	fResult->fStatusCode = statusCode;
}


BString&
BUrlProtocol::_ResultStatusText()
{
	return fResult->fStatusString;
}
