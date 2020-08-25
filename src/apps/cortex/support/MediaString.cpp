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


// MediaString.cpp

#include "MediaString.h"

// Media Kit
#include <MediaFormats.h>
// Support Kit
#include <String.h>

#include <Catalog.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)

static BCatalog sCatalog("application/Haiku-cortex-support");

// -------------------------------------------------------- //
// *** media_node strings (public)
// -------------------------------------------------------- //

BString	MediaString::getStringFor(
	node_kind kinds,
	bool complete) {
	D_METHOD(("MediaString::getStringFor(node_kind)\n"));

	BString list, last;
	bool first = true;

	if (kinds & B_BUFFER_PRODUCER) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Buffer producer"));
			first = false;
		}
	}
	if (kinds & B_BUFFER_CONSUMER) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Buffer consumer"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Buffer consumer"));
		}
	}
	if (kinds & B_TIME_SOURCE) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Time source"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Time source"));
		}
	}
	if (kinds & B_CONTROLLABLE) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Controllable"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Controllable"));
		}
	}
	if (kinds & B_FILE_INTERFACE) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("File interface"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("File interface"));
		}
	}
	if (kinds & B_ENTITY_INTERFACE) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Entity interface"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Entity interface"));
		}
	}
	if (kinds & B_PHYSICAL_INPUT) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Physical input"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Physical input"));
		}
	}
	if (kinds & B_PHYSICAL_OUTPUT) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Physical output"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Physical output"));
		}
	}
	if (kinds & B_SYSTEM_MIXER) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("System mixer"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("System mixer"));
		}
	}

	if (last != "")
		list << " & " << last;
	return list;
}

BString MediaString::getStringFor(
	BMediaNode::run_mode runMode,
	bool complete)
{
	D_METHOD(("MediaString::getStringFor(run_mode)\n"));

	switch (runMode) {
		case BMediaNode::B_OFFLINE:
			return sCatalog.GetString(B_TRANSLATE_MARK("Offline"));
		case BMediaNode::B_RECORDING:
			return sCatalog.GetString(B_TRANSLATE_MARK("Recording"));
		case BMediaNode::B_DECREASE_PRECISION:
			return sCatalog.GetString(B_TRANSLATE_MARK("Decrease precision"));
		case BMediaNode::B_INCREASE_LATENCY:
			return sCatalog.GetString(B_TRANSLATE_MARK("Increase latency"));
		case BMediaNode::B_DROP_DATA:
			return sCatalog.GetString(B_TRANSLATE_MARK("Drop data"));
		default:
			return sCatalog.GetString(B_TRANSLATE_MARK("(unknown run mode)"));
	}
}

// -------------------------------------------------------- //
// *** media_format strings (public)
// -------------------------------------------------------- //

BString	MediaString::getStringFor(
	media_type type,
	bool complete) {
	D_METHOD(("MediaString::getStringFor(media_type)\n"));

	switch (type) {
		case B_MEDIA_NO_TYPE:
			return sCatalog.GetString(B_TRANSLATE_MARK("Typeless media"));
		case B_MEDIA_UNKNOWN_TYPE:
			return sCatalog.GetString(B_TRANSLATE_MARK("Unknown media type"));
		case B_MEDIA_RAW_AUDIO:
			return sCatalog.GetString(B_TRANSLATE_MARK("Raw audio"));
		case B_MEDIA_RAW_VIDEO:
			return sCatalog.GetString(B_TRANSLATE_MARK("Raw video"));
		case B_MEDIA_VBL:
			return sCatalog.GetString(B_TRANSLATE_MARK("Raw data from VBL area"));
		case B_MEDIA_TIMECODE:
			return sCatalog.GetString(B_TRANSLATE_MARK("Timecode"));
		case B_MEDIA_MIDI:
			return sCatalog.GetString(B_TRANSLATE_MARK("MIDI"));
		case B_MEDIA_TEXT:
			return sCatalog.GetString(B_TRANSLATE_MARK("Text"));
		case B_MEDIA_HTML:
			return sCatalog.GetString(B_TRANSLATE_MARK("HTML"));
		case B_MEDIA_MULTISTREAM:
			return sCatalog.GetString(B_TRANSLATE_MARK("Multistream media"));
		case B_MEDIA_PARAMETERS:
			return sCatalog.GetString(B_TRANSLATE_MARK("Parameters"));
		case B_MEDIA_ENCODED_AUDIO:
			return sCatalog.GetString(B_TRANSLATE_MARK("Encoded audio"));
		case B_MEDIA_ENCODED_VIDEO:
			return sCatalog.GetString(B_TRANSLATE_MARK("Encoded video"));
		default: {
			if (type >= B_MEDIA_FIRST_USER_TYPE) {
				return sCatalog.GetString(B_TRANSLATE_MARK(
					"User-defined media type"));
			}
			if (type >= B_MEDIA_PRIVATE) {
				return sCatalog.GetString(B_TRANSLATE_MARK(
					"Private Be media type"));
			}
		}
	}
	return "Unknown Media Type";
}

