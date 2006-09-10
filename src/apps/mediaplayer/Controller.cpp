/*
 * Controller.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
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
#include <stdio.h>
#include <string.h>
#include <Debug.h>
#include <Entry.h>
#include <Window.h>
#include <Bitmap.h>
#include <Autolock.h>
#include <MediaFile.h>
#include <MediaTrack.h>

#include "Controller.h"
#include "ControllerView.h"
#include "VideoView.h"
#include "SoundOutput.h"


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


Controller::Controller()
 :	fVideoView(NULL)
 ,	fControllerView(NULL)
 ,	fName()
 ,	fPaused(false)
 ,	fStopped(true)
 ,	fMediaFile(0)
 ,	fAudioTrack(0)
 ,	fVideoTrack(0)
 ,	fAudioTrackLock("audio track lock")
 ,	fVideoTrackLock("video track lock")
 ,	fAudioTrackList(new BList)
 ,	fVideoTrackList(new BList)
 ,	fPosition(0)
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
 ,	fCurrentBitmap(NULL)
{
	for (int i = 0; i < MAX_AUDIO_BUFFERS; i++) {
		fAudioBuffer[i].bitmap = NULL;
		fAudioBuffer[i].buffer = NULL;
		fAudioBuffer[i].sizeMax = 0;
	}
	for (int i = 0; i < MAX_VIDEO_BUFFERS; i++) {
		fVideoBuffer[i].bitmap = NULL;
		fVideoBuffer[i].buffer = NULL;
		fVideoBuffer[i].sizeMax = 0;
	}

	fStopped = fAutoplay ? false : true;

}


Controller::~Controller()
{
	StopThreads();
	
	if (fMediaFile)
		fMediaFile->ReleaseAllTracks();
	delete fMediaFile;
	delete fAudioTrackList;
	delete fVideoTrackList;
}


void
Controller::SetVideoView(VideoView *view)
{
	fVideoView = view;
	fVideoView->SetController(this);
}


void
Controller::SetControllerView(ControllerView *view)
{
	fControllerView = view;
}


status_t
Controller::SetTo(const entry_ref &ref)
{
	StopThreads();

	UpdatePosition(0);

	BAutolock al(fAudioTrackLock);
	BAutolock vl(fVideoTrackLock);

	fAudioTrackList->MakeEmpty();
	fVideoTrackList->MakeEmpty();
	if (fMediaFile)
		fMediaFile->ReleaseAllTracks();
	delete fMediaFile;
	fAudioTrack = 0;
	fVideoTrack = 0;
	fMediaFile = 0;
	fPosition = 0;
	fSeekAudio = false;
	fSeekVideo = false;
	fPaused = false;
	fStopped = fAutoplay ? false : true;
	fDuration = 0;
	fName = ref.name;

	status_t err;
	
	BMediaFile *mf = new BMediaFile(&ref);
	err = mf->InitCheck();
	if (err != B_OK) {
		printf("Controller::SetTo: initcheck failed\n");
		delete mf;
		return err;
	}
	
	int trackcount = mf->CountTracks();
	if (trackcount <= 0) {
		printf("Controller::SetTo: trackcount %d\n", trackcount);
		delete mf;
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
		return B_MEDIA_NO_HANDLER;
	}

	printf("Controller::SetTo: %d audio track, %d video track\n", AudioTrackCount(), VideoTrackCount());

	fMediaFile = mf;

	SelectAudioTrack(0);
	SelectVideoTrack(0);
	
	StartThreads();
	
	return B_OK;
}


void
Controller::GetSize(int *width, int *height)
{
/*
	media_format fmt;
			buffer->mediaFormat.type = B_MEDIA_RAW_VIDEO;
			buffer->mediaFormat.u.raw_video = media_raw_video_format::wildcard;
			buffer->mediaFormat.u.raw_video.display.format = IsOverlayActive() ? B_YCbCr422 : B_RGB32;
	if (fVideoTrack || B_OK != fVideoTrack->DecodedFormat(&fmt)) {
*/
		*height = 480;
		*width = 640;
/*
	} else {
		*height = fmt.u.raw_video.display.line_count;
		*width = fmt.u.raw_video.display.line_width;
	}
*/
}


int
Controller::VideoTrackCount()
{
	return fVideoTrackList->CountItems();
}


int
Controller::AudioTrackCount()
{
	return fAudioTrackList->CountItems();
}


status_t
Controller::SelectAudioTrack(int n)
{	
	BAutolock al(fAudioTrackLock);

	BMediaTrack *t = (BMediaTrack *)fAudioTrackList->ItemAt(n);
	if (!t)
		return B_ERROR;
	fAudioTrack = t;

	bigtime_t a = fAudioTrack ? fAudioTrack->Duration() : 0;
	bigtime_t v = fVideoTrack ? fVideoTrack->Duration() : 0;
	fDuration = max_c(a, v);
	
	return B_OK;
}


