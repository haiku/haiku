/*
 * Controller.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007-2008 Stephan Aßmus <superstippi@gmx.de> (MIT Ok)
 * Copyright (C) 2007 Fredrik Modéen <fredrik@modeen.se>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include "Controller.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <Bitmap.h>
#include <Debug.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <Path.h>
#include <Window.h> // for debugging only

#include "AutoDeleter.h"
#include "ControllerView.h"
#include "PlaybackState.h"
#include "Settings.h"
#include "VideoView.h"

// suppliers
#include "AudioTrackSupplier.h"
#include "MediaTrackAudioSupplier.h"
#include "MediaTrackVideoSupplier.h"
#include "ProxyAudioSupplier.h"
#include "ProxyVideoSupplier.h"
#include "VideoTrackSupplier.h"

using std::nothrow;


void 
HandleError(const char *text, status_t err)
{
	if (err != B_OK) { 
		printf("%s. error 0x%08x (%s)\n",text, (int)err, strerror(err));
		fflush(NULL);
		exit(1);
	}
}


// #pragma mark - Controller::Listener


Controller::Listener::Listener() {}
Controller::Listener::~Listener() {}
void Controller::Listener::FileFinished() {}
void Controller::Listener::FileChanged() {}
void Controller::Listener::VideoTrackChanged(int32) {}
void Controller::Listener::AudioTrackChanged(int32) {}
void Controller::Listener::VideoStatsChanged() {}
void Controller::Listener::AudioStatsChanged() {}
void Controller::Listener::PlaybackStateChanged(uint32) {}
void Controller::Listener::PositionChanged(float) {}
void Controller::Listener::VolumeChanged(float) {}
void Controller::Listener::MutedChanged(bool) {}


// #pragma mark - Controller


Controller::Controller()
	: NodeManager(),
	  fVideoView(NULL),
	  fVolume(1.0),
	  fActiveVolume(1.0),
		// TODO: Implement background volume for inactive players,
		// but use only if there are multiple players running at all!
	  fMuted(false),

	  fRef(),
	  fMediaFile(NULL),

	  fVideoSupplier(new ProxyVideoSupplier()),
	  fAudioSupplier(new ProxyAudioSupplier(this)),
	  fVideoTrackSupplier(NULL),
	  fAudioTrackSupplier(NULL),

	  fAudioTrackList(4),
	  fVideoTrackList(2),

	  fPosition(0),
	  fDuration(0),
	  fVideoFrameRate(25.0),
	  fSeekFrame(-1),
	  fLastSeekEventTime(LONGLONG_MIN),

	  fGlobalSettingsListener(this),

	  fListeners(4)
{
	Settings::Default()->AddListener(&fGlobalSettingsListener);
	_AdoptGlobalSettings();
}


Controller::~Controller()
{
	Settings::Default()->RemoveListener(&fGlobalSettingsListener);

	if (fMediaFile)
		fMediaFile->ReleaseAllTracks();
	delete fMediaFile;
}


// #pragma mark - NodeManager interface


void
Controller::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_OBJECT_CHANGED:
			// received from fGlobalSettingsListener
			// TODO: find out which object, if we ever watch more than
			// the global settings instance...
			_AdoptGlobalSettings();
			break;
		default:
			NodeManager::MessageReceived(message);
	}
}


int64
Controller::Duration()
{
	// This should really be total frames (video frames at that)
	// TODO: It is not so nice that the MediaPlayer still measures
	// in video frames if only playing audio. Here for example, it will
	// return a duration of 0 if the audio clip happens to be shorter than
	// one video frame at 25 fps.
	return (int64)((double)fDuration * fVideoFrameRate / 1000000.0);
}


VideoTarget*
Controller::CreateVideoTarget()
{
	return fVideoView;
}


VideoSupplier*
Controller::CreateVideoSupplier()
{
	return fVideoSupplier;
}


AudioSupplier*
Controller::CreateAudioSupplier()
{
	return fAudioSupplier;
}


// #pragma mark -


status_t
Controller::SetTo(const entry_ref &ref)
{
	BAutolock _(this);

	if (fRef == ref) {
		if (InitCheck() == B_OK) {
			SetPosition(0.0);
			StartPlaying();
		}
		return B_OK;
	}

	fRef = ref;

	fAudioSupplier->SetSupplier(NULL, fVideoFrameRate);
	fVideoSupplier->SetSupplier(NULL);

	fAudioTrackList.MakeEmpty();
	fVideoTrackList.MakeEmpty();
	
	ObjectDeleter<BMediaFile> oldMediaFileDeleter(fMediaFile);
		// BMediaFile destructor will call ReleaseAllTracks()
	fMediaFile = NULL;

	// Do not delete the supplier chain until after we called
	// NodeManager::Init() to setup a new media node chain
	ObjectDeleter<VideoTrackSupplier> videoSupplierDeleter(
		fVideoTrackSupplier);
	ObjectDeleter<AudioTrackSupplier> audioSupplierDeleter(
		fAudioTrackSupplier);

	fVideoTrackSupplier = NULL;
	fAudioTrackSupplier = NULL;

	fDuration = 0;
	fVideoFrameRate = 25.0;

	status_t err;
	
	BMediaFile* mf = new BMediaFile(&ref);
	ObjectDeleter<BMediaFile> mediaFileDeleter(mf);

	err = mf->InitCheck();
	if (err != B_OK) {
		printf("Controller::SetTo: initcheck failed\n");
		_NotifyFileChanged();
		return err;
	}
	
	int trackcount = mf->CountTracks();
	if (trackcount <= 0) {
		printf("Controller::SetTo: trackcount %d\n", trackcount);
		_NotifyFileChanged();
		return B_MEDIA_NO_HANDLER;
	}
	
	for (int i = 0; i < trackcount; i++) {
		BMediaTrack* t = mf->TrackAt(i);
		media_format f;
		err = t->EncodedFormat(&f);
		if (err != B_OK) {
			printf("Controller::SetTo: EncodedFormat failed for track index %d, error 0x%08lx (%s)\n",
				i, err, strerror(err));
			mf->ReleaseTrack(t);
			continue;
		}
		
		if (t->Duration() <= 0) {
			printf("Controller::SetTo: track index %d has no duration\n",i);
			mf->ReleaseTrack(t);
			continue;
		}
		
		if (f.IsAudio()) {
			if (!fAudioTrackList.AddItem(t))
				return B_NO_MEMORY;
		} else if (f.IsVideo()) {
			if (!fVideoTrackList.AddItem(t))
				return B_NO_MEMORY;
		} else {
			printf("Controller::SetTo: track index %d has unknown type\n", i);
			mf->ReleaseTrack(t);
		}
	}

	printf("Controller::SetTo: %d audio track, %d video track\n",
		AudioTrackCount(), VideoTrackCount());

	fMediaFile = mf;
	mediaFileDeleter.Detach();

	SelectAudioTrack(0);
	SelectVideoTrack(0);

	if (fAudioTrackSupplier == NULL && fVideoTrackSupplier == NULL) {
		printf("Controller::SetTo: no audio or video tracks found or "
			"no decoders\n");
		_NotifyFileChanged();
		delete fMediaFile;
		fMediaFile = NULL;
		return B_MEDIA_NO_HANDLER;
	}

	// prevent blocking the creation of new overlay buffers
	fVideoView->DisableOverlay();

	// get video properties (if there is video at all)
	bool useOverlays = fVideoView ? fVideoView->UseOverlays() : true;

	int width;
	int height;
	GetSize(&width, &height);
	color_space preferredVideoFormat = B_NO_COLOR_SPACE;
	if (fVideoTrackSupplier) {
		const media_format& format = fVideoTrackSupplier->Format();
		preferredVideoFormat = format.u.raw_video.display.format;
	}

	uint32 enabledNodes;
	if (!fVideoTrackSupplier)
		enabledNodes = AUDIO_ONLY;
	else if (!fAudioTrackSupplier)
		enabledNodes = VIDEO_ONLY;
	else
		enabledNodes = AUDIO_AND_VIDEO;

	if (InitCheck() != B_OK) {
		Init(BRect(0, 0, width - 1, height - 1), fVideoFrameRate,
			preferredVideoFormat, LOOPING_ALL, false, 1.0, enabledNodes,
			useOverlays);
	} else {
		FormatChanged(BRect(0, 0, width - 1, height - 1), fVideoFrameRate,
			preferredVideoFormat, enabledNodes, useOverlays);
	}

	_NotifyFileChanged();

	SetPosition(0.0);
	if (fAutoplay)
		StartPlaying(true);

	return B_OK;
}


void
Controller::GetSize(int *width, int *height)
{
	BAutolock _(this);

	if (fVideoTrackSupplier) {
		media_format format = fVideoTrackSupplier->Format();
		// TODO: take aspect ratio into account!
		*height = format.u.raw_video.display.line_count;
		*width = format.u.raw_video.display.line_width;
	} else {
		*height = 0;
		*width = 0;
	}
}


int
Controller::AudioTrackCount()
{
	BAutolock _(this);

	return fAudioTrackList.CountItems();
}


int
Controller::VideoTrackCount()
{
	BAutolock _(this);

	return fVideoTrackList.CountItems();
}


status_t
Controller::SelectAudioTrack(int n)
{	
	BAutolock _(this);

	BMediaTrack* track = (BMediaTrack *)fAudioTrackList.ItemAt(n);
	if (!track)
		return B_ERROR;

	ObjectDeleter<AudioTrackSupplier> deleter(fAudioTrackSupplier);
	fAudioTrackSupplier = new MediaTrackAudioSupplier(track, n);

	bigtime_t a = fAudioTrackSupplier->Duration();
	bigtime_t v = fVideoTrackSupplier ? fVideoTrackSupplier->Duration() : 0;;
	fDuration = max_c(a, v);
	DurationChanged();
	// TODO: notify duration changed!

	fAudioSupplier->SetSupplier(fAudioTrackSupplier, fVideoFrameRate);

	_NotifyAudioTrackChanged(n);
	return B_OK;
}


int
Controller::CurrentAudioTrack()
{
	BAutolock _(this);

	if (fAudioTrackSupplier == NULL)
		return -1;

	return fAudioTrackSupplier->TrackIndex();
}


status_t
Controller::SelectVideoTrack(int n)
{
	BAutolock _(this);

	BMediaTrack* track = (BMediaTrack *)fVideoTrackList.ItemAt(n);
	if (!track)
		return B_ERROR;

	status_t initStatus;
	ObjectDeleter<VideoTrackSupplier> deleter(fVideoTrackSupplier);
	fVideoTrackSupplier = new MediaTrackVideoSupplier(track, n, initStatus);
	if (initStatus < B_OK) {
		delete fVideoTrackSupplier;
		fVideoTrackSupplier = NULL;
		return initStatus;
	}

	bigtime_t a = fAudioTrackSupplier ? fAudioTrackSupplier->Duration() : 0;
	bigtime_t v = fVideoTrackSupplier->Duration();
	fDuration = max_c(a, v);
	fVideoFrameRate = fVideoTrackSupplier->Format().u.raw_video.field_rate;
	if (fVideoFrameRate <= 0.0) {
		printf("Controller::SelectVideoTrack(%d) - invalid video frame rate: %.1f\n", n,
			fVideoFrameRate);
		fVideoFrameRate = 25.0;
	}

	DurationChanged();
	// TODO: notify duration changed!

	fVideoSupplier->SetSupplier(fVideoTrackSupplier);

	_NotifyVideoTrackChanged(n);
	return B_OK;
}


int
Controller::CurrentVideoTrack()
{
	BAutolock _(this);

	if (fVideoTrackSupplier == NULL)
		return -1;

	return fVideoTrackSupplier->TrackIndex();
}


// #pragma mark -


void
Controller::Stop()
{
	printf("Controller::Stop\n");

	BAutolock _(this);

	StopPlaying();
	SetPosition(0.0);
}


void
Controller::Play()
{
	printf("Controller::Play\n");

	BAutolock _(this);
	
	StartPlaying();
}


void
Controller::Pause()
{
	printf("Controller::Pause\n");

	BAutolock _(this);

	PausePlaying();
}


void
Controller::TogglePlaying()
{
	printf("Controller::TogglePlaying\n");

	BAutolock _(this);

	if (InitCheck() == B_OK)
		NodeManager::TogglePlaying();
}


uint32
Controller::PlaybackState()
{
	BAutolock _(this);

	return _PlaybackState(PlaybackManager::PlayMode());
}


bigtime_t
Controller::TimeDuration()
{
	BAutolock _(this);

	return fDuration;
}


bigtime_t
Controller::TimePosition()
{
	BAutolock _(this);

	return fPosition;
}


void
Controller::SetVolume(float value)
{
	printf("Controller::SetVolume %.4f\n", value);
	if (!Lock())
		return;

	value = max_c(0.0, min_c(2.0, value));

	if (fVolume != value) {
		if (fMuted)
			ToggleMute();

		fVolume = value;
		fAudioSupplier->SetVolume(fVolume);

		_NotifyVolumeChanged(fVolume);
	}

	Unlock();
}

void
Controller::VolumeUp()
{
	// TODO: linear <-> exponential
	SetVolume(Volume() + 0.05);
}

void
Controller::VolumeDown()
{
	// TODO: linear <-> exponential
	SetVolume(Volume() - 0.05);
}

void
Controller::ToggleMute()
{	
	if (!Lock())
		return;

	fMuted = !fMuted;

	if (fMuted)
		fAudioSupplier->SetVolume(0.0);
	else
		fAudioSupplier->SetVolume(fVolume);

	_NotifyMutedChanged(fMuted);

	Unlock();
}


float
Controller::Volume()
{
	BAutolock _(this);

	return fVolume;	
}


void
Controller::SetPosition(float value)
{
	BAutolock _(this);

	fSeekFrame = (int32)(Duration() * value);
	int32 currentFrame = CurrentFrame();
	if (fSeekFrame != currentFrame) {
		SetCurrentFrame(fSeekFrame);
		fLastSeekEventTime = system_time();
	} else
		fSeekFrame = -1;
}


// #pragma mark -


bool
Controller::HasFile()
{
	// you need to hold the data lock
	return fMediaFile != NULL;
}


status_t
Controller::GetFileFormatInfo(media_file_format* fileFormat)
{
	// you need to hold the data lock
	if (!fMediaFile)
		return B_NO_INIT;
	return fMediaFile->GetFileFormatInfo(fileFormat);
}


status_t
Controller::GetCopyright(BString* copyright)
{
	// you need to hold the data lock
	if (!fMediaFile)
		return B_NO_INIT;
	*copyright = fMediaFile->Copyright();
	return B_OK;
}


status_t
Controller::GetLocation(BString* location)
{
	// you need to hold the data lock
	if (!fMediaFile)
		return B_NO_INIT;
	BPath path(&fRef);
	status_t ret = path.InitCheck();
	if (ret < B_OK)
		return ret;
	*location = "";
	*location << "file://" << path.Path();
	return B_OK;
}


status_t
Controller::GetName(BString* name)
{
	// you need to hold the data lock
	if (!fMediaFile)
		return B_NO_INIT;
	*name = fRef.name;
	return B_OK;
}


status_t
Controller::GetEncodedVideoFormat(media_format* format)
{
	// you need to hold the data lock
	if (fVideoTrackSupplier)
		return fVideoTrackSupplier->GetEncodedFormat(format);
	return B_NO_INIT;
}


status_t
Controller::GetVideoCodecInfo(media_codec_info* info)
{
	// you need to hold the data lock
	if (fVideoTrackSupplier)
		return fVideoTrackSupplier->GetCodecInfo(info);
	return B_NO_INIT;
}


status_t
Controller::GetEncodedAudioFormat(media_format* format)
{
	// you need to hold the data lock
	if (fAudioTrackSupplier)
		return fAudioTrackSupplier->GetEncodedFormat(format);
	return B_NO_INIT;
}


status_t
Controller::GetAudioCodecInfo(media_codec_info* info)
{
	// you need to hold the data lock
	if (fAudioTrackSupplier)
		return fAudioTrackSupplier->GetCodecInfo(info);
	return B_NO_INIT;
}


// #pragma mark -


void
Controller::SetVideoView(VideoView *view)
{
	BAutolock _(this);

	fVideoView = view;
}


bool
Controller::IsOverlayActive()
{
	if (fVideoView)
		return fVideoView->IsOverlayActive();

	return false;
}


// #pragma mark -


bool
Controller::AddListener(Listener* listener)
{
	BAutolock _(this);

	if (listener && !fListeners.HasItem(listener))
		return fListeners.AddItem(listener);
	return false;
}


void
Controller::RemoveListener(Listener* listener)
{
	BAutolock _(this);

	fListeners.RemoveItem(listener);
}


// #pragma mark - Private


void
Controller::_AdoptGlobalSettings()
{
	mpSettings settings = Settings::CurrentSettings();
		// thread safe

	fAutoplay = settings.autostart;
	// not yet used:
	fLoopMovies = settings.loopMovie;
	fLoopSounds = settings.loopSound;
	fBackgroundMovieVolumeMode = settings.backgroundMovieVolumeMode;
}


uint32
Controller::_PlaybackState(int32 playingMode) const
{
	uint32 state = 0;
	switch (playingMode) {
		case MODE_PLAYING_PAUSED_FORWARD:
		case MODE_PLAYING_PAUSED_BACKWARD:
			state = PLAYBACK_STATE_PAUSED;
			break;
		case MODE_PLAYING_FORWARD:
		case MODE_PLAYING_BACKWARD:
			state = PLAYBACK_STATE_PLAYING;
			break;

		default:
			state = PLAYBACK_STATE_STOPPED;
			break;
	}
	return state;
}


// #pragma mark - Notifications


void
Controller::_NotifyFileChanged() const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->FileChanged();
	}
}


void
Controller::_NotifyFileFinished() const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->FileFinished();
	}
}


void
Controller::_NotifyVideoTrackChanged(int32 index) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->VideoTrackChanged(index);
	}
}


void
Controller::_NotifyAudioTrackChanged(int32 index) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->AudioTrackChanged(index);
	}
}


void
Controller::_NotifyVideoStatsChanged() const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->VideoStatsChanged();
	}
}


void
Controller::_NotifyAudioStatsChanged() const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->AudioStatsChanged();
	}
}


void
Controller::_NotifyPlaybackStateChanged(uint32 state) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->PlaybackStateChanged(state);
	}
}


void
Controller::_NotifyPositionChanged(float position) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->PositionChanged(position);
	}
}


void
Controller::_NotifyVolumeChanged(float volume) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->VolumeChanged(volume);
	}
}


void
Controller::_NotifyMutedChanged(bool muted) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->MutedChanged(muted);
	}
}


void
Controller::NotifyPlayModeChanged(int32 mode) const
{
	uint32 state = _PlaybackState(mode);
	if (fVideoView)
		fVideoView->SetPlaying(state == PLAYBACK_STATE_PLAYING);
	_NotifyPlaybackStateChanged(state);
}


void
Controller::NotifyLoopModeChanged(int32 mode) const
{
}


void
Controller::NotifyLoopingEnabledChanged(bool enabled) const
{
}


void
Controller::NotifyVideoBoundsChanged(BRect bounds) const
{
}


void
Controller::NotifyFPSChanged(float fps) const
{
}


void
Controller::NotifyCurrentFrameChanged(int32 frame) const
{
	// check if we are still waiting to reach the seekframe,
	// don't pass the event on to the listeners in that case
	if ((system_time() - fLastSeekEventTime) < 1000000
		&& fSeekFrame >= 0 && frame != fSeekFrame) {
		return;
	}
	fSeekFrame = -1;

	float position = 0.0;
	double duration = (double)fDuration * fVideoFrameRate / 1000000.0;
	if (duration > 0)
		position = (float)frame / duration;
	fPosition = (bigtime_t)(position * fDuration + 0.5);
	_NotifyPositionChanged(position);
}


void
Controller::NotifySpeedChanged(float speed) const
{
}


void
Controller::NotifyFrameDropped() const
{
//	printf("Controller::NotifyFrameDropped()\n");
}


void
Controller::NotifyStopFrameReached() const
{
	// Currently, this means we reached the end of the current
	// file and should play the next file
	_NotifyFileFinished();
}

