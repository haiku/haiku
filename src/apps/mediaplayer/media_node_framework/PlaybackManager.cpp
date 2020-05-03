// PlaybackManager.cpp


#include <algorithm>
#include <stdio.h>

#include <Message.h>
#include <OS.h>
#include <Window.h>

#include "EventQueue.h"
#include "MessageEvent.h"
#include "PlaybackListener.h"

#include "PlaybackManager.h"


using namespace std;


//#define TRACE_NODE_MANAGER
#ifdef TRACE_NODE_MANAGER
#	define TRACE(x...)	printf(x)
#	define ERROR(x...)	fprintf(stderr, x)
#else
#	define TRACE(x...)
#	define ERROR(x...)	fprintf(stderr, x)
#endif


void
tdebug(const char* str)
{
	TRACE("[%lx, %lld] ", find_thread(NULL), system_time());
	TRACE(str);
}


#define SUPPORT_SPEED_CHANGES 0


struct PlaybackManager::PlayingState {
	int64		start_frame;
	int64		end_frame;
	int64		frame_count;
	int64		first_visible_frame;
	int64		last_visible_frame;
	int64		max_frame_count;
	int32		play_mode;
	int32		loop_mode;
	bool		looping_enabled;
	bool		is_seek_request;
	int64		current_frame;			// Playlist frame
	int64		range_index;			// playing range index of current_frame
	int64		activation_frame;		// absolute video frame

	PlayingState()
	{
	}

	PlayingState(const PlayingState& other)
		:
		start_frame(other.start_frame),
		end_frame(other.end_frame),
		frame_count(other.frame_count),
		first_visible_frame(other.first_visible_frame),
		last_visible_frame(other.last_visible_frame),
		max_frame_count(other.max_frame_count),
		play_mode(other.play_mode),
		loop_mode(other.loop_mode),
		looping_enabled(other.looping_enabled),
		is_seek_request(false),
		current_frame(other.current_frame),
		range_index(other.range_index),
		activation_frame(other.activation_frame)
	{
	}
};


#if SUPPORT_SPEED_CHANGES
struct PlaybackManager::SpeedInfo {
	int64		activation_frame;		// absolute video frame
	bigtime_t	activation_time;		// performance time
	float		speed;					// speed to be used for calculations,
										// is 1.0 if not playing
	float		set_speed;				// speed set by the user
};
#endif


// #pragma mark - PlaybackManager


PlaybackManager::PlaybackManager()
	:
	BLooper("playback manager"),
	fStates(10),
	fSpeeds(10),
	fCurrentAudioTime(0),
	fCurrentVideoTime(0),
	fPerformanceTime(0),
	fPerformanceTimeEvent(NULL),
	fFrameRate(1.0),
	fStopPlayingFrame(-1),
	fListeners(),
	fNoAudio(false)
{
	Run();
}


PlaybackManager::~PlaybackManager()
{
	Cleanup();
}


void
PlaybackManager::Init(float frameRate, bool initPerformanceTimes,
	int32 loopingMode, bool loopingEnabled, float playbackSpeed,
	int32 playMode, int32 currentFrame)
{
	// cleanup first
	Cleanup();

	// set the new frame rate
	fFrameRate = frameRate;
	if (initPerformanceTimes) {
		fCurrentAudioTime = 0;
		fCurrentVideoTime = 0;
		fPerformanceTime = 0;
	}
	fStopPlayingFrame = -1;

#if SUPPORT_SPEED_CHANGES
	// set up the initial speed
	SpeedInfo* speed = new SpeedInfo;
	speed->activation_frame = 0;
	speed->activation_time = 0;
	speed->speed = playbackSpeed;
	speed->set_speed = playbackSpeed;
	_PushSpeedInfo(speed);
#endif

	// set up the initial state
	PlayingState* state = new PlayingState;
	state->frame_count = Duration();
	state->start_frame = 0;
	state->end_frame = 0;
	state->first_visible_frame = 0;
	state->last_visible_frame = 0;
	state->max_frame_count = state->frame_count;

	state->play_mode = MODE_PLAYING_PAUSED_FORWARD;
	state->loop_mode = loopingMode;
	state->looping_enabled = loopingEnabled;
	state->is_seek_request = false;
	state->current_frame = currentFrame;
	state->activation_frame = 0;
	fStates.AddItem(state);

	TRACE("initial state pushed: state count: %ld\n", fStates.CountItems());

	// notify the listeners
	NotifyPlayModeChanged(PlayMode());
	NotifyLoopModeChanged(LoopMode());
	NotifyLoopingEnabledChanged(IsLoopingEnabled());
	NotifyVideoBoundsChanged(VideoBounds());
	NotifyFPSChanged(FramesPerSecond());
	NotifySpeedChanged(Speed());
	NotifyCurrentFrameChanged(CurrentFrame());
	SetPlayMode(playMode);
}


void
PlaybackManager::Cleanup()
{
	if (EventQueue::Default().RemoveEvent(fPerformanceTimeEvent))
		delete fPerformanceTimeEvent;
	fPerformanceTimeEvent = NULL;
	// delete states
	for (int32 i = 0; PlayingState* state = _StateAt(i); i++)
		delete state;
	fStates.MakeEmpty();

#if SUPPORT_SPEED_CHANGES
	// delete speed infos
	for (int32 i = 0; SpeedInfo* speed = _SpeedInfoAt(i); i++)
		delete speed;
	fSpeeds.MakeEmpty();
#endif
}