status_t
Controller::SelectVideoTrack(int n)
{
	BAutolock vl(fVideoTrackLock);

	BMediaTrack *t = (BMediaTrack *)fVideoTrackList->ItemAt(n);
	if (!t)
		return B_ERROR;
	fVideoTrack = t;

	bigtime_t a = fAudioTrack ? fAudioTrack->Duration() : 0;
	bigtime_t v = fVideoTrack ? fVideoTrack->Duration() : 0;
	fDuration = max_c(a, v);

	return B_OK;
}


bigtime_t
Controller::Duration()
{
	return fDuration;
}


bigtime_t
Controller::Position()
{
	return fPosition;
}


status_t
Controller::Seek(bigtime_t pos)
{
	return B_OK;
}


void
Controller::VolumeUp()
{
}


void
Controller::VolumeDown()
{
}


void
Controller::SetVolume(float value)
{
	printf("Controller::SetVolume %.4f\n", value);
	if (fSoundOutput) // hack...
		fSoundOutput->SetVolume(value);
}


void
Controller::SetPosition(float value)
{
	printf("Controller::SetPosition %.4f\n", value);
	fSeekPosition = bigtime_t(value * Duration());
	fSeekAudio = true;
	fSeekVideo = true;
}


void
Controller::UpdateVolume(float value)
{
	fControllerView->SetVolume(value);
}


void
Controller::UpdatePosition(float value)
{
	fControllerView->SetPosition(value);
}


bool
Controller::IsOverlayActive()
{
//	return true;
	return false;
}


void
Controller::LockBitmap()
{
}


void
Controller::UnlockBitmap()
{
}


BBitmap *
Controller::Bitmap()
{
	return fCurrentBitmap;
}


void
Controller::Stop()
{
	printf("Controller::Stop\n");

	if (fStopped)
		return;
	
	fPaused = false;
	fStopped = true;
	
	// böse...
	snooze(30000);
	fSeekAudio = true;
	fSeekVideo = true;
	fSeekPosition = 0;
	snooze(30000);
	UpdatePosition(0);
}


void
Controller::Play()
{
	printf("Controller::Play\n");
	
	if (!fPaused && !fStopped)
		return;

	fStopped =
	fPaused = false;
}


void
Controller::Pause()
{
	printf("Controller::Pause\n");

	if (fStopped || fPaused)
		return;

	fPaused = true;
}


bool
Controller::IsPaused()
{
	return fPaused;
}


bool
Controller::IsStopped()
{
	return fStopped;
}


void
Controller::AudioDecodeThread()
{
	BMediaTrack *lastTrack = 0;
	size_t bufferSize = 0;
	size_t frameSize = 0;
	bool decodeError = false;
	status_t status;
	
printf("audio decode start\n");	
	if (fAudioTrack == 0) {
		return;
	}


	while (acquire_sem(fAudioDecodeSem) == B_OK) {
//printf("audio decoding..\n");	
		buffer_info *buffer = &fAudioBuffer[fAudioBufferWriteIndex];
		fAudioBufferWriteIndex = (fAudioBufferWriteIndex + 1) % fAudioBufferCount;
		BAutolock lock(fAudioTrackLock);
		if (lastTrack != fAudioTrack) {
//printf("audio track changed\n");
			lastTrack = fAudioTrack;
			buffer->formatChanged = true;
			buffer->mediaFormat.type = B_MEDIA_RAW_AUDIO;
			buffer->mediaFormat.u.raw_audio = media_multi_audio_format::wildcard;
			#ifdef __HAIKU__
			  buffer->mediaFormat.u.raw_audio.format = media_multi_audio_format::B_AUDIO_FLOAT;
			#endif
			status = fAudioTrack->DecodedFormat(&buffer->mediaFormat);
			if (status != B_OK) {
				printf("audio decoded format status %08lx %s\n", status, strerror(status));
				return;
			}
			bufferSize = buffer->mediaFormat.u.raw_audio.buffer_size;
			frameSize = (buffer->mediaFormat.u.raw_audio.format & 0xf) * buffer->mediaFormat.u.raw_audio.channel_count;
		} else {
			buffer->formatChanged = false;
		}
		if (buffer->sizeMax < bufferSize) {
			delete buffer->buffer;
			buffer->buffer = new char [bufferSize];
			buffer->sizeMax = bufferSize;;
		}

		if (fSeekAudio) {
			bigtime_t pos = fSeekPosition;
			fAudioTrack->SeekToTime(&pos);
			fSeekAudio = false;
			decodeError = false;
		}

		int64 frames;
		media_header mh;
		if (!decodeError) {
			decodeError = B_OK != fAudioTrack->ReadFrames(buffer->buffer, &frames, &mh);
		}
		if (!decodeError) {
			buffer->sizeUsed = frames * frameSize;
			buffer->startTime = mh.start_time;
		} else {
			buffer->sizeUsed = 0;
			buffer->startTime = 0;
		}
		
		release_sem(fAudioPlaySem);
	}
}


