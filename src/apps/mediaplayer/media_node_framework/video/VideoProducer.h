/*	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
 *	Distributed under the terms of the Be Sample Code license.
 *
 *	Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 *	Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 *	All Rights Reserved. Distributed under the terms of the MIT license.
 */
#ifndef _VIDEO_PRODUCER_H
#define _VIDEO_PRODUCER_H


#include <BufferProducer.h>
#include <Controllable.h>
#include <Locker.h>
#include <MediaDefs.h>
#include <MediaEventLooper.h>
#include <MediaNode.h>
#include <OS.h>
#include <Rect.h>


class NodeManager;
class VideoSupplier;


class VideoProducer : public virtual BMediaEventLooper,
					  public virtual BBufferProducer {
public:
							VideoProducer(BMediaAddOn* addon, const char* name,
								int32 internalId, NodeManager* manager,
								VideoSupplier* supplier);
	virtual					~VideoProducer();
	
	virtual	status_t		InitCheck() const
								{ return fInitStatus; }
	
	// BMediaNode interface
public:
	virtual	BMediaAddOn*	AddOn(int32* _internalId) const;
	virtual	status_t	 	HandleMessage(int32 message, const void* data,
								size_t size);
protected:	
	virtual void			SetTimeSource(BTimeSource* timeSource);
	
	// BMediaEventLooper interface
protected:
	virtual	void 			NodeRegistered();
	virtual void			Start(bigtime_t performanceTime);
	virtual void			Stop(bigtime_t performanceTime, bool immediate);
	virtual void			Seek(bigtime_t mediaTime,
								bigtime_t performanceTime);
	virtual void			HandleEvent(const media_timed_event* event,
								bigtime_t lateness,
								bool realTimeEvent = false);
	virtual status_t		DeleteHook(BMediaNode* node);
	
	// BBufferProducer interface
protected:
	virtual	status_t		FormatSuggestionRequested(media_type type,
								int32 quality, media_format* format);
	virtual	status_t 		FormatProposal(const media_source &output,
								media_format* format);
	virtual	status_t		FormatChangeRequested(const media_source& source,
								const media_destination& destination,
								media_format* ioFormat, int32* _deprecated_);
	virtual	status_t 		GetNextOutput(int32* cookie,
								media_output* outOutput);
	virtual	status_t		DisposeOutputCookie(int32 cookie);
	virtual	status_t		SetBufferGroup(const media_source& forSource,
								BBufferGroup* group);
	virtual	status_t 		VideoClippingChanged(const media_source& forSource,
								int16 numShorts, int16* clipData,
								const media_video_display_info& display,
								int32* _deprecated_);
	virtual	status_t		GetLatency(bigtime_t* out_latency);
	virtual	status_t		PrepareToConnect(const media_source& what,
								const media_destination& where,
								media_format* format, media_source* outSource,
								char* out_name);
	virtual	void			Connect(status_t error, const media_source& source,
								const media_destination& destination,
								const media_format& format, char* ioName);
	virtual	void 			Disconnect(const media_source& what,
								const media_destination& where);
	virtual	void 			LateNoticeReceived(const media_source& what,
								bigtime_t howMuch, bigtime_t performanceTime);
	virtual	void 			EnableOutput(const media_source& what, bool enabled,
								int32* _deprecated_);
	virtual	status_t		SetPlayRate(int32 numer, int32 denom);
	virtual	void 			AdditionalBufferRequested(
								const media_source& source,
								media_buffer_id prevBuffer,
								bigtime_t prevTime,
								const media_seek_tag* prevTag);
	virtual	void			LatencyChanged(const media_source& source,
								const media_destination& destination,
								bigtime_t newLatency, uint32 flags);
	
 private:
			void			_HandleStart(bigtime_t performance_time);
			void			_HandleStop();
			void			_HandleTimeWarp(bigtime_t performance_time);
			void			_HandleSeek(bigtime_t performance_time);

	static	int32			_FrameGeneratorThreadEntry(void* data);
			int32			_FrameGeneratorThread();

		status_t			fInitStatus;

		int32				fInternalID;
		BMediaAddOn*		fAddOn;

		BLocker				fLock;
		BBufferGroup*		fBufferGroup;
		BBufferGroup*		fUsedBufferGroup;

		thread_id			fThread;
		sem_id				fFrameSync;

		// The remaining variables should be declared volatile, but they
		// are not here to improve the legibility of the sample code.
		int64				fFrame;
		int64				fFrameBase;
		bigtime_t			fPerformanceTimeBase;
		bigtime_t			fBufferDuration;
		bigtime_t			fBufferLatency;
		media_output		fOutput;
		media_raw_video_format	fConnectedFormat;
		bool				fRunning;
		bool				fConnected;
		bool				fEnabled;

		NodeManager*		fManager;
		VideoSupplier*		fSupplier;
};

#endif // VIDEO_PRODUCER_H