void
PlaybackManager::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_EVENT:
		{
			if (fPerformanceTimeEvent == NULL) {
				// Stale event message. There is a natural race
				// condition when removing the event from the queue,
				// it may have already fired, but we have not processed
				// the message yet. Simply ignore the event.
				break;
			}

			bigtime_t eventTime;
			message->FindInt64("time", &eventTime);
//			bigtime_t now = system_time();
			fPerformanceTimeEvent = NULL;

//			SetPerformanceTime(TimeForRealTime(now));
			SetPerformanceTime(TimeForRealTime(eventTime));
//TRACE("MSG_EVENT: rt: %lld, pt: %lld\n", now, fPerformanceTime);
//printf("MSG_EVENT: et: %lld, rt: %lld, pt: %lld\n", eventTime, now, fPerformanceTime);
			break;
		}

		case MSG_PLAYBACK_FORCE_UPDATE:
		{
			int64 frame;
			if (message->FindInt64("frame", &frame) < B_OK)
				frame = CurrentFrame();
			SetCurrentFrame(frame);
			break;
		}

		case MSG_PLAYBACK_SET_RANGE:
		{
			int64 startFrame = _LastState()->start_frame;
			int64 endFrame = _LastState()->end_frame;
			message->FindInt64("start frame", &startFrame);
			message->FindInt64("end frame", &endFrame);
			if (startFrame != _LastState()->start_frame
				|| endFrame != _LastState()->end_frame) {
				PlayingState* state = new PlayingState(*_LastState());
				state->start_frame = startFrame;
				state->end_frame = endFrame;
				_PushState(state, true);
			}
			break;
		}

		case MSG_PLAYBACK_SET_VISIBLE:
		{
			int64 startFrame = _LastState()->first_visible_frame;
			int64 endFrame = _LastState()->last_visible_frame;
			message->FindInt64("start frame", &startFrame);
			message->FindInt64("end frame", &endFrame);
			if (startFrame != _LastState()->first_visible_frame
				|| endFrame != _LastState()->last_visible_frame) {
				PlayingState* state = new PlayingState(*_LastState());
				state->first_visible_frame = startFrame;
				state->last_visible_frame = endFrame;
				_PushState(state, true);
			}
			break;
		}

		case MSG_PLAYBACK_SET_LOOP_MODE:
		{
			int32 loopMode = _LastState()->loop_mode;
			message->FindInt32("mode", &loopMode);
			if (loopMode != _LastState()->loop_mode) {
				PlayingState* state = new PlayingState(*_LastState());
				state->loop_mode = loopMode;
				_PushState(state, true);
			}
			break;
		}

		// other messages
		default:
			BLooper::MessageReceived(message);
			break;
	}
}

// #pragma mark - playback control


void
PlaybackManager::StartPlaying(bool atBeginning)
{
	TRACE("PlaybackManager::StartPlaying()\n");
	int32 playMode = PlayMode();
	if (playMode == MODE_PLAYING_PAUSED_FORWARD)
		SetPlayMode(MODE_PLAYING_FORWARD, !atBeginning);
	if (playMode == MODE_PLAYING_PAUSED_BACKWARD)
		SetPlayMode(MODE_PLAYING_BACKWARD, !atBeginning);
}


void
PlaybackManager::StopPlaying()
{
	TRACE("PlaybackManager::StopPlaying()\n");
	int32 playMode = PlayMode();
	if (playMode == MODE_PLAYING_FORWARD)
		SetPlayMode(MODE_PLAYING_PAUSED_FORWARD, true);
	if (playMode == MODE_PLAYING_BACKWARD)
		SetPlayMode(MODE_PLAYING_PAUSED_BACKWARD, true);
}


void
PlaybackManager::TogglePlaying(bool atBeginning)
{
	// playmodes (paused <-> playing) are the negative of each other
	SetPlayMode(-PlayMode(), !atBeginning);
}


void
PlaybackManager::PausePlaying()
{
	if (PlayMode() > 0)
		TogglePlaying();
}


bool
PlaybackManager::IsPlaying() const
{
	return (PlayMode() > 0);
}


int32
PlaybackManager::PlayMode() const
{
	if (!_LastState())
		return MODE_PLAYING_PAUSED_FORWARD;
	return _LastState()->play_mode;
}


int32
PlaybackManager::LoopMode() const
{
	if (!_LastState())
		return LOOPING_ALL;
	return _LastState()->loop_mode;
}


bool
PlaybackManager::IsLoopingEnabled() const
{
	if (!_LastState())
		return true;
	return _LastState()->looping_enabled;
}


int64
PlaybackManager::CurrentFrame() const
{
	return PlaylistFrameAtFrame(FrameForTime(fPerformanceTime));
}


float
PlaybackManager::Speed() const
{
#if SUPPORT_SPEED_CHANGES
	if (!_LastState())
		return 1.0;
	return _LastSpeedInfo()->set_speed;
#else
	return 1.0;
#endif
}


void
PlaybackManager::SetFramesPerSecond(float framesPerSecond)
{
	if (framesPerSecond != fFrameRate) {
		fFrameRate = framesPerSecond;
		NotifyFPSChanged(fFrameRate);
	}
}


float
PlaybackManager::FramesPerSecond() const
{
	return fFrameRate;
}


void
PlaybackManager::FrameDropped() const
{
	NotifyFrameDropped();
}


void
PlaybackManager::DurationChanged()
{
	if (!_LastState())
		return;
	int32 frameCount = Duration();
	if (frameCount != _LastState()->frame_count) {
		PlayingState* state = new PlayingState(*_LastState());
		state->frame_count = frameCount;
		state->max_frame_count = frameCount;

		_PushState(state, true);
	}
}

/*! Creates a new playing state that equals the previous one aside from
	its current frame which is set to /frame/.
	The new state will be activated as soon as possible. */
void
PlaybackManager::SetCurrentFrame(int64 frame)
{
	if (CurrentFrame() == frame) {
		NotifySeekHandled(frame);
		return;
	}
	PlayingState* newState = new PlayingState(*_LastState());
	newState->current_frame = frame;
	newState->is_seek_request = true;
	_PushState(newState, false);
}


/*!	Creates a new playing state that equals the previous one aside from
	its playing mode which is set to /mode/.
	The new state will be activated as soon as possible.
	If /continuePlaying/ is true and the new state is a `not stopped'
	state, playing continues at the frame the last state reaches when the
	new state becomes active (or the next frame in the playing range).
	If /continuePlaying/ is false, playing starts at the beginning of the
	playing range (taking the playing direction into consideration). */
void
PlaybackManager::SetPlayMode(int32 mode, bool continuePlaying)
{
//printf("PlaybackManager::SetPlayMode(%ld, %d)\n", mode, continuePlaying);
	PlayingState* newState = new PlayingState(*_LastState());
	newState->play_mode = mode;
	// Jump to the playing start frame if we should not continue, where we
	// stop.
	if (!continuePlaying && !(_PlayingDirectionFor(newState) == 0))
		newState->current_frame = _PlayingStartFrameFor(newState);
	_PushState(newState, continuePlaying);
	NotifyPlayModeChanged(mode);
}


/*!	Creates a new playing state that equals the previous one aside from
	its loop mode which is set to /mode/.
	The new state will be activated as soon as possible.
	If /continuePlaying/ is true and the new state is a `not stopped'
	state, playing continues at the frame the last state reaches when the
	new state becomes active (or the next frame in the playing range).
	If /continuePlaying/ is false, playing starts at the beginning of the
	playing range (taking the playing direction into consideration). */
void
PlaybackManager::SetLoopMode(int32 mode, bool continuePlaying)
{
	PlayingState* newState = new PlayingState(*_LastState());
	newState->loop_mode = mode;
	// Jump to the playing start frame if we should not continue, where we
	// stop.
	if (!continuePlaying && !(_PlayingDirectionFor(newState) == 0))
		newState->current_frame = _PlayingStartFrameFor(newState);
	_PushState(newState, continuePlaying);
	NotifyLoopModeChanged(mode);
}


