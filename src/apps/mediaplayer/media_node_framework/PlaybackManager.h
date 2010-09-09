/*
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 * Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */

/*!	This class controls our playback
	Note: Add/RemoveListener() are the only methods that lock the object
	themselves. All other methods need it to be locked before.
*/
#ifndef PLAYBACK_MANAGER_H
#define PLAYBACK_MANAGER_H


#include <List.h>
#include <Looper.h>
#include <Rect.h>


class MessageEvent;
class PlaybackListener;


enum {
	MODE_PLAYING_FORWARD			= 1,
	MODE_PLAYING_BACKWARD			= 2,
	MODE_PLAYING_PAUSED_FORWARD		= -1,
	MODE_PLAYING_PAUSED_BACKWARD	= -2,
};

enum {
	LOOPING_ALL						= 0,
	LOOPING_RANGE					= 1,
	LOOPING_SELECTION				= 2,
	LOOPING_VISIBLE					= 3,
};

enum {
	MSG_PLAYBACK_FORCE_UPDATE		= 'pbfu',
	MSG_PLAYBACK_SET_RANGE			= 'pbsr',
	MSG_PLAYBACK_SET_VISIBLE		= 'pbsv',
	MSG_PLAYBACK_SET_LOOP_MODE		= 'pbsl',
};


class PlaybackManager : public BLooper {
private:
			struct PlayingState;
			struct SpeedInfo;

public:
								PlaybackManager();
	virtual						~PlaybackManager();

			void				Init(float frameRate,
									bool initPerformanceTimes,
									int32 loopingMode = LOOPING_ALL,
									bool loopingEnabled = true,
									float speed = 1.0,
									int32 playMode
										= MODE_PLAYING_PAUSED_FORWARD,
									int32 currentFrame = 0);
			void				Cleanup();

	// BHandler interface
	virtual	void				MessageReceived(BMessage* message);

			void				StartPlaying(bool atBeginning = false);
			void				StopPlaying();
			void				TogglePlaying(bool atBeginning = false);
			void				PausePlaying();
			bool				IsPlaying() const;

			int32				PlayMode() const;
			int32				LoopMode() const;
			bool				IsLoopingEnabled() const;
			int64				CurrentFrame() const;
			float				Speed() const;

	virtual	void				SetFramesPerSecond(float framesPerSecond);
			float				FramesPerSecond() const;

	virtual	BRect				VideoBounds() const = 0;

	virtual	void				FrameDropped() const;

			void				DurationChanged();
	virtual	int64				Duration() = 0;

	virtual	void				SetVolume(float percent) = 0;

			// playing state manipulation
			void				SetCurrentFrame(int64 frame);
	virtual	void				SetPlayMode(int32 mode,
									bool continuePlaying = true);
			void				SetLoopMode(int32 mode,
									bool continuePlaying = true);
			void				SetLoopingEnabled(bool enabled,
									bool continuePlaying = true);
			void				SetSpeed(float speed);

			// playing state change info
			int64				NextFrame() const;
			int64				NextPlaylistFrame() const;

			int64				FirstPlaybackRangeFrame();
			int64				LastPlaybackRangeFrame();

			// playing state access
			int64				StartFrameAtFrame(int64 frame);
			int64				StartFrameAtTime(bigtime_t time);
			int64				EndFrameAtFrame(int64 frame);
			int64				EndFrameAtTime(bigtime_t time);
			int64				FrameCountAtFrame(int64 frame);
			int64				FrameCountAtTime(bigtime_t time);
			int32				PlayModeAtFrame(int64 frame);
			int32				PlayModeAtTime(bigtime_t time);
			int32				LoopModeAtFrame(int64 frame);
			int32				LoopModeAtTime(bigtime_t time);
			// ...

			// playing frame/time/interval info
			int64				PlaylistFrameAtFrame(int64 frame,
									int32& playingDirection,
									bool& newState) const;
			int64				PlaylistFrameAtFrame(int64 frame,
									int32& playingDirection) const;
			int64				PlaylistFrameAtFrame(int64 frame) const;
			int64				NextChangeFrame(int64 startFrame,
												int64 endFrame) const;
			bigtime_t			NextChangeTime(bigtime_t startTime,
											   bigtime_t endTime) const;
			void				GetPlaylistFrameInterval(
									int64 startFrame, int64& endFrame,
									int64& xStartFrame, int64& xEndFrame,
									int32& playingDirection) const;

