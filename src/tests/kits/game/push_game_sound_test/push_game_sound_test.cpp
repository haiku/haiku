/*
 * Copyright 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Entry.h>
#include <InterfaceDefs.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <PushGameSound.h>


int
main(int argc, char *argv[])
{
	// 256 frames * 4 buffer parts * 2 channels * 2 bytes per sample
	// will give us internal buffer of 4096 bytes
	size_t framesPerBufferPart = 256;
	size_t bufferPartCount = 4;

	if (argc != 2 && argc != 4) {
		printf("Usage: %s <sound file name> [<frames per part> <parts>]\n",
			argv[0]);
		return 0;
	}

	if (argc == 4) {
		size_t size = strtoul(argv[2], NULL, 10);
		if (size > 0)
			framesPerBufferPart = size;

		size = strtoul(argv[3], NULL,  10);
		if (size == 1) {
			printf("at least 2 buffer parts are needed\n");
			return 1;
		}
		if (size > 0)
			bufferPartCount = size;
	}

	printf("frames per buffer part: %ld\n", framesPerBufferPart);
	printf("buffer part count: %ld\n", bufferPartCount);

	BEntry entry(argv[1]);
	if (entry.InitCheck() != B_OK || !entry.Exists()) {
		printf("cannot open input file\n");
		return 1;
	}

	entry_ref entryRef;
	entry.GetRef(&entryRef);

	BMediaFile mediaFile(&entryRef);
	if (mediaFile.InitCheck() != B_OK) {
		printf("file not supported\n");
		return 1;
	}

	if (mediaFile.CountTracks() == 0) {
		printf("no tracks found in file\n");
		return 1;
	}

	BMediaTrack *mediaTrack = mediaFile.TrackAt(0);
	if (mediaTrack == NULL) {
		printf("problem getting track from file\n");
		return 1;
	}

	// propose format, let it decide frame rate, channels number and buf size
	media_format format;
	memset(&format, 0, sizeof(format));
	format.type = B_MEDIA_RAW_AUDIO;
	format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	format.u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;

	if (mediaTrack->DecodedFormat(&format) != B_OK) {
		printf("cannot set decoder output format\n");
		return 1;
	}

	printf("negotiated format:\n");
	printf("frame rate: %g Hz\n", format.u.raw_audio.frame_rate);
	printf("channel count: %ld\n", format.u.raw_audio.channel_count);
	printf("buffer size: %ld bytes\n", format.u.raw_audio.buffer_size);

	gs_audio_format gsFormat;
	memset(&gsFormat, 0, sizeof(gsFormat));
	gsFormat.frame_rate = format.u.raw_audio.frame_rate;
	gsFormat.channel_count = format.u.raw_audio.channel_count;
	gsFormat.format = format.u.raw_audio.format;
	gsFormat.byte_order = format.u.raw_audio.byte_order;

	BPushGameSound pushGameSound(framesPerBufferPart, &gsFormat,
		bufferPartCount);
	if (pushGameSound.InitCheck() != B_OK) {
		printf("trouble initializing push game sound: %s\n",
			strerror(pushGameSound.InitCheck()));
		return 1;
	}

	uint8 *buffer;
	size_t bufferSize;
	if (pushGameSound.LockForCyclic((void **)&buffer, &bufferSize)
			!= BPushGameSound::lock_ok) {
		printf("cannot lock buffer\n");
		return 1;
	}
	memset(buffer, 0, bufferSize);

	if (pushGameSound.StartPlaying() != B_OK) {
		printf("cannot start playback\n");
		return 1;
	}

	printf("playing, press [esc] to exit...\n");

	uint8 decoded[format.u.raw_audio.buffer_size * 2];
	size_t bufferPartSize = framesPerBufferPart
		* format.u.raw_audio.channel_count
		* (format.u.raw_audio.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK);
	size_t decodedSize = 0;
	size_t partPos = 0;
	size_t pos = 0; /*pushGameSound.CurrentPosition();*/
	key_info keyInfo;

	while (true) {
		// fill buffer part with data from decoded buffer
		while (partPos < bufferPartSize && decodedSize) {
			size_t size = min_c(bufferPartSize - partPos, decodedSize);

			memcpy(buffer + pos + partPos, decoded, size);
			partPos += size;

			decodedSize -= size;
			memmove(decoded, decoded + size, decodedSize);
		}

		// if there are too little data to fill next buffer part
		// read next decoded frames
		if (partPos < bufferPartSize) {
			int64 frameCount;
			if (mediaTrack->ReadFrames(decoded + decodedSize, &frameCount)
					!= B_OK)
				break;
			if (frameCount == 0)
				break;

			decodedSize += frameCount * format.u.raw_audio.channel_count
				* (format.u.raw_audio.format
					& media_raw_audio_format::B_AUDIO_SIZE_MASK);

			printf("\rtime: %.2f",
				(double)mediaTrack->CurrentTime() / 1000000LL);
			fflush(stdout);

			continue;
		}

		// this buffer part is done
		partPos = 0;
		pos += bufferPartSize;
		if (bufferSize <= pos)
			pos = 0;

		// playback sync - wait for the buffer part we're about to fill to be
		// played
		while (pushGameSound.CurrentPosition() >= pos + bufferPartSize
			|| pushGameSound.CurrentPosition() < pos)
			snooze(1000 * framesPerBufferPart / gsFormat.frame_rate);

		// check escape key state
		if (get_key_info(&keyInfo) != B_OK) {
			printf("\nkeyboard state read error\n");
			break;
		}
		if ((keyInfo.key_states[0] & 0x40) != 0)
			break;
	}

	pushGameSound.StopPlaying();

	mediaFile.ReleaseTrack(mediaTrack);
	mediaFile.CloseFile();

	printf("\nfinished.\n");

	return 0;
}
