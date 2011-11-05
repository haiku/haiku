/*
 * Controller.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007-2008 Stephan Aßmus <superstippi@gmx.de> (MIT Ok)
 * Copyright (C) 2007-2009 Fredrik Modéen <[FirstName]@[LastName].se> (MIT ok)
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
#include <Path.h>
#include <Window.h> // for debugging only

#include "AutoDeleter.h"
#include "ControllerView.h"
#include "MainApp.h"
#include "PlaybackState.h"
#include "Settings.h"
#include "VideoView.h"

// suppliers
#include "AudioTrackSupplier.h"
#include "MediaTrackAudioSupplier.h"
#include "MediaTrackVideoSupplier.h"
#include "ProxyAudioSupplier.h"
#include "ProxyVideoSupplier.h"
#include "SubTitles.h"
#include "TrackSupplier.h"
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
void Controller::Listener::FileChanged(PlaylistItem* item, status_t result) {}
void Controller::Listener::VideoTrackChanged(int32) {}
void Controller::Listener::AudioTrackChanged(int32) {}
void Controller::Listener::SubTitleTrackChanged(int32) {}
void Controller::Listener::VideoStatsChanged() {}
void Controller::Listener::AudioStatsChanged() {}
void Controller::Listener::PlaybackStateChanged(uint32) {}
void Controller::Listener::PositionChanged(float) {}
void Controller::Listener::SeekHandled(int64 seekFrame) {}
void Controller::Listener::VolumeChanged(float) {}
void Controller::Listener::MutedChanged(bool) {}


// #pragma mark - Controller


enum {
	MSG_SET_TO = 'stto'
};


Controller::Controller()
	:
	NodeManager(),
	fVideoView(NULL),
	fVolume(1.0),
	fActiveVolume(1.0),
	fMuted(false),

	fItem(NULL),
	fTrackSupplier(NULL),

	fVideoSupplier(new ProxyVideoSupplier()),
	fAudioSupplier(new ProxyAudioSupplier(this)),
	fVideoTrackSupplier(NULL),
	fAudioTrackSupplier(NULL),
	fSubTitles(NULL),
	fSubTitlesIndex(-1),

	fCurrentFrame(0),
	fDuration(0),
	fVideoFrameRate(25.0),

	fPendingSeekRequests(0),
	fSeekFrame(-1),
	fRequestedSeekFrame(-1),

	fGlobalSettingsListener(this),

	fListeners(4)
{
	Settings::Default()->AddListener(&fGlobalSettingsListener);
	_AdoptGlobalSettings();

	fAutoplay = fAutoplaySetting;
}


Controller::~Controller()
{
	Settings::Default()->RemoveListener(&fGlobalSettingsListener);
	SetTo(NULL);
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

		case MSG_SET_TO:
		{
			PlaylistItem* item;
			if (message->FindPointer("item", (void**)&item) == B_OK) {
				PlaylistItemRef itemRef(item, true);
					// The reference was passed with the message.
				SetTo(itemRef);
			} else
				_NotifyFileChanged(NULL, B_BAD_VALUE);

			break;
		}

		default:
			NodeManager::MessageReceived(message);
	}
}


int64
Controller::Duration()
{
	return _FrameDuration();
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
Controller::SetToAsync(const PlaylistItemRef& item)
{
	PlaylistItemRef additionalReference(item);

	BMessage message(MSG_SET_TO);
	status_t ret = message.AddPointer("item", item.Get());
	if (ret != B_OK)
		return ret;

	ret = PostMessage(&message);
	if (ret != B_OK)
		return ret;

	// The additional reference is now passed along with the message...
	additionalReference.Detach();

	return B_OK;
}


status_t
Controller::SetTo(const PlaylistItemRef& item)
{
	BAutolock _(this);

	if (fItem == item) {
		if (InitCheck() == B_OK) {
			if (fAutoplay) {
				SetPosition(0.0);
				StartPlaying(true);
			}
		}
		return B_OK;
	}

	fItem = item;

	fAudioSupplier->SetSupplier(NULL, fVideoFrameRate);
	fVideoSupplier->SetSupplier(NULL);

	ObjectDeleter<TrackSupplier> oldTrackSupplierDeleter(fTrackSupplier);
	fTrackSupplier = NULL;

	// Do not delete the supplier chain until after we called
	// NodeManager::Init() to setup a new media node chain
	ObjectDeleter<VideoTrackSupplier> videoSupplierDeleter(
		fVideoTrackSupplier);
	ObjectDeleter<AudioTrackSupplier> audioSupplierDeleter(
		fAudioTrackSupplier);

	fVideoTrackSupplier = NULL;
	fAudioTrackSupplier = NULL;
	fSubTitles = NULL;
	fSubTitlesIndex = -1;

	fCurrentFrame = 0;
	fDuration = 0;
	fVideoFrameRate = 25.0;

	fPendingSeekRequests = 0;
	fSeekFrame = -1;
	fRequestedSeekFrame = -1;

	if (fItem.Get() == NULL)
		return B_BAD_VALUE;

	TrackSupplier* trackSupplier = fItem->CreateTrackSupplier();
	if (trackSupplier == NULL) {
		_NotifyFileChanged(item.Get(), B_NO_MEMORY);
		return B_NO_MEMORY;
	}
	ObjectDeleter<TrackSupplier> trackSupplierDeleter(trackSupplier);

	status_t err = trackSupplier->InitCheck();
	if (err != B_OK) {
		printf("Controller::SetTo: InitCheck failed\n");
		_NotifyFileChanged(item.Get(), err);
		return err;
	}

	if (trackSupplier->CountAudioTracks() == 0
		&& trackSupplier->CountVideoTracks() == 0) {
		_NotifyFileChanged(item.Get(), B_MEDIA_NO_HANDLER);
		return B_MEDIA_NO_HANDLER;
	}

	fTrackSupplier = trackSupplier;

	SelectAudioTrack(0);
	SelectVideoTrack(0);

	if (fAudioTrackSupplier == NULL && fVideoTrackSupplier == NULL) {
		printf("Controller::SetTo: no audio or video tracks found or "
			"no decoders\n");
		fTrackSupplier = NULL;
		_NotifyFileChanged(item.Get(), B_MEDIA_NO_HANDLER);
		return B_MEDIA_NO_HANDLER;
	}

	trackSupplierDeleter.Detach();

	// prevent blocking the creation of new overlay buffers
	fVideoView->DisableOverlay();

	// get video properties (if there is video at all)
	bool useOverlays = fVideoView ? fVideoView->UseOverlays() : true;

	int width;
	int height;
	GetSize(&width, &height);
	color_space preferredVideoFormat = B_NO_COLOR_SPACE;
	if (fVideoTrackSupplier != NULL) {
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

	float audioFrameRate = 44100.0f;
	uint32 audioChannels = 2;
	if (fAudioTrackSupplier != NULL) {
		const media_format& audioTrackFormat = fAudioTrackSupplier->Format();
		audioFrameRate = audioTrackFormat.u.raw_audio.frame_rate;
		audioChannels = audioTrackFormat.u.raw_audio.channel_count;
	}

	if (InitCheck() != B_OK) {
		Init(BRect(0, 0, width - 1, height - 1), fVideoFrameRate,
			preferredVideoFormat, audioFrameRate, audioChannels, LOOPING_ALL,
			false, 1.0, enabledNodes, useOverlays);
	} else {
		FormatChanged(BRect(0, 0, width - 1, height - 1), fVideoFrameRate,
			preferredVideoFormat, audioFrameRate, audioChannels, enabledNodes,
			useOverlays);
	}

	_NotifyFileChanged(item.Get(), B_OK);

	if (fAutoplay)
		StartPlaying(true);

	return B_OK;
}


void
Controller::PlayerActivated(bool active)
{
	if (LockWithTimeout(5000) != B_OK)
		return;

	if (active && gMainApp->PlayerCount() > 1) {
		if (fActiveVolume != fVolume)
			SetVolume(fActiveVolume);
	} else {
		fActiveVolume = fVolume;
		if (gMainApp->PlayerCount() > 1)
			switch (fBackgroundMovieVolumeMode) {
				case mpSettings::BG_MOVIES_MUTED:
					SetVolume(0.0);
					break;
				case mpSettings::BG_MOVIES_HALF_VLUME:
					SetVolume(fVolume * 0.25);
					break;
				case mpSettings::BG_MOVIES_FULL_VOLUME:
				default:
					break;
			}
	}

	Unlock();
}


void
Controller::GetSize(int *width, int *height, int* _widthAspect,
	int* _heightAspect)
{
	BAutolock _(this);

	if (fVideoTrackSupplier) {
		media_format format = fVideoTrackSupplier->Format();
		*height = format.u.raw_video.display.line_count;
		*width = format.u.raw_video.display.line_width;
		int widthAspect = 0;
		int heightAspect = 0;
		// Ignore format aspect when both values are 1. If they have been
		// intentionally at 1:1 then no harm is done for quadratic videos,
		// only if the video is indeed encoded anamorphotic, but supposed
		// to be displayed quadratic... extremely unlikely.
		if (format.u.raw_video.pixel_width_aspect
			!= format.u.raw_video.pixel_height_aspect
			&& format.u.raw_video.pixel_width_aspect != 1) {
			widthAspect = format.u.raw_video.pixel_width_aspect;
			heightAspect = format.u.raw_video.pixel_height_aspect;
		}
		if (_widthAspect != NULL)
			*_widthAspect = widthAspect;
		if (_heightAspect != NULL)
			*_heightAspect = heightAspect;
	} else {
		*height = 0;
		*width = 0;
		if (_widthAspect != NULL)
			*_widthAspect = 1;
		if (_heightAspect != NULL)
			*_heightAspect = 1;
	}
}


int
Controller::AudioTrackCount()
{
	BAutolock _(this);

	if (fTrackSupplier != NULL)
		return fTrackSupplier->CountAudioTracks();
	return 0;
}


int
Controller::VideoTrackCount()
{
	BAutolock _(this);

	if (fTrackSupplier != NULL)
		return fTrackSupplier->CountVideoTracks();
	return 0;
}


int
Controller::SubTitleTrackCount()
{
	BAutolock _(this);

	if (fTrackSupplier != NULL)
		return fTrackSupplier->CountSubTitleTracks();
	return 0;
}


status_t
Controller::SelectAudioTrack(int n)
{
	BAutolock _(this);
	if (fTrackSupplier == NULL)
		return B_NO_INIT;

	ObjectDeleter<AudioTrackSupplier> deleter(fAudioTrackSupplier);
	fAudioTrackSupplier = fTrackSupplier->CreateAudioTrackForIndex(n);
	if (fAudioTrackSupplier == NULL)
		return B_BAD_INDEX;

	bigtime_t a = fAudioTrackSupplier->Duration();
	bigtime_t v = fVideoTrackSupplier != NULL
		? fVideoTrackSupplier->Duration() : 0;
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


int
Controller::AudioTrackChannelCount()
{
	media_format format;
	if (GetEncodedAudioFormat(&format) == B_OK)
		return format.u.encoded_audio.output.channel_count;

	return 2;
}


status_t
Controller::SelectVideoTrack(int n)
{
	BAutolock _(this);

	if (fTrackSupplier == NULL)
		return B_NO_INIT;

	ObjectDeleter<VideoTrackSupplier> deleter(fVideoTrackSupplier);
	fVideoTrackSupplier = fTrackSupplier->CreateVideoTrackForIndex(n);
	if (fVideoTrackSupplier == NULL)
		return B_BAD_INDEX;

	bigtime_t a = fAudioTrackSupplier ? fAudioTrackSupplier->Duration() : 0;
	bigtime_t v = fVideoTrackSupplier->Duration();
	fDuration = max_c(a, v);
	fVideoFrameRate = fVideoTrackSupplier->Format().u.raw_video.field_rate;
	if (fVideoFrameRate <= 0.0) {
		printf("Controller::SelectVideoTrack(%d) - invalid video frame rate: %.1f\n",
			n, fVideoFrameRate);
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


status_t
Controller::SelectSubTitleTrack(int n)
{
	BAutolock _(this);

	if (fTrackSupplier == NULL)
		return B_NO_INIT;

	fSubTitlesIndex = n;
	fSubTitles = fTrackSupplier->SubTitleTrackForIndex(n);

	const SubTitle* subTitle = NULL;
	if (fSubTitles != NULL)
		subTitle = fSubTitles->SubTitleAt(_TimePosition());
	if (subTitle != NULL)
		fVideoView->SetSubTitle(subTitle->text.String());
	else
		fVideoView->SetSubTitle(NULL);

	_NotifySubTitleTrackChanged(n);
	return B_OK;
}


int
Controller::CurrentSubTitleTrack()
{
	BAutolock _(this);

	if (fSubTitles == NULL)
		return -1;

	return fSubTitlesIndex;
}


const char*
Controller::SubTitleTrackName(int n)
{
	BAutolock _(this);

	if (fTrackSupplier == NULL)
		return NULL;

	const SubTitles* subTitles = fTrackSupplier->SubTitleTrackForIndex(n);
	if (subTitles == NULL)
		return NULL;

	return subTitles->Name();
}


// #pragma mark -


void
Controller::Stop()
{
	//printf("Controller::Stop\n");

	BAutolock _(this);

	StopPlaying();
	SetPosition(0.0);

	fAutoplay = fAutoplaySetting;
}


void
Controller::Play()
{
	//printf("Controller::Play\n");

	BAutolock _(this);

	StartPlaying();
	fAutoplay = true;
}


void
Controller::Pause()
{
//	printf("Controller::Pause\n");

	BAutolock _(this);

	PausePlaying();

	fAutoplay = fAutoplaySetting;
}


void
Controller::TogglePlaying()
{
//	printf("Controller::TogglePlaying\n");

	BAutolock _(this);

	if (InitCheck() == B_OK) {
		NodeManager::TogglePlaying();

		fAutoplay = IsPlaying() || fAutoplaySetting;
	}
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

	return _TimePosition();
}


void
Controller::SetVolume(float value)
{
//	printf("Controller::SetVolume %.4f\n", value);
	BAutolock _(this);

	value = max_c(0.0, min_c(2.0, value));

	if (fVolume != value) {
		if (fMuted)
			ToggleMute();

		fVolume = value;
		fAudioSupplier->SetVolume(fVolume);

		_NotifyVolumeChanged(fVolume);
	}
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
	BAutolock _(this);

	fMuted = !fMuted;

	if (fMuted)
		fAudioSupplier->SetVolume(0.0);
	else
		fAudioSupplier->SetVolume(fVolume);

	_NotifyMutedChanged(fMuted);
}


float
Controller::Volume()
{
	BAutolock _(this);

	return fVolume;
}


int64
Controller::SetPosition(float value)
{
	BAutolock _(this);

	return SetFramePosition(_FrameDuration() * value);
}


int64
Controller::SetFramePosition(int64 value)
{
	BAutolock _(this);

	fPendingSeekRequests++;
	fRequestedSeekFrame = max_c(0, min_c(_FrameDuration(), value));
	fSeekFrame = fRequestedSeekFrame;

	int64 currentFrame = CurrentFrame();

	// Snap to a video keyframe, since that will be the fastest
	// to display and seeking will feel more snappy. Note that we
	// don't store this change in fSeekFrame, since we still want
	// to report the originally requested seek frame in TimePosition()
	// until we could reach that frame.
	if (Duration() > 240 && fVideoTrackSupplier != NULL
		&& abs(value - currentFrame) > 5) {
		fVideoTrackSupplier->FindKeyFrameForFrame(&fSeekFrame);
	}

//printf("SetFramePosition(%lld) -> %lld (current: %lld, duration: %lld) "
//"(video: %p)\n", value, fSeekFrame, currentFrame, _FrameDuration(),
//fVideoTrackSupplier);
	if (fSeekFrame != currentFrame) {
		int64 seekFrame = fSeekFrame;
		SetCurrentFrame(fSeekFrame);
			// May trigger the notification and reset fSeekFrame,
			// if next current frame == seek frame.
		return seekFrame;
	} else
		NotifySeekHandled(fRequestedSeekFrame);
	return currentFrame;
}


int64
Controller::SetTimePosition(bigtime_t value)
{
	BAutolock _(this);

	return SetPosition((float)value / TimeDuration());
}


// #pragma mark -


bool
Controller::HasFile()
{
	// you need to hold the data lock
	return fTrackSupplier != NULL;
}


status_t
Controller::GetFileFormatInfo(media_file_format* fileFormat)
{
	// you need to hold the data lock
	if (!fTrackSupplier)
		return B_NO_INIT;
	return fTrackSupplier->GetFileFormatInfo(fileFormat);
}


status_t
Controller::GetCopyright(BString* copyright)
{
	// you need to hold the data lock
	if (!fTrackSupplier)
		return B_NO_INIT;
	return fTrackSupplier->GetCopyright(copyright);
}


status_t
Controller::GetLocation(BString* location)
{
	// you need to hold the data lock
	if (fItem.Get() == NULL)
		return B_NO_INIT;
	*location = fItem->LocationURI();
	return B_OK;
}


status_t
Controller::GetName(BString* name)
{
	// you need to hold the data lock
	if (fItem.Get() == NULL)
		return B_NO_INIT;
	*name = fItem->Name();
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


status_t
Controller::GetMetaData(BMessage* metaData)
{
	// you need to hold the data lock
	if (fTrackSupplier == NULL)
		return B_NO_INIT;
	return fTrackSupplier->GetMetaData(metaData);
}


status_t
Controller::GetVideoMetaData(int32 index, BMessage* metaData)
{
	// you need to hold the data lock
	if (fTrackSupplier == NULL)
		return B_NO_INIT;
	return fTrackSupplier->GetVideoMetaData(index, metaData);
}


status_t
Controller::GetAudioMetaData(int32 index, BMessage* metaData)
{
	// you need to hold the data lock
	if (fTrackSupplier == NULL)
		return B_NO_INIT;
	return fTrackSupplier->GetAudioMetaData(index, metaData);
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
	mpSettings settings;
	Settings::Default()->Get(settings);

	fAutoplaySetting = settings.autostart;
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


bigtime_t
Controller::_TimePosition() const
{
	if (fDuration == 0)
		return 0;

	// Check if we are still waiting to reach the seekframe,
	// pass the last pending seek frame back to the caller, so
	// that the view of the current frame/time from the outside
	// does not depend on the internal latency to reach requested
	// frames asynchronously.
	int64 frame;
	if (fPendingSeekRequests > 0)
		frame = fRequestedSeekFrame;
	else
		frame = fCurrentFrame;

	return frame * fDuration / _FrameDuration();
}


int64
Controller::_FrameDuration() const
{
	// This should really be total frames (video frames at that)
	// TODO: It is not so nice that the MediaPlayer still measures
	// in video frames if only playing audio. Here for example, it will
	// return a duration of 0 if the audio clip happens to be shorter than
	// one video frame at 25 fps.
	return (int64)((double)fDuration * fVideoFrameRate / 1000000.0);
}


// #pragma mark - Notifications


void
Controller::_NotifyFileChanged(PlaylistItem* item, status_t result) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->FileChanged(item, result);
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
Controller::_NotifySubTitleTrackChanged(int32 index) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->SubTitleTrackChanged(index);
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
Controller::_NotifySeekHandled(int64 seekFrame) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->SeekHandled(seekFrame);
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
Controller::NotifyCurrentFrameChanged(int64 frame) const
{
	fCurrentFrame = frame;
	bigtime_t timePosition = _TimePosition();
	_NotifyPositionChanged((float)timePosition / fDuration);

	if (fSubTitles != NULL) {
		const SubTitle* subTitle = fSubTitles->SubTitleAt(timePosition);
		if (subTitle != NULL)
			fVideoView->SetSubTitle(subTitle->text.String());
		else
			fVideoView->SetSubTitle(NULL);
	}
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


void
Controller::NotifySeekHandled(int64 seekedFrame) const
{
	if (fPendingSeekRequests == 0)
		return;

	fPendingSeekRequests--;
	if (fPendingSeekRequests == 0) {
		fSeekFrame = -1;
		fRequestedSeekFrame = -1;
	}

	_NotifySeekHandled(seekedFrame);
}