BString	MediaString::getStringFor(
	media_format_family family,
	bool complete) {
	D_METHOD(("MediaString::getStringFor(media_format_family)\n"));

	switch (family) {
		case B_ANY_FORMAT_FAMILY:
			return sCatalog.GetString(B_TRANSLATE_MARK("Any format family"));
		case B_BEOS_FORMAT_FAMILY:
			return sCatalog.GetString(B_TRANSLATE_MARK("BeOS format family"));
		case B_QUICKTIME_FORMAT_FAMILY:
			return sCatalog.GetString(B_TRANSLATE_MARK("QuickTime format family"));
		case B_AVI_FORMAT_FAMILY:
			return sCatalog.GetString(B_TRANSLATE_MARK("AVI format family"));
		case B_ASF_FORMAT_FAMILY:
			return sCatalog.GetString(B_TRANSLATE_MARK("ASF format family"));
		case B_MPEG_FORMAT_FAMILY:
			return sCatalog.GetString(B_TRANSLATE_MARK("MPEG format family"));
		case B_WAV_FORMAT_FAMILY:
			return sCatalog.GetString(B_TRANSLATE_MARK("WAV format family"));
		case B_AIFF_FORMAT_FAMILY:
			return sCatalog.GetString(B_TRANSLATE_MARK("AIFF format family"));
		default:
			return sCatalog.GetString(B_TRANSLATE_MARK("Miscellaneous format family"));
	}
}

BString MediaString::getStringFor(
	const media_multi_audio_format &format,
	bool complete) {
	D_METHOD(("MediaString::getStringFor(media_raw_audio_format)\n"));

	BString s = "";
	bool empty = true;

	// sample rate
	if (format.frame_rate != media_raw_audio_format::wildcard.frame_rate) {
		s << (empty ? "" : ", ") << forAudioFrameRate(format.frame_rate);
		empty = false;
	}

	// format
	if (format.format != media_raw_audio_format::wildcard.format) {
		s << (empty ? "" : ", ") << forAudioFormat(format.format, format.valid_bits);
		empty = false;
	}

	// channels
	if (format.channel_count != media_raw_audio_format::wildcard.channel_count) {
		s << (empty ? "" : ", ") << forAudioChannelCount(format.channel_count);
		empty = false;
	}

	if (complete) {
		// channel mask
		if (format.channel_mask != media_multi_audio_format::wildcard.channel_mask) {
			s << (empty ? "" : " ") << "(" << forAudioChannelMask(format.channel_mask) << ")";
			empty = false;
		}

		// matrix mask
		if (format.matrix_mask != media_multi_audio_format::wildcard.matrix_mask) {
			s << (empty ? "" : " ") << "(" << forAudioMatrixMask(format.matrix_mask) << ")";
			empty = false;
		}

		// endianess
		if (format.byte_order != media_raw_audio_format::wildcard.byte_order) {
			s << (empty ? "" : ", ") << forAudioByteOrder(format.byte_order);
			empty = false;
		}

		// buffer size
		if (format.buffer_size != media_raw_audio_format::wildcard.buffer_size) {
			s << (empty ? "" : ", ") << forAudioBufferSize(format.buffer_size);
			empty = false;
		}
	}

	return s;
}

BString MediaString::getStringFor(
	const media_raw_video_format &format,
	bool complete) {
	D_METHOD(("MediaString::getStringFor(media_raw_video_format)\n"));

	BString s = "";
	bool empty = true;

	// format / color space
	if (format.display.format != media_video_display_info::wildcard.format) {
		s << (empty ? "" : ", ") << forVideoFormat(format.display.format);
		empty = false;
	}

	// resolution
	if ((format.display.line_width != media_video_display_info::wildcard.line_width)
	 && (format.display.line_count != media_video_display_info::wildcard.line_count)) {
		s << (empty ? "" : ", ") << forVideoResolution(format.display.line_width,
													   format.display.line_count);
	}

	// field rate / interlace
	if (format.field_rate != media_raw_video_format::wildcard.field_rate) {
		s << (empty ? "" : ", ") << forVideoFieldRate(format.field_rate,
													  format.interlace);
		empty = false;
	}

	if (complete) {

		// aspect ratio
		if ((format.pixel_width_aspect != media_raw_video_format::wildcard.pixel_width_aspect)
		 && (format.pixel_height_aspect != media_raw_video_format::wildcard.pixel_height_aspect)) {
			s << (empty ? "" : ", ") << forVideoAspectRatio(format.pixel_width_aspect,
															format.pixel_height_aspect);
			empty = false;
		}

		// orientation
		if (format.orientation != media_raw_video_format::wildcard.orientation) {
			s << (empty ? "" : ", ") << forVideoOrientation(format.orientation);
			empty = false;
		}

		// active lines
		if ((format.first_active != media_raw_video_format::wildcard.first_active)
		 && (format.last_active != media_raw_video_format::wildcard.last_active)) {
			s << (empty ? "" : ", ") << forVideoActiveLines(format.first_active,
															format.last_active);
			empty = false;
		}
	}
	return s;
}

