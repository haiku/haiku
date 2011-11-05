/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// MediaStrings.h (Cortex/Support)
//
// * PURPOSE
//   Provide routines to translate MediaKit objects, values
//   and constants into human-readable text
//
// * HISTORY
//   c.lenz		2nov99			Begun
//	 c.lenz		11nov99			Format strings are now aware
//								of wildcard-fields
//	 c.lenz		25dec99			Added functions to retrieve strings
//								for single format fields
//

#ifndef __MediaString_H__
#define __MediaString_H__

// Interface Kit
#include <InterfaceDefs.h>
// Media Kit
#include <MediaDefs.h>
#include <MediaNode.h>

class BString;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class MediaString
{

public:						// media_node strings

	static BString			getStringFor(
								node_kind kinds,
								bool complete = true);
	static BString			getStringFor(
								BMediaNode::run_mode runMode,
								bool complete = true);

public:						// media_format strings

	static BString			getStringFor(
								const media_format &format,
								bool complete = true);
	static BString			getStringFor(
								media_format_family family,
								bool complete = true);
	static BString			getStringFor(
								media_type type,
								bool complete = true);
	static BString			getStringFor(
								const media_multi_audio_format &format,
								bool complete = true);
	static BString			getStringFor(
								const media_raw_video_format &format,
								bool complete = true);
	static BString			getStringFor(
								const media_encoded_audio_format &format,
								bool complete = true);
	static BString			getStringFor(
								const media_encoded_video_format &format,
								bool complete = true);
	static BString			getStringFor(
								const media_multistream_format &format,
								bool complete = true);

public:						// media_source / media_destination strings

	static BString			getStringFor(
								const media_source &source,
								bool complete = true);
	static BString			getStringFor(
								const media_destination &destination,
								bool complete = true);

public:						// strings for single fields in media_raw_audio_format

	static BString			forAudioFormat(
								uint32 format,
								int32 validBits);
	static BString			forAudioFrameRate(
								float frameRate);
	static BString			forAudioChannelCount(
								uint32 channelCount);
	static BString			forAudioByteOrder(
								uint32 byteOrder);
	static BString			forAudioBufferSize(
								size_t bufferSize);
	static BString			forAudioChannelMask(
								uint32 channelMask);
	static BString			forAudioMatrixMask(
								uint16 matrixMask);

public:						// strings for single fields in media_encoded_audio_format

	static BString			forAudioBitRate(
								float bitRate);
	static BString			forAudioFrameSize(
								size_t frameSize);

public:						// strings for single fields in media_raw_video_format

	static BString			forVideoFormat(
								color_space format);
	static BString			forVideoFieldRate(
								float fieldRate,
								uint32 interlace);
	static BString			forVideoResolution(
								uint32 lineWidth,
								uint32 lineCount);
	static BString			forVideoAspectRatio(
								uint16 pixelWidth,
								uint16 pixelHeight);
	static BString			forVideoOrientation(
								uint32 orientation);
	static BString			forVideoActiveLines(
								uint32 firstActive,
								uint32 lastActive);
	static BString			forVideoBytesPerRow(
								uint32 bytesPerRow);
	static BString			forVideoOffset(
								uint32 pixelOffset,
								uint32 lineOffset);

public:						// strings for single fields in media_encoded_video_format

	static BString			forVideoBitRate(
								float avgBitRate,
								float maxBitRate);
	static BString			forVideoFrameSize(
								size_t frameSize);
	static BString			forVideoHistory(
								int16 forwardHistory,
								int16 backwardHistory);

public:						// strings for single fields media_multistream_format

	static BString			forMultistreamFormat(
								int32 format);
	static BString			forMultistreamBitRate(
								float avgBitRate,
								float maxBitRate);
	static BString			forMultistreamChunkSize(
								uint32 avgChunkSize,
								uint32 maxChunkSize);
	static BString			forMultistreamFlags(
								uint32 flags);
};

__END_CORTEX_NAMESPACE
#endif /* __MediaStrings_H__ */