/*	Creates a new playing state that equals the previous one aside from
	its looping enabled flag which is set to /enabled/.
	The new state will be activated as soon as possible.
	If /continuePlaying/ is true and the new state is a `not stopped'
	state, playing continues at the frame the last state reaches when the
	new state becomes active (or the next frame in the playing range).
	If /continuePlaying/ is false, playing starts at the beginning of the
	playing range (taking the playing direction into consideration). */
void
PlaybackManager::SetLoopingEnabled(bool enabled, bool continuePlaying)
{
	PlayingState* newState = new PlayingState(*_LastState());
	newState->looping_enabled = enabled;
	// Jump to the playing start frame if we should not continue, where we
	// stop.
	if (!continuePlaying && !(_PlayingDirectionFor(newState) == 0))
		newState->current_frame = _PlayingStartFrameFor(newState);
	_PushState(newState, continuePlaying);
	NotifyLoopingEnabledChanged(enabled);
}


void
PlaybackManager::SetSpeed(float speed)
{
#if SUPPORT_SPEED_CHANGES
	SpeedInfo* lastSpeed = _LastSpeedInfo();
	if (speed != lastSpeed->set_speed) {
		SpeedInfo* info = new SpeedInfo(*lastSpeed);
		info->activation_frame = NextFrame();
		info->set_speed = speed;
		if (_PlayingDirectionFor(_StateAtFrame(info->activation_frame)) != 0)
			info->speed = info->set_speed;
		else
			info->speed = 1.0;
		_PushSpeedInfo(info);
	}
#endif
}


// #pragma mark -


/*!	Returns the first frame at which a new playing state could become active,
	that is the first frame for that neither the audio nor the video producer
	have fetched data.*/
int64
PlaybackManager::NextFrame() const
{
	if (fNoAudio)
		return FrameForTime(fCurrentVideoTime - 1) + 1;

	return FrameForTime(max((bigtime_t)fCurrentAudioTime,
		(bigtime_t)fCurrentVideoTime) - 1) + 1;
}


//! Returns the Playlist frame for NextFrame().
int64
PlaybackManager::NextPlaylistFrame() const
{
	return PlaylistFrameAtFrame(NextFrame());
}


int64
PlaybackManager::FirstPlaybackRangeFrame()
{
	PlayingState* state = _StateAtFrame(CurrentFrame());
	switch (state->loop_mode) {
		case LOOPING_RANGE:
			return state->start_frame;
		case LOOPING_SELECTION:
			// TODO: ...
			return 0;
		case LOOPING_VISIBLE:
			return state->first_visible_frame;

		case LOOPING_ALL:
		default:
			return 0;
	}
}


int64
PlaybackManager::LastPlaybackRangeFrame()
{
	PlayingState* state = _StateAtFrame(CurrentFrame());
	switch (state->loop_mode) {
		case LOOPING_RANGE:
			return state->end_frame;
		case LOOPING_SELECTION:
			// TODO: ...
			return state->frame_count - 1;
		case LOOPING_VISIBLE:
			return state->last_visible_frame;

		case LOOPING_ALL:
		default:
			return state->frame_count - 1;
	}
}


// #pragma mark -


int64
PlaybackManager::StartFrameAtFrame(int64 frame)
{
	return _StateAtFrame(frame)->start_frame;
}


int64
PlaybackManager::StartFrameAtTime(bigtime_t time)
{
	return _StateAtTime(time)->start_frame;
}


int64
PlaybackManager::EndFrameAtFrame(int64 frame)
{
	return _StateAtTime(frame)->end_frame;
}


int64
PlaybackManager::EndFrameAtTime(bigtime_t time)
{
	return _StateAtTime(time)->end_frame;
}


int64
PlaybackManager::FrameCountAtFrame(int64 frame)
{
	return _StateAtTime(frame)->frame_count;
}


int64
PlaybackManager::FrameCountAtTime(bigtime_t time)
{
	return _StateAtTime(time)->frame_count;
}


int32
PlaybackManager::PlayModeAtFrame(int64 frame)
{
	return _StateAtTime(frame)->play_mode;
}


int32
PlaybackManager::PlayModeAtTime(bigtime_t time)
{
	return _StateAtTime(time)->play_mode;
}


int32
PlaybackManager::LoopModeAtFrame(int64 frame)
{
	return _StateAtTime(frame)->loop_mode;
}


int32
PlaybackManager::LoopModeAtTime(bigtime_t time)
{
	return _StateAtTime(time)->loop_mode;
}


/*!	Returns which Playlist frame should be displayed at (performance) video
	frame /frame/. Additionally the playing direction (0, 1, -1) is returned,
	as well as if a new playing state becomes active with this frame.
	A new playing state is installed, if either some data directly concerning
	the playing (play mode, loop mode, playing ranges, selection...) or the
	Playlist has changed. */
int64
PlaybackManager::PlaylistFrameAtFrame(int64 frame, int32& playingDirection,
	bool& newState) const
{
//TRACE("PlaybackManager::PlaylistFrameAtFrame(%lld)\n", frame);
	PlayingState* state = _StateAtFrame(frame);
	newState = (state->activation_frame == frame);
	playingDirection = _PlayingDirectionFor(state);
//TRACE("playing state: activation frame: %lld, current frame: %lld, dir: %ld\n",
//state->activation_frame, state->current_frame, state->play_mode);
	// The first part of the index calculation is invariable for a state. We
	// could add it to the state data.
	int64 result = state->current_frame;
	if (playingDirection != 0) {
//		int64 index = _RangeFrameForFrame(state, state->current_frame)
		int64 index = state->range_index
			+ (frame - state->activation_frame) * playingDirection;
		result = _FrameForRangeFrame(state, index);
	}
//TRACE("PlaybackManager::PlaylistFrameAtFrame() done: %lld\n", result);
//printf("PlaybackManager::PlaylistFrameAtFrame(%lld): %lld, direction: %ld\n",
//	frame, result, playingDirection);
	return result;
}


int64
PlaybackManager::PlaylistFrameAtFrame(int64 frame, int32& playingDirection) const
{
	bool newState;
	return PlaylistFrameAtFrame(frame, playingDirection, newState);
}


int64
PlaybackManager::PlaylistFrameAtFrame(int64 frame) const
{
	bool newState;
	int32 playingDirection;
	return PlaylistFrameAtFrame(frame, playingDirection, newState);
}


/*!	Returns the index of the next frame after /startFrame/ at which a
	playing state or speed change occurs or /endFrame/, if this happens to be
	earlier. */
