/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)
This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.
You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2016, Live Networks, Inc.  All rights reserved
// Copyright (c) 2016, Dario Casalinuovo. All rights reserved.


#include "rtsp.h"

#include <AdapterIO.h>

#include "RTSPMediaIO.h"


#define REQUEST_STREAMING_OVER_TCP False
#define RECEIVE_BUFFER_SIZE 100000


UsageEnvironment& operator<<(UsageEnvironment& env,
	const RTSPClient& rtspClient)
{
	return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}


UsageEnvironment& operator<<(UsageEnvironment& env,
	const MediaSubsession& subsession)
{
	return env << subsession.mediumName() << "/" << subsession.codecName();
}


class AdapterSink : public MediaSink
{
public:
			static	 			AdapterSink* createNew(UsageEnvironment& env,
									MediaSubsession& subsession,
									BInputAdapter* inputAdapter,
									char const* streamId = NULL);

private:
								AdapterSink(UsageEnvironment& env,
									MediaSubsession& subsession,
									char const* streamId,
									BInputAdapter* inputAdapter);

	virtual 					~AdapterSink();

			static void			afterGettingFrame(void* clientData,
									unsigned frameSize,
									unsigned numTruncatedBytes,
									struct timeval presentationTime,
									unsigned durationInMicroseconds);

			void				afterGettingFrame(unsigned frameSize,
									unsigned numTruncatedBytes,
									struct timeval presentationTime,
									unsigned durationInMicroseconds);

private:
	// redefined virtual functions:
	virtual Boolean				continuePlaying();

private:
			BInputAdapter*		fInputAdapter;
			u_int8_t*			fReceiveBuffer;
			MediaSubsession&	fSubsession;
			char*				fStreamId;
};

// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient* rtspClient,
	int resultCode, char* resultString)
{
	UsageEnvironment& env = rtspClient->envir();
	HaikuRTSPClient* client = (HaikuRTSPClient*) rtspClient;
	do {
		if (resultCode != 0) {
			env << *rtspClient << "Failed to get a SDP description: "
				<< resultString << "\n";
			delete[] resultString;

			break;
		}

		char* const sdpDescription = resultString;
		env << *rtspClient << "Got a SDP description:\n"
			<< sdpDescription << "\n";

		// Create a media session object from this SDP description:
		client->session = MediaSession::createNew(env, sdpDescription);
		delete[] sdpDescription; // because we don't need it anymore
		if (client->session == NULL) {
			env << *rtspClient
				<< "Failed to create a MediaSession object "
					"from the SDP description: "
				<< env.getResultMsg() << "\n";

			break;
		} else if (!client->session->hasSubsessions()) {
			env << *rtspClient << "This session has no media subsessions"
				" (i.e., no \"m=\" lines)\n";

			break;
		}

		// Then, create and set up our data source objects for the session.
		// We do this by iterating over the session's 'subsessions',
		// calling "MediaSubsession::initiate()",
		// and then sending a RTSP "SETUP" command, on each one.
		// (Each 'subsession' will have its own data source.)
		client->iter = new MediaSubsessionIterator(*client->session);
		setupNextSubsession(rtspClient);
		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	shutdownStream(rtspClient);
}


void setupNextSubsession(RTSPClient* rtspClient)
{
	UsageEnvironment& env = rtspClient->envir();
	HaikuRTSPClient* client = (HaikuRTSPClient*) rtspClient;

	client->subsession = client->iter->next();
	if (client->subsession != NULL) {
		if (!client->subsession->initiate()) {

			env << *rtspClient << "Failed to initiate the \""
				<< *client->subsession << "\" subsession: "
				<< env.getResultMsg() << "\n";

			// give up on this subsession; go to the next one
			setupNextSubsession(rtspClient);
		}
		else {
			env << *rtspClient << "Initiated the \""
				<< *client->subsession << "\" subsession (";

			if (client->subsession->rtcpIsMuxed()) {
				env << "client port " << client->subsession->clientPortNum();
			} else {
				env << "client ports " << client->subsession->clientPortNum()
					<< "-" << client->subsession->clientPortNum() + 1;
			}
			env << ")\n";

			// Continue setting up this subsession,
			// by sending a RTSP "SETUP" command:
			rtspClient->sendSetupCommand(*client->subsession,
				continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
		}
		return;
	}

	// We've finished setting up all of the subsessions.
	// Now, send a RTSP "PLAY" command to start the streaming:
	if (client->session->absStartTime() != NULL) {
		// Special case: The stream is indexed by 'absolute' time,
		// so send an appropriate "PLAY" command:
		rtspClient->sendPlayCommand(*client->session, continueAfterPLAY,
			client->session->absStartTime(), client->session->absEndTime());
	} else {
		client->duration = client->session->playEndTime()
			- client->session->playStartTime();
		rtspClient->sendPlayCommand(*client->session, continueAfterPLAY);
	}
}


void continueAfterSETUP(RTSPClient* rtspClient,
	int resultCode, char* resultString)
{
	do {
		UsageEnvironment& env = rtspClient->envir();
		HaikuRTSPClient* client = (HaikuRTSPClient*) rtspClient;

		if (resultCode != 0) {
			env << *rtspClient << "Failed to set up the \""
				<< *client->subsession << "\" subsession: "
				<< resultString << "\n";
			break;
		}

		env << *rtspClient << "Set up the \""
			<< *client->subsession << "\" subsession (";
		if (client->subsession->rtcpIsMuxed()) {
			env << "client port " << client->subsession->clientPortNum();
		} else {
			env << "client ports " << client->subsession->clientPortNum()
				<< "-" << client->subsession->clientPortNum() + 1;
		}
		env << ")\n";

		// Having successfully setup the subsession, create a data sink for it,
		// and call "startPlaying()" on it.
		// (This will prepare the data sink to receive data; the actual
		// flow of data from the client won't start happening until later,
		// after we've sent a RTSP "PLAY" command.)

		client->subsession->sink = AdapterSink::createNew(env, *client->subsession,
			((HaikuRTSPClient*)rtspClient)->GetInputAdapter(), rtspClient->url());
		// perhaps use your own custom "MediaSink" subclass instead
		if (client->subsession->sink == NULL) {
			env << *rtspClient << "Failed to create a data sink for the \""
				<< *client->subsession << "\" subsession: "
				<< env.getResultMsg() << "\n";
			break;
		}

		env << *rtspClient << "Created a data sink for the \""
			<< *client->subsession << "\" subsession\n";
		// a hack to let subsession handler functions
		// get the "RTSPClient" from the subsession
		client->subsession->miscPtr = rtspClient;
		client->subsession->sink
				->startPlaying(*(client->subsession->readSource()),
					subsessionAfterPlaying, client->subsession);
		// Also set a handler to be called if a RTCP "BYE"
		// arrives for this subsession:
		if (client->subsession->rtcpInstance() != NULL) {
			client->subsession->rtcpInstance()->setByeHandler(
				subsessionByeHandler,
				client->subsession);
		}
	} while (0);
	delete[] resultString;

	// Set up the next subsession, if any:
	setupNextSubsession(rtspClient);
}


void continueAfterPLAY(RTSPClient* rtspClient,
	int resultCode, char* resultString)
{
	Boolean success = False;
	UsageEnvironment& env = rtspClient->envir();
	HaikuRTSPClient* client = (HaikuRTSPClient*) rtspClient;

	do {
		if (resultCode != 0) {
			env << *rtspClient << "Failed to start playing session: "
				<< resultString << "\n";
			break;
		}

		// Set a timer to be handled at the end of the stream's
		// expected duration (if the stream does not already signal its end
		// using a RTCP "BYE").  This is optional.  If, instead, you want
		// to keep the stream active - e.g., so you can later
		// 'seek' back within it and do another RTSP "PLAY"
		// - then you can omit this code.
		// (Alternatively, if you don't want to receive the entire stream,
		// you could set this timer for some shorter value.)
		if (client->duration > 0) {
			// number of seconds extra to delay,
			// after the stream's expected duration.  (This is optional.)
			unsigned const delaySlop = 2;
			client->duration += delaySlop;
			unsigned uSecsToDelay = (unsigned)(client->duration * 1000000);
			client->streamTimerTask
				= env.taskScheduler().scheduleDelayedTask(uSecsToDelay,
					(TaskFunc*)streamTimerHandler, rtspClient);
		}

		env << *rtspClient << "Started playing session";
		if (client->duration > 0) {
			env << " (for up to " << client->duration << " seconds)";
		}
		env << "...\n";

		success = True;
	} while (0);
	delete[] resultString;

	if (!success) {
		// An unrecoverable error occurred with this stream.
		shutdownStream(rtspClient);
	} else
		client->NotifySucces();
}

// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData)
{
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

	// Begin by closing this subsession's stream:
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL)
			return; // this subsession is still active
	}

	// All subsessions' streams have now been closed, so shutdown the client:
	shutdownStream(rtspClient);
}


