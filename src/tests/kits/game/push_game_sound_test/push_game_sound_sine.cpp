/*
 * Copyright 2012, Adrien Destugues <pulkomandy@gmail.com>
 * Distributed under the terms of the MIT License.
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

	if (argc != 1 && argc != 3) {
		printf("Usage: %s [<frames per part> <parts>]\n",
			argv[0]);
		return 0;
	}

	if (argc == 3) {
		size_t size = strtoul(argv[1], NULL, 10);
		if (size > 0)
			framesPerBufferPart = size;

		size = strtoul(argv[2], NULL,  10);
		if (size > 0)
			bufferPartCount = size;
	}

	printf("frames per buffer part: %ld\n", framesPerBufferPart);
	printf("buffer part count: %ld\n", bufferPartCount);

	gs_audio_format gsFormat;
	memset(&gsFormat, 0, sizeof(gsFormat));
	gsFormat.frame_rate = 48000;
	gsFormat.channel_count = 1;
	gsFormat.format = gs_audio_format::B_GS_S16;
	gsFormat.byte_order = B_MEDIA_LITTLE_ENDIAN;
	gsFormat.buffer_size = framesPerBufferPart;

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

	key_info keyInfo;

	size_t sampleCount = framesPerBufferPart * bufferPartCount;
	for(size_t pos = 0; pos < sampleCount; pos++)
	{
		*(int16_t*)(buffer + pos * sizeof(int16_t))
			= (int16_t)(2000 * sin(pos * gsFormat.frame_rate
					/ ((float)sampleCount * 440)));
	}

	while (true) {
		usleep(1000000 * framesPerBufferPart / gsFormat.frame_rate);

		// check escape key state
		if (get_key_info(&keyInfo) != B_OK) {
			printf("\nkeyboard state read error\n");
			break;
		}
		if ((keyInfo.key_states[0] & 0x40) != 0)
			break;
	}

	pushGameSound.StopPlaying();

	printf("\nfinished.\n");

	return 0;
}