BString MediaString::getStringFor(
	const media_encoded_audio_format &format,
	bool complete) {
	D_METHOD(("MediaString::getStringFor(media_encoded_audio_format)\n"));

	BString s = "";
	bool empty = true;

	// bit rate
	if (format.bit_rate != media_encoded_audio_format::wildcard.bit_rate) {
		s << (empty ? "" : ", ") << forAudioBitRate(format.bit_rate);
		empty = false;
	}

	// frame size
	if (format.frame_size != media_encoded_audio_format::wildcard.frame_size) {
		s << (empty ? "" : ", ") << forAudioFrameSize(format.frame_size);
		empty = false;
	}
	return s;
}

BString MediaString::getStringFor(
	const media_encoded_video_format &format,
	bool complete) {
	D_METHOD(("MediaString::getStringFor(media_encoded_video_format)\n"));

	BString s = "";
	bool empty = true;

	// bit rate
	if ((format.avg_bit_rate != media_encoded_video_format::wildcard.avg_bit_rate)
	 && (format.max_bit_rate != media_encoded_video_format::wildcard.max_bit_rate))	{
		s << (empty ? "" : ", ") << forVideoBitRate(format.avg_bit_rate,
													format.max_bit_rate);
		empty = false;
	}

	if (complete) {

		// frame size
		if (format.frame_size != media_encoded_video_format::wildcard.frame_size) {
			s << (empty ? "" : ", ") << forVideoFrameSize(format.frame_size);
			empty = false;
		}

		// history
		if ((format.forward_history != media_encoded_video_format::wildcard.forward_history)
		 && (format.backward_history != media_encoded_video_format::wildcard.backward_history)) {
			s << (empty ? "" : ", ") << forVideoHistory(format.forward_history,
														format.backward_history);
			empty = false;
		}
	}

	return s;
}

BString MediaString::getStringFor(
	const media_multistream_format &format,
	bool complete) {
	D_METHOD(("MediaString::getStringFor(media_multistream_format)\n"));

	BString s = "";
	bool empty = true;
	
	// format
	if (format.format != media_multistream_format::wildcard.format) {
		s << (empty ? "" : ", ") << forMultistreamFormat(format.format);
		empty = false;
	}

	// bit rate
	if ((format.avg_bit_rate != media_multistream_format::wildcard.avg_bit_rate)
	 && (format.max_bit_rate != media_multistream_format::wildcard.max_bit_rate)) {
		s << (empty ? "" : ", ") << forMultistreamBitRate(format.avg_bit_rate,
														  format.max_bit_rate);
		empty = false;
	}

	if (complete) {

		// chunk size
		if ((format.avg_chunk_size != media_multistream_format::wildcard.avg_chunk_size)
		 && (format.max_chunk_size != media_multistream_format::wildcard.max_chunk_size)) {
			s << (empty ? "" : ", ") << forMultistreamChunkSize(format.avg_chunk_size,
																format.max_chunk_size);
			empty = false;
		}

		// flags
		if (format.flags != media_multistream_format::wildcard.flags) {
			s << (empty ? "" : ", ") << forMultistreamFlags(format.flags);
			empty = false;
		}
	}

	return s;
}

BString MediaString::getStringFor(
	const media_format &format,
	bool complete) {
	D_METHOD(("MediaString::getStringFor(media_format)\n"));

	BString s = getStringFor(format.type, complete);
	switch (format.type) {
		case B_MEDIA_RAW_AUDIO:	{
			BString formatInfo = getStringFor(format.u.raw_audio, complete);
			if (formatInfo.Length() > 0)
				s << " (" << formatInfo << ")";
			break;
		}
		case B_MEDIA_RAW_VIDEO:	{
			BString formatInfo = getStringFor(format.u.raw_video, complete);
			if (formatInfo.Length() > 0)
				s << " (" << formatInfo << ")";
			break;
		}
		case B_MEDIA_ENCODED_AUDIO:	{
			BString formatInfo = getStringFor(format.u.encoded_audio, complete);
			if (formatInfo.Length() > 0)
				s << " (" << formatInfo << ")";
			break;
		}
		case B_MEDIA_ENCODED_VIDEO: {
			BString formatInfo = getStringFor(format.u.encoded_video, complete);
			if (formatInfo.Length() > 0)
				s << " (" << formatInfo << ")";
			break;
		}
		case B_MEDIA_MULTISTREAM: {
			BString formatInfo = getStringFor(format.u.multistream, complete);
			if (formatInfo.Length() > 0)
				s << " (" << formatInfo << ")";
			break;
		}
		default: {
			break;
		}
	}
	return s;
}

