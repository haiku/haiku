/*
 * Copyright 2016, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "RTSPMediaIO.h"


#define LIVE555_VERBOSITY 1


RTSPMediaIO::RTSPMediaIO(BUrl* ourUrl)
	:
	BAdapterIO(
		B_MEDIA_STREAMING | B_MEDIA_MUTABLE_SIZE | B_MEDIA_SEEK_BACKWARD,
		B_INFINITE_TIMEOUT),
	fUrl(ourUrl),
	fClient(NULL),
	fInputAdapter(NULL),
	fScheduler(NULL),
	fLoopWatchVariable(0),
	fLoopThread(-1),
	fInitErr(B_OK)
{
	fInputAdapter = BuildInputAdapter();
	fScheduler = BasicTaskScheduler::createNew();
	fEnv = BasicUsageEnvironment::createNew(*fScheduler);

	fClient = new HaikuRTSPClient(*fEnv, fUrl->UrlString(),
		0, this);
	if (fClient == NULL) {
		fInitErr = B_ERROR;
		return;
	}

	fClient->sendDescribeCommand(continueAfterDESCRIBE);

	fLoopThread = spawn_thread(_LoopThread, "two minutes hate thread",
		B_NORMAL_PRIORITY, this);

	if (fLoopThread <= 0 || resume_thread(fLoopThread) != B_OK) {
		fInitErr = B_ERROR;
		return;
	}

	fInitErr = fClient->WaitForInit(5000000);
}


RTSPMediaIO::~RTSPMediaIO()
{
	fClient->Close();

	ShutdownLoop();

	if (fLoopThread != -1)
		exit_thread(fLoopThread);
}


status_t
RTSPMediaIO::InitCheck() const
{
	return fInitErr;
}


void
RTSPMediaIO::ShutdownLoop()
{
	fLoopWatchVariable = 1;
}


ssize_t
RTSPMediaIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	return B_NOT_SUPPORTED;
}


int32
RTSPMediaIO::_LoopThread(void* data)
{
	static_cast<RTSPMediaIO *>(data)->LoopThread();
	return 0;
}


void
RTSPMediaIO::LoopThread()
{
	fEnv->taskScheduler().doEventLoop(&fLoopWatchVariable);
	fLoopThread = -1;
}


HaikuRTSPClient::HaikuRTSPClient(UsageEnvironment& env, char const* rtspURL,
		portNumBits tunnelOverHTTPPortNum, RTSPMediaIO* interface)
	:
	RTSPClient(env, rtspURL, LIVE555_VERBOSITY, "Haiku RTSP Streamer",
		tunnelOverHTTPPortNum, -1),
	iter(NULL),
	session(NULL),
	subsession(NULL),
	streamTimerTask(NULL),
	duration(0.0f),
	fInterface(interface),
	fInitPort(-1)
{
	fInitPort = create_port(1, "RTSP Client wait port");
}


HaikuRTSPClient::~HaikuRTSPClient()
{
}


void
HaikuRTSPClient::Close()
{
	delete iter;
	if (session != NULL) {
		UsageEnvironment& env = session->envir();
		env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
		Medium::close(session);
	}
}


status_t
HaikuRTSPClient::WaitForInit(bigtime_t timeout)
{
	status_t status = B_ERROR;
	read_port_etc(fInitPort, NULL, &status,
		sizeof(status), B_RELATIVE_TIMEOUT, timeout);

	close_port(fInitPort);
	delete_port(fInitPort);
	return status;
}


void
HaikuRTSPClient::NotifyError()
{
	fInterface->ShutdownLoop();
	status_t status = B_ERROR;
	write_port(fInitPort, NULL, &status, sizeof(status));
}


void
HaikuRTSPClient::NotifySucces()
{
	status_t status = B_OK;
	write_port(fInitPort, NULL, &status, sizeof(status));
}


BInputAdapter*
HaikuRTSPClient::GetInputAdapter() const
{
	return fInterface->BuildInputAdapter();
}