int64
PlaybackManager::NextChangeFrame(int64 startFrame, int64 endFrame) const
{
	int32 startIndex = _IndexForFrame(startFrame);
	int32 endIndex = _IndexForFrame(endFrame);
	if (startIndex < endIndex)
		endFrame = _StateAt(startIndex + 1)->activation_frame;
#if SUPPORT_SPEED_CHANGES
	startIndex = _SpeedInfoIndexForFrame(startFrame);
	endIndex = _SpeedInfoIndexForFrame(endFrame);
	if (startIndex < endIndex)
		endFrame = _SpeedInfoAt(startIndex + 1)->activation_frame;
#endif
	return endFrame;
}


/*!	Returns the next time after /startTime/ at which a playing state or
	speed change occurs or /endTime/, if this happens to be earlier. */
bigtime_t
PlaybackManager::NextChangeTime(bigtime_t startTime, bigtime_t endTime) const
{
	int32 startIndex = _IndexForTime(startTime);
	int32 endIndex = _IndexForTime(endTime);
	if (startIndex < endIndex)
		endTime = TimeForFrame(_StateAt(startIndex + 1)->activation_frame);
#if SUPPORT_SPEED_CHANGES
	startIndex = _SpeedInfoIndexForTime(startTime);
	endIndex = _SpeedInfoIndexForTime(endTime);
	if (startIndex < endIndex)
		endTime = TimeForFrame(_SpeedInfoAt(startIndex + 1)->activation_frame);
#endif
	return endTime;
}


/*!	Returns a contiguous Playlist frame interval for a given frame interval.
	The returned interval may be smaller than the supplied one. Therefore
	the supplied /endFrame/ is adjusted.
	The value written to /xEndFrame/ is the first frame that doesn't belong
	to the interval. /playingDirection/ may either be -1 for backward,
	1 for forward or 0 for not playing. */
void
PlaybackManager::GetPlaylistFrameInterval(int64 startFrame, int64& endFrame,
	int64& xStartFrame, int64& xEndFrame, int32& playingDirection) const
{
	// limit the interval to one state and speed info
	endFrame = NextChangeFrame(startFrame, endFrame);
	// init return values
	xStartFrame = PlaylistFrameAtFrame(startFrame);
	xEndFrame = xStartFrame;
	playingDirection = _PlayingDirectionFor(_StateAtFrame(startFrame));
	// Step through the interval and check whether the respective Playlist
	// frames belong to the Playlist interval.
	bool endOfInterval = false;
	int64 intervalLength = 0;
	while (startFrame < endFrame && !endOfInterval) {
		intervalLength++;
		startFrame++;
		xEndFrame += playingDirection;
		endOfInterval = (PlaylistFrameAtFrame(startFrame) != xEndFrame);
	}
	// order the interval bounds, if playing backwards
	if (xStartFrame > xEndFrame) {
		swap(xStartFrame, xEndFrame);
		xStartFrame++;
		xEndFrame++;
	}
}


/*!	The same as GetPlaylistFrameInterval() just for time instead of frame
	intervals. Note, that /startTime/ and /endTime/ measure
	performance times whereas /xStartTime/ and /xEndTime/ specifies an
	Playlist time interval. In general it does not hold
	xEndTime - xStartTime == endTime - startTime, even if playing (think
	of a playing speed != 1.0). */
void
PlaybackManager::GetPlaylistTimeInterval(bigtime_t startTime,
	bigtime_t& endTime, bigtime_t& xStartTime, bigtime_t& xEndTime,
	float& playingSpeed) const
{
	// Get the frames that bound the given time interval. The end frame might
	// be greater than necessary, but that doesn't harm.
	int64 startFrame = FrameForTime(startTime);
	int64 endFrame = FrameForTime(endTime) + 1;
#if SUPPORT_SPEED_CHANGES
	SpeedInfo* info = _SpeedInfoForFrame(startFrame)->speed;
#endif
	// Get the Playlist frame interval that belongs to the frame interval.
	int64 xStartFrame;
	int64 xEndFrame;
	int32 playingDirection;
	GetPlaylistFrameInterval(startFrame, endFrame, xStartFrame, xEndFrame,
		playingDirection);
	// Calculate the performance time interval end/length.
	bigtime_t endTimeForFrame = TimeForFrame(endFrame);
	endTime = min(endTime, endTimeForFrame);
	bigtime_t intervalLength = endTime - startTime;

	// Finally determine the time bounds for the Playlist interval (depending
	// on the playing direction).
	switch (playingDirection) {
		// forward
		case 1:
		{
#if SUPPORT_SPEED_CHANGES
// TODO: The current method does not handle the times the same way.
//       It may happen, that for the same performance time different
//       Playlist times (within a frame) are returned when passing it
//       one time as a start time and another time as an end time.
			xStartTime = PlaylistTimeForFrame(xStartFrame)
				+ bigtime_t(double(startTime - TimeForFrame(startFrame))
					* info->speed);
			xEndTime = xStartTime
				+ bigtime_t((double)intervalLength * info->speed);
#else
			xStartTime = PlaylistTimeForFrame(xStartFrame)
				+ startTime - TimeForFrame(startFrame);
			xEndTime = xStartTime + intervalLength;
#endif
			break;
		}
		// backward
		case -1:
		{
#if SUPPORT_SPEED_CHANGES
			xEndTime = PlaylistTimeForFrame(xEndFrame)
				- bigtime_t(double(startTime - TimeForFrame(startFrame))
					* info->speed);
			xStartTime = xEndTime
				- bigtime_t((double)intervalLength * info->speed);
#else
			xEndTime = PlaylistTimeForFrame(xEndFrame)
				- startTime + TimeForFrame(startFrame);
			xStartTime = xEndTime - intervalLength;
#endif
			break;
		}
		// not playing
		case 0:
		default:
			xStartTime = PlaylistTimeForFrame(xStartFrame);
			xEndTime = xStartTime;
			break;
	}
#if SUPPORT_SPEED_CHANGES
	playingSpeed = (float)playingDirection * info->speed;
#else
	playingSpeed = (float)playingDirection;
#endif
}


/*!	Returns the frame that is being performed at the supplied time.
	It holds TimeForFrame(frame) <= time < TimeForFrame(frame + 1). */