// -------------------------------------------------------- //
// *** media_source / media_destination strings (public)
// -------------------------------------------------------- //

BString MediaString::getStringFor(
	const media_source &source,
	bool complete) {
	D_METHOD(("MediaString::getStringFor(media_source)\n"));

	BString s;
	if ((source.port != media_source::null.port)
	 && (source.id != media_source::null.id)) {
		s << sCatalog.GetString(B_TRANSLATE_MARK("Port ")) << source.port
			<< sCatalog.GetString(B_TRANSLATE_MARK(", ID ")) << source.id;
	}
	else {
		s = sCatalog.GetString(B_TRANSLATE_MARK("(none)"));
	}
	return s;
}

BString MediaString::getStringFor(
	const media_destination &destination,
	bool complete) {
	D_METHOD(("MediaString::getStringFor(media_destination)\n"));

	BString s;
	if ((destination.port != media_destination::null.port)
	 && (destination.id != media_destination::null.id)) {
		s << sCatalog.GetString(B_TRANSLATE_MARK("Port ")) << destination.port
			<< sCatalog.GetString(B_TRANSLATE_MARK(", ID ")) << destination.id;
	}
	else {
		s = sCatalog.GetString(B_TRANSLATE_MARK("(none)"));
	}
	return s;
}

// -------------------------------------------------------- //
// *** strings for single fields in media_raw_audio_format 
// 	   (public)
// -------------------------------------------------------- //

BString MediaString::forAudioFormat(
	uint32 format,
	int32 validBits) {
	D_METHOD(("MediaString::forAudioFormat()\n"));

	if (format == media_raw_audio_format::wildcard.format) {
		return "*";
	}

	switch (format) {
		case media_raw_audio_format::B_AUDIO_UCHAR: {
			return sCatalog.GetString(B_TRANSLATE_MARK("8 bit integer"));
		}
		case media_raw_audio_format::B_AUDIO_SHORT:	{
			return sCatalog.GetString(B_TRANSLATE_MARK("16 bit integer"));
		}
		case media_raw_audio_format::B_AUDIO_FLOAT:	{
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit float"));
		}
		case media_raw_audio_format::B_AUDIO_INT: {
			BString s = "";
			if (validBits != media_multi_audio_format::wildcard.valid_bits)
				s << validBits << sCatalog.GetString(B_TRANSLATE_MARK(" bit "));
			else
				s << sCatalog.GetString(B_TRANSLATE_MARK("32 bit "));
			s << sCatalog.GetString(B_TRANSLATE_MARK("integer"));
			return s;
		}
		default: {
			return sCatalog.GetString(B_TRANSLATE_MARK("(unknown format)"));
		}
	}
}

BString MediaString::forAudioFrameRate(
	float frameRate)
{
	D_METHOD(("MediaString::forAudioFrameRate()\n"));

	if (frameRate == media_raw_audio_format::wildcard.frame_rate) {
		return "*";
	}
	
	BString s;
	s << (frameRate / 1000) << sCatalog.GetString(B_TRANSLATE_MARK(" kHz"));
	return s;
}

BString MediaString::forAudioChannelCount(
	uint32 channelCount)
{
	D_METHOD(("MediaString::forAudioChannelCount()\n"));

	if (channelCount == media_raw_audio_format::wildcard.channel_count) {
		return "*";
	}
	
	switch (channelCount) {
		case 1: {
			return sCatalog.GetString(B_TRANSLATE_MARK("Mono"));
		}
		case 2: {
			return sCatalog.GetString(B_TRANSLATE_MARK("Stereo"));
		}
		default: {
			BString s = "";
			s << channelCount
				<< sCatalog.GetString(B_TRANSLATE_MARK(" Channels"));
			return s;
		}	
	}
}

BString MediaString::forAudioByteOrder(
	uint32 byteOrder)
{
	D_METHOD(("MediaString::forAudioByteOrder()\n"));

	if (byteOrder == media_raw_audio_format::wildcard.byte_order) {
		return "*";
	}

	switch (byteOrder) {
		case B_MEDIA_BIG_ENDIAN: {
			return sCatalog.GetString(B_TRANSLATE_MARK("Big endian"));
		}
		case B_MEDIA_LITTLE_ENDIAN: {
			return sCatalog.GetString(B_TRANSLATE_MARK("Little endian"));
		}
		default: {
			return sCatalog.GetString(B_TRANSLATE_MARK("(unknown byte order)"));
		}
	}
}

