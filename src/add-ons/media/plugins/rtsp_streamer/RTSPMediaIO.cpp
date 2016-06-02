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
	fInitErr(B_OK),
	fScheduler(NULL),
	fEnv(NULL),
	loopWatchVariable(0)
{
	fScheduler = BasicTaskScheduler::createNew();
	fEnv = BasicUsageEnvironment::createNew(*fScheduler);

	HaikuRTSPClient* client = new HaikuRTSPClient(*fEnv, fUrl->UrlString(),
		0, BuildInputAdapter());
	if (client == NULL) {
		fInitErr = B_ERROR;
		return;
	}

	client->sendDescribeCommand(continueAfterDESCRIBE);

	fEnv->taskScheduler().doEventLoop(&loopWatchVariable);

	fInitErr = client->WaitForInit(5000000);
}


RTSPMediaIO::~RTSPMediaIO()
{
}


status_t
RTSPMediaIO::InitCheck() const
{
	return fInitErr;
}


ssize_t
RTSPMediaIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	return B_NOT_SUPPORTED;
}


HaikuRTSPClient::HaikuRTSPClient(UsageEnvironment& env, char const* rtspURL,
		portNumBits tunnelOverHTTPPortNum, BInputAdapter* inputAdapter)
	:
	RTSPClient(env, rtspURL, LIVE555_VERBOSITY, "Haiku RTSP Streamer",
		tunnelOverHTTPPortNum, -1),
	iter(NULL),
	session(NULL),
	subsession(NULL),
	streamTimerTask(NULL),
	duration(0.0f),
	fInputAdapter(inputAdapter)
{
	fInitPort = create_port(1, "RTSP Client wait port");
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
	status_t status = B_ERROR;
	write_port(fInitPort, NULL, &status, sizeof(status));
}


void
HaikuRTSPClient::NotifySucces()
{
	status_t status = B_OK;
	write_port(fInitPort, NULL, &status, sizeof(status));
}


HaikuRTSPClient::~HaikuRTSPClient()
{
	delete iter;
	if (session != NULL) {
		UsageEnvironment& env = session->envir();
		env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
		Medium::close(session);
	}
}


BInputAdapter*
HaikuRTSPClient::GetInputAdapter() const
{
	return fInputAdapter;
}