int64
PlaybackManager::FrameForTime(bigtime_t time) const
{
//TRACE("PlaybackManager::FrameForTime(%lld)\n", time);
	// In order to avoid problems caused by rounding errors, we check
	// if for the resulting frame holds
	// TimeForFrame(frame) <= time < TimeForFrame(frame + 1).
#if SUPPORT_SPEED_CHANGES
	SpeedInfo* info = _SpeedInfoForTime(time);
if (!info) {
	fprintf(stderr, "PlaybackManager::FrameForTime() - no SpeedInfo!\n");
	return 0;
}
	int64 frame = (int64)(((double)time - info->activation_time)
		* (double)fFrameRate * info->speed / 1000000.0)
		+ info->activation_frame;

#else
	int64 frame = (int64)((double)time * (double)fFrameRate / 1000000.0);
#endif
	if (TimeForFrame(frame) > time)
		frame--;
	else if (TimeForFrame(frame + 1) <= time)
		frame++;
//TRACE("PlaybackManager::FrameForTime() done: %lld\n", frame);
//printf("PlaybackManager::FrameForTime(%lld): %lld\n", time, frame);
	return frame;
}


/*!	Returns the time at which the supplied frame should be performed (started
	to be performed). */
bigtime_t
PlaybackManager::TimeForFrame(int64 frame) const
{
#if SUPPORT_SPEED_CHANGES
	SpeedInfo* info = _SpeedInfoForFrame(frame);
if (!info) {
	fprintf(stderr, "PlaybackManager::TimeForFrame() - no SpeedInfo!\n");
	return 0;
}
	return (bigtime_t)((double)(frame - info->activation_frame) * 1000000.0
					   / ((double)fFrameRate * info->speed))
 		   + info->activation_time;
#else
	return (bigtime_t)((double)frame * 1000000.0 / (double)fFrameRate);
#endif
}


/*!	Returns the Playlist frame for an Playlist time.
	It holds PlaylistTimeForFrame(frame) <= time <
	PlaylistTimeForFrame(frame + 1). */
int64
PlaybackManager::PlaylistFrameForTime(bigtime_t time) const
{
	// In order to avoid problems caused by rounding errors, we check
	// if for the resulting frame holds
	// PlaylistTimeForFrame(frame) <= time < PlaylistTimeForFrame(frame + 1).
	int64 frame = (int64)((double)time * (double)fFrameRate / 1000000.0);
	if (TimeForFrame(frame) > time)
		frame--;
	else if (TimeForFrame(frame + 1) <= time)
		frame++;
	return frame;
}


//! Returns the Playlist start time for an Playlist frame.
bigtime_t
PlaybackManager::PlaylistTimeForFrame(int64 frame) const
{
	return (bigtime_t)((double)frame * 1000000.0 / (double)fFrameRate);
}


//! To be called when done with all activities concerning audio before /time/.
void
PlaybackManager::SetCurrentAudioTime(bigtime_t time)
{
	TRACE("PlaybackManager::SetCurrentAudioTime(%lld)\n", time);
	bigtime_t lastFrameTime = _TimeForLastFrame();
	fCurrentAudioTime = time;
	bigtime_t newLastFrameTime = _TimeForLastFrame();
	if (lastFrameTime != newLastFrameTime) {
		if (fPerformanceTimeEvent == NULL) {
			bigtime_t eventTime = RealTimeForTime(newLastFrameTime);
			fPerformanceTimeEvent = new MessageEvent(eventTime, this);
			EventQueue::Default().AddEvent(fPerformanceTimeEvent);
		}
		_CheckStopPlaying();
	}
}


//! To be called when done with all activities concerning video before /frame/.
void
PlaybackManager::SetCurrentVideoFrame(int64 frame)
{
	SetCurrentVideoTime(TimeForFrame(frame));
}


//! To be called when done with all activities concerning video before /time/.
void
PlaybackManager::SetCurrentVideoTime(bigtime_t time)
{
	TRACE("PlaybackManager::SetCurrentVideoTime(%lld)\n", time);
	bigtime_t lastFrameTime = _TimeForLastFrame();
	fCurrentVideoTime = time;
	bigtime_t newLastFrameTime = _TimeForLastFrame();
	if (lastFrameTime != newLastFrameTime) {
		if (fPerformanceTimeEvent == NULL) {
			bigtime_t eventTime = RealTimeForTime(newLastFrameTime);
			fPerformanceTimeEvent = new MessageEvent(eventTime, this);
			EventQueue::Default().AddEvent(fPerformanceTimeEvent);
		}
		_CheckStopPlaying();
	}
}


//! To be called as soon as video frame /frame/ is being performed.
void
PlaybackManager::SetPerformanceFrame(int64 frame)
{
	SetPerformanceTime(TimeForFrame(frame));
}


/*!	Similar to SetPerformanceFrame() just with a time instead of a frame
	argument. */
void
PlaybackManager::SetPerformanceTime(bigtime_t time)
{
	int64 oldCurrentFrame = CurrentFrame();
	fPerformanceTime = time;
	_UpdateStates();
	_UpdateSpeedInfos();
	int64 currentFrame = CurrentFrame();

//printf("PlaybackManager::SetPerformanceTime(%lld): %ld -> %ld\n",
//	time, oldCurrentFrame, currentFrame);

	if (currentFrame != oldCurrentFrame)
		NotifyCurrentFrameChanged(currentFrame);
}


// #pragma mark - Listeners


void
PlaybackManager::AddListener(PlaybackListener* listener)
{
	if (!listener)
		return;

	if (!Lock())
		return;

	if (!fListeners.HasItem(listener) && fListeners.AddItem(listener)) {
		// bring listener up2date, if we have been initialized
		if (_LastState()) {
			listener->PlayModeChanged(PlayMode());
			listener->LoopModeChanged(LoopMode());
			listener->LoopingEnabledChanged(IsLoopingEnabled());
			listener->VideoBoundsChanged(VideoBounds());
			listener->FramesPerSecondChanged(FramesPerSecond());
			listener->SpeedChanged(Speed());
			listener->CurrentFrameChanged(CurrentFrame());
		}
	}

	Unlock();
}


void
PlaybackManager::RemoveListener(PlaybackListener* listener)
{
	if (listener && Lock()) {
		fListeners.RemoveItem(listener);
		Unlock();
	}
}


void
PlaybackManager::NotifyPlayModeChanged(int32 mode) const
{
	for (int32 i = 0;
		 PlaybackListener* listener = (PlaybackListener*)fListeners.ItemAt(i);
		 i++) {
		listener->PlayModeChanged(mode);
	}
}


void
PlaybackManager::NotifyLoopModeChanged(int32 mode) const
{
	for (int32 i = 0;
		 PlaybackListener* listener = (PlaybackListener*)fListeners.ItemAt(i);
		 i++) {
		listener->LoopModeChanged(mode);
	}
}


void
PlaybackManager::NotifyLoopingEnabledChanged(bool enabled) const
{
	for (int32 i = 0;
		 PlaybackListener* listener = (PlaybackListener*)fListeners.ItemAt(i);
		 i++) {
		listener->LoopingEnabledChanged(enabled);
	}
}