BString MediaString::forAudioBufferSize(
	size_t bufferSize)
{
	D_METHOD(("MediaString::forAudioBufferSize()\n"));

	if (bufferSize == media_raw_audio_format::wildcard.buffer_size) {
		return "*";
	}

	BString s = "";
	s << bufferSize << sCatalog.GetString(B_TRANSLATE_MARK(" bytes per buffer"));
	return s;
}

BString MediaString::forAudioChannelMask(
	uint32 channelMask) {
	D_METHOD(("MediaString::forAudioChannelMask()\n"));

	BString list, last;
	bool first = true;

	if (channelMask & B_CHANNEL_LEFT) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Left"));
			first = false;
		}
	}
	if (channelMask & B_CHANNEL_RIGHT) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Right"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Right"));
		}
	}
	if (channelMask & B_CHANNEL_CENTER) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Center"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Center"));
		}
	}
	if (channelMask & B_CHANNEL_SUB) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Sub"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Sub"));
		}
	}
	if (channelMask & B_CHANNEL_REARLEFT) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Rear-left"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Rear-left"));
		}
	}
	if (channelMask & B_CHANNEL_REARRIGHT) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Rear-right"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Rear-right"));
		}
	}
	if (channelMask & B_CHANNEL_FRONT_LEFT_CENTER) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Front-left-center"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Front-left-center"));
		}
	}
	if (channelMask & B_CHANNEL_FRONT_RIGHT_CENTER) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Front-right-center"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Front-right-center"));
		}
	}
	if (channelMask & B_CHANNEL_BACK_CENTER) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Back-center"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Back-center"));
		}
	}
	if (channelMask & B_CHANNEL_SIDE_LEFT) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Side-left"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Side-left"));
		}
	}
	if (channelMask & B_CHANNEL_SIDE_RIGHT) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Side-right"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Side-right"));
		}
	}
	if (channelMask & B_CHANNEL_TOP_CENTER) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Top-center"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Top-center"));
		}
	}
	if (channelMask & B_CHANNEL_TOP_FRONT_LEFT) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Top-Front-left"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Top-Front-left"));
		}
	}
	if (channelMask & B_CHANNEL_TOP_FRONT_CENTER) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Top-Front-center"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Top-Front-center"));
		}
	}
	if (channelMask & B_CHANNEL_TOP_FRONT_RIGHT) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Top-Front-right"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Top-Front-right"));
		}
	}
	if (channelMask & B_CHANNEL_TOP_BACK_LEFT) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Top-Back-left"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Top-Back-left"));
		}
	}
	if (channelMask & B_CHANNEL_TOP_BACK_CENTER) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Top-Back-center"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Top-Back-center"));
		}
	}
	if (channelMask & B_CHANNEL_TOP_BACK_RIGHT) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Top-Back-right"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Top-Back-right"));
		}
	}
	if (last != "") {
		list << " & " << last;
	}
	if (list == "") {
		list = sCatalog.GetString(B_TRANSLATE_MARK("(none)"));
	}

	return list;
}

BString MediaString::forAudioMatrixMask(
	uint16 matrixMask) {
	D_METHOD(("MediaString::forAudioMatrixMask()\n"));

	switch (matrixMask) {
		case 0:
			return sCatalog.GetString(B_TRANSLATE_MARK("(none)"));
		case B_MATRIX_PROLOGIC_LR:
			return sCatalog.GetString(B_TRANSLATE_MARK("ProLogic LR"));
		case B_MATRIX_AMBISONIC_WXYZ:
			return sCatalog.GetString(B_TRANSLATE_MARK("Ambisonic WXYZ"));
		default:
			return sCatalog.GetString(B_TRANSLATE_MARK("(unknown matrix mask)"));
	}
}

// -------------------------------------------------------- //
// *** strings for single fields in media_encoded_audio_format 
// 	   (public)
// -------------------------------------------------------- //

BString MediaString::forAudioBitRate(
	float bitRate)
{
	D_METHOD(("MediaString::forAudioBufferSize()\n"));

	if (bitRate == media_encoded_audio_format::wildcard.bit_rate) {
		return "*";
	}

	BString s = "";
	s << bitRate / 1000.0f << sCatalog.GetString(B_TRANSLATE_MARK(" kb/s"));
	return s;
}

BString MediaString::forAudioFrameSize(
	size_t frameSize)
{
	D_METHOD(("MediaString::forAudioFrameSize()\n"));

	if (frameSize == media_encoded_audio_format::wildcard.frame_size) {
		return "*";
	}

	BString s = "";
	s << frameSize << sCatalog.GetString(B_TRANSLATE_MARK(" bytes per frame"));
	return s;
}

