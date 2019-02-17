/*
 * Copyright 2016, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "RTSPMediaIO.h"


#define LIVE555_VERBOSITY 1


using namespace BCodecKit;


RTSPMediaIO::RTSPMediaIO(BUrl ourUrl)
	:
	BAdapterIO(
		B_MEDIA_STREAMING | B_MEDIA_MUTABLE_SIZE | B_MEDIA_SEEK_BACKWARD,
		B_INFINITE_TIMEOUT),
	fUrl(ourUrl),
	fClient(NULL),
	fScheduler(NULL),
	fLoopWatchVariable(0),
	fLoopThread(-1)
{
	fScheduler = BasicTaskScheduler::createNew();
	fEnv = BasicUsageEnvironment::createNew(*fScheduler);
}


RTSPMediaIO::~RTSPMediaIO()
{
	fClient->Close();

	ShutdownLoop();

	status_t status;
	if (fLoopThread != -1)
		wait_for_thread(fLoopThread, &status);
}


ssize_t
RTSPMediaIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
RTSPMediaIO::SetSize(off_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
RTSPMediaIO::Open()
{
	fClient = new HaikuRTSPClient(*fEnv, fUrl.UrlString(),
		0, this);
	if (fClient == NULL)
		return B_ERROR;

	fClient->sendDescribeCommand(continueAfterDESCRIBE);

	fLoopThread = spawn_thread(_LoopThread, "two minutes hate thread",
		B_NORMAL_PRIORITY, this);

	if (fLoopThread <= 0 || resume_thread(fLoopThread) != B_OK)
		return B_ERROR;

	return fClient->WaitForInit(5000000);
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


void
RTSPMediaIO::ShutdownLoop()
{
	fLoopWatchVariable = 1;
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
	if (read_port_etc(fInitPort, NULL, &status,
			sizeof(status), B_RELATIVE_TIMEOUT, timeout) < 0) {
		return B_ERROR;
	}

	close_port(fInitPort);
	delete_port(fInitPort);
	fInitPort = -1;
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
