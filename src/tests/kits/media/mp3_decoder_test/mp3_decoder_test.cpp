/*
 * Copyright 2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Colin Günther, coling@gmx.de
 */


/*!	Tests audio stream decoding functionality of the FFMPEG decoder plugin.

	This test is designed with testing the dvb media-addon audio decoding
	capability in mind. Thus we are restricting this test to MP3-Audio.

	The test requires a MP3 test file at the same directory you start the
	test from. Normally there is a test file included at the same location
	this source file is located if not have a look at the git history.

	Successful completion of this test results in audio being played on the
	standard system audio output. So turn on your speakers or put on your head
	phones.

	The originally included test file results in an audio signal containing
	jingles.
	This test file has the following properties:
		- encoded_audio.output.frame_rate = 48000
		- encoded_audio.output.channel_count = 2
		- encoded_audio.output.buffer_size = 1024
		- encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT

	In any way, there -MUST- be no need to properly initialize those audio
	properties for this test to succeed. To put it in other terms: The
	FFMPEG decoder plugin should determine those properties by its own and
	decode the audio accordingly.

*/


#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <AppKit.h>
#include <InterfaceKit.h>
#include <MediaKit.h>
#include <SupportKit.h>

#include <media/Buffer.h>
#include <media/BufferGroup.h>
#include <media/MediaDecoder.h>
#include <media/Sound.h>
#include <media/SoundPlayer.h>
#include <storage/File.h>
#include <support/Errors.h>


extern "C" {
	#include "avcodec.h"

#ifdef DEBUG
  // Needed to fix debug build, otherwise the linker complains about
  // "undefined reference to `ff_log2_tab'"
  const uint8_t ff_log2_tab[256] = {0};
#endif

}  // extern "C"


const char*	kTestAudioFilename = "./AVCodecTestMp3AudioStreamRaw";
	

class FileDecoder : public BMediaDecoder {
private:
	BFile* sourceFile;

public:
	FileDecoder(BFile* file) : BMediaDecoder() {
		sourceFile = file;
	}

protected:
	virtual status_t	GetNextChunk(const void** chunkData, size_t* chunkLen,
							media_header* mh) {
		static const uint kReadSizeInBytes = 4096;

		memset(mh, 0, sizeof(media_header));

		void* fileData = malloc(kReadSizeInBytes);
		ssize_t readLength = this->sourceFile->Read(fileData,
			kReadSizeInBytes);
		if (readLength < 0)
			return B_ERROR;

		if (readLength == 0)
			return B_LAST_BUFFER_ERROR;

		*chunkData = fileData;
		*chunkLen = readLength;

		return B_OK;
	}
};


typedef struct cookie_decode {
	BFile* inputFile;
	BMediaDecoder* decoder;
	BBufferGroup* decodedDataGroup;
	uint32 decodedDataBufferSizeMax;
	pthread_cond_t playingFinishedCondition;
} cookie_decode;


status_t InitializeMp3DecodingCookie(cookie_decode* cookie);
void FreeMp3DecodingCookie(cookie_decode* cookie);
media_format* CreateMp3MediaFormat();
media_format CreateRawMediaFormat();
void Mp3Decoding(void* cookie, void* buffer, size_t bufferSize,
	const media_raw_audio_format& format);


int
main(int argc, char* argv[])
{
	BApplication app("application/x-vnd.mp3-decoder-test");

	cookie_decode decodingCookie;
	if (InitializeMp3DecodingCookie(&decodingCookie) != B_OK)
		exit(1);

	media_format rawAudioFormat = CreateRawMediaFormat();

	media_raw_audio_format* audioOutputFormat
		= &rawAudioFormat.u.raw_audio;

	BSoundPlayer player(audioOutputFormat, "wave_player", Mp3Decoding,
		NULL, &decodingCookie);
	player.Start();
	player.SetHasData(true);
	player.SetVolume(0.5);

	// Wait as long as we are playing sound
	pthread_mutex_t playingFinishedMutex;
	pthread_mutex_init(&playingFinishedMutex, NULL);
	pthread_mutex_lock(&playingFinishedMutex);
	pthread_cond_wait(&decodingCookie.playingFinishedCondition,
		&playingFinishedMutex);

	player.SetHasData(false);
	player.Stop();

	// Cleaning up
	FreeMp3DecodingCookie(&decodingCookie);
	pthread_mutex_destroy(&playingFinishedMutex);
}