	virtual	void				GetPlaylistTimeInterval(
									bigtime_t startTime, bigtime_t& endTime,
									bigtime_t& xStartTime, bigtime_t& xEndTime,
									float& playingSpeed) const;

			// conversion: video frame <-> (performance) time
			int64				FrameForTime(bigtime_t time) const;
			bigtime_t			TimeForFrame(int64 frame) const;
			// conversion: (performance) time <-> real time
	virtual	bigtime_t			RealTimeForTime(bigtime_t time) const = 0;
	virtual	bigtime_t			TimeForRealTime(bigtime_t time) const = 0;
			// conversion: Playist frame <-> Playlist time
			int64				PlaylistFrameForTime(bigtime_t time) const;
			bigtime_t			PlaylistTimeForFrame(int64 frame) const;

			// to be called by audio/video producers
	virtual	void				SetCurrentAudioTime(bigtime_t time);
			void				SetCurrentVideoFrame(int64 frame);
			void				SetCurrentVideoTime(bigtime_t time);
			void				SetPerformanceFrame(int64 frame);
			void				SetPerformanceTime(bigtime_t time);

			// listener support
			void				AddListener(PlaybackListener* listener);
			void				RemoveListener(PlaybackListener* listener);

	virtual	void				NotifyPlayModeChanged(int32 mode) const;
	virtual	void				NotifyLoopModeChanged(int32 mode) const;
	virtual	void				NotifyLoopingEnabledChanged(
									bool enabled) const;
	virtual	void				NotifyVideoBoundsChanged(BRect bounds) const;
	virtual	void				NotifyFPSChanged(float fps) const;
	virtual	void				NotifyCurrentFrameChanged(int64 frame) const;
	virtual	void				NotifySpeedChanged(float speed) const;
	virtual	void				NotifyFrameDropped() const;
	virtual	void				NotifyStopFrameReached() const;
	virtual	void				NotifySeekHandled(int64 seekedFrame) const;

			// debugging
			void				PrintState(PlayingState* state);
			void				PrintStateAtFrame(int64 frame);

 private:
			// state management
			void				_PushState(PlayingState* state,
										   bool adjustCurrentFrame);
			void				_UpdateStates();
			int32				_IndexForFrame(int64 frame) const;
			int32				_IndexForTime(bigtime_t time) const;
			PlayingState*		_LastState() const;
			PlayingState*		_StateAt(int32 index) const;
			PlayingState*		_StateAtTime(bigtime_t time) const;
			PlayingState*		_StateAtFrame(int64 frame) const;

	static	int32				_PlayingDirectionFor(int32 playingMode);
	static	int32				_PlayingDirectionFor(PlayingState* state);
	static	void				_GetPlayingBoundsFor(PlayingState* state,
													 int64& startFrame,
													 int64& endFrame,
													 int64& frameCount);
	static	int64				_PlayingStartFrameFor(PlayingState* state);
	static	int64				_PlayingEndFrameFor(PlayingState* state);
	static	int64				_RangeFrameForFrame(PlayingState* state,
													int64 frame);
	static	int64				_FrameForRangeFrame(PlayingState* state,
													int64 index);
	static	int64				_NextFrameInRange(PlayingState* state,
												  int64 frame);

			// speed management
			void				_PushSpeedInfo(SpeedInfo* info);
			SpeedInfo*			_LastSpeedInfo() const;
			SpeedInfo*			_SpeedInfoAt(int32 index) const;
			int32				_SpeedInfoIndexForFrame(int64 frame) const;
			int32				_SpeedInfoIndexForTime(bigtime_t time) const;
			SpeedInfo*			_SpeedInfoForFrame(int64 frame) const;
			SpeedInfo*			_SpeedInfoForTime(bigtime_t time) const;
			void				_UpdateSpeedInfos();

			bigtime_t			_TimeForLastFrame() const;
			bigtime_t			_TimeForNextFrame() const;

			void				_NotifySeekHandledIfNecessary(
									PlayingState* state);
			void				_CheckStopPlaying();

private:
			BList				fStates;
			BList				fSpeeds;
	volatile bigtime_t			fCurrentAudioTime;
	volatile bigtime_t			fCurrentVideoTime;
	volatile bigtime_t			fPerformanceTime;

			MessageEvent*		fPerformanceTimeEvent;

	volatile float				fFrameRate;			// video frame rate
	volatile bigtime_t			fStopPlayingFrame;	// frame at which playing
													// shall be stopped,
													// disabled: -1

			BList				fListeners;
protected:
 			bool				fNoAudio;
};


#endif	// PLAYBACK_MANAGER_H