void
PlaybackManager::NotifyVideoBoundsChanged(BRect bounds) const
{
	for (int32 i = 0;
		 PlaybackListener* listener = (PlaybackListener*)fListeners.ItemAt(i);
		 i++) {
		listener->VideoBoundsChanged(bounds);
	}
}


void
PlaybackManager::NotifyFPSChanged(float fps) const
{
	for (int32 i = 0;
		 PlaybackListener* listener = (PlaybackListener*)fListeners.ItemAt(i);
		 i++) {
		listener->FramesPerSecondChanged(fps);
	}
}


void
PlaybackManager::NotifyCurrentFrameChanged(int64 frame) const
{
	for (int32 i = 0;
		 PlaybackListener* listener = (PlaybackListener*)fListeners.ItemAt(i);
		 i++) {
		listener->CurrentFrameChanged(frame);
	}
}


void
PlaybackManager::NotifySpeedChanged(float speed) const
{
	for (int32 i = 0;
		 PlaybackListener* listener = (PlaybackListener*)fListeners.ItemAt(i);
		 i++) {
		listener->SpeedChanged(speed);
	}
}


void
PlaybackManager::NotifyFrameDropped() const
{
	for (int32 i = 0;
		 PlaybackListener* listener = (PlaybackListener*)fListeners.ItemAt(i);
		 i++) {
		listener->FrameDropped();
	}
}


void
PlaybackManager::NotifyStopFrameReached() const
{
	// not currently implemented in PlaybackListener interface
}


void
PlaybackManager::NotifySeekHandled(int64 frame) const
{
	// not currently implemented in PlaybackListener interface
}


void
PlaybackManager::PrintState(PlayingState* state)
{
	TRACE("state: activation frame: %lld, current frame: %lld, "
		   "start frame: %lld, end frame: %lld, frame count: %lld, "
		   "first visible: %lld, last visible: %lld, selection (...), "
		   "play mode: %ld, loop mode: %ld\n",
			state->activation_frame,
			state->current_frame,
			state->start_frame,
			state->end_frame,
			state->frame_count,
			state->first_visible_frame,
			state->last_visible_frame,
//			state->selection,
			state->play_mode,
			state->loop_mode);
}


void
PlaybackManager::PrintStateAtFrame(int64 frame)
{
	TRACE("frame %lld: ", frame);
	PrintState(_StateAtFrame(frame));
}


// #pragma mark -


/*!	Appends the supplied state to the list of states. If the state would
	become active at the same time as _LastState() the latter is removed
	and deleted. However, the activation time for the new state is adjusted,
	so that it is >= that of the last state and >= the current audio and
	video time. If the additional parameter /adjustCurrentFrame/ is true,
	the new state's current frame is tried to be set to the frame that is
	reached at the time the state will become active. In every case
	it is ensured that the current frame lies within the playing range
	(if playing). */
void
PlaybackManager::_PushState(PlayingState* state, bool adjustCurrentFrame)
{
//	if (!_LastState())
//		debugger("PlaybackManager::_PushState() used before Init()\n");

TRACE("PlaybackManager::_PushState()\n");
	if (state == NULL)
		return;

	// unset fStopPlayingFrame
	int64 oldStopPlayingFrame = fStopPlayingFrame;
	fStopPlayingFrame = -1;
	// get last state
	PlayingState* lastState = _LastState();
	int64 activationFrame = max(max(state->activation_frame,
									lastState->activation_frame),
								NextFrame());
	int64 currentFrame = 0;
	// remember the current frame, if necessary
	if (adjustCurrentFrame) {
		currentFrame = PlaylistFrameAtFrame(activationFrame);
		if (currentFrame == CurrentFrame()) {
			// Seems to be paused, force using the next frame
			currentFrame++;
		}
	}
	// Check whether the last state has already become active
	// (NOTE: We may want to keep the last state, if it is not active,
	//  but then the new state should become active after the last one.
	//  Thus we had to replace `NextFrame()' with `activationFrame'.)
TRACE("  state activation frame: %lld, last state activation frame: %lld, "
"NextFrame(): %lld, currentFrame: %lld, next currentFrame: %lld\n",
state->activation_frame, lastState->activation_frame, NextFrame(),
CurrentFrame(), currentFrame);
	if (lastState->activation_frame >= NextFrame()) {
		// it isn't -- remove it
		fStates.RemoveItem(fStates.CountItems() - 1);
TRACE("deleting last \n");
PrintState(lastState);
		_NotifySeekHandledIfNecessary(lastState);
		delete lastState;
	} else {
		// it is -- keep it
	}
	// adjust the new state's current frame and activation frame
	if (adjustCurrentFrame)
		state->current_frame = currentFrame;
	int32 playingDirection = _PlayingDirectionFor(state);
	if (playingDirection != 0) {
		state->current_frame
			= _NextFrameInRange(state, state->current_frame);
	} else {
		// If not playing, we check at least, if the current frame lies
		// within the interval [0, max_frame_count).
		if (state->current_frame >= state->max_frame_count)
			state->current_frame = state->max_frame_count - 1;
		if (state->current_frame < 0)
			state->current_frame = 0;
	}
	state->range_index = _RangeFrameForFrame(state, state->current_frame);
	state->activation_frame = activationFrame;
	fStates.AddItem(state);
PrintState(state);
TRACE("_PushState: state count: %ld\n", fStates.CountItems());
#if SUPPORT_SPEED_CHANGES
	// push a new speed info
	SpeedInfo* speedInfo = new SpeedInfo(*_LastSpeedInfo());
	if (playingDirection == 0)
		speedInfo->speed = 1.0;
	else
		speedInfo->speed = speedInfo->set_speed;
	speedInfo->activation_frame = state->activation_frame;
	_PushSpeedInfo(speedInfo);
#endif
	// If the new state is a playing state and looping is turned off,
	// determine when playing shall stop.
	if (playingDirection != 0 && !state->looping_enabled) {
		int64 startFrame, endFrame, frameCount;
		_GetPlayingBoundsFor(state, startFrame, endFrame, frameCount);
		if (playingDirection == -1)
			swap(startFrame, endFrame);
		// If we shall stop at the frame we start, set the current frame
		// to the beginning of the range.
		// We have to take care, since this state may equal the one
		// before (or probably differs in just one (unimportant)
		// parameter). This happens for instance, if the user changes the
		// data or start/end frame... while playing. In this case setting
		// the current frame to the start frame is unwanted. Therefore
		// we check whether the previous state was intended to stop
		// at the activation frame of this state.
		if (oldStopPlayingFrame != state->activation_frame
			&& state->current_frame == endFrame && frameCount > 1) {
			state->current_frame = startFrame;
			state->range_index
				= _RangeFrameForFrame(state, state->current_frame);
		}
		if (playingDirection == 1) {	// forward
			fStopPlayingFrame = state->activation_frame
								+ frameCount - state->range_index - 1;
		} else {						// backwards
			fStopPlayingFrame = state->activation_frame
								+ state->range_index;
		}
		_CheckStopPlaying();
	}
TRACE("PlaybackManager::_PushState() done\n");
}