status_t
InitializeMp3DecodingCookie(cookie_decode* cookie)
{
	cookie->inputFile = new BFile(kTestAudioFilename, O_RDONLY);
	cookie->decoder = new FileDecoder(cookie->inputFile); 

	media_format* mp3MediaFormat = CreateMp3MediaFormat();
	cookie->decoder->SetTo(mp3MediaFormat);
	status_t settingDecoderStatus = cookie->decoder->InitCheck();
	if (settingDecoderStatus < B_OK)
		return B_ERROR;

	media_format rawMediaFormat = CreateRawMediaFormat();
	status_t settingDecoderOutputStatus
		= cookie->decoder->SetOutputFormat(&rawMediaFormat);
	if (settingDecoderOutputStatus < B_OK)
		return B_ERROR;

	cookie->decodedDataBufferSizeMax
		= rawMediaFormat.u.raw_audio.buffer_size * 3;
	cookie->decodedDataGroup
		= new BBufferGroup(cookie->decodedDataBufferSizeMax, 25);

	if (pthread_cond_init(&cookie->playingFinishedCondition, NULL) < 0)
		return B_ERROR;

	return B_OK;
}


void
FreeMp3DecodingCookie(cookie_decode* cookie)
{
	pthread_cond_destroy(&cookie->playingFinishedCondition);
	cookie->decodedDataGroup->ReclaimAllBuffers();
	free(cookie->decodedDataGroup);
	free(cookie->decoder);
	free(cookie->inputFile);
}


/*!	The caller takes ownership of the returned media_format value.
	Thus the caller needs to free the returned value.
	The returned value may be NULL, when there was an error.
*/
media_format*
CreateMp3MediaFormat()
{
	// copy 'n' paste from src/add-ons/media/media-add-ons/dvb/MediaFormat.cpp:
	// GetHeaderFormatMpegAudio()
	status_t status;
	media_format_description desc;
	desc.family = B_MISC_FORMAT_FAMILY;
	desc.u.misc.file_format = 'ffmp';
	desc.u.misc.codec = CODEC_ID_MP3;
	static media_format* sNoMp3MediaFormat = NULL;

	BMediaFormats formats;
	status = formats.InitCheck();
	if (status < B_OK) {
		printf("formats.InitCheck failed, error %lu\n", status);
		return sNoMp3MediaFormat;
	}

	media_format* mp3MediaFormat
		= static_cast<media_format*>(malloc(sizeof(media_format)));
	memset(mp3MediaFormat, 0, sizeof(media_format));
	status = formats.GetFormatFor(desc, mp3MediaFormat);
	if (status < B_OK) {
		printf("formats.GetFormatFor failed, error %lu\n", status);
		return sNoMp3MediaFormat;
	}

	return mp3MediaFormat;
}


media_format
CreateRawMediaFormat()
{
	media_format rawMediaFormat;
	memset(&rawMediaFormat, 0, sizeof(media_format));

	rawMediaFormat.type = B_MEDIA_RAW_AUDIO;
	rawMediaFormat.u.raw_audio.frame_rate = 48000;
	rawMediaFormat.u.raw_audio.channel_count = 2;
	rawMediaFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	rawMediaFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
	rawMediaFormat.u.raw_audio.buffer_size = 32768;
		// comment from src/add-ons/media/media-add-ons/dvb/
		// DVBMediaNode.cpp::InitDefaultFormats(): when set to anything
		// different from 32768 haiku mixer has problems

	return rawMediaFormat;
}


void
Mp3Decoding(void* cookie, void* buffer, size_t bufferSize,
	const media_raw_audio_format& format)
{
	cookie_decode* decodingCookie = static_cast<cookie_decode*>(cookie);
	int64 rawAudioFrameCount = 0;
	media_header mh;
	status_t decodingAudioFramesStatus
		= decodingCookie->decoder->Decode(buffer, &rawAudioFrameCount, &mh,
			NULL);
	if (decodingAudioFramesStatus < B_OK) {
		sleep(2);
			// Give the player some time to catch up playing all decoded data.
			// The player may still have some data to play, even though we run
			// out of new data, so don't just tell that we finished playing
			// quiet yet.
		pthread_cond_signal(&decodingCookie->playingFinishedCondition);
	}
}
