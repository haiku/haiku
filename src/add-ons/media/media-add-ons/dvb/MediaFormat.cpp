/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
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
#include <stdio.h>
#include "MediaFormat.h"

void
PrintFormat(const media_output &output)
{
	PrintFormat(output.format);
}


void
PrintFormat(const media_format &format)
{
	char s[200];
	string_for_format(format, s, sizeof(s));
	printf("%s\n", s);
}


status_t
GetHeaderFormat(media_format *out_format, const void *header, size_t size, int stream_id)
{
	
	const uint8 *d = (const uint8 *)header;
	printf("GetHeaderFormat: stream_id %02x\n", stream_id);
	printf("frame header: "
		   "%02x %02x %02x %02x %02x %02x %02x %02x "
		   "%02x %02x %02x %02x %02x %02x %02x %02x\n",
		   d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], 
		   d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);

	if (d[0] == 0xff && d[1] == 0xfc) {
		
		printf("GetHeaderFormat: assuming mepeg audio\n");
		
		status_t err;
	
		media_format_description desc;
		desc.family = B_MPEG_FORMAT_FAMILY;
		desc.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_3;

		BMediaFormats formats;
		media_format format;
	
		err = formats.InitCheck();
		if (err)
			return err;
	
		err = formats.GetFormatFor(desc, out_format);
		if (err)
			return err;

		out_format->u.encoded_audio.output.frame_rate = 48000;
		out_format->u.encoded_audio.output.channel_count = 2;
		out_format->u.encoded_audio.output.buffer_size = 1024;

		printf("GetHeaderFormat: ");
		PrintFormat(*out_format);
	} else {
		
		printf("GetHeaderFormat: assuming mepeg video\n");
		
		status_t err;
	
		media_format_description desc;
		desc.family = B_MPEG_FORMAT_FAMILY;
		desc.u.mpeg.id = B_MPEG_1_VIDEO;

		BMediaFormats formats;
		media_format format;
	
		err = formats.InitCheck();
		if (err)
			return err;
	
		err = formats.GetFormatFor(desc, out_format);
		if (err)
			return err;
		
	}

	return B_OK;
}
