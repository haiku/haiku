// MediaString.cpp

#include "MediaString.h"

// Media Kit
#include <MediaFormats.h>
// Support Kit
#include <String.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)

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
			list = "Buffer Producer";
			first = false;
		}
	}
	if (kinds & B_BUFFER_CONSUMER) {
		if (first) {
			list = "Buffer Consumer";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Buffer Consumer";
		}
	}
	if (kinds & B_TIME_SOURCE) {
		if (first) {
			list = "Time Source";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Time Source";
		}
	}
	if (kinds & B_CONTROLLABLE) {
		if (first) {
			list = "Controllable";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Controllable";
		}
	}
	if (kinds & B_FILE_INTERFACE) {
		if (first) {
			list = "File Interface";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "File Interface";
		}
	}
	if (kinds & B_ENTITY_INTERFACE) {
		if (first) {
			list = "Entity Interface";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Entity Interface";
		}
	}
	if (kinds & B_PHYSICAL_INPUT) {
		if (first) {
			list = "Physical Input";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Physical Input";
		}
	}
	if (kinds & B_PHYSICAL_OUTPUT) {
		if (first) {
			list = "Physical Output";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Physical Output";
		}
	}
	if (kinds & B_SYSTEM_MIXER) {
		if (first) {
			list = "System Mixer";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "System Mixer";
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
		case BMediaNode::B_OFFLINE:				return "Offline";
		case BMediaNode::B_RECORDING:			return "Recording";
		case BMediaNode::B_DECREASE_PRECISION:	return "Decrease Precision";
		case BMediaNode::B_INCREASE_LATENCY:	return "Increase Latency";
		case BMediaNode::B_DROP_DATA:			return "Drop Data";
		default:								return "(unknown run mode)";
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
		case B_MEDIA_NO_TYPE:			return "Typeless Media";
		case B_MEDIA_UNKNOWN_TYPE:		return "Unknown Media Type";
		case B_MEDIA_RAW_AUDIO:			return "Raw Audio";
		case B_MEDIA_RAW_VIDEO:			return "Raw Video";
		case B_MEDIA_VBL:				return "Raw data from VBL area";
		case B_MEDIA_TIMECODE:			return "Timecode";
		case B_MEDIA_MIDI:				return "MIDI";
		case B_MEDIA_TEXT:				return "Text";
		case B_MEDIA_HTML:				return "HTML";
		case B_MEDIA_MULTISTREAM:		return "Multistream Media";
		case B_MEDIA_PARAMETERS:		return "Parameters";
		case B_MEDIA_ENCODED_AUDIO:		return "Encoded Audio";
		case B_MEDIA_ENCODED_VIDEO:		return "Encoded Video";
		default: {
			if (type >= B_MEDIA_PRIVATE)
				return "Private Be Media Type";
			if (type >= B_MEDIA_FIRST_USER_TYPE)
				return "User-defined Media Type";
		}
	}
	return "Unknown Media Type";
}

