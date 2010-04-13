/*
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 * Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#ifndef NODE_MANAGER_H
#define NODE_MANAGER_H


#include <MediaNode.h>

#include "PlaybackManager.h"


class BMediaRoster;
class AudioProducer;
class VideoTarget;
class VideoProducer;
class VideoConsumer;
class AudioSupplier;
class VideoSupplier;


class NodeManager : public PlaybackManager {
public:
								NodeManager();
	virtual						~NodeManager();

	// must be implemented in derived classes
	virtual	VideoTarget*		CreateVideoTarget() = 0;
	virtual	VideoSupplier*		CreateVideoSupplier() = 0;
	virtual	AudioSupplier*		CreateAudioSupplier() = 0;

	// NodeManager
	enum {
		AUDIO_AND_VIDEO	= 0,
		VIDEO_ONLY,
		AUDIO_ONLY
	};

			status_t			Init(BRect videoBounds, float videoFrameRate,
									color_space preferredVideoFormat,
									float audioFrameRate, uint32 audioChannels,
									int32 loopingMode, bool loopingEnabled,
									float speed, uint32 enabledNodes,
									bool useOverlays);
			status_t			InitCheck();
								// only call this if the
								// media_server has died!
			status_t			CleanupNodes();

			status_t			FormatChanged(BRect videoBounds,
									float videoFrameRate,
									color_space preferredVideoFormat,
									float audioFrameRate, uint32 audioChannels,
									uint32 enabledNodes,
									bool useOverlays,
									bool force = false);

	virtual	void				SetPlayMode(int32 mode,
									bool continuePlaying = true);

	virtual	bigtime_t			RealTimeForTime(bigtime_t time) const;
	virtual	bigtime_t			TimeForRealTime(bigtime_t time) const;

	virtual	void				SetCurrentAudioTime(bigtime_t time);

			void				SetVideoBounds(BRect bounds);
	virtual	BRect				VideoBounds() const;

			void				SetVideoTarget(VideoTarget* vcTarget);
			VideoTarget*		GetVideoTarget() const;

	virtual	void				SetVolume(float percent);

			void				SetPeakListener(BHandler* handler);

 private:
			status_t			_SetUpNodes(color_space preferredVideoFormat,
									uint32 enabledNodes, bool useOverlays,
									float audioFrameRate,
									uint32 audioChannels);
			status_t			_SetUpVideoNodes(
									color_space preferredVideoFormat,
									bool useOverlays);
			status_t			_SetUpAudioNodes(float audioFrameRate,
									uint32 audioChannels);
			status_t			_TearDownNodes(bool disconnect = true);
			status_t			_StartNodes();
			void				_StopNodes();

private:
	struct Connection {
			Connection();

			media_node			producer;
			media_node			consumer;
			media_source		source;
			media_destination	destination;
			media_format		format;
			bool				connected;
	};

private:
			BMediaRoster*		fMediaRoster;

			// media nodes
			AudioProducer*		fAudioProducer;
			VideoConsumer*		fVideoConsumer;
			VideoProducer*		fVideoProducer;
			media_node			fTimeSource;

			Connection			fAudioConnection;
			Connection			fVideoConnection;

			bigtime_t			fPerformanceTimeBase;

			status_t			fStatus;
			//
			VideoTarget*		fVideoTarget;
			AudioSupplier*		fAudioSupplier;
			VideoSupplier*		fVideoSupplier;
			BRect				fVideoBounds;

			BHandler*			fPeakListener;
};


#endif	// NODE_MANAGER_H
