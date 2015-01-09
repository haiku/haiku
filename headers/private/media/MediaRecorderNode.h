/*
 * Copyright 2014, Dario Casalinuovo
 * Copyright 1999, Be Incorporated
 * All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef _MEDIA_RECORDER_NODE_H
#define _MEDIA_RECORDER_NODE_H


#include <BufferConsumer.h>
#include <MediaEventLooper.h>
#include <String.h>


namespace BPrivate { namespace media {

class BMediaRecorder;

class BMediaRecorderNode : public BMediaEventLooper,
	public BBufferConsumer {
public:
							BMediaRecorderNode(const char* name,
								BMediaRecorder* recorder,
								media_type type
									= B_MEDIA_UNKNOWN_TYPE);

			//	TODO these are not thread safe; we should fix that...
			void			SetAcceptedFormat(const media_format& format);

			status_t		GetInput(media_input* outInput);

			void			SetDataEnabled(bool enabled);

protected:

	virtual	BMediaAddOn*	AddOn(int32* id) const;

	virtual void			NodeRegistered();

	virtual void			SetRunMode(run_mode mode);

	virtual void			HandleEvent(const media_timed_event* event,
								bigtime_t lateness,
								bool realTimeEvent);

	virtual	void			Start(bigtime_t performanceTime);

	virtual	void			Stop(bigtime_t performanceTime,
								bool immediate);

	virtual	void			Seek(bigtime_t mediaTime,
								bigtime_t performanceTime);

	virtual	void			TimeWarp(bigtime_t realTime,
								bigtime_t performanceTime);

	virtual	status_t		HandleMessage(int32 message,
								const void* data,
								size_t size);

			// Someone, probably the producer, is asking you about
			// this format. Give your honest opinion, possibly
			// modifying *format. Do not ask upstream producer about
			//	the format, since he's synchronously waiting for your
			// reply.

	virtual	status_t		AcceptFormat(const media_destination& dest,
								media_format* format);

	virtual	status_t		GetNextInput(int32* cookie,
								media_input* outInput);

	virtual	void			DisposeInputCookie(int32 cookie);

	virtual	void			BufferReceived(BBuffer* buffer);

	virtual	void			ProducerDataStatus(
								const media_destination& destination,
								int32 status,
								bigtime_t performanceTime);

	virtual	status_t		GetLatencyFor(const media_destination& destination,
								bigtime_t* outLatency,
								media_node_id* outTimesource);

	virtual	status_t		Connected(const media_source& producer,
								const media_destination& where,
								const media_format& format,
								media_input* outInput);

	virtual	void			Disconnected(const media_source& producer,
								const media_destination& where);

	virtual	status_t		FormatChanged(const media_source& producer,
								const media_destination& consumer,
								int32 tag,
								const media_format& format);

protected:

	virtual					~BMediaRecorderNode();

			BMediaRecorder*	fRecorder;
			media_format	fOKFormat;
			media_input		fInput;
			BString			fName;
};

}
}

using namespace BPrivate::media;

#endif	//	_MEDIA_RECORDER_NODE_H