/*!	Removes and deletes all states that are obsolete, that is which stopped
	being active at or before the current audio and video time. */
void
PlaybackManager::_UpdateStates()
{
//	int32 firstActive = min(_IndexForTime(fCurrentAudioTime),
//							_IndexForTime(fCurrentVideoTime));
	// Performance time should always be the least one.
	int32 firstActive = _IndexForTime(fPerformanceTime);
//TRACE("firstActive: %ld, numStates: %ld\n", firstActive, fStates.CountItems());
	for (int32 i = 0; i < firstActive; i++) {
		PlayingState* state = _StateAt(i);
		_NotifySeekHandledIfNecessary(state);
		delete state;
	}
	if (firstActive > 0)
{
		fStates.RemoveItems(0, firstActive);
TRACE("_UpdateStates: states removed: %ld, state count: %ld\n",
firstActive, fStates.CountItems());
}
	_NotifySeekHandledIfNecessary(_StateAt(0));
}


/*!	Returns the index of the state, that is active at frame /frame/.
	The index of the first frame (0) is returned, if /frame/ is even less
	than the frame at which this state becomes active. */
int32
PlaybackManager::_IndexForFrame(int64 frame) const
{
	int32 index = 0;
	PlayingState* state;
	while (((state = _StateAt(index + 1))) && state->activation_frame <= frame)
		index++;
	return index;
}

/*!	Returns the index of the state, that is active at time /time/.
	The index of the first frame (0) is returned, if /time/ is even less
	than the time at which this state becomes active. */
int32
PlaybackManager::_IndexForTime(bigtime_t time) const
{
	return _IndexForFrame(FrameForTime(time));
}


//! Returns the state that reflects the most recent changes.
PlaybackManager::PlayingState*
PlaybackManager::_LastState() const
{
	return _StateAt(fStates.CountItems() - 1);
}


PlaybackManager::PlayingState*
PlaybackManager::_StateAt(int32 index) const
{
	return (PlayingState*)fStates.ItemAt(index);
}


PlaybackManager::PlayingState*
PlaybackManager::_StateAtFrame(int64 frame) const
{
	return _StateAt(_IndexForFrame(frame));
}


PlaybackManager::PlayingState*
PlaybackManager::_StateAtTime(bigtime_t time) const
{
	return _StateAt(_IndexForTime(time));
}


int32
PlaybackManager::_PlayingDirectionFor(int32 playingMode)
{
	int32 direction = 0;
	switch (playingMode) {
		case MODE_PLAYING_FORWARD:
			direction = 1;
			break;
		case MODE_PLAYING_BACKWARD:
			direction = -1;
			break;
		case MODE_PLAYING_PAUSED_FORWARD:
		case MODE_PLAYING_PAUSED_BACKWARD:
			break;
	}
	return direction;
}


int32
PlaybackManager::_PlayingDirectionFor(PlayingState* state)
{
	return _PlayingDirectionFor(state->play_mode);
}


/*!	Returns the Playlist frame range that bounds the playing range of a given
	state.
	\a startFrame is the lower and \a endFrame the upper bound of the range,
	and \a frameCount the number of frames in the range; it is guaranteed to
	be >= 1, even for an empty selection. */
void
PlaybackManager::_GetPlayingBoundsFor(PlayingState* state, int64& startFrame,
	int64& endFrame, int64& frameCount)
{
	switch (state->loop_mode) {
		case LOOPING_ALL:
			startFrame = 0;
			endFrame = max(startFrame, state->frame_count - 1);
			frameCount = endFrame - startFrame + 1;
			break;
		case LOOPING_RANGE:
			startFrame = state->start_frame;
			endFrame = state->end_frame;
			frameCount = endFrame - startFrame + 1;
			break;
//		case LOOPING_SELECTION:
//			if (!state->selection.IsEmpty()) {
//				startFrame = state->selection.FirstIndex();
//				endFrame = state->selection.LastIndex();
//				frameCount = state->selection.CountIndices();
//TRACE("  LOOPING_SELECTION: %lld - %lld (%lld)\n", startFrame, endFrame,
//frameCount);
//			} else {
//				startFrame = state->current_frame;
//				endFrame = state->current_frame;
//				frameCount = 1;
//			}
//			break;
		case LOOPING_VISIBLE:
			startFrame = state->first_visible_frame;
			endFrame = state->last_visible_frame;
			frameCount = endFrame - startFrame + 1;
			break;
	}
}


/*!	Returns the frame at which playing would start for a given playing
	state. If the playing mode for the supplied state specifies a stopped
	or undefined mode, the result is the state's current frame. */
int64
PlaybackManager::_PlayingStartFrameFor(PlayingState* state)
{
	int64 startFrame, endFrame, frameCount, frame = 0;
	_GetPlayingBoundsFor(state, startFrame, endFrame, frameCount);
	switch (_PlayingDirectionFor(state)) {
		case -1:
			frame = endFrame;
			break;
		case 1:
			frame = startFrame;
			break;
		default:
			frame = state->current_frame;
			break;
	}
	return frame;
}


/*!	Returns the frame at which playing would end for a given playing
	state. If the playing mode for the supplied state specifies a stopped
	or undefined mode, the result is the state's current frame. */
int64
PlaybackManager::_PlayingEndFrameFor(PlayingState* state)
{
	int64 startFrame, endFrame, frameCount, frame = 0;
	_GetPlayingBoundsFor(state, startFrame, endFrame, frameCount);
	switch (_PlayingDirectionFor(state)) {
		case -1:
			frame = startFrame;
			break;
		case 1:
			frame = endFrame;
			break;
		default:
			frame = state->current_frame;
			break;
	}
	return frame;
}


/*!	Returns the index that the supplied frame has within a playing range.
	If the state specifies a not-playing mode, 0 is returned. The supplied
	frame has to lie within the bounds of the playing range, but it doesn't
	need to be contained, e.g. if the range is not contiguous. In this case
	the index of the next frame within the range (in playing direction) is
	returned. */
