/*
 * Copyright (c) 2004, Marcus Overhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <string.h>
#include <MediaFormats.h>
#include "matroska_codecs.h"

#define TRACE_MATROSKA
#ifdef TRACE_MATROSKA
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)

struct bitmap_info_header
{
	uint32 size;
	uint32 width;
	uint32 height;
	uint16 planes;
	uint16 bit_count;
	uint32 compression;
	uint32 image_size;
	uint32 x_pels_per_meter;
	uint32 y_pels_per_meter;
	uint32 clr_used;
	uint32 clr_important;
} _PACKED;

#include "ogg/ogg.h"
#include <vector>



status_t
GetAudioFormat(media_format *format, const char *codec, void *private_data, int private_size)
{
	TRACE("private_data: codec '%s', private data size %d\n", codec, private_size);

	BMediaFormats formats;
	media_format_description description;
	
	if (IS_CODEC(codec, "A_VORBIS")) {
		description.family = B_MISC_FORMAT_FAMILY;
		description.u.misc.file_format = 'OggS';
		description.u.misc.codec = 'vorb';
		if (B_OK != formats.GetFormatFor(description, format)) 
			format->type = B_MEDIA_ENCODED_AUDIO;

		uint8_t * data_bytes = (uint8_t*)private_data;
		int packet_count = data_bytes[0] + 1;
		if (packet_count != 3) {
			return B_ERROR;
		}

		int info_packet_size = data_bytes[1];
		int comment_packet_size = data_bytes[2];
		int codebook_packet_size = private_size - info_packet_size - comment_packet_size - 3;

		static uint8_t * vorbis_bytes = new uint8_t[private_size - 3];
		memcpy(vorbis_bytes, &(data_bytes[3]), private_size - 3);

		ogg_packet packet;
		static std::vector<ogg_packet> header_packets;

		packet.b_o_s = 1;
		packet.e_o_s = 0;
		packet.granulepos = 0;
		packet.packetno = 0;
		packet.packet = &(vorbis_bytes[0]);
		packet.bytes = info_packet_size;
		header_packets.push_back(packet);

		packet.b_o_s = 0;
		packet.e_o_s = 0;
		packet.granulepos = 0;
		packet.packetno = 1;
		packet.packet = &(vorbis_bytes[info_packet_size]);
		packet.bytes = comment_packet_size;
		header_packets.push_back(packet);

		packet.b_o_s = 0;
		packet.e_o_s = 0;
		packet.granulepos = 0;
		packet.packetno = 2;
		packet.packet = &(vorbis_bytes[info_packet_size+comment_packet_size]);
		packet.bytes = codebook_packet_size;
		header_packets.push_back(packet);

		format->SetMetaData(&header_packets, sizeof(header_packets));

		return B_OK;
	}

	if (IS_CODEC(codec, "A_MPEG/L3")) {
		description.family = B_MPEG_FORMAT_FAMILY;
		description.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_3;
		if (formats.GetFormatFor(description, format) != B_OK) 
			format->type = B_MEDIA_ENCODED_AUDIO;

		return B_OK;
	}

	if (IS_CODEC(codec, "A_AC3")) {
		description.family =  B_WAV_FORMAT_FAMILY;
		description.u.wav.codec = 0x2000;

		if (B_OK != formats.GetFormatFor(description, format)) 
			format->type = B_MEDIA_ENCODED_AUDIO;

		// Set the DecoderConfigSize (Not that haiku seems to use it)
		if (private_size > 0) {
			TRACE("AC3 private data found, size is %d\n", private_size);
			if (format->SetMetaData(private_data, private_size) != B_OK) {
				ERROR("Failed to set Decoder Config\n");
				return B_ERROR;
			}
		}

		return B_OK;
	}
	
	// The codec name only has to start with A_AAC to be AAC
	// The rest is some sort of hint for creating the codec specific data
	if (IS_CODEC(codec, "A_AAC")) {
		uint64 misc_codec = 'mp4a';
		media_format_description description;
		description.family = B_MISC_FORMAT_FAMILY;
		description.u.misc.file_format = (uint32)(misc_codec >> 32);
		description.u.misc.codec = (uint32)misc_codec;
		if (formats.GetFormatFor(description, format) != B_OK) 
			format->type = B_MEDIA_ENCODED_AUDIO;

		// Set the DecoderConfigSize (Not that haiku seems to use it)
		if (private_size > 0) {
			TRACE("AAC private data found, size is %d\n", private_size);
			if (format->SetMetaData(private_data, private_size) != B_OK) {
				ERROR("Failed to set Decoder Config\n");
				return B_ERROR;
			}
		}

		return B_OK;
	}
	
	return B_ERROR;
}


status_t
GetVideoFormat(media_format *format, const char *codec, void *private_data, int private_size)
{
	TRACE("private_data: codec '%s', private data size %d\n", codec, private_size);
	
	BMediaFormats formats;
	media_format_description description;

	if (IS_CODEC(codec, "V_MS/VFW/FOURCC")) {
		if (private_size < (int)sizeof(bitmap_info_header)) {
			TRACE("private_size too small!\n");
			return B_ERROR;
		}
		const bitmap_info_header *bih = (const bitmap_info_header *)private_data;

		description.family = B_AVI_FORMAT_FAMILY;
		description.u.avi.codec = bih->compression;
		if (B_OK != formats.GetFormatFor(description, format)) 
			format->type = B_MEDIA_ENCODED_VIDEO;

		format->user_data_type = B_CODEC_TYPE_INFO;
		*(uint32 *)format->user_data = bih->compression; format->user_data[4] = 0;

		return B_OK;
	}
	if (IS_CODEC(codec, "V_MPEG4/ISO/AVC")) {
		uint64 codecid = 'avc1';

		description.family = B_QUICKTIME_FORMAT_FAMILY;
		description.u.quicktime.codec = codecid;
		if (B_OK != formats.GetFormatFor(description, format)) 
			format->type = B_MEDIA_ENCODED_VIDEO;

		format->user_data_type = B_CODEC_TYPE_INFO;
		*(uint32 *)format->user_data = codecid; format->user_data[4] = 0;

		// Set the DecoderConfigSize (Not that haiku seems to use it)
		if (private_size > 0) {
			TRACE("AVC private data found, size is %d\n", private_size);
			if (format->SetMetaData(private_data, private_size) != B_OK) {
				ERROR("Failed to set Decoder Config\n");
				return B_ERROR;
			}
		}

		return B_OK;
	}
	TRACE("not a codec!\n");

	return B_ERROR;
}

