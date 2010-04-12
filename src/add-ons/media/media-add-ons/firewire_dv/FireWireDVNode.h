/*
 * FireWire DV media addon for Haiku
 *
 * Copyright (c) 2008, JiSheng Zhang (jszhang3@mail.ustc.edu.cn)
 * Distributed under the terms of the MIT License.
 *
 * Based on DVB media addon
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 */

#ifndef __FIREWIRE_DV_NODE_H
#define __FIREWIRE_DV_NODE_H


#include <OS.h>
#include <media/BufferProducer.h>
//#include <media/BufferConsumer.h>
#include <media/Controllable.h>
#include <media/MediaDefs.h>
#include <media/MediaEventLooper.h>
#include <media/MediaNode.h>
#include <support/Locker.h>


class FireWireCard;
class BParameterGroup;


class FireWireDVNode : public virtual BBufferProducer,
	public virtual BControllable, public virtual BMediaEventLooper {
public:
				FireWireDVNode(BMediaAddOn *addon,
					const char *name, int32 internal_id,
					FireWireCard *card);
	virtual			~FireWireDVNode();

	virtual	status_t	InitCheck() const { return fInitStatus; }

/* BMediaNode */
public:
	virtual	BMediaAddOn	*AddOn(int32 * internal_id) const;
	virtual	status_t 	HandleMessage(int32 message, const void *data,
					size_t size);
protected:
	virtual	void 		Preroll();
	virtual void		SetTimeSource(BTimeSource * time_source);
	virtual void		SetRunMode(run_mode mode);

/* BMediaEventLooper */
protected:
	virtual	void 		NodeRegistered();
	virtual void		HandleEvent(const media_timed_event *event,
					bigtime_t lateness,
					bool realTimeEvent = false);

/* BBufferProducer */
protected:
	virtual	status_t	FormatSuggestionRequested(media_type type,
					int32 quality, media_format* format);
	virtual	status_t 	FormatProposal(const media_source &source,
					media_format* format);
	virtual	status_t	FormatChangeRequested(
					const media_source &source,
					const media_destination &destination,
					media_format* io_format,
					int32* _deprecated_);
	virtual	status_t 	GetNextOutput(int32* cookie,
					media_output* out_output);
	virtual	status_t	DisposeOutputCookie(int32 cookie);
	virtual	status_t	SetBufferGroup(const media_source& for_source,
					BBufferGroup* group);
	virtual	status_t 	VideoClippingChanged(
					const media_source& for_source,
					int16 num_shorts, int16* clip_data,
					const media_video_display_info &display,
					int32* _deprecated_);
	virtual	status_t	GetLatency(bigtime_t* out_latency);
	virtual	status_t	PrepareToConnect(const media_source& what,
					const media_destination& where,
					media_format* format,
					media_source* out_source,
					char* out_name);
	virtual	void		Connect(status_t error,
					const media_source& source,
					const media_destination& destination,
					const media_format& format,
					char* io_name);
	virtual	void 		Disconnect(const media_source& what,
					const media_destination& where);
	virtual	void 		LateNoticeReceived(const media_source& what,
					bigtime_t how_much,
					bigtime_t performance_time);
	virtual	void 		EnableOutput(const media_source& what,
					bool enabled, int32* _deprecated_);
	virtual	void 		AdditionalBufferRequested(
					const media_source& source,
					media_buffer_id prev_buffer,
					bigtime_t prev_time,
					const media_seek_tag* prev_tag);

/* BControllable */
protected:
	virtual status_t	GetParameterValue(int32 id,
					bigtime_t* last_change, void* value,
					size_t* size);
	virtual void		SetParameterValue(int32 id, bigtime_t when,
					const void* value, size_t size);

/* state */
private:
	void			HandleStart(bigtime_t performance_time);
	void			HandleStop();
	void			HandleTimeWarp(bigtime_t performance_time);
	void			HandleSeek(bigtime_t performance_time);


	BParameterWeb*		CreateParameterWeb();
	void			SetAboutInfo(BParameterGroup *about);

	void			RefreshParameterWeb();

	status_t		StartCapture();
	status_t		StopCapture();
	status_t		StartCaptureThreads();
	status_t		StopCaptureThreads();

	static int32		_card_reader_thread_(void *arg);

	void			card_reader_thread();

	status_t		fInitStatus;
	int32			fInternalID;
	BMediaAddOn*		fAddOn;
	bool			fOutputEnabledEncVideo;
	media_output		fOutputEncVideo;
	media_format		fDefaultFormatEncVideo;
	FireWireCard*		fCard;
	bool			fCaptureThreadsActive;
	thread_id		fThreadIdCardReader;
	volatile bool		fTerminateThreads;
	BBufferGroup*		fBufferGroupEncVideo;
	BParameterWeb*		fWeb;
	bool			fCaptureActive;
	BLocker			fLock;

};

#endif	// __FIREWIRE_DV_NODE_H