void subsessionByeHandler(void* clientData)
{
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
	UsageEnvironment& env = rtspClient->envir();

	env << *rtspClient << "Received RTCP \"BYE\" on \""
		<< *subsession << "\" subsession\n";

	// Now act as if the subsession had closed:
	subsessionAfterPlaying(subsession);
}


void streamTimerHandler(void* clientData)
{
	HaikuRTSPClient* client = (HaikuRTSPClient*)clientData;

	client->streamTimerTask = NULL;

	// Shut down the stream:
	shutdownStream(client);
}


void shutdownStream(RTSPClient* rtspClient, int exitCode)
{
	UsageEnvironment& env = rtspClient->envir();
	HaikuRTSPClient* client = (HaikuRTSPClient*) rtspClient;

	// First, check whether any subsessions have still to be closed:
	if (client->session != NULL) {
		Boolean someSubsessionsWereActive = False;
		MediaSubsessionIterator iter(*client->session);
		MediaSubsession* subsession;

		while ((subsession = iter.next()) != NULL) {
			if (subsession->sink != NULL) {
				Medium::close(subsession->sink);
				subsession->sink = NULL;

				if (subsession->rtcpInstance() != NULL) {
					// in case the server sends a RTCP "BYE"
					// while handling "TEARDOWN"
					subsession->rtcpInstance()->setByeHandler(NULL, NULL);
				}

				someSubsessionsWereActive = True;
			}
		}

		if (someSubsessionsWereActive) {
			// Send a RTSP "TEARDOWN" command,
			// to tell the server to shutdown the stream.
			// Don't bother handling the response to the "TEARDOWN".
			rtspClient->sendTeardownCommand(*client->session, NULL);
		}
	}

	env << *rtspClient << "Closing the stream.\n";
	Medium::close(rtspClient);
	// Note that this will also cause this stream's
	// "StreamClientState" structure to get reclaimed.
	client->NotifyError();
}


AdapterSink* AdapterSink::createNew(UsageEnvironment& env,
	MediaSubsession& subsession, BInputAdapter* inputAdapter,
	char const* streamId)
{
	return new AdapterSink(env, subsession, streamId, inputAdapter);
}


AdapterSink::AdapterSink(UsageEnvironment& env, MediaSubsession& subsession,
	char const* streamId, BInputAdapter* inputAdapter)
	:
	MediaSink(env),
	fSubsession(subsession),
	fInputAdapter(inputAdapter)
{
	fStreamId = strDup(streamId);
	fReceiveBuffer = new u_int8_t[RECEIVE_BUFFER_SIZE];
}


AdapterSink::~AdapterSink()
{
	delete[] fReceiveBuffer;
	delete[] fStreamId;
}


void AdapterSink::afterGettingFrame(void* clientData, unsigned frameSize,
	unsigned numTruncatedBytes, struct timeval presentationTime,
	unsigned durationInMicroseconds)
{
	AdapterSink* sink = (AdapterSink*)clientData;
	sink->afterGettingFrame(frameSize, numTruncatedBytes,
		presentationTime, durationInMicroseconds);
}


void
AdapterSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
	struct timeval presentationTime, unsigned /*durationInMicroseconds*/)
{
	fInputAdapter->Write(fReceiveBuffer, frameSize);
	continuePlaying();
}


Boolean
AdapterSink::continuePlaying()
{
	if (fSource == NULL)
		return False;

	fSource->getNextFrame(fReceiveBuffer, RECEIVE_BUFFER_SIZE,
		afterGettingFrame, this,
		onSourceClosure, this);
	return True;
}