// -------------------------------------------------------- //
// *** strings for single fields in media_raw_video_format 
// 	   (public)
// -------------------------------------------------------- //

BString MediaString::forVideoFormat(
	color_space format)
{
	D_METHOD(("MediaString::forVideoFormat()\n"));

	if (format == media_video_display_info::wildcard.format) {
		return "*";
	}

	switch (format) {
		case B_RGB32:
		case B_RGB32_BIG:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit RGB"));
		case B_RGBA32:
		case B_RGBA32_BIG:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit RGBA"));
		case B_RGB24:
		case B_RGB24_BIG:
			return sCatalog.GetString(B_TRANSLATE_MARK("24 bit RGB"));
		case B_RGB16:
		case B_RGB16_BIG:
			return sCatalog.GetString(B_TRANSLATE_MARK("16 bit RGB"));
		case B_RGB15:
		case B_RGB15_BIG:
			return sCatalog.GetString(B_TRANSLATE_MARK("15 bit RGB"));
		case B_RGBA15:
		case B_RGBA15_BIG:
			return sCatalog.GetString(B_TRANSLATE_MARK("15 bit RGBA"));
		case B_CMAP8:
			return sCatalog.GetString(B_TRANSLATE_MARK("8 bit color-index"));
		case B_GRAY8:
			return sCatalog.GetString(B_TRANSLATE_MARK("8 bit grayscale-index"));
		case B_GRAY1:
			return sCatalog.GetString(B_TRANSLATE_MARK("Monochrome"));
		case B_YUV422:
			return sCatalog.GetString(B_TRANSLATE_MARK("YUV422"));
		case B_YUV411:
			return sCatalog.GetString(B_TRANSLATE_MARK("YUV411"));
		case B_YUV420:
			return sCatalog.GetString(B_TRANSLATE_MARK("YUV420"));
		case B_YUV444:
			return sCatalog.GetString(B_TRANSLATE_MARK("YUV444"));
		case B_YUV9:
			return sCatalog.GetString(B_TRANSLATE_MARK("YUV9"));
		case B_YUV12:
			return sCatalog.GetString(B_TRANSLATE_MARK("YUV12"));
		case B_YCbCr422:
			return sCatalog.GetString(B_TRANSLATE_MARK("YCbCr422"));
		case B_YCbCr411:
			return sCatalog.GetString(B_TRANSLATE_MARK("YCbCr411"));
		case B_YCbCr444:
			return sCatalog.GetString(B_TRANSLATE_MARK("YCbCr444"));
		case B_YCbCr420:
			return sCatalog.GetString(B_TRANSLATE_MARK("YCbCr420"));
		case B_UVL24:
			return sCatalog.GetString(B_TRANSLATE_MARK("24 bit UVL"));
		case B_UVL32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit UVL"));
		case B_UVLA32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit UVLA"));
		case B_LAB24:
			return sCatalog.GetString(B_TRANSLATE_MARK("24 bit LAB"));
		case B_LAB32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit LAB"));
		case B_LABA32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit LABA"));
		case B_HSI24:
			return sCatalog.GetString(B_TRANSLATE_MARK("24 bit HSI"));
		case B_HSI32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit HSI"));
		case B_HSIA32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit HSIA"));
		case B_HSV24:
			return sCatalog.GetString(B_TRANSLATE_MARK("24 bit HSV"));
		case B_HSV32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit HSV"));
		case B_HSVA32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit HSVA"));
		case B_HLS24:
			return sCatalog.GetString(B_TRANSLATE_MARK("24 bit HLS"));
		case B_HLS32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit HLS"));
		case B_HLSA32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit HLSA"));
		case B_CMY24:
			return sCatalog.GetString(B_TRANSLATE_MARK("24 bit CMY"));
		case B_CMY32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit CMY"));
		case B_CMYA32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit CMYA"));
		case B_CMYK32:
			return sCatalog.GetString(B_TRANSLATE_MARK("32 bit CMYK"));
		default: {
			return sCatalog.GetString(B_TRANSLATE_MARK("(unknown video format)"));
		}
	}
}

BString MediaString::forVideoResolution(
	uint32 lineWidth,
	uint32 lineCount)
{
	D_METHOD(("MediaString::forVideoResolution()\n"));

	if ((lineWidth == media_video_display_info::wildcard.line_width)
	 || (lineCount == media_video_display_info::wildcard.line_count)) {
		return "*";
	}

	BString s = "";
	s << lineWidth << " x " << lineCount;
	return s;
}

