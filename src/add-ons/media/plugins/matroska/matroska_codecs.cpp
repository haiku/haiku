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

#define IS_CODEC(a,b) !memcmp(a, b, strlen(b))

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
	printf("GetAudioFormat: codec '%s', private data size %d\n", codec, private_size);
	
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
	
	if (IS_CODEC(codec, "A_AAC/MPEG4/LC/SBR")) {
	}
	
	return B_ERROR;
}


status_t
GetVideoFormat(media_format *format, const char *codec, void *private_data, int private_size)
{
	printf("private_data: codec '%s', private data size %d\n", codec, private_size);
	
	BMediaFormats formats;
	media_format_description description;

	if (IS_CODEC(codec, "V_MS/VFW/FOURCC")) {
		if (private_size < (int)sizeof(bitmap_info_header)) {
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

	return B_ERROR;
}