int64
PlaybackManager::_RangeFrameForFrame(PlayingState* state, int64 frame)
{
TRACE("PlaybackManager::_RangeFrameForFrame(%lld)\n", frame);
	int64 startFrame, endFrame, frameCount;
	int64 index = 0;
	_GetPlayingBoundsFor(state, startFrame, endFrame, frameCount);
TRACE("  start frame: %lld, end frame: %lld, frame count: %lld\n",
startFrame, endFrame, frameCount);
	switch (state->loop_mode) {
		case LOOPING_ALL:
		case LOOPING_RANGE:
		case LOOPING_VISIBLE:
			index = frame - startFrame;
			break;
	}
TRACE("PlaybackManager::_RangeFrameForFrame() done: %lld\n", index);
	return index;
}


/*!	Returns the Playlist frame for a playing range index. /index/ doesn't need
	to be in the playing range -- it is mapped into. */
int64
PlaybackManager::_FrameForRangeFrame(PlayingState* state, int64 index)
{
TRACE("PlaybackManager::_FrameForRangeFrame(%lld)\n", index);
	int64 startFrame, endFrame, frameCount;
	_GetPlayingBoundsFor(state, startFrame, endFrame, frameCount);
TRACE("  frame range: %lld - %lld, count: %lld\n", startFrame, endFrame,
frameCount);
	// map the index into the index interval of the playing range
	if (frameCount > 1)
		index = (index % frameCount + frameCount) % frameCount;

	// get the frame for the index
	int64 frame = startFrame;
	switch (state->loop_mode) {
		case LOOPING_ALL:
		case LOOPING_RANGE:
		case LOOPING_VISIBLE:
			frame = startFrame + index;
			break;
	}
	TRACE("PlaybackManager::_FrameForRangeFrame() done: %" PRId64 "\n", frame);
	return frame;
}


/*!	Given an arbitrary Playlist frame this function returns the next frame within
	the playing range for the supplied playing state. */
int64
PlaybackManager::_NextFrameInRange(PlayingState* state, int64 frame)
{
	int64 newFrame = frame;
	int64 startFrame, endFrame, frameCount;
	_GetPlayingBoundsFor(state, startFrame, endFrame, frameCount);
	if (frame < startFrame || frame > endFrame)
		newFrame = _PlayingStartFrameFor(state);
	else {
		int64 index = _RangeFrameForFrame(state, frame);
		newFrame = _FrameForRangeFrame(state, index);
		if (newFrame > frame && _PlayingDirectionFor(state) == -1)
			newFrame = _FrameForRangeFrame(state, index - 1);
	}
	return newFrame;
}


void
PlaybackManager::_PushSpeedInfo(SpeedInfo* info)
{
#if SUPPORT_SPEED_CHANGES
	if (info == NULL)
		return;
	// check the last state
	if (SpeedInfo* lastSpeed = _LastSpeedInfo()) {
		// set the activation time
		info->activation_time = TimeForFrame(info->activation_frame);
		// Remove the last state, if it won't be activated.
		if (lastSpeed->activation_frame == info->activation_frame) {
//fprintf(stderr, "  replacing last speed info\n");
			fSpeeds.RemoveItem(lastSpeed);
			delete lastSpeed;
		}
	}
	fSpeeds.AddItem(info);
//fprintf(stderr, "  speed info pushed: activation frame: %lld, "
//"activation time: %lld, speed: %f\n", info->activation_frame,
//info->activation_time, info->speed);
	// ...
#endif
}


PlaybackManager::SpeedInfo*
PlaybackManager::_LastSpeedInfo() const
{
#if SUPPORT_SPEED_CHANGES
	return (SpeedInfo*)fSpeeds.ItemAt(fSpeeds.CountItems() - 1);
#else
	return NULL;
#endif
}


PlaybackManager::SpeedInfo*
PlaybackManager::_SpeedInfoAt(int32 index) const
{
#if SUPPORT_SPEED_CHANGES
	return (SpeedInfo*)fSpeeds.ItemAt(index);
#else
	return NULL;
#endif
}


int32
PlaybackManager::_SpeedInfoIndexForFrame(int64 frame) const
{
	int32 index = 0;
#if SUPPORT_SPEED_CHANGES
	SpeedInfo* info;
	while (((info = _SpeedInfoAt(index + 1)))
		   && info->activation_frame <= frame) {
		index++;
	}
//fprintf(stderr, "PlaybackManager::_SpeedInfoIndexForFrame(%lld): %ld\n",
//frame, index);
#endif
	return index;
}


int32
PlaybackManager::_SpeedInfoIndexForTime(bigtime_t time) const
{
	int32 index = 0;
#if SUPPORT_SPEED_CHANGES
	SpeedInfo* info;
	while (((info = _SpeedInfoAt(index + 1)))
		   && info->activation_time <= time) {
		index++;
	}
//fprintf(stderr, "PlaybackManager::_SpeedInfoIndexForTime(%lld): %ld\n",
//time, index);
#endif
	return index;
}


PlaybackManager::SpeedInfo*
PlaybackManager::_SpeedInfoForFrame(int64 frame) const
{
	return _SpeedInfoAt(_SpeedInfoIndexForFrame(frame));
}


PlaybackManager::SpeedInfo*
PlaybackManager::_SpeedInfoForTime(bigtime_t time) const
{
	return _SpeedInfoAt(_SpeedInfoIndexForTime(time));
}


void
PlaybackManager::_UpdateSpeedInfos()
{
#if SUPPORT_SPEED_CHANGES
	int32 firstActive = _SpeedInfoIndexForTime(fPerformanceTime);
	for (int32 i = 0; i < firstActive; i++)
		delete _SpeedInfoAt(i);
	if (firstActive > 0)
		fSpeeds.RemoveItems(0, firstActive);
//fprintf(stderr, "  speed infos 0 - %ld removed\n", firstActive);
#endif
}


/*!	Returns the end time for the last video frame that the video and the
	audio producer both have completely processed. */
bigtime_t
PlaybackManager::_TimeForLastFrame() const
{
	if (fNoAudio)
		return TimeForFrame(FrameForTime(fCurrentVideoTime));

	return TimeForFrame(FrameForTime(min((bigtime_t)fCurrentAudioTime,
										 (bigtime_t)fCurrentVideoTime)));
}


/*!	Returns the start time for the first video frame for which
	neither the video nor the audio producer have fetched data. */
bigtime_t
PlaybackManager::_TimeForNextFrame() const
{
	return TimeForFrame(NextFrame());
}


void
PlaybackManager::_CheckStopPlaying()
{
//printf("_CheckStopPlaying() - %lld, next: %lld\n", fStopPlayingFrame, NextFrame());
	if (fStopPlayingFrame > 0 && fStopPlayingFrame <= NextFrame()) {
		StopPlaying();
		NotifyStopFrameReached();
	}
}


void
PlaybackManager::_NotifySeekHandledIfNecessary(PlayingState* state)
{
	if (state->is_seek_request) {
		state->is_seek_request = false;
		NotifySeekHandled(state->current_frame);
	}
}
