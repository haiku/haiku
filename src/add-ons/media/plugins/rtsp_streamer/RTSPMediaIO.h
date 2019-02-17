/*
 * Copyright 2016, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _RTSP_MEDIA_IO_H
#define _RTSP_MEDIA_IO_H


#include <AdapterIO.h>
#include <Url.h>

#include "rtsp.h"

using BCodecKit::BAdapterIO;
using BCodecKit::BInputAdapter;

class HaikuRTSPClient;


class RTSPMediaIO : public BAdapterIO
{
public:
										RTSPMediaIO(BUrl ourUrl);
	virtual								~RTSPMediaIO();

	virtual	ssize_t						WriteAt(off_t position,
											const void* buffer,
											size_t size);

	virtual	status_t					SetSize(off_t size);

	virtual status_t					Open();

			void						LoopThread();
			void						ShutdownLoop();
private:
			static int32				_LoopThread(void* data);

			BUrl						fUrl;

			HaikuRTSPClient*			fClient;
			UsageEnvironment*			fEnv;
			TaskScheduler*				fScheduler;

			char						fLoopWatchVariable;
			thread_id					fLoopThread;
};


class HaikuRTSPClient : public RTSPClient
{
public:
										HaikuRTSPClient(UsageEnvironment& env,
											char const* rtspURL,
											portNumBits tunnelOverHTTPPortNum,
											RTSPMediaIO* fInputAdapter);

			void						Close();

			BInputAdapter*				GetInputAdapter() const;

			status_t					WaitForInit(bigtime_t timeout);

			void						NotifyError();
			void						NotifySucces();

protected:

	virtual 							~HaikuRTSPClient();
public:
			MediaSubsessionIterator* 	iter;
			MediaSession*				session;
			MediaSubsession*			subsession;
			TaskToken					streamTimerTask;
			double						duration;

private:
			RTSPMediaIO*				fInterface;

			port_id						fInitPort;
};

#endif