void
Controller::AudioPlayThread()
{
	SoundOutput *output = NULL;
	bigtime_t bufferDuration = 0;
	status_t status;

	
printf("audio play start\n");	
	if (fAudioTrack == 0) {
		fTimeSourceSysTime = 0;
		fTimeSourcePerfTime = 0;
		return;
	}

	while (acquire_sem(fAudioPlaySem) == B_OK) {
//printf("audio playing..\n");	
		buffer_info *buffer = &fAudioBuffer[fAudioBufferReadIndex];
		fAudioBufferReadIndex = (fAudioBufferReadIndex + 1) % fAudioBufferCount;
		wait:
		if (fPaused || fStopped) {
//printf("waiting..\n");	
			status = acquire_sem_etc(fThreadWaitSem, 1, B_RELATIVE_TIMEOUT, 50000);
			if (status != B_TIMED_OUT)
				break;
			goto wait;
		}
		if (buffer->formatChanged) {
//printf("format changed..\n");	
			delete output;
			output = new SoundOutput(fName.String(), buffer->mediaFormat.u.raw_audio);
			bufferDuration = bigtime_t(1000000.0 * (buffer->mediaFormat.u.raw_audio.buffer_size / (buffer->mediaFormat.u.raw_audio.channel_count * (buffer->mediaFormat.u.raw_audio.format & 0xf))) / buffer->mediaFormat.u.raw_audio.frame_rate);
			printf("audio format changed, new buffer duration %Ld\n", bufferDuration);
			fSoundOutput = output; // hack...
		}
		if (output) {
			if (buffer->sizeUsed)
				output->Play(buffer->buffer, buffer->sizeUsed);
			BAutolock lock(fTimeSourceLock);
			fTimeSourceSysTime = system_time() + output->Latency() - bufferDuration;
			fTimeSourcePerfTime = buffer->startTime;
//			printf("timesource: sys: %Ld perf: %Ld\n", fTimeSourceSysTime, fTimeSourcePerfTime);	
			UpdatePosition(buffer->startTime / (float)Duration());
		} else {
			debugger("kein SoundOutput");
		}
		release_sem(fAudioDecodeSem);
	}
	delete output;
	fSoundOutput = NULL; // hack...
}


void
Controller::VideoDecodeThread()
{
	BMediaTrack *lastTrack = 0;
	size_t bufferSize = 0;
	size_t bytePerRow = 0;
	int lineWidth = 0;
	int lineCount = 0;
	bool decodeError = false;
	status_t status;

printf("video decode start\n");	
	if (fVideoTrack == 0) {
		return;
	}

	while (acquire_sem(fVideoDecodeSem) == B_OK) {
		buffer_info *buffer = &fVideoBuffer[fVideoBufferWriteIndex];
		fVideoBufferWriteIndex = (fVideoBufferWriteIndex + 1) % fVideoBufferCount;
		BAutolock lock(fVideoTrackLock);
		if (lastTrack != fVideoTrack) {
//printf("Video track changed\n");
			lastTrack = fVideoTrack;
			buffer->formatChanged = true;
			buffer->mediaFormat.type = B_MEDIA_RAW_VIDEO;
			buffer->mediaFormat.u.raw_video = media_raw_video_format::wildcard;
			buffer->mediaFormat.u.raw_video.display.format = IsOverlayActive() ? B_YCbCr422 : B_RGB32;
			status = fVideoTrack->DecodedFormat(&buffer->mediaFormat);
			if (status != B_OK) {
				printf("video decoded format status %08lx %s\n", status, strerror(status));
				return;
			}
			bytePerRow = buffer->mediaFormat.u.raw_video.display.bytes_per_row;
			lineWidth = buffer->mediaFormat.u.raw_video.display.line_width;
			lineCount = buffer->mediaFormat.u.raw_video.display.line_count;
			bufferSize = lineCount * bytePerRow;
		} else {
			buffer->formatChanged = false;
		}
		if (buffer->sizeMax != bufferSize) {
			BRect r(0, 0, lineWidth - 1, lineCount - 1);
			delete buffer->bitmap;
			printf("allocating bitmap %d %d %ld\n", lineWidth, lineCount, bytePerRow);
			if (IsOverlayActive())
				buffer->bitmap = new BBitmap(r, B_BITMAP_WILL_OVERLAY | (fVideoBufferWriteIndex == 1) ? B_BITMAP_RESERVE_OVERLAY_CHANNEL : 0, IsOverlayActive() ? B_YCbCr422 : B_RGB32, bytePerRow);
			else
				buffer->bitmap = new BBitmap(r, 0, B_RGB32, bytePerRow);
			status = buffer->bitmap->InitCheck();
			if (status != B_OK) {
				printf("video decoded format status %08lx %s\n", status, strerror(status));
				return;
			}
			buffer->buffer = (char *)buffer->bitmap->Bits();
			buffer->sizeMax = bufferSize;
		}

		if (fSeekVideo) {
			bigtime_t pos = fSeekPosition;
			fVideoTrack->SeekToTime(&pos);
			fSeekVideo = false;
			decodeError = false;
		}

		int64 frames;
		media_header mh;
		if (!decodeError) {
//			printf("reading video frame...\n");
			decodeError = B_OK != fVideoTrack->ReadFrames(buffer->buffer, &frames, &mh);
		}
		if (!decodeError) {
			buffer->sizeUsed = buffer->sizeMax;
			buffer->startTime = mh.start_time;
		} else {
			buffer->sizeUsed = 0;
			buffer->startTime = 0;
		}
		
		release_sem(fVideoPlaySem);
	}
}