BString	MediaString::getStringFor(
	media_format_family family,
	bool complete) {
	D_METHOD(("MediaString::getStringFor(media_format_family)\n"));

	switch (family) {
		case B_ANY_FORMAT_FAMILY:		return "Any Format Family";
		case B_BEOS_FORMAT_FAMILY:		return "BeOS Format Family";
		case B_QUICKTIME_FORMAT_FAMILY:	return "QuickTime Format Family";
		case B_AVI_FORMAT_FAMILY:		return "AVI Format Family";
		case B_ASF_FORMAT_FAMILY:		return "ASF Format Family";
		case B_MPEG_FORMAT_FAMILY:		return "MPEG Format Family";
		case B_WAV_FORMAT_FAMILY:		return "WAV Format Family";
		case B_AIFF_FORMAT_FAMILY:		return "AIFF Format Family";
		default:						return "Miscellaneous Format Family";
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
		s << "Port " << source.port << ", ID " << source.id;
	}
	else {
		s = "(None)";
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
		s << "Port " << destination.port << ", ID " << destination.id;
	}
	else {
		s = "(None)";
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
			return "8 bit Integer";
		}
		case media_raw_audio_format::B_AUDIO_SHORT:	{
			return "16 bit Integer";
		}
		case media_raw_audio_format::B_AUDIO_FLOAT:	{
			return "32 bit Float";
		}
		case media_raw_audio_format::B_AUDIO_INT: {
			BString s = "";
			if (validBits != media_multi_audio_format::wildcard.valid_bits)
				s << validBits << " bit ";
			else
				s << "32 bit ";
			s << "Integer";
			return s;
		}
		default: {
			return "(unknown format)";
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
	s << (frameRate / 1000) << " kHz";
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
			return "Mono";
		}
		case 2: {
			return "Stereo";
		}
		default: {
			BString s = "";
			s << channelCount << " Channels";
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
			return "Big Endian";
		}
		case B_MEDIA_LITTLE_ENDIAN: {
			return "Little Endian";
		}
		default: {
			return "(unknown byte order)";
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
	s << bufferSize << " bytes per buffer";
	return s;
}

BString MediaString::forAudioChannelMask(
	uint32 channelMask) {
	D_METHOD(("MediaString::forAudioChannelMask()\n"));

	BString list, last;
	bool first = true;

	if (channelMask & B_CHANNEL_LEFT) {
		if (first) {
			list = "Left";
			first = false;
		}
	}
	if (channelMask & B_CHANNEL_RIGHT) {
		if (first) {
			list = "Right";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Right";
		}
	}
	if (channelMask & B_CHANNEL_CENTER) {
		if (first) {
			list = "Center";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Center";
		}
	}
	if (channelMask & B_CHANNEL_SUB) {
		if (first) {
			list = "Sub";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Sub";
		}
	}
	if (channelMask & B_CHANNEL_REARLEFT) {
		if (first) {
			list = "Rear-Left";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Rear-Left";
		}
	}
	if (channelMask & B_CHANNEL_REARRIGHT) {
		if (first) {
			list = "Rear-Right";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Rear-Right";
		}
	}
	if (channelMask & B_CHANNEL_FRONT_LEFT_CENTER) {
		if (first) {
			list = "Front-Left-Center";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Front-Left-Center";
		}
	}
	if (channelMask & B_CHANNEL_FRONT_RIGHT_CENTER) {
		if (first) {
			list = "Front-Right-Center";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Front-Right-Center";
		}
	}
	if (channelMask & B_CHANNEL_BACK_CENTER) {
		if (first) {
			list = "Back-Center";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Back-Center";
		}
	}
	if (channelMask & B_CHANNEL_SIDE_LEFT) {
		if (first) {
			list = "Side-Left";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Side-Left";
		}
	}
	if (channelMask & B_CHANNEL_SIDE_RIGHT) {
		if (first) {
			list = "Side-Right";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Side-Right";
		}
	}
	if (channelMask & B_CHANNEL_TOP_CENTER) {
		if (first) {
			list = "Top-Center";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Top-Center";
		}
	}
	if (channelMask & B_CHANNEL_TOP_FRONT_LEFT) {
		if (first) {
			list = "Top-Front-Left";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Top-Front-Left";
		}
	}
	if (channelMask & B_CHANNEL_TOP_FRONT_CENTER) {
		if (first) {
			list = "Top-Front-Center";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Top-Front-Center";
		}
	}
	if (channelMask & B_CHANNEL_TOP_FRONT_RIGHT) {
		if (first) {
			list = "Top-Front-Right";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Top-Front-Right";
		}
	}
	if (channelMask & B_CHANNEL_TOP_BACK_LEFT) {
		if (first) {
			list = "Top-Back-Left";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Top-Back-Left";
		}
	}
	if (channelMask & B_CHANNEL_TOP_BACK_CENTER) {
		if (first) {
			list = "Top-Back-Center";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Top-Back-Center";
		}
	}
	if (channelMask & B_CHANNEL_TOP_BACK_RIGHT) {
		if (first) {
			list = "Top-Back-Right";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Top-Back-Right";
		}
	}
	if (last != "") {
		list << " & " << last;
	}
	if (list == "") {
		list = "(none)";
	}

	return list;
}

BString MediaString::forAudioMatrixMask(
	uint16 matrixMask) {
	D_METHOD(("MediaString::forAudioMatrixMask()\n"));

	switch (matrixMask) {
		case 0:									return "(none)";
		case B_MATRIX_PROLOGIC_LR:				return "ProLogic LR";
		case B_MATRIX_AMBISONIC_WXYZ:			return "Ambisonic WXYZ";
		default:								return "(unknown matrix mask)";
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
	s << bitRate / 1000.0f << " kb/s";
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
	s << frameSize << " bytes per frame";
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
		case B_RGB32_BIG: 		return "32 bit RGB";
		case B_RGBA32:
		case B_RGBA32_BIG:		return "32 bit RGBA";
		case B_RGB24:
		case B_RGB24_BIG:		return "24 bit RGB";
		case B_RGB16:
		case B_RGB16_BIG:		return "16 bit RGB";
		case B_RGB15:
		case B_RGB15_BIG:		return "15 bit RGB";
		case B_RGBA15:
		case B_RGBA15_BIG:		return "15 bit RGBA";
		case B_CMAP8:			return "8 bit Color-Index";
		case B_GRAY8:			return "8 bit Grayscale-Index";
		case B_GRAY1:			return "Monochrome";
		case B_YUV422:			return "YUV422";
		case B_YUV411:			return "YUV411";
		case B_YUV420:			return "YUV420";
		case B_YUV444:			return "YUV444";
		case B_YUV9:			return "YUV9";
		case B_YUV12:			return "YUV12";
		case B_YCbCr422:		return "YCbCr422";
		case B_YCbCr411:		return "YCbCr411";
		case B_YCbCr444:		return "YCbCr444";
		case B_YCbCr420:		return "YCbCr420";
		case B_UVL24:			return "24 bit UVL";
		case B_UVL32:			return "32 bit UVL";
		case B_UVLA32:			return "32 bit UVLA";
		case B_LAB24:			return "24 bit LAB";
		case B_LAB32:			return "32 bit LAB";
		case B_LABA32:			return "32 bit LABA";
		case B_HSI24:			return "24 bit HSI";
		case B_HSI32:			return "32 bit HSI";
		case B_HSIA32:			return "32 bit HSIA";
		case B_HSV24:			return "24 bit HSV";
		case B_HSV32:			return "32 bit HSV";
		case B_HSVA32:			return "32 bit HSVA";
		case B_HLS24:			return "24 bit HLS";
		case B_HLS32:			return "32 bit HLS";
		case B_HLSA32:			return "32 bit HLSA";
		case B_CMY24:			return "24 bit CMY";
		case B_CMY32:			return "32 bit CMY";
		case B_CMYA32:			return "32 bit CMYA";
		case B_CMYK32:			return "32 bit CMYK";
		default: {
			return "(unknown video format)";
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
		s << "Non-Interlaced ";
	}
	else {
		s << "Interlaced ";
	}
	s << fieldRate << " Hz";
	if ((fieldRate > 49.9) && (fieldRate < 50.1)) {
		s << " (PAL)";
	}
	else if (((interlace == 2) && (fieldRate > 59.9) && (fieldRate < 60.0))
		  || ((interlace == 1) && (fieldRate > 29.9) && (fieldRate < 30.0))) {
		s << " (NTSC)";
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
			return "Top to Bottom, Left to Right";
		}
		case B_VIDEO_BOTTOM_LEFT_RIGHT: {
			return "Bottom to Top, Left to Right";
		}
		default: {
			return "(unkown video orientation)";
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

	BString s = "Video data between";
	s << " line " << firstActive;
	s << " and " << lastActive;
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
	s << bytesPerRow << " bytes per row";
	return s;
}

BString MediaString::forVideoOffset(
	uint32 pixelOffset,
	uint32 lineOffset)
{
	D_METHOD(("MediaString::forVideoResolution()\n"));

	BString s = "";
	if (pixelOffset != media_video_display_info::wildcard.pixel_offset) {
		s << pixelOffset << " pixels";
	}
	if (lineOffset != media_video_display_info::wildcard.line_offset) {
		if (s != "") {
			s << ", ";
		}
		s << pixelOffset << " lines";
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
		s << avgBitRate / 1000.0f << " kb/s (avg)";
	}
	if (maxBitRate != media_encoded_video_format::wildcard.max_bit_rate) {
		if (s != "") {
			s << ", ";
		}
		s << maxBitRate / 1000.0f << " kb/s (max)";
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
	s << frameSize << " bytes per frame";
	return s;
}

BString MediaString::forVideoHistory(
	int16 forwardHistory,
	int16 backwardHistory)
{
	D_METHOD(("MediaString::forVideoHistory()\n"));

	BString s = "";
	if (forwardHistory != media_encoded_video_format::wildcard.forward_history) {
		s << static_cast<int32>(forwardHistory) << " frames forward";
	}
	if (backwardHistory != media_encoded_video_format::wildcard.backward_history) {
		if (s != "") {
			s << ", ";
		}
		s << static_cast<int32>(backwardHistory) << " frames backward";
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
		case media_multistream_format::B_VID:		return "BeOS Video";
		case media_multistream_format::B_AVI:		return "AVI";
		case media_multistream_format::B_MPEG1:		return "MPEG1";
		case media_multistream_format::B_MPEG2:		return "MPEG2";
		case media_multistream_format::B_QUICKTIME:	return "QuickTime";
		default:									return "(unknown multistream format)";
	}
}

BString MediaString::forMultistreamBitRate(
	float avgBitRate,
	float maxBitRate)
{
	D_METHOD(("MediaString::forMultistreamFormat()\n"));

	BString s = "";
	if (avgBitRate != media_multistream_format::wildcard.avg_bit_rate) {
		s << avgBitRate / 1000.0f << " kb/s (avg)";
	}
	if (maxBitRate != media_multistream_format::wildcard.max_bit_rate) {
		if (s != "") {
			s << ", ";
		}
		s << maxBitRate / 1000.0f << " kb/s (max)";
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
		s << avgChunkSize << " bytes (avg)";
	}
	if (maxChunkSize != media_multistream_format::wildcard.max_chunk_size) {
		if (s != "") {
			s << ", ";
		}
		s << maxChunkSize << " bytes (max)";
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
			list = "Header has flags";
			first = false;
		}
	}
	if (flags & media_multistream_format::B_CLEAN_BUFFERS) {
		if (first) {
			list = "Clean buffers";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Clean buffers";
		}
	}
	if (flags & media_multistream_format::B_HOMOGENOUS_BUFFERS) {
		if (first) {
			list = "Homogenous buffers";
			first = false;
		}
		else {
			if (last != "")
				list << ", " << last;
			last = "Homogenous buffers";
		}
	}

	if (last != "")
		list << " & " << last;

	if (list == "")
		list = "(none)";

	return list;
}

// END -- MediaString.cpp --
