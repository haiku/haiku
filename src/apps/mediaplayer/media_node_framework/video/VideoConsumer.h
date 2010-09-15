/*	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
 *	Distributed under the terms of the Be Sample Code license.
 *
 *	Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 *	Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 *	All Rights Reserved. Distributed under the terms of the MIT license.
 */
#ifndef VIDEO_CONSUMER_H
#define VIDEO_CONSUMER_H


#include <BufferConsumer.h>
#include <Locker.h>
#include <MediaEventLooper.h>


class BBitmap;
class NodeManager;
class VideoTarget;


// TODO: The buffer count should depend on the latency!
static const unsigned int kBufferCount = 4;


class VideoConsumer : public BMediaEventLooper, public BBufferConsumer {
public:
								VideoConsumer(
									const char* name,
									BMediaAddOn* addon,
									const uint32 internal_id,
									NodeManager* manager,
									VideoTarget* target);
								~VideoConsumer();
	
	// BMediaNode interface
public:
	
	virtual	BMediaAddOn*		AddOn(long* cookie) const;
	
protected:
	virtual void				NodeRegistered();
	virtual	status_t		 	RequestCompleted(
									const media_request_info& info);
							
	virtual	status_t 			HandleMessage(int32 message, const void* data,
									size_t size);

	// BMediaEventLooper interface
protected:
	virtual void				HandleEvent(const media_timed_event* event,
									bigtime_t lateness, bool realTimeEvent);

	// BBufferConsumer interface
public:
	virtual	status_t			AcceptFormat(const media_destination& dest,
									media_format* format);
	virtual	status_t			GetNextInput(int32* cookie,
									media_input* _input);
							
	virtual	void				DisposeInputCookie(int32 cookie);
	
protected:
	virtual	void				BufferReceived(BBuffer* buffer);
	
private:
	virtual	void				ProducerDataStatus(
									const media_destination& forWhom,
									int32 status,
									bigtime_t atMediaTime);									
	virtual	status_t			GetLatencyFor(
									const media_destination& forWhom,
									bigtime_t* outLatency,
									media_node_id* outId);	
	virtual	status_t			Connected(const media_source& producer,
									const media_destination& where,
									const media_format& withFormat,
									media_input* outInput);
	virtual	void				Disconnected(const media_source& producer,
									const media_destination& where);							
	virtual	status_t			FormatChanged(const media_source& producer,
									const media_destination& consumer, 
									int32 from_change_count,
									const media_format& format);
							
	// VideoConsumer
public:
			status_t			CreateBuffers(
									const media_format& withFormat);
							
			void				DeleteBuffers();

			void				SetTarget(VideoTarget* target);
			void				SetTryOverlay(bool tryOverlay);

private:
			void				_SetPerformanceTimeBase(
									bigtime_t performanceTime);
			void				_HandleBuffer(BBuffer* buffer);
			void				_UnsetTargetBuffer();

private:
			uint32				fInternalID;
			BMediaAddOn*		fAddOn;

			bool				fConnectionActive;
			media_input			fIn;
			bigtime_t			fMyLatency;
			bigtime_t			fPerformanceTimeBase;

			BBitmap*			fBitmap[kBufferCount];
			bool				fOurBuffers;
			BBufferGroup*		fBuffers;
			BBuffer*			fBufferMap[kBufferCount];	

			NodeManager*		fManager;
			BLocker				fTargetLock;	// locks the following variable
			VideoTarget* volatile	fTarget;
			int32				fLastBufferIndex;

			bool				fTryOverlay;
};

#endif // VIDEO_CONSUMER_H
