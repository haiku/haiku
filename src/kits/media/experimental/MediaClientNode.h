/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _MEDIA_CLIENT_NODE_H
#define _MEDIA_CLIENT_NODE_H


#include <BufferConsumer.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <MediaDefs.h>
#include <MediaEventLooper.h>


namespace BPrivate { namespace media {


class BMediaClient;
class BMediaConnection;
class BMediaOutput;

class BMediaClientNode : public BBufferConsumer, public BBufferProducer,
	public BMediaEventLooper {
public:
							BMediaClientNode(const char* name,
								BMediaClient* owner,
								media_type type
									= B_MEDIA_UNKNOWN_TYPE);

	// Various useful stuff

			status_t		SendBuffer(BBuffer* buffer, BMediaConnection* conn);

protected:

	virtual	BMediaAddOn*	AddOn(int32* id) const;

	virtual void			NodeRegistered();

	virtual void			SetRunMode(run_mode mode);

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

	// BBufferConsumer

	virtual	status_t		AcceptFormat(const media_destination& dest,
								media_format* format);

	virtual	status_t		GetNextInput(int32* cookie,
								media_input* input);

	virtual	void			DisposeInputCookie(int32 cookie);

	virtual	void			BufferReceived(BBuffer* buffer);

	virtual	status_t		GetLatencyFor(const media_destination& dest,
								bigtime_t* latency,
								media_node_id* timesource);

	virtual	status_t		Connected(const media_source& source,
								const media_destination& dest,
								const media_format& format,
								media_input* outInput);

	virtual	void			Disconnected(const media_source& source,
								const media_destination& dest);

	virtual	status_t		FormatChanged(const media_source& source,
								const media_destination& consumer,
								int32 tag,
								const media_format& format);

	// BBufferProducer

	virtual 	status_t 	FormatSuggestionRequested(media_type type,
									int32 quality, media_format* format);
	virtual 	status_t 	FormatProposal(const media_source& source,
									media_format *format);
	virtual 	status_t 	FormatChangeRequested(const media_source& source,
									const media_destination& dest,
									media_format *format,
									int32* _deprecated_);
	virtual 	void 		LateNoticeReceived(const media_source& source,
									bigtime_t late,	bigtime_t when);
	virtual 	status_t	GetNextOutput(int32 *cookie, media_output *output);
	virtual 	status_t 	DisposeOutputCookie(int32 cookie);
	virtual 	status_t	SetBufferGroup(const media_source& source,
									BBufferGroup *group);
	virtual 	status_t 	PrepareToConnect(const media_source& source,
									const media_destination& dest,
									media_format *format,
									media_source *out_source,
									char *name);
	virtual 	void 		Connect(status_t status,
									const media_source& source,
									const media_destination& dest,
									const media_format &format,
									char* name);
	virtual		void 		Disconnect(const media_source& source,
									const media_destination& dest);
	virtual 	void 		EnableOutput(const media_source& source,
									bool enabled, int32* _deprecated_);
	virtual 	status_t 	GetLatency(bigtime_t *outLatency);
	virtual 	void 		LatencyChanged(	const media_source& source,
									const media_destination& dest,
									bigtime_t latency, uint32 flags);

				void 		ProducerDataStatus(const media_destination& dest,
								int32 status, bigtime_t when);
protected:
	virtual 	void 		HandleEvent(const media_timed_event *event,
									bigtime_t late,
									bool realTimeEvent=false);

	virtual					~BMediaClientNode();

private:
				void		_ScheduleConnections(bigtime_t eventTime);
				void		_HandleBuffer(BBuffer* buffer);
				void		_ProduceNewBuffer(const media_timed_event* event,
								bigtime_t late);
				BBuffer*	_GetNextBuffer(BMediaOutput* output,
								bigtime_t eventTime);

			BMediaClient*	fOwner;
			bigtime_t		fStartTime;
};

}
}

using namespace BPrivate::media;

#endif