BString MediaString::forVideoFieldRate(
	float fieldRate,
	uint32 interlace)
{
	D_METHOD(("MediaString::forVideoFieldRate()\n"));

	if (fieldRate == media_raw_video_format::wildcard.field_rate) {
		return "*";
	}

	BString s = "";
	if (interlace == 1) {
		s << sCatalog.GetString(B_TRANSLATE_MARK("Non-interlaced "));
	}
	else {
		s << sCatalog.GetString(B_TRANSLATE_MARK("Interlaced "));
	}
	s << fieldRate << sCatalog.GetString(B_TRANSLATE_MARK(" Hz"));
	if ((fieldRate > 49.9) && (fieldRate < 50.1)) {
		s << sCatalog.GetString(B_TRANSLATE_MARK(" (PAL)"));
	}
	else if (((interlace == 2) && (fieldRate > 59.9) && (fieldRate < 60.0))
		  || ((interlace == 1) && (fieldRate > 29.9) && (fieldRate < 30.0))) {
		s << sCatalog.GetString(B_TRANSLATE_MARK(" (NTSC)"));
	}

	return s;	
}

BString MediaString::forVideoOrientation(
	uint32 orientation)
{
	D_METHOD(("MediaString::forVideoOrientation()\n"));

	if (orientation == media_raw_video_format::wildcard.orientation) {
		return "*";
	}

	switch (orientation) {
		case B_VIDEO_TOP_LEFT_RIGHT: {
			return sCatalog.GetString(B_TRANSLATE_MARK(
				"Top to bottom, left to right"));
		}
		case B_VIDEO_BOTTOM_LEFT_RIGHT: {
			return sCatalog.GetString(B_TRANSLATE_MARK(
				"Bottom to top, left to right"));
		}
		default: {
			return sCatalog.GetString(B_TRANSLATE_MARK(
				"(unkown video orientation)"));
		}
	}
}

BString MediaString::forVideoAspectRatio(
	uint16 pixelWidth,
	uint16 pixelHeight)
{
	D_METHOD(("MediaString::forVideoPixelAspect()\n"));

	if ((pixelWidth == media_raw_video_format::wildcard.pixel_width_aspect)
	 || (pixelHeight == media_raw_video_format::wildcard.pixel_height_aspect)) {
		return "*";
	}

	BString s = "";
	s << static_cast<uint32>(pixelWidth);
	s << ":" << static_cast<uint32>(pixelHeight);
	return s;
}

BString MediaString::forVideoActiveLines(
	uint32 firstActive,
	uint32 lastActive)
{
	D_METHOD(("MediaString::forVideoActiveLines()\n"));

	if ((firstActive == media_raw_video_format::wildcard.first_active)
	 || (lastActive == media_raw_video_format::wildcard.last_active)) {
	 	return "*";
	}

	BString s = sCatalog.GetString(B_TRANSLATE_MARK("Video data between"));
	s << sCatalog.GetString(B_TRANSLATE_MARK(" line ")) << firstActive;
	s << sCatalog.GetString(B_TRANSLATE_MARK(" and ")) << lastActive;
	return s;
}

BString MediaString::forVideoBytesPerRow(
	uint32 bytesPerRow)
{
	D_METHOD(("MediaString::forVideoBytesPerRow()\n"));

	if (bytesPerRow == media_video_display_info::wildcard.bytes_per_row) {
		return "*";
	}

	BString s = "";
	s << bytesPerRow << sCatalog.GetString(B_TRANSLATE_MARK(" bytes per row"));
	return s;
}

BString MediaString::forVideoOffset(
	uint32 pixelOffset,
	uint32 lineOffset)
{
	D_METHOD(("MediaString::forVideoResolution()\n"));

	BString s = "";
	if (pixelOffset != media_video_display_info::wildcard.pixel_offset) {
		s << pixelOffset << sCatalog.GetString(B_TRANSLATE_MARK(" pixels"));
	}
	if (lineOffset != media_video_display_info::wildcard.line_offset) {
		if (s != "") {
			s << ", ";
		}
		s << pixelOffset << sCatalog.GetString(B_TRANSLATE_MARK(" lines"));
	}
	if (s == "") {
		s = "*";
	}
	return s;
}

// -------------------------------------------------------- //
// *** strings for single fields in media_encoded_video_format 
// 	   (public)
// -------------------------------------------------------- //

BString MediaString::forVideoBitRate(
	float avgBitRate,
	float maxBitRate)
{
	D_METHOD(("MediaString::forVideoBitRate()\n"));

	BString s = "";
	if (avgBitRate != media_encoded_video_format::wildcard.avg_bit_rate) {
		s << avgBitRate / 1000.0f << sCatalog.GetString(B_TRANSLATE_MARK(" kb/s (avg)"));
	}
	if (maxBitRate != media_encoded_video_format::wildcard.max_bit_rate) {
		if (s != "") {
			s << ", ";
		}
		s << maxBitRate / 1000.0f << sCatalog.GetString(B_TRANSLATE_MARK(" kb/s (max)"));
	}
	if (s == "") {
		s = "*";
	}
	return s;
}

