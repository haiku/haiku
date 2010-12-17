/*
 * Copyright (c) 2004-2010 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <media/MediaFormats.h>
#include <media/MediaNode.h>
#include <stdio.h>
#include "MediaFormat.h"

#define UINT64_C(c) (c ## ULL)
extern "C" {
  #include "avcodec.h"
}

void
PrintFormat(const media_format &format)
{
	char s[200];
	string_for_format(format, s, sizeof(s));
	printf("%s\n", s);
}


void
PrintFormat(const media_output &output)
{
	PrintFormat(output.format);
}


static status_t
GetHeaderFormatAc3Audio(media_format *out_format, const uint8 *header, size_t size)
{
	printf("GetHeaderFormatAc3Audio\n");

	status_t status;
	media_format_description desc;
//	desc.family = B_WAV_FORMAT_FAMILY;
//	desc.u.wav.codec = 0x2000;
	desc.family = B_MISC_FORMAT_FAMILY;
	desc.u.misc.file_format = 'ffmp';
	desc.u.misc.codec = CODEC_ID_AC3;

	BMediaFormats formats;	
	status = formats.InitCheck();
	if (status) {
		printf("formats.InitCheck failed, error %lu\n", status);
		return status;
	}
	
	status = formats.GetFormatFor(desc, out_format);
	if (status) {
		printf("formats.GetFormatFor failed, error %lu\n", status);
		return status;
	}
	
	return B_OK;
}



static status_t
GetHeaderFormatDtsAudio(media_format *out_format, const uint8 *header, size_t size)
{
	printf("GetHeaderFormatDtsAudio: unsupported\n");
	return B_ERROR;
}


static status_t
GetHeaderFormatLpcmAudio(media_format *out_format, const uint8 *header, size_t size)
{
	printf("GetHeaderFormatLpcmAudio: unsupported\n");
	return B_ERROR;
}


static status_t
GetHeaderFormatPrivateStream(media_format *out_format, const uint8 *header, size_t size)
{
	printf("GetHeaderFormatPrivateStream: unsupported, assuming AC3\n");
	return GetHeaderFormatAc3Audio(out_format, header, size);
}


static status_t
GetHeaderFormatMpegAudio(media_format *out_format, const uint8 *header, size_t size)
{
	printf("GetHeaderFormatMpegAudio\n");

	status_t status;
	media_format_description desc;
//	desc.family = B_MPEG_FORMAT_FAMILY;
//	desc.u.mpeg.id = B_MPEG_2_AUDIO_LAYER_2;
	desc.family = B_MISC_FORMAT_FAMILY;
	desc.u.misc.file_format = 'ffmp';
	desc.u.misc.codec = CODEC_ID_MP3;
	
	
	BMediaFormats formats;	
	status = formats.InitCheck();
	if (status) {
		printf("formats.InitCheck failed, error %lu\n", status);
		return status;
	}
	
	status = formats.GetFormatFor(desc, out_format);
	if (status) {
		printf("formats.GetFormatFor failed, error %lu\n", status);
		return status;
	}

	out_format->u.encoded_audio.output.frame_rate = 48000;
	out_format->u.encoded_audio.output.channel_count = 2;
	out_format->u.encoded_audio.output.buffer_size = 1024;
	return B_OK;
}


static status_t
GetHeaderFormatMpegVideo(media_format *out_format, const uint8 *header, size_t size)
{
	printf("GetHeaderFormatMpegVideo\n");

	status_t status;
	media_format_description desc;
//	desc.family = B_MPEG_FORMAT_FAMILY;
//	desc.u.mpeg.id = B_MPEG_2_VIDEO;
	desc.family = B_MISC_FORMAT_FAMILY;
	desc.u.misc.file_format = 'ffmp';
	desc.u.misc.codec = CODEC_ID_MPEG2VIDEO;

	BMediaFormats formats;
	status = formats.InitCheck();
	if (status) {
		printf("formats.InitCheck failed, error %lu\n", status);
		return status;
	}
	
	status = formats.GetFormatFor(desc, out_format);
	if (status) {
		printf("formats.GetFormatFor failed, error %lu\n", status);
		return status;
	}

	return B_OK;
}


status_t
GetHeaderFormat(media_format *out_format, const void *header, size_t size, int stream_id)
{
	const uint8 *h = (const uint8 *)header;
	status_t status;

	printf("GetHeaderFormat: stream_id %02x\n", stream_id);
	printf("inner frame header: "
		   "%02x %02x %02x %02x %02x %02x %02x %02x "
		   "%02x %02x %02x %02x %02x %02x %02x %02x\n",
		   h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7], 
		   h[8], h[9], h[10], h[11], h[12], h[13], h[14], h[15]);

	if (stream_id >= 0x80 && stream_id <= 0x87)
		status = GetHeaderFormatAc3Audio(out_format, h, size);
	else if (stream_id >= 0x88 && stream_id <= 0x8F)
		status = GetHeaderFormatDtsAudio(out_format, h, size);
	else if (stream_id >= 0xA0 && stream_id <= 0xA7)
		status = GetHeaderFormatLpcmAudio(out_format, h, size);
	else if (stream_id == 0xBD)
		status = GetHeaderFormatPrivateStream(out_format, h, size);
	else if (stream_id >= 0xC0 && stream_id <= 0xDF)
		status = GetHeaderFormatMpegAudio(out_format, h, size);
	else if (stream_id >= 0xE0 && stream_id <= 0xEF)
		status = GetHeaderFormatMpegVideo(out_format, h, size);
	else {
		printf("GetHeaderFormat: don't know what this stream_id means\n");
		status = B_ERROR;
	}
	
	if (status != B_OK) {
		printf("GetHeaderFormat: failed!\n");
	} else {
		printf("GetHeaderFormat: out_format ");
		PrintFormat(*out_format);
	}
	return status;
}
