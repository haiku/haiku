/*
 * Copyright 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 * Copyright 2011, Adrien Destugues (pulkomandy@pulkomandy.ath.cx)
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GameSoundDefs.h>
#include <InterfaceDefs.h>
#include <PushGameSound.h>
#include <View.h>


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
		if (size == 1) {
			printf("at least 2 buffer parts are needed\n");
			return 1;
		}
		if (size > 0)
			bufferPartCount = size;
	}

	printf("frames per buffer part: %ld\n", framesPerBufferPart);
	printf("buffer part count: %ld\n", bufferPartCount);

	gs_audio_format gsFormat;
	memset(&gsFormat, 0, sizeof(gsFormat));
	gsFormat.frame_rate = 44100;
	gsFormat.channel_count = 1;
	gsFormat.format = gs_audio_format::B_GS_U8;

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

	for (int i = 0; i < bufferSize; i++)
		buffer[i] = i;

	if (pushGameSound.StartPlaying() != B_OK) {
		printf("cannot start playback\n");
		return 1;
	}

	getchar();

	pushGameSound.StopPlaying();

	printf("\nfinished.\n");

	return 0;
}
