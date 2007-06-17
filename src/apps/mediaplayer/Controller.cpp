/*
 * Controller.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007 Stephan Aßmus <superstippi@gmx.de>
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

#include "ControllerView.h"
#include "PlaybackState.h"
#include "SoundOutput.h"
#include "VideoView.h"

// suppliers
#include "AudioSupplier.h"
#include "MediaTrackAudioSupplier.h"
#include "MediaTrackVideoSupplier.h"
#include "VideoSupplier.h"

using std::nothrow;


#define AUDIO_PLAY_PRIORITY		110
#define VIDEO_PLAY_PRIORITY		20
#define AUDIO_DECODE_PRIORITY	13
#define VIDEO_DECODE_PRIORITY	13

void 
HandleError(const char *text, status_t err)
{
	if (err != B_OK) { 
		printf("%s. error 0x%08x (%s)\n",text, (int)err, strerror(err));
		fflush(NULL);
		exit(1);
	}
}


// #pragma mark -


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


// #pragma mark -


Controller::Controller()
 :	fVideoView(NULL)
 ,	fPaused(false)
 ,	fStopped(true)
 ,	fVolume(1.0)

 ,	fRef()
 ,	fMediaFile(0)
 ,	fDataLock("controller data lock")

 ,	fVideoSupplier(NULL)
 ,	fAudioSupplier(NULL)

 ,	fVideoSupplierLock("video supplier lock")
 ,	fAudioSupplierLock("audio supplier lock")

 ,	fAudioTrackList(new BList)
 ,	fVideoTrackList(new BList)
 ,	fAudioDecodeSem(-1)
 ,	fVideoDecodeSem(-1)
 ,	fAudioPlaySem(-1)
 ,	fVideoPlaySem(-1)
 ,	fAudioDecodeThread(-1)
 ,	fVideoDecodeThread(-1)
 ,	fAudioPlayThread(-1)
 ,	fVideoPlayThread(-1)
 ,	fSoundOutput(NULL)
 ,	fSeekAudio(false)
 ,	fSeekVideo(false)
 ,	fSeekPosition(0)
 ,	fPosition(0)
 ,	fDuration(0)
 ,	fAudioBufferCount(MAX_AUDIO_BUFFERS)
 ,	fAudioBufferReadIndex(0)
 ,	fAudioBufferWriteIndex(0)
 ,	fVideoBufferCount(MAX_VIDEO_BUFFERS)
 ,	fVideoBufferReadIndex(0)
 ,	fVideoBufferWriteIndex(0)
 ,	fTimeSourceLock("time source lock")
 ,	fTimeSourceSysTime(0)
 ,	fTimeSourcePerfTime(0)
 ,	fAutoplay(true)
 ,	fPauseAtEndOfStream(false)
 ,	fSeekToStartAfterPause(false)
 ,	fCurrentBitmap(NULL)
 ,	fBitmapLock("bitmap lock")
 ,	fListeners(4)
{
	for (int i = 0; i < MAX_AUDIO_BUFFERS; i++) {
		fAudioBuffer[i].bitmap = NULL;
		fAudioBuffer[i].buffer = NULL;
		fAudioBuffer[i].sizeMax = 0;
		fAudioBuffer[i].formatChanged = false;
		fAudioBuffer[i].endOfStream = false;
	}
	for (int i = 0; i < MAX_VIDEO_BUFFERS; i++) {
		fVideoBuffer[i].bitmap = NULL;
		fVideoBuffer[i].buffer = NULL;
		fVideoBuffer[i].sizeMax = 0;
		fVideoBuffer[i].formatChanged = false;
		fVideoBuffer[i].endOfStream = false;
	}

	fStopped = fAutoplay ? false : true;

	_StartThreads();
}


Controller::~Controller()
{
	_StopThreads();

	if (fMediaFile)
		fMediaFile->ReleaseAllTracks();
	delete fMediaFile;
	delete fAudioTrackList;
	delete fVideoTrackList;

	delete fSoundOutput;
}


// #pragma mark -


bool
Controller::Lock()
{
	return fDataLock.Lock();
}


status_t
Controller::LockWithTimeout(bigtime_t timeout)
{
	return fDataLock.LockWithTimeout(timeout);
}


void
Controller::Unlock()
{
	return fDataLock.Unlock();
}


// #pragma mark -


status_t
Controller::SetTo(const entry_ref &ref)
{
	BAutolock dl(fDataLock);
	BAutolock al(fVideoSupplierLock);
	BAutolock vl(fAudioSupplierLock);

	if (fRef == ref)
		return B_OK;

	fRef = ref;

	fAudioTrackList->MakeEmpty();
	fVideoTrackList->MakeEmpty();
	if (fMediaFile)
		fMediaFile->ReleaseAllTracks();
	delete fMediaFile;
	fMediaFile = NULL;
	delete fVideoSupplier;
	fVideoSupplier = NULL;
	delete fAudioSupplier;
	fAudioSupplier = NULL;
	
	fSeekAudio = true;
	fSeekVideo = true;
	fSeekPosition = 0;
	fPaused = false;
	fStopped = fAutoplay ? false : true;
	fPauseAtEndOfStream = false;
	fSeekToStartAfterPause = false;
	fDuration = 0;

	_UpdatePosition(0);

	status_t err;
	
	BMediaFile *mf = new BMediaFile(&ref);
	err = mf->InitCheck();
	if (err != B_OK) {
		printf("Controller::SetTo: initcheck failed\n");
		delete mf;
		_NotifyFileChanged();
		return err;
	}
	
	int trackcount = mf->CountTracks();
	if (trackcount <= 0) {
		printf("Controller::SetTo: trackcount %d\n", trackcount);
		delete mf;
		_NotifyFileChanged();
		return B_MEDIA_NO_HANDLER;
	}
	
	for (int i = 0; i < trackcount; i++) {
		BMediaTrack *t = mf->TrackAt(i);
		media_format f;
		err = t->EncodedFormat(&f);
		if (err != B_OK) {
			printf("Controller::SetTo: EncodedFormat failed for track index %d, error 0x%08lx (%s)\n",
				i, err, strerror(err));
			mf->ReleaseTrack(t);
			continue;
		}
		if (f.IsAudio()) {
			fAudioTrackList->AddItem(t);
		} else if (f.IsVideo()) {
			fVideoTrackList->AddItem(t);
		} else {
			printf("Controller::SetTo: track index %d has unknown type\n", i);
			mf->ReleaseTrack(t);
		}
	}

	if (AudioTrackCount() == 0 && VideoTrackCount() == 0) {
		printf("Controller::SetTo: no audio or video tracks found\n");
		delete mf;
		_NotifyFileChanged();
		return B_MEDIA_NO_HANDLER;
	}

	printf("Controller::SetTo: %d audio track, %d video track\n", AudioTrackCount(), VideoTrackCount());

	fMediaFile = mf;

	SelectAudioTrack(0);
	SelectVideoTrack(0);

	_NotifyFileChanged();
	_NotifyPlaybackStateChanged();

	return B_OK;
}


void
Controller::GetSize(int *width, int *height)
{
	BAutolock _(fVideoSupplierLock);

	if (fVideoSupplier) {
		media_format format = fVideoSupplier->Format();
		// TODO: take aspect ratio into account!
		*height = format.u.raw_video.display.line_count;
		*width = format.u.raw_video.display.line_width;
	} else {
		*height = 480;
		*width = 640;
	}
}


int
Controller::AudioTrackCount()
{
	BAutolock _(fDataLock);

	return fAudioTrackList->CountItems();
}


int
Controller::VideoTrackCount()
{
	BAutolock _(fDataLock);

	return fVideoTrackList->CountItems();
}


status_t
Controller::SelectAudioTrack(int n)
{	
	BAutolock _(fAudioSupplierLock);

	BMediaTrack* track = (BMediaTrack *)fAudioTrackList->ItemAt(n);
	if (!track)
		return B_ERROR;

	delete fAudioSupplier;
	fAudioSupplier = new MediaTrackAudioSupplier(track);

	bigtime_t a = fAudioSupplier->Duration();
	bigtime_t v = fVideoSupplier ? fVideoSupplier->Duration() : 0;;
	fDuration = max_c(a, v);

	_NotifyAudioTrackChanged(n);
	return B_OK;
}


status_t
Controller::SelectVideoTrack(int n)
{
	BAutolock _(fVideoSupplierLock);

	BMediaTrack* track = (BMediaTrack *)fVideoTrackList->ItemAt(n);
	if (!track)
		return B_ERROR;

	delete fVideoSupplier;
	fVideoSupplier = new MediaTrackVideoSupplier(track,
		IsOverlayActive() ? B_YCbCr422 : B_RGB32);

	bigtime_t a = fAudioSupplier ? fAudioSupplier->Duration() : 0;
	bigtime_t v = fVideoSupplier->Duration();
	fDuration = max_c(a, v);

	_NotifyVideoTrackChanged(n);
	return B_OK;
}


// #pragma mark -


void
Controller::Stop()
{
	printf("Controller::Stop\n");

	BAutolock _(fDataLock);

	if (fStopped)
		return;
	
	fPaused = false;
	fStopped = true;
	
	fSeekAudio = true;
	fSeekVideo = true;
	fSeekPosition = 0;

	_NotifyPlaybackStateChanged();
	_UpdatePosition(0, false, true);
}


void
Controller::Play()
{
	printf("Controller::Play\n");

	BAutolock _(fDataLock);
	
	if (!fPaused && !fStopped)
		return;

	fStopped = false;
	fPaused = false;

	if (fSeekToStartAfterPause) {
printf("seeking to start after pause\n");
		SetPosition(0);
		fSeekToStartAfterPause = false;
	}

	_NotifyPlaybackStateChanged();
}


void
Controller::Pause()
{
	printf("Controller::Pause\n");

	BAutolock _(fDataLock);

	if (fStopped || fPaused)
		return;

	fPaused = true;

	_NotifyPlaybackStateChanged();
}


void
Controller::TogglePlaying()
{
	BAutolock _(fDataLock);

	if (IsPaused() || IsStopped())
		Play();
	else
		Pause();
}


bool
Controller::IsPaused() const
{
	BAutolock _(fDataLock);

	return fPaused;
}


bool
Controller::IsStopped() const
{
	BAutolock _(fDataLock);

	return fStopped;
}


uint32
Controller::PlaybackState() const
{
	BAutolock _(fDataLock);

	if (fStopped)
		return PLAYBACK_STATE_STOPPED;
	if (fPaused)
		return PLAYBACK_STATE_PAUSED;
	return PLAYBACK_STATE_PLAYING;
}


bigtime_t
Controller::Duration()
{
	BAutolock _(fDataLock);

	return fDuration;
}


bigtime_t
Controller::Position()
{
	BAutolock _(fDataLock);

	return fPosition;
}


void
Controller::SetVolume(float value)
{
	printf("Controller::SetVolume %.4f\n", value);
	if (Lock()) {
		fVolume = value;
		if (fSoundOutput)
			fSoundOutput->SetVolume(fVolume);
		Unlock();
	}
}


float
Controller::Volume() const
{
	BAutolock _(fDataLock);

	return fVolume;	
}


void
Controller::SetPosition(float value)
{
	printf("Controller::SetPosition %.4f\n", value);

	BAutolock _(fDataLock);

	fSeekPosition = bigtime_t(value * Duration());
	fSeekAudio = true;
	fSeekVideo = true;

	fSeekToStartAfterPause = false;

	release_sem(fAudioWaitSem);
	release_sem(fVideoWaitSem);
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
	if (fVideoSupplier)
		return fVideoSupplier->GetEncodedFormat(format);
	return B_NO_INIT;
}


status_t
Controller::GetVideoCodecInfo(media_codec_info* info)
{
	// you need to hold the data lock
	if (fVideoSupplier)
		return fVideoSupplier->GetCodecInfo(info);
	return B_NO_INIT;
}


status_t
Controller::GetEncodedAudioFormat(media_format* format)
{
	// you need to hold the data lock
	if (fAudioSupplier)
		return fAudioSupplier->GetEncodedFormat(format);
	return B_NO_INIT;
}


status_t
Controller::GetAudioCodecInfo(media_codec_info* info)
{
	// you need to hold the data lock
	if (fAudioSupplier)
		return fAudioSupplier->GetCodecInfo(info);
	return B_NO_INIT;
}


// #pragma mark -


void
Controller::SetVideoView(VideoView *view)
{
	BAutolock _(fDataLock);

	fVideoView = view;
	fVideoView->SetController(this);
}


bool
Controller::IsOverlayActive()
{
//	return true;
	return false;
}


bool
Controller::LockBitmap()
{
	return fBitmapLock.Lock();
}


void
Controller::UnlockBitmap()
{
	fBitmapLock.Unlock();
}


BBitmap *
Controller::Bitmap()
{
	return fCurrentBitmap;
}


// #pragma mark -


bool
Controller::AddListener(Listener* listener)
{
	BAutolock _(fDataLock);

	if (listener && !fListeners.HasItem(listener))
		return fListeners.AddItem(listener);
	return false;
}


void
Controller::RemoveListener(Listener* listener)
{
	BAutolock _(fDataLock);

	fListeners.RemoveItem(listener);
}


// #pragma mark -


void
Controller::_AudioDecodeThread()
{
	AudioSupplier* lastSupplier = NULL;
	size_t bufferSize = 0;
	size_t frameSize = 0;
	bool decodeError = false;
	
printf("audio decode start\n");	

	while (acquire_sem(fAudioDecodeSem) == B_OK) {
//printf("audio decoding..\n");	
		buffer_info *buffer = &fAudioBuffer[fAudioBufferWriteIndex];
		fAudioBufferWriteIndex = (fAudioBufferWriteIndex + 1) % fAudioBufferCount;

		if (!fAudioSupplierLock.Lock())
			return;

		buffer->formatChanged = lastSupplier != fAudioSupplier;
		lastSupplier = fAudioSupplier;

		if (!lastSupplier) {
			// no audio supplier
			buffer->sizeUsed = 0;
			buffer->startTime = 0;

			fAudioSupplierLock.Unlock();
			snooze(10000);
			release_sem(fAudioPlaySem);
			continue;
		}

		if (buffer->formatChanged) {
printf("audio format changed\n");
			buffer->mediaFormat = lastSupplier->Format();
			bufferSize = buffer->mediaFormat.u.raw_audio.buffer_size;
			frameSize = (buffer->mediaFormat.u.raw_audio.format & 0xf)
				* buffer->mediaFormat.u.raw_audio.channel_count;
		}

		if (buffer->sizeMax < bufferSize) {
			delete[] buffer->buffer;
			buffer->buffer = new char [bufferSize];
			buffer->sizeMax = bufferSize;
		}

		if (fSeekAudio) {
			bigtime_t pos = fSeekPosition;
			lastSupplier->SeekToTime(&pos);
			fSeekAudio = false;
			decodeError = false;
		}

		int64 frames;
		if (!decodeError) {
			buffer->endOfStream = false;
			status_t ret = lastSupplier->ReadFrames(buffer->buffer,
				&frames, &buffer->startTime);
			decodeError = B_OK != ret;
if (ret != B_OK)
printf("audio decode error: %s\n", strerror(ret));
			if (ret == B_LAST_BUFFER_ERROR) {
				_EndOfStreamReached();
				buffer->endOfStream = true;
			}
		}
		if (!decodeError) {
			buffer->sizeUsed = frames * frameSize;
		} else {
			buffer->sizeUsed = 0;
			buffer->startTime = 0;

			fAudioSupplierLock.Unlock();
			snooze(10000);
			release_sem(fAudioPlaySem);
			continue;
		}

		fAudioSupplierLock.Unlock();

		release_sem(fAudioPlaySem);
	}
}


void
Controller::_AudioPlayThread()
{
	bigtime_t bufferDuration = 0;
	bigtime_t audioLatency = 0;
	status_t status;

printf("audio play start\n");	

	while (acquire_sem(fAudioPlaySem) == B_OK) {
//printf("audio playing..\n");	
		buffer_info *buffer = &fAudioBuffer[fAudioBufferReadIndex];
		fAudioBufferReadIndex = (fAudioBufferReadIndex + 1) % fAudioBufferCount;
		wait:
		if (fPaused || fStopped) {
//printf("waiting..\n");	
			status = acquire_sem_etc(fVideoWaitSem, 1, B_RELATIVE_TIMEOUT, 50000);
			if (status != B_OK && status != B_TIMED_OUT)
				break;
			goto wait;
		}

		if (!fDataLock.Lock())
			return;

		// TODO: fix performance time update (in case no audio)
		if (fTimeSourceLock.Lock()) {
			fTimeSourceSysTime = system_time() + audioLatency - bufferDuration;
			fTimeSourceLock.Unlock();
		}
//		printf("timesource: sys: %Ld perf: %Ld\n", fTimeSourceSysTime,
//			fTimeSourcePerfTime);

		if (buffer->sizeUsed == 0) {
			bool pause = fPauseAtEndOfStream && buffer->endOfStream;
			fDataLock.Unlock();
			release_sem(fAudioDecodeSem);
			if (pause) {
				fPauseAtEndOfStream = false;
				fSeekToStartAfterPause = true;
				Pause();
			} else
				snooze(25000);
			continue;
		}

		if (!fSoundOutput || buffer->formatChanged) {
			if (!fSoundOutput
				|| !(fSoundOutput->Format() == buffer->mediaFormat.u.raw_audio)) {
				delete fSoundOutput;
				fSoundOutput = new (nothrow) SoundOutput(fRef.name,
					buffer->mediaFormat.u.raw_audio);
				fSoundOutput->SetVolume(fVolume);
bufferDuration = bigtime_t(1000000.0
	* (buffer->mediaFormat.u.raw_audio.buffer_size
	/ (buffer->mediaFormat.u.raw_audio.channel_count
	* (buffer->mediaFormat.u.raw_audio.format & 0xf)))
	/ buffer->mediaFormat.u.raw_audio.frame_rate);
printf("audio format changed, new buffer duration %Ld\n", bufferDuration);
			}
		}

		if (fSoundOutput) {
			fSoundOutput->Play(buffer->buffer, buffer->sizeUsed);
			audioLatency = fSoundOutput->Latency();
			_UpdatePosition(buffer->startTime);
		}

		fDataLock.Unlock();

		release_sem(fAudioDecodeSem);
	}
}


// #pragma mark -


void
Controller::_VideoDecodeThread()
{
	VideoSupplier* lastSupplier = NULL;
	size_t bufferSize = 0;
	size_t bytePerRow = 0;
	int lineWidth = 0;
	int lineCount = 0;
	bool decodeError = false;
	status_t status;

printf("video decode start\n");	

	while (acquire_sem(fVideoDecodeSem) == B_OK) {
		buffer_info *buffer = &fVideoBuffer[fVideoBufferWriteIndex];
		fVideoBufferWriteIndex = (fVideoBufferWriteIndex + 1) % fVideoBufferCount;

		if (!fVideoSupplierLock.Lock())
			return;

		buffer->formatChanged = lastSupplier != fVideoSupplier;
		lastSupplier = fVideoSupplier;

		if (!lastSupplier) {
			// no video supplier
			buffer->sizeUsed = 0;
			buffer->startTime = 0;

			fVideoSupplierLock.Unlock();
			snooze(10000);
			release_sem(fVideoDecodeSem);
			continue;
		}
		
		
		if (buffer->formatChanged) {
//printf("Video track changed\n");
			buffer->mediaFormat = lastSupplier->Format();

			bytePerRow = buffer->mediaFormat.u.raw_video.display.bytes_per_row;
			lineWidth = buffer->mediaFormat.u.raw_video.display.line_width;
			lineCount = buffer->mediaFormat.u.raw_video.display.line_count;
			bufferSize = lineCount * bytePerRow;
		}

		if (buffer->sizeMax != bufferSize) {
			LockBitmap();

			BRect r(0, 0, lineWidth - 1, lineCount - 1);
			if (buffer->bitmap == fCurrentBitmap)
				fCurrentBitmap = NULL;
			delete buffer->bitmap;
printf("allocating bitmap #%ld %d %d %ld\n",
	fVideoBufferWriteIndex, lineWidth, lineCount, bytePerRow);
			if (buffer->mediaFormat.u.raw_video.display.format == B_YCbCr422) {
				buffer->bitmap = new BBitmap(r, B_BITMAP_WILL_OVERLAY
					| (fVideoBufferWriteIndex == 0) ?
						B_BITMAP_RESERVE_OVERLAY_CHANNEL : 0,
					B_YCbCr422, bytePerRow);
			} else {
				buffer->bitmap = new BBitmap(r, 0, B_RGB32, bytePerRow);
			}
			status = buffer->bitmap->InitCheck();
			if (status != B_OK) {
				printf("bitmap init status %08lx %s\n", status, strerror(status));
				return;
			}
			buffer->buffer = (char *)buffer->bitmap->Bits();
			buffer->sizeMax = bufferSize;

			UnlockBitmap();
		}

		if (fSeekVideo) {
			bigtime_t pos = fSeekPosition;
			lastSupplier->SeekToTime(&pos);
			fSeekVideo = false;
			decodeError = false;
		}

		if (!decodeError) {
//			printf("reading video frame...\n");
			status_t ret = lastSupplier->ReadFrame(buffer->buffer,
				&buffer->startTime);
			decodeError = B_OK != ret;
			if (ret == B_LAST_BUFFER_ERROR) {
				buffer->endOfStream = true;
				_EndOfStreamReached(true);
			}
		}
		if (!decodeError) {
			buffer->sizeUsed = buffer->sizeMax;
		} else {
			buffer->sizeUsed = 0;
			buffer->startTime = 0;

			fVideoSupplierLock.Unlock();
			snooze(10000);
			release_sem(fVideoPlaySem);
			continue;
		}

		fVideoSupplierLock.Unlock();

		release_sem(fVideoPlaySem);
	}
}


void
Controller::_VideoPlayThread()
{
	status_t status;
printf("video decode start\n");	

	while (acquire_sem(fVideoPlaySem) == B_OK) {
		
		buffer_info *buffer = &fVideoBuffer[fVideoBufferReadIndex];
		fVideoBufferReadIndex = (fVideoBufferReadIndex + 1) % fVideoBufferCount;
		wait:
		if (fPaused || fStopped) {
//printf("waiting..\n");	
			status = acquire_sem_etc(fVideoWaitSem, 1, B_RELATIVE_TIMEOUT, 50000);
			if (status != B_OK && status != B_TIMED_OUT)
				break;
			goto wait;
		}

#if 0
		bigtime_t waituntil;
		bigtime_t waitdelta;
		char test[100];
		{
			BAutolock lock(fTimeSourceLock);

			waituntil = fTimeSourceSysTime - fTimeSourcePerfTime + buffer->startTime;
			waitdelta = waituntil - system_time();
			sprintf(test, "sys %.6f perf %.6f, vid %.6f, waituntil %.6f, waitdelta %.6f",
			 fTimeSourceSysTime / 1000000.0f,
			 fTimeSourcePerfTime / 1000000.0f,
			 buffer->startTime / 1000000.0f,
			 waituntil / 1000000.0f,
			 waitdelta / 1000000.0f);
		}
		if (fVideoView->LockLooperWithTimeout(5000) == B_OK) {
			fVideoView->Window()->SetTitle(test);
			fVideoView->UnlockLooper();
		}
#else
		bigtime_t waituntil;
		waituntil = fTimeSourceSysTime - fTimeSourcePerfTime + buffer->startTime;
#endif

		status = acquire_sem_etc(fVideoWaitSem, 1, B_ABSOLUTE_TIMEOUT, waituntil);
		if (status == B_OK) { // interrupted by seeking
			printf("video: video wait interruped\n");
			continue;
		}
		if (status != B_TIMED_OUT)
			break;

		if (!fDataLock.Lock())
			return;

		if (fVideoView) {
			if (buffer->sizeUsed == buffer->sizeMax) {
				LockBitmap();
				fCurrentBitmap = buffer->bitmap;
				fVideoView->DrawFrame();
				UnlockBitmap();

				_UpdatePosition(buffer->startTime, true);
			} else {
				bool pause = fPauseAtEndOfStream && buffer->endOfStream;
				fDataLock.Unlock();

				release_sem(fVideoDecodeSem);

				if (pause) {
					fPauseAtEndOfStream = false;
					fSeekToStartAfterPause = true;
					Pause();
				} else
					snooze(25000);
				continue;
			}
		} else
			debugger("fVideoView == NULL");

		fDataLock.Unlock();

	//	snooze(60000);
		release_sem(fVideoDecodeSem);
	}
		
//		status_t status = acquire_sem_etc(fVideoWaitSem, 1, B_ABSOLUTE_TIMEOUT, buffer->startTime);
//		if (status != B_TIMED_OUT)
//			return;
}


// #pragma mark -


void
Controller::_StartThreads()
{
	if (fAudioDecodeSem > 0)
		return;

	fAudioBufferReadIndex = 0;
	fAudioBufferWriteIndex = 0;
	fVideoBufferReadIndex = 0;
	fVideoBufferWriteIndex = 0;
	fAudioDecodeSem = create_sem(fAudioBufferCount, "audio decode sem");
	fVideoDecodeSem = create_sem(fVideoBufferCount - 2, "video decode sem");
	fAudioPlaySem = create_sem(0, "audio play sem");
	fVideoPlaySem = create_sem(0, "video play sem");
	fAudioWaitSem = create_sem(0, "audio wait sem");
	fVideoWaitSem = create_sem(0, "video wait sem");
	fAudioDecodeThread = spawn_thread(_AudioDecodeThreadEntry, "audio decode", AUDIO_DECODE_PRIORITY, this);
	fVideoDecodeThread = spawn_thread(_VideoDecodeThreadEntry, "video decode", VIDEO_DECODE_PRIORITY, this);
	fAudioPlayThread = spawn_thread(_AudioPlayThreadEntry, "audio play", AUDIO_PLAY_PRIORITY, this);
	fVideoPlayThread = spawn_thread(_VideoPlayThreadEntry, "video play", VIDEO_PLAY_PRIORITY, this);
	resume_thread(fAudioDecodeThread);
	resume_thread(fVideoDecodeThread);
	resume_thread(fAudioPlayThread);
	resume_thread(fVideoPlayThread);
}


void
Controller::_StopThreads()
{
	if (fAudioDecodeSem < 0)
		return;
		
	delete_sem(fAudioDecodeSem);
	delete_sem(fVideoDecodeSem);
	delete_sem(fAudioPlaySem);
	delete_sem(fVideoPlaySem);
	delete_sem(fAudioWaitSem);
	delete_sem(fVideoWaitSem);

	status_t dummy;
	wait_for_thread(fAudioDecodeThread, &dummy);
	wait_for_thread(fVideoDecodeThread, &dummy);
	wait_for_thread(fAudioPlayThread, &dummy);
	wait_for_thread(fVideoPlayThread, &dummy);
	fAudioDecodeThread = -1;
	fVideoDecodeThread = -1;
	fAudioPlayThread = -1;
	fVideoPlayThread = -1;
	fAudioWaitSem = -1;
	fVideoWaitSem = -1;
	fAudioDecodeSem = -1;
	fVideoDecodeSem = -1;
	fAudioPlaySem = -1;
	fVideoPlaySem = -1;
}


// #pragma mark -


void
Controller::_EndOfStreamReached(bool isVideo)
{
	// you need to hold the fDataLock

	// prefer end of audio stream over end of video stream
	if (isVideo && fAudioSupplier)
		return;

	// NOTE: executed in audio/video decode thread
	if (!fStopped) {
		fPauseAtEndOfStream = true;
		_NotifyFileFinished();
	}
}


void
Controller::_UpdatePosition(bigtime_t position, bool isVideoPosition, bool force)
{
	// you need to hold the fDataLock

	if (!force && fStopped)
		return;

	// prefer audio position over video position
	if (isVideoPosition && fAudioSupplier)
		return;

	BAutolock _(fTimeSourceLock);
	fTimeSourcePerfTime = position;

	fPosition = position;
	float scale = fDuration > 0 ? (float)fPosition / fDuration : 0.0;
	_NotifyPositionChanged(scale);
}


// #pragma mark -


int32
Controller::_AudioDecodeThreadEntry(void *self)
{
	static_cast<Controller *>(self)->_AudioDecodeThread();
	return 0;
}


int32
Controller::_VideoDecodeThreadEntry(void *self)
{
	static_cast<Controller *>(self)->_VideoDecodeThread();
	return 0;
}


int32
Controller::_AudioPlayThreadEntry(void *self)
{
	static_cast<Controller *>(self)->_AudioPlayThread();
	return 0;
}


int32
Controller::_VideoPlayThreadEntry(void *self)
{
	static_cast<Controller *>(self)->_VideoPlayThread();
	return 0;
}


// #pragma mark -


void
Controller::_NotifyFileChanged()
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->FileChanged();
	}
}


void
Controller::_NotifyFileFinished()
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->FileFinished();
	}
}


void
Controller::_NotifyVideoTrackChanged(int32 index)
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->VideoTrackChanged(index);
	}
}


void
Controller::_NotifyAudioTrackChanged(int32 index)
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->AudioTrackChanged(index);
	}
}


void
Controller::_NotifyVideoStatsChanged()
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->VideoStatsChanged();
	}
}


void
Controller::_NotifyAudioStatsChanged()
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->AudioStatsChanged();
	}
}


void
Controller::_NotifyPlaybackStateChanged()
{
	uint32 state = PlaybackState();
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->PlaybackStateChanged(state);
	}
}


void
Controller::_NotifyPositionChanged(float position)
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->PositionChanged(position);
	}
}


void
Controller::_NotifyVolumeChanged(float volume)
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->VolumeChanged(volume);
	}
}