void
Controller::VideoPlayThread()
{
	status_t status;
printf("video decode start\n");	
	if (fVideoTrack == 0) {
		return;
	}

	while (acquire_sem(fVideoPlaySem) == B_OK) {
		
		buffer_info *buffer = &fVideoBuffer[fVideoBufferReadIndex];
		fVideoBufferReadIndex = (fVideoBufferReadIndex + 1) % fVideoBufferCount;
		wait:
		if (fPaused || fStopped) {
//printf("waiting..\n");	
			status = acquire_sem_etc(fThreadWaitSem, 1, B_RELATIVE_TIMEOUT, 50000);
			if (status != B_TIMED_OUT)
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

		status = acquire_sem_etc(fThreadWaitSem, 1, B_ABSOLUTE_TIMEOUT, waituntil);
		if (status != B_TIMED_OUT)
			break;
			
		fCurrentBitmap = buffer->bitmap;
		fVideoView->DrawFrame();

	//	snooze(60000);
		release_sem(fVideoDecodeSem);
	}
		
//		status_t status = acquire_sem_etc(fThreadWaitSem, 1, B_ABSOLUTE_TIMEOUT, buffer->startTime);
//		if (status != B_TIMED_OUT)
//			return;
}


void
Controller::StartThreads()
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
	fThreadWaitSem = create_sem(0, "thread wait sem");
	fAudioDecodeThread = spawn_thread(audio_decode_thread, "audio decode", AUDIO_DECODE_PRIORITY, this);
	fVideoDecodeThread = spawn_thread(video_decode_thread, "video decode", VIDEO_DECODE_PRIORITY, this);
	fAudioPlayThread = spawn_thread(audio_play_thread, "audio play", AUDIO_PLAY_PRIORITY, this);
	fVideoPlayThread = spawn_thread(video_play_thread, "video play", VIDEO_PLAY_PRIORITY, this);
	resume_thread(fAudioDecodeThread);
	resume_thread(fVideoDecodeThread);
	resume_thread(fAudioPlayThread);
	resume_thread(fVideoPlayThread);
}


void
Controller::StopThreads()
{
	if (fAudioDecodeSem < 0)
		return;
		
	delete_sem(fAudioDecodeSem);
	delete_sem(fVideoDecodeSem);
	delete_sem(fAudioPlaySem);
	delete_sem(fVideoPlaySem);
	delete_sem(fThreadWaitSem);

	status_t dummy;
	wait_for_thread(fAudioDecodeThread, &dummy);
	wait_for_thread(fVideoDecodeThread, &dummy);
	wait_for_thread(fAudioPlayThread, &dummy);
	wait_for_thread(fVideoPlayThread, &dummy);
	fAudioDecodeThread = -1;
	fVideoDecodeThread = -1;
	fAudioPlayThread = -1;
	fVideoPlayThread = -1;
	fThreadWaitSem = -1;
	fAudioDecodeSem = -1;
	fVideoDecodeSem = -1;
	fAudioPlaySem = -1;
	fVideoPlaySem = -1;
}

int32
Controller::audio_decode_thread(void *self)
{
	static_cast<Controller *>(self)->AudioDecodeThread();
	return 0;
}


int32
Controller::video_decode_thread(void *self)
{
	static_cast<Controller *>(self)->VideoDecodeThread();
	return 0;
}


int32
Controller::audio_play_thread(void *self)
{
	static_cast<Controller *>(self)->AudioPlayThread();
	return 0;
}


int32
Controller::video_play_thread(void *self)
{
	static_cast<Controller *>(self)->VideoPlayThread();
	return 0;
}