BString MediaString::forVideoFrameSize(
	size_t frameSize)
{
	D_METHOD(("MediaString::forVideoFrameSize()\n"));

	if (frameSize == media_encoded_video_format::wildcard.frame_size) {
		return "*";
	}

	BString s = "";
	s << frameSize << sCatalog.GetString(B_TRANSLATE_MARK(" bytes per frame"));
	return s;
}

BString MediaString::forVideoHistory(
	int16 forwardHistory,
	int16 backwardHistory)
{
	D_METHOD(("MediaString::forVideoHistory()\n"));

	BString s = "";
	if (forwardHistory != media_encoded_video_format::wildcard.forward_history) {
		s << static_cast<int32>(forwardHistory)
			<< sCatalog.GetString(B_TRANSLATE_MARK(" frames forward"));
	}
	if (backwardHistory != media_encoded_video_format::wildcard.backward_history) {
		if (s != "") {
			s << ", ";
		}
		s << static_cast<int32>(backwardHistory)
			<< sCatalog.GetString(B_TRANSLATE_MARK(" frames backward"));
	}
	if (s == "") {
		s = "*";
	}
	return s;
}

// -------------------------------------------------------- //
// *** strings for single fields in media_multistream_format 
// 	   (public)
// -------------------------------------------------------- //

BString MediaString::forMultistreamFormat(
	int32 format)
{
	D_METHOD(("MediaString::forMultistreamFormat()\n"));

	if (format == media_multistream_format::wildcard.format) {
		return "*";
	}

	switch (format) {
		case media_multistream_format::B_VID:
			return sCatalog.GetString(B_TRANSLATE_MARK("BeOS video"));
		case media_multistream_format::B_AVI:
			return sCatalog.GetString(B_TRANSLATE_MARK("AVI"));
		case media_multistream_format::B_MPEG1:
			return sCatalog.GetString(B_TRANSLATE_MARK("MPEG1"));
		case media_multistream_format::B_MPEG2:
			return sCatalog.GetString(B_TRANSLATE_MARK("MPEG2"));
		case media_multistream_format::B_QUICKTIME:
			return sCatalog.GetString(B_TRANSLATE_MARK("QuickTime"));
		default:
			return sCatalog.GetString(B_TRANSLATE_MARK("(unknown multistream format)"));
	}
}

BString MediaString::forMultistreamBitRate(
	float avgBitRate,
	float maxBitRate)
{
	D_METHOD(("MediaString::forMultistreamFormat()\n"));

	BString s = "";
	if (avgBitRate != media_multistream_format::wildcard.avg_bit_rate) {
		s << avgBitRate / 1000.0f << sCatalog.GetString(B_TRANSLATE_MARK(" kb/s (avg)"));
	}
	if (maxBitRate != media_multistream_format::wildcard.max_bit_rate) {
		if (s != "") {
			s << ", ";
		}
		s << maxBitRate / 1000.0f << sCatalog.GetString(B_TRANSLATE_MARK(" kb/s (max)"));
	}
	if (s == "") {
		s = "*";
	}
	return s;
}

BString MediaString::forMultistreamChunkSize(
	uint32 avgChunkSize,
	uint32 maxChunkSize)
{
	D_METHOD(("MediaString::forMultistreamChunkSize()\n"));

	BString s = "";
	if (avgChunkSize != media_multistream_format::wildcard.avg_chunk_size) {
		s << avgChunkSize << sCatalog.GetString(B_TRANSLATE_MARK(" bytes (avg)"));
	}
	if (maxChunkSize != media_multistream_format::wildcard.max_chunk_size) {
		if (s != "") {
			s << ", ";
		}
		s << maxChunkSize << sCatalog.GetString(B_TRANSLATE_MARK(" bytes (max)"));
	}
	if (s == "") {
		s = "*";
	}
	return s;
}

BString MediaString::forMultistreamFlags(
	uint32 flags)
{
	D_METHOD(("MediaString::forMultistreamFlags()\n"));

	BString list, last;
	bool first = true;

	if (flags & media_multistream_format::B_HEADER_HAS_FLAGS) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Header has flags"));
			first = false;
		}
	}
	if (flags & media_multistream_format::B_CLEAN_BUFFERS) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Clean buffers"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Clean buffers"));
		}
	}
	if (flags & media_multistream_format::B_HOMOGENOUS_BUFFERS) {
		if (first) {
			list = sCatalog.GetString(B_TRANSLATE_MARK("Homogenous buffers"));
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = sCatalog.GetString(B_TRANSLATE_MARK("Homogenous buffers"));
		}
	}

	if (last != "")
		list << " & " << last;

	if (list == "")
		list = sCatalog.GetString(B_TRANSLATE_MARK("(none)"));

	return list;
}

// END -- MediaString.cpp --
