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


// MediaFormatIO.cpp
// e.moon 2jul99

#include "MediaFormatIO.h"
//#include "xml_export_utils.h"

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** constants
// -------------------------------------------------------- //

// these tags map directly to MediaFormatIO
const char* const MediaFormatIO::s_multi_audio_tag 			= "multi_audio_format";
const char* const MediaFormatIO::s_raw_audio_tag 				= "raw_audio_format";
const char* const MediaFormatIO::s_raw_video_tag 				= "raw_video_format";
const char* const MediaFormatIO::s_multistream_tag 			= "multistream_format";
const char* const MediaFormatIO::s_encoded_audio_tag 		= "encoded_audio_format";
const char* const MediaFormatIO::s_encoded_video_tag 		= "encoded_video_format";

// nested tags
const char* const MediaFormatIO::s_video_display_info_tag		= "video_display_info";
const char* const MediaFormatIO::s_multistream_flags_tag			= "multistream_flags";
const char* const MediaFormatIO::s_multistream_vid_info_tag	= "multistream_vid_info";
const char* const MediaFormatIO::s_multistream_avi_info_tag	= "multistream_avi_info";
const char* const MediaFormatIO::s_multi_audio_info_tag			= "multi_audio_info";
const char* const MediaFormatIO::s_media_type_tag						= "media_type";

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

MediaFormatIO::~MediaFormatIO() {}

MediaFormatIO::MediaFormatIO() :
	m_complete(false) {}
MediaFormatIO::MediaFormatIO(const media_format& format) :
	m_complete(true),
	m_format(format) {}

// -------------------------------------------------------- //
// *** accessors
// -------------------------------------------------------- //

// returns B_OK if the object contains a valid format,
// or B_ERROR if not.

status_t MediaFormatIO::getFormat(media_format& outFormat) const {
	if(!m_complete)
		return B_ERROR;
	outFormat = m_format;
	return B_OK;
}

// -------------------------------------------------------- //
// *** static setup method
// -------------------------------------------------------- //

// call this method to install hooks for the tags needed by
// MediaFormatIO

/*static*/
void MediaFormatIO::AddTo(XML::DocumentType* pDocType) {

	pDocType->addMapping(new Mapping
		<MediaFormatIO>(s_multi_audio_tag));
	pDocType->addMapping(new Mapping
		<MediaFormatIO>(s_raw_audio_tag));
	pDocType->addMapping(new Mapping
		<MediaFormatIO>(s_raw_video_tag));
	pDocType->addMapping(new Mapping
		<MediaFormatIO>(s_multistream_tag));
	pDocType->addMapping(new Mapping
		<MediaFormatIO>(s_encoded_audio_tag));
	pDocType->addMapping(new Mapping
		<MediaFormatIO>(s_encoded_video_tag));
}

// -------------------------------------------------------- //
// *** IPersistent
// -------------------------------------------------------- //

// -------------------------------------------------------- //
// attribute constants
// -------------------------------------------------------- //

// *** raw_audio_format
const char* const gKey_frame_rate							= "frame_rate";
const char* const gKey_channel_count					= "channel_count";
const char* const gKey_format									= "format";
const char* const gKey_byte_order							= "byte_order";
const char* const gKey_buffer_size						= "buffer_size";

// *** +multi_audio_format
const char* const gKey_channel_mask						= "channel_mask";
const char* const gKey_valid_bits							= "valid_bits";
const char* const gKey_matrix_mask						= "matrix_mask";

// *** raw_video_format
const char* const gKey_field_rate							= "field_rate";
const char* const gKey_interlace							= "interlace";
const char* const gKey_first_active						= "first_active";
const char* const gKey_last_active						= "last_active";
const char* const gKey_orientation						= "orientation";
const char* const gKey_pixel_width_aspect			= "pixel_width_aspect";
const char* const gKey_pixel_height_aspect		= "pixel_height_aspect";

// *** video_display_info
const char* const gKey_color_space						= "color_space";
const char* const gKey_line_width							= "line_width";
const char* const gKey_line_count							= "line_count";
const char* const gKey_bytes_per_row					= "bytes_per_row";
const char* const gKey_pixel_offset						= "pixel_offset";
const char* const gKey_line_offset						= "line_offset";

// *** multistream_format
const char* const gKey_multistream_format			= "format";
const char* const gKey_avg_bit_rate						= "avg_bit_rate";
const char* const gKey_max_bit_rate						= "max_bit_rate";
const char* const gKey_avg_chunk_size					= "avg_chunk_size";
const char* const gKey_max_chunk_size					= "max_chunk_size";

// *** multistream_flags
const char* const gKey_header_has_flags				= "header_has_flags";
const char* const gKey_clean_buffers					= "clean_buffers";
const char* const gKey_homogenous_buffers			= "homogenous_buffers";

// *** multistream_vid_info
// frame_rate
const char* const gKey_width									= "width";
const char* const gKey_height									= "height";
const char* const gKey_space									= "space";
const char* const gKey_sampling_rate					= "sampling_rate";
const char* const gKey_sample_format					= "sample_format";
// byte_order
// channel_count

// *** multistream_avi_info
const char* const gKey_us_per_frame						= "us_per_frame";
// width
// height

// *** encoded_audio_format
const char* const gKey_encoding								= "encoding";
const char* const gKey_bit_rate								= "bit_rate";
const char* const gKey_frame_size							= "frame_size";

// *** encoded_video_format
// encoding
// avg_bit_rate
// max_bit_rate
// frame_size
const char* const gKey_forward_history				= "forward_history";
const char* const gKey_backward_history				= "backward_history";

// padding (number of spaces allowed for attribute name)
const int16 g_padAttributes		= 30;

// -------------------------------------------------------- //
// export
// -------------------------------------------------------- //

void write_colorspace_attr(
	const char* key,
	color_space c,
	ExportContext& context) {
	
	switch(c) {
		case B_RGB32:
			context.writeAttr(key, "B_RGB32");
			break;
		case B_RGBA32:
			context.writeAttr(key, "B_RGBA32");
			break;
		case B_RGB24:
			context.writeAttr(key, "B_RGB24");
			break;
		case B_RGB16:
			context.writeAttr(key, "B_RGB16");
			break;
		case B_RGB15:
			context.writeAttr(key, "B_RGB15");
			break;
		case B_RGBA15:
			context.writeAttr(key, "B_RGBA15");
			break;
		case B_CMAP8:
			context.writeAttr(key, "B_CMAP8");
			break;
		case B_GRAY8:
			context.writeAttr(key, "B_GRAY8");
			break;
		case B_GRAY1:
			context.writeAttr(key, "B_GRAY1");
			break;
		case B_RGB32_BIG:
			context.writeAttr(key, "B_RGB32_BIG");
			break;
		case B_RGBA32_BIG:
			context.writeAttr(key, "B_RGBA32_BIG");
			break;
		case B_RGB24_BIG:
			context.writeAttr(key, "B_RGB24_BIG");
			break;
		case B_RGB16_BIG:
			context.writeAttr(key, "B_RGB16_BIG");
			break;
		case B_RGB15_BIG:
			context.writeAttr(key, "B_RGB15_BIG");
			break;
		case B_RGBA15_BIG:
			context.writeAttr(key, "B_RGBA15_BIG");
			break;
		case B_YCbCr422:
			context.writeAttr(key, "B_YCbCr422");
			break;
		case B_YCbCr411:
			context.writeAttr(key, "B_YCbCr411");
			break;
		case B_YCbCr444:
			context.writeAttr(key, "B_YCbCr444");
			break;
		case B_YCbCr420:
			context.writeAttr(key, "B_YCbCr420");
			break;
		case B_YUV422:
			context.writeAttr(key, "B_YUV422");
			break;
		case B_YUV411:
			context.writeAttr(key, "B_YUV411");
			break;
		case B_YUV444:
			context.writeAttr(key, "B_YUV444");
			break;
		case B_YUV420:
			context.writeAttr(key, "B_YUV420");
			break;
		case B_YUV9:
			context.writeAttr(key, "B_YUV9");
			break;
		case B_YUV12:
			context.writeAttr(key, "B_YUV12");
			break;
		case B_UVL24:
			context.writeAttr(key, "B_UVL24");
			break;
		case B_UVL32:
			context.writeAttr(key, "B_UVL32");
			break;
		case B_UVLA32:
			context.writeAttr(key, "B_UVLA32");
			break;
		case B_LAB24:
			context.writeAttr(key, "B_LAB24");
			break;
		case B_LAB32:
			context.writeAttr(key, "B_LAB32");
			break;
		case B_LABA32:
			context.writeAttr(key, "B_LABA32");
			break;
		case B_HSI24:
			context.writeAttr(key, "B_HSI24");
			break;
		case B_HSI32:
			context.writeAttr(key, "B_HSI32");
			break;
		case B_HSIA32:
			context.writeAttr(key, "B_HSIA32");
			break;
		case B_HSV24:
			context.writeAttr(key, "B_HSV24");
			break;
		case B_HSV32:
			context.writeAttr(key, "B_HSV32");
			break;
		case B_HSVA32:
			context.writeAttr(key, "B_HSVA32");
			break;
		case B_HLS24:
			context.writeAttr(key, "B_HLS24");
			break;
		case B_HLS32:
			context.writeAttr(key, "B_HLS32");
			break;
		case B_HLSA32:
			context.writeAttr(key, "B_HLSA32");
			break;
		case B_CMY24:
			context.writeAttr(key, "B_CMY24");
			break;
		case B_CMY32:
			context.writeAttr(key, "B_CMY32");
			break;
		case B_CMYA32:
			context.writeAttr(key, "B_CMYA32");
			break;
		case B_CMYK32:
			context.writeAttr(key, "B_CMYK32");
			break;
		default:
			break;
	}
}

void import_color_space(
	const char* value,
	color_space& dest) {
	
	if(!strcmp(value, "B_RGB32"))
		dest = B_RGB32;
	else if(!strcmp(value, "B_RGBA32"))
		dest = B_RGBA32;
	else if(!strcmp(value, "B_RGB24"))
		dest = B_RGB24;
	else if(!strcmp(value, "B_RGB16"))
		dest = B_RGB16;
	else if(!strcmp(value, "B_RGB15"))
		dest = B_RGB15;
	else if(!strcmp(value, "B_RGBA15"))
		dest = B_RGBA15;
	else if(!strcmp(value, "B_CMAP8"))
		dest = B_CMAP8;
	else if(!strcmp(value, "B_GRAY8"))
		dest = B_GRAY8;
	else if(!strcmp(value, "B_GRAY1"))
		dest = B_GRAY1;
	else if(!strcmp(value, "B_RGB32_BIG"))
		dest = B_RGB32_BIG;
	else if(!strcmp(value, "B_RGBA32_BIG"))
		dest = B_RGBA32_BIG;
	else if(!strcmp(value, "B_RGB24_BIG"))
		dest = B_RGB24_BIG;
	else if(!strcmp(value, "B_RGB16_BIG"))
		dest = B_RGB16_BIG;
	else if(!strcmp(value, "B_RGB15_BIG"))
		dest = B_RGB15_BIG;
	else if(!strcmp(value, "B_RGBA15_BIG"))
		dest = B_RGBA15_BIG;
	else if(!strcmp(value, "B_RGB32_LITTLE"))
		dest = B_RGB32_LITTLE;
	else if(!strcmp(value, "B_RGBA32_LITTLE"))
		dest = B_RGBA32_LITTLE;
	else if(!strcmp(value, "B_RGB24_LITTLE"))
		dest = B_RGB24_LITTLE;
	else if(!strcmp(value, "B_RGB16_LITTLE"))
		dest = B_RGB16_LITTLE;
	else if(!strcmp(value, "B_RGB15_LITTLE"))
		dest = B_RGB15_LITTLE;
	else if(!strcmp(value, "B_RGBA15_LITTLE"))
		dest = B_RGBA15_LITTLE;
	else if(!strcmp(value, "B_YCbCr422"))
		dest = B_YCbCr422;
	else if(!strcmp(value, "B_YCbCr411"))
		dest = B_YCbCr411;
	else if(!strcmp(value, "B_YCbCr444"))
		dest = B_YCbCr444;
	else if(!strcmp(value, "B_YCbCr420"))
		dest = B_YCbCr420;
	else if(!strcmp(value, "B_YUV422"))
		dest = B_YUV422;
	else if(!strcmp(value, "B_YUV411"))
		dest = B_YUV411;
	else if(!strcmp(value, "B_YUV444"))
		dest = B_YUV444;
	else if(!strcmp(value, "B_YUV420"))
		dest = B_YUV420;
	else if(!strcmp(value, "B_YUV9"))
		dest = B_YUV9;
	else if(!strcmp(value, "B_YUV12"))
		dest = B_YUV12;
	else if(!strcmp(value, "B_UVL24"))
		dest = B_UVL24;
	else if(!strcmp(value, "B_UVL32"))
		dest = B_UVL32;
	else if(!strcmp(value, "B_UVLA32"))
		dest = B_UVLA32;
	else if(!strcmp(value, "B_LAB24"))
		dest = B_LAB24;
	else if(!strcmp(value, "B_LAB32"))
		dest = B_LAB32;
	else if(!strcmp(value, "B_LABA32"))
		dest = B_LABA32;
	else if(!strcmp(value, "B_HSI24"))
		dest = B_HSI24;
	else if(!strcmp(value, "B_HSI32"))
		dest = B_HSI32;
	else if(!strcmp(value, "B_HSIA32"))
		dest = B_HSIA32;
	else if(!strcmp(value, "B_HSV24"))
		dest = B_HSV24;
	else if(!strcmp(value, "B_HSV32"))
		dest = B_HSV32;
	else if(!strcmp(value, "B_HSVA32"))
		dest = B_HSVA32;
	else if(!strcmp(value, "B_HLS24"))
		dest = B_HLS24;
	else if(!strcmp(value, "B_HLS32"))
		dest = B_HLS32;
	else if(!strcmp(value, "B_HLSA32"))
		dest = B_HLSA32;
	else if(!strcmp(value, "B_CMY24"))
		dest = B_CMY24;
	else if(!strcmp(value, "B_CMY32"))
		dest = B_CMY32;
	else if(!strcmp(value, "B_CMYA32"))
		dest = B_CMYA32;
	else if(!strcmp(value, "B_CMYK32"))
		dest = B_CMYK32;
}

void write_media_type(
	media_type t,
	ExportContext& context) {

	context.beginElement(MediaFormatIO::s_media_type_tag);
	context.beginContent();
	
	switch(t) {
		case B_MEDIA_NO_TYPE:
			context.writeString("B_MEDIA_NO_TYPE");
			break;
		case B_MEDIA_UNKNOWN_TYPE:
			context.writeString("B_MEDIA_UNKNOWN_TYPE");
			break;
		case B_MEDIA_RAW_AUDIO:
			context.writeString("B_MEDIA_RAW_AUDIO");
			break;
		case B_MEDIA_RAW_VIDEO:
			context.writeString("B_MEDIA_RAW_VIDEO");
			break;
		case B_MEDIA_VBL:
			context.writeString("B_MEDIA_VBL");
			break;
		case B_MEDIA_TIMECODE:
			context.writeString("B_MEDIA_TIMECODE");
			break;
		case B_MEDIA_MIDI:
			context.writeString("B_MEDIA_MIDI");
			break;
		case B_MEDIA_TEXT:
			context.writeString("B_MEDIA_TEXT");
			break;
		case B_MEDIA_HTML:
			context.writeString("B_MEDIA_HTML");
			break;
		case B_MEDIA_MULTISTREAM:
			context.writeString("B_MEDIA_MULTISTREAM");
			break;
		case B_MEDIA_PARAMETERS:
			context.writeString("B_MEDIA_PARAMETERS");
			break;
		case B_MEDIA_ENCODED_AUDIO:
			context.writeString("B_MEDIA_ENCODED_AUDIO");
			break;
		case B_MEDIA_ENCODED_VIDEO:
			context.writeString("B_MEDIA_ENCODED_VIDEO");
			break;
		default: {
			BString val;
			val << (uint32)t;
			context.writeString(val);
		}
	}
	context.endElement();
}

void import_media_type_content(
	media_multistream_format::avi_info& f,
	const char* value,
	ImportContext& context) {
	
	if(f.type_count == 5) {
		// ignore
		// +++++ should this be an error?
		context.reportWarning("Ignoring media_type: maximum of 5 reached.");
		return;
	}

	if(!strcmp(value, "B_MEDIA_NO_TYPE"))
		f.types[f.type_count] = B_MEDIA_NO_TYPE;
	else if(!strcmp(value, "B_MEDIA_UNKNOWN_TYPE"))
		f.types[f.type_count] = B_MEDIA_UNKNOWN_TYPE;
	else if(!strcmp(value, "B_MEDIA_RAW_AUDIO"))
		f.types[f.type_count] = B_MEDIA_RAW_AUDIO;
	else if(!strcmp(value, "B_MEDIA_RAW_VIDEO"))
		f.types[f.type_count] = B_MEDIA_RAW_VIDEO;
	else if(!strcmp(value, "B_MEDIA_VBL"))
		f.types[f.type_count] = B_MEDIA_VBL;
	else if(!strcmp(value, "B_MEDIA_TIMECODE"))
		f.types[f.type_count] = B_MEDIA_TIMECODE;
	else if(!strcmp(value, "B_MEDIA_MIDI"))
		f.types[f.type_count] = B_MEDIA_MIDI;
	else if(!strcmp(value, "B_MEDIA_TEXT"))
		f.types[f.type_count] = B_MEDIA_TEXT;
	else if(!strcmp(value, "B_MEDIA_HTML"))
		f.types[f.type_count] = B_MEDIA_HTML;
	else if(!strcmp(value, "B_MEDIA_MULTISTREAM"))
		f.types[f.type_count] = B_MEDIA_MULTISTREAM;
	else if(!strcmp(value, "B_MEDIA_PARAMETERS"))
		f.types[f.type_count] = B_MEDIA_PARAMETERS;
	else if(!strcmp(value, "B_MEDIA_ENCODED_AUDIO"))
		f.types[f.type_count] = B_MEDIA_ENCODED_AUDIO;
	else if(!strcmp(value, "B_MEDIA_ENCODED_VIDEO"))
		f.types[f.type_count] = B_MEDIA_ENCODED_VIDEO;
	else
		f.types[f.type_count] = (media_type)atol(value);

	++f.type_count;
}
	

void export_raw_audio_attr(
	const media_raw_audio_format& f,
	ExportContext& context) {

	media_raw_audio_format& w = media_raw_audio_format::wildcard;

	if(f.frame_rate != w.frame_rate)
		context.writeAttr(gKey_frame_rate, f.frame_rate);	
	if(f.channel_count != w.channel_count)
		context.writeAttr(gKey_channel_count, f.channel_count);
	if(f.buffer_size != w.buffer_size)
		context.writeAttr(gKey_buffer_size, f.buffer_size);

	switch(f.format) {
		case media_raw_audio_format::B_AUDIO_UCHAR:
			context.writeAttr(gKey_format, "B_AUDIO_UCHAR");
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			context.writeAttr(gKey_format, "B_AUDIO_SHORT");
			break;
		case media_raw_audio_format::B_AUDIO_FLOAT:
			context.writeAttr(gKey_format, "B_AUDIO_FLOAT");
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			context.writeAttr(gKey_format, "B_AUDIO_INT");
			break;
		default:
			break;
	}
	
	switch(f.byte_order) {
		case B_MEDIA_BIG_ENDIAN:
			context.writeAttr(gKey_byte_order, "B_MEDIA_BIG_ENDIAN");
			break;
		case B_MEDIA_LITTLE_ENDIAN:
			context.writeAttr(gKey_byte_order, "B_MEDIA_LITTLE_ENDIAN");
			break;
		default:
			break;
	}
}

#if B_BEOS_VERSION > B_BEOS_VERSION_4_5

void export_multi_audio_info_attr(
	const media_multi_audio_info& f,
	ExportContext& context) {

	media_multi_audio_format& w = media_multi_audio_format::wildcard;

	if(f.channel_mask != w.channel_mask)
		context.writeAttr(gKey_channel_mask, f.channel_mask);

	if(f.valid_bits != w.valid_bits)
		context.writeAttr(gKey_valid_bits, f.valid_bits);

	if(f.matrix_mask != w.matrix_mask)
		context.writeAttr(gKey_matrix_mask, f.matrix_mask);
}
#endif

void export_video_display_info_attr(
	const media_video_display_info& d,
	ExportContext& context) {

	media_video_display_info& w = media_video_display_info::wildcard;
	
	if(d.line_width != w.line_width)
		context.writeAttr(gKey_line_width, d.line_width);
	if(d.line_count != w.line_count)
		context.writeAttr(gKey_line_count, d.line_count);
	if(d.bytes_per_row != w.bytes_per_row)
		context.writeAttr(gKey_bytes_per_row, d.bytes_per_row);
	if(d.pixel_offset != w.pixel_offset)
		context.writeAttr(gKey_pixel_offset, d.pixel_offset);
	if(d.line_offset != w.line_offset)
		context.writeAttr(gKey_line_offset, d.line_offset);

	if(d.format != w.format)
		write_colorspace_attr(gKey_format, d.format, context);
}


void export_raw_video_attr(
	const media_raw_video_format& f,
	ExportContext& context) {

	media_raw_video_format& w = media_raw_video_format::wildcard;

	// attributes
	if(f.field_rate != w.field_rate)
		context.writeAttr(gKey_field_rate, f.field_rate);	
	if(f.interlace != w.interlace)
		context.writeAttr(gKey_interlace, f.interlace);	
	if(f.first_active != w.first_active)
		context.writeAttr(gKey_first_active, f.first_active);	
	if(f.last_active != w.last_active)
		context.writeAttr(gKey_last_active, f.last_active);	
	if(f.pixel_width_aspect != w.pixel_width_aspect)
		context.writeAttr(gKey_pixel_width_aspect, (uint32)f.pixel_width_aspect);	
	if(f.pixel_height_aspect != w.pixel_height_aspect)
		context.writeAttr(gKey_pixel_height_aspect, (uint32)f.pixel_height_aspect);	

	switch(f.orientation) {
		case B_VIDEO_TOP_LEFT_RIGHT:
			context.writeAttr(gKey_orientation, "B_VIDEO_TOP_LEFT_RIGHT");
			break;
		case B_VIDEO_BOTTOM_LEFT_RIGHT:
			context.writeAttr(gKey_orientation, "B_VIDEO_BOTTOM_LEFT_RIGHT");
			break;
		default:
			break;
	}
}

void export_raw_video_content(
	const media_raw_video_format& f,
	ExportContext& context) {
	
	context.beginContent();
	context.beginElement(MediaFormatIO::s_video_display_info_tag);
	export_video_display_info_attr(f.display, context);
	context.endElement();
}

void export_multistream_flags_attr(
	uint32 flags,
	ExportContext& context) {

	if(flags & media_multistream_format::B_HEADER_HAS_FLAGS)
		context.writeAttr(gKey_header_has_flags, (int32)1);

	if(flags & media_multistream_format::B_CLEAN_BUFFERS)
		context.writeAttr(gKey_clean_buffers, (int32)1);

	if(flags & media_multistream_format::B_HOMOGENOUS_BUFFERS)
		context.writeAttr(gKey_homogenous_buffers, (int32)1);
}

void export_multistream_vid_info_attr(
	media_multistream_format::vid_info f,
	ExportContext& context) {
	
	// +++++ no wildcard to compare against (assume 0 == wildcard?)
	
	context.writeAttr(gKey_frame_rate, f.frame_rate);
	context.writeAttr(gKey_width, (uint32)f.width);
	context.writeAttr(gKey_height, (uint32)f.height);
	write_colorspace_attr(gKey_space, f.space, context);
	context.writeAttr(gKey_sampling_rate, f.sampling_rate);
	
	switch(f.sample_format) {
		case B_UNDEFINED_SAMPLES:
			context.writeAttr(gKey_sample_format, "B_UNDEFINED_SAMPLES");
			break;
		case B_LINEAR_SAMPLES:
			context.writeAttr(gKey_sample_format, "B_LINEAR_SAMPLES");
			break;
		case B_FLOAT_SAMPLES:
			context.writeAttr(gKey_sample_format, "B_FLOAT_SAMPLES");
			break;
		case B_MULAW_SAMPLES:
			context.writeAttr(gKey_sample_format, "B_MULAW_SAMPLES");
			break;
		default:
			break;
	}

	switch(f.byte_order) {
		case B_MEDIA_BIG_ENDIAN:
			context.writeAttr(gKey_byte_order, "B_MEDIA_BIG_ENDIAN");
			break;
		case B_MEDIA_LITTLE_ENDIAN:
			context.writeAttr(gKey_byte_order, "B_MEDIA_LITTLE_ENDIAN");
			break;
		default:
			break;
	}
	
	context.writeAttr(gKey_channel_count, (uint32)f.channel_count);
}

void export_multistream_avi_info_attr(
	media_multistream_format::avi_info f,
	ExportContext& context) {

	context.writeAttr(gKey_us_per_frame, f.us_per_frame);
	context.writeAttr(gKey_width, (uint32)f.width);
	context.writeAttr(gKey_height, (uint32)f.height);
}

void export_multistream_avi_info_content(
	media_multistream_format::avi_info f,
	ExportContext& context) {

	context.beginContent();
		
	for(uint16 n = 0; n < f.type_count; ++n)
		write_media_type(f.types[n], context);
}

void export_multistream_attr(
	const media_multistream_format& f,
	ExportContext& context) {
	
	media_multistream_format& w = media_multistream_format::wildcard;

	// attributes
	switch(f.format) {
		case media_multistream_format::B_ANY:
			context.writeAttr(gKey_multistream_format, "B_ANY");
			break;
		case media_multistream_format::B_VID:
			context.writeAttr(gKey_multistream_format, "B_VID");
			break;
		case media_multistream_format::B_AVI:
			context.writeAttr(gKey_multistream_format, "B_AVI");
			break;
		case media_multistream_format::B_MPEG1:
			context.writeAttr(gKey_multistream_format, "B_MPEG1");
			break;
		case media_multistream_format::B_MPEG2:
			context.writeAttr(gKey_multistream_format, "B_MPEG2");
			break;
		case media_multistream_format::B_QUICKTIME:
			context.writeAttr(gKey_multistream_format, "B_QUICKTIME");
			break;
		default:
			if(f.format != w.format) {
				// write numeric value
				context.writeAttr(gKey_multistream_format, f.format);
			}
			break;
	}

	if(f.avg_bit_rate != w.avg_bit_rate)
		context.writeAttr(gKey_avg_bit_rate, f.avg_bit_rate);
	if(f.max_bit_rate != w.max_bit_rate)
		context.writeAttr(gKey_max_bit_rate, f.max_bit_rate);
	if(f.avg_chunk_size != w.avg_chunk_size)
		context.writeAttr(gKey_avg_chunk_size, f.avg_chunk_size);
	if(f.max_chunk_size != w.max_chunk_size)
		context.writeAttr(gKey_max_chunk_size, f.max_chunk_size);
}

void export_multistream_content(
	const media_multistream_format& f,
	ExportContext& context) {

	context.beginContent();
	
	// write flags
	context.beginElement(MediaFormatIO::s_multistream_flags_tag);
	export_multistream_flags_attr(f.flags, context);
	context.endElement();
	
	// write format-specific info
	if(f.format == media_multistream_format::B_VID) {
		context.beginElement(MediaFormatIO::s_multistream_vid_info_tag);
		export_multistream_vid_info_attr(f.u.vid, context);
		context.endElement();
	}
	else if(f.format == media_multistream_format::B_AVI) {
		context.beginElement(MediaFormatIO::s_multistream_avi_info_tag);
		export_multistream_avi_info_attr(f.u.avi, context);
		context.beginContent();
		export_multistream_avi_info_content(f.u.avi, context);
		context.endElement();
	}
}

void export_encoded_audio_attr(
	const media_encoded_audio_format& f,
	ExportContext& context) {
	
	media_encoded_audio_format& w = media_encoded_audio_format::wildcard;

	switch(f.encoding) {
		case media_encoded_audio_format::B_ANY:
			context.writeAttr(gKey_encoding, "B_ANY");
			break;
		default:
			break;
	}

	if(f.bit_rate != w.bit_rate)
		context.writeAttr(gKey_bit_rate, f.bit_rate);

	if(f.frame_size != w.frame_size)
		context.writeAttr(gKey_frame_size, f.frame_size);
}

void export_encoded_audio_content(
	const media_encoded_audio_format& f,
	ExportContext& context) {
	
	context.beginContent();

	context.beginElement(MediaFormatIO::s_raw_audio_tag);
	export_raw_audio_attr(f.output, context);

#if B_BEOS_VERSION > B_BEOS_VERSION_4_5
	export_multi_audio_info_attr(f.multi_info, context);
#endif

	context.endElement();
}

void export_encoded_video_attr(
	const media_encoded_video_format& f,
	ExportContext& context) {

	media_encoded_video_format& w = media_encoded_video_format::wildcard;

	switch(f.encoding) {
		case media_encoded_video_format::B_ANY:
			context.writeAttr(gKey_encoding, "B_ANY");
			break;
		default:
			break;
	}

	if(f.avg_bit_rate != w.avg_bit_rate)
		context.writeAttr(gKey_avg_bit_rate, f.avg_bit_rate);
	if(f.max_bit_rate != w.max_bit_rate)
		context.writeAttr(gKey_max_bit_rate, f.max_bit_rate);
	if(f.frame_size != w.frame_size)
		context.writeAttr(gKey_frame_size, f.frame_size);
	if(f.forward_history != w.forward_history)
		context.writeAttr(gKey_forward_history, (int32)f.forward_history);
	if(f.backward_history != w.backward_history)
		context.writeAttr(gKey_backward_history, (int32)f.backward_history);
}

void export_encoded_video_content(
	const media_encoded_video_format& f,
	ExportContext& context) {

	context.beginContent();

	context.beginElement(MediaFormatIO::s_raw_video_tag);
	export_raw_video_attr(f.output, context);
	context.endElement();
}


void MediaFormatIO::xmlExportBegin(
	ExportContext&		context) const {
	
	switch(m_format.type) {

		case B_MEDIA_RAW_AUDIO:
			context.beginElement(s_raw_audio_tag);
			break;
		
		case B_MEDIA_RAW_VIDEO:
			context.beginElement(s_raw_video_tag);
			break;
			
		case B_MEDIA_MULTISTREAM:
			context.beginElement(s_multistream_tag);
			break;
			
		case B_MEDIA_ENCODED_AUDIO:
			context.beginElement(s_encoded_audio_tag);
			break;
			
		case B_MEDIA_ENCODED_VIDEO:
			context.beginElement(s_encoded_video_tag);
			break;
			
		default:
			// +++++ not very polite
			context.reportError("MediaFormatIO: type not supported\n");
			break;
	}	
}

void MediaFormatIO::xmlExportAttributes(
	ExportContext&		context) const {
	
	switch(m_format.type) {
		case B_MEDIA_RAW_AUDIO:
			export_raw_audio_attr(m_format.u.raw_audio, context);
#if B_BEOS_VERSION > B_BEOS_VERSION_4_5
			export_multi_audio_info_attr(m_format.u.raw_audio, context);
#endif
			break;
		
		case B_MEDIA_RAW_VIDEO:
			export_raw_video_attr(m_format.u.raw_video, context);
			break;
			
		case B_MEDIA_MULTISTREAM:
			export_multistream_attr(m_format.u.multistream, context);
			break;
			
		case B_MEDIA_ENCODED_AUDIO:
			export_encoded_audio_attr(m_format.u.encoded_audio, context);
			break;
			
		case B_MEDIA_ENCODED_VIDEO:
			export_encoded_video_attr(m_format.u.encoded_video, context);
			break;
			
		default:
			break;
	}	
}

void MediaFormatIO::xmlExportContent(
	ExportContext&		context) const {
	
	switch(m_format.type) {
		case B_MEDIA_RAW_VIDEO:
			export_raw_video_content(m_format.u.raw_video, context);
			break;
			
		case B_MEDIA_MULTISTREAM:
			export_multistream_content(m_format.u.multistream, context);
			break;
			
		case B_MEDIA_ENCODED_AUDIO:
			export_encoded_audio_content(m_format.u.encoded_audio, context);
			break;
			
		case B_MEDIA_ENCODED_VIDEO:
			export_encoded_video_content(m_format.u.encoded_video, context);
			break;
			
		default:
			break;
	}	
}

void MediaFormatIO::xmlExportEnd(
	ExportContext&		context) const {

	context.endElement();
}

// -------------------------------------------------------- //
// import
// -------------------------------------------------------- //

void import_raw_audio_attribute(
	media_raw_audio_format& f,
	const char* key,
	const char* value,
	ImportContext& context) {
	
	if(!strcmp(key, gKey_frame_rate))
		f.frame_rate = atof(value);
	else if(!strcmp(key, gKey_channel_count))
		f.channel_count = atol(value);
	else if(!strcmp(key, gKey_buffer_size))
		f.buffer_size = atol(value);
	else if(!strcmp(key, gKey_format)) {
		if(!strcmp(value, "B_AUDIO_UCHAR"))
			f.format = media_raw_audio_format::B_AUDIO_UCHAR;
		else if(!strcmp(value, "B_AUDIO_SHORT"))
			f.format = media_raw_audio_format::B_AUDIO_SHORT;
		else if(!strcmp(value, "B_AUDIO_FLOAT"))
			f.format = media_raw_audio_format::B_AUDIO_FLOAT;
		else if(!strcmp(value, "B_AUDIO_INT"))
			f.format = media_raw_audio_format::B_AUDIO_INT;
	}
	else if(!strcmp(key, gKey_byte_order)) {
		if(!strcmp(value, "B_MEDIA_BIG_ENDIAN"))
			f.byte_order = B_MEDIA_BIG_ENDIAN;
		else if(!strcmp(value, "B_MEDIA_LITTLE_ENDIAN"))
			f.byte_order = B_MEDIA_LITTLE_ENDIAN;
	}
}

#if B_BEOS_VERSION > B_BEOS_VERSION_4_5

void import_multi_audio_info_attribute(
	media_multi_audio_info& f,
	const char* key,
	const char* value,
	ImportContext& context) {
	
	if(!strcmp(key, gKey_channel_mask))
		f.channel_mask = atol(value);
	else if(!strcmp(key, gKey_valid_bits))
		f.valid_bits = atoi(value);
	else if(!strcmp(key, gKey_matrix_mask))
		f.matrix_mask = atoi(value);		
}

#endif

void import_raw_video_attribute(
	media_raw_video_format& f,
	const char* key,
	const char* value,
	ImportContext& context) {

	if(!strcmp(key, gKey_field_rate))
		f.field_rate = atof(value);
	else if(!strcmp(key, gKey_interlace))
		f.interlace = atol(value);
	else if(!strcmp(key, gKey_first_active))
		f.first_active = atol(value);
	else if(!strcmp(key, gKey_last_active))
		f.last_active = atol(value);
	else if(!strcmp(key, gKey_pixel_width_aspect))
		f.pixel_width_aspect = atol(value);
	else if(!strcmp(key, gKey_pixel_height_aspect))
		f.pixel_height_aspect = atol(value);
	else if(!strcmp(key, gKey_orientation)) {
		if(!strcmp(value, "B_VIDEO_TOP_LEFT_RIGHT"))
			f.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		else if(!strcmp(value, "B_VIDEO_BOTTOM_LEFT_RIGHT"))
			f.orientation = B_VIDEO_BOTTOM_LEFT_RIGHT;
	}
}

void import_video_display_info_attribute(
	media_video_display_info& d,
	const char* key,
	const char* value,
	ImportContext& context) {
	
	if(!strcmp(key, gKey_line_width))
		d.line_width = atol(value);
	else if(!strcmp(key, gKey_line_count))
		d.line_count = atol(value);
	else if(!strcmp(key, gKey_bytes_per_row))
		d.bytes_per_row = atol(value);
	else if(!strcmp(key, gKey_pixel_offset))
		d.pixel_offset = atol(value);
	else if(!strcmp(key, gKey_line_offset))
		d.line_offset = atol(value);
	else if(!strcmp(key, gKey_format)) {
		import_color_space(value, d.format);
	}
}

void import_multistream_attribute(
	media_multistream_format& f,
	const char* key,
	const char* value,
	ImportContext& context) {

	if(!strcmp(key, gKey_format)) {
		if(!strcmp(value, "B_ANY"))
			f.format = media_multistream_format::B_ANY;
		else if(!strcmp(value, "B_VID"))
			f.format = media_multistream_format::B_VID;
		else if(!strcmp(value, "B_AVI"))
			f.format = media_multistream_format::B_AVI;
		else if(!strcmp(value, "B_MPEG1"))
			f.format = media_multistream_format::B_MPEG1;
		else if(!strcmp(value, "B_MPEG2"))
			f.format = media_multistream_format::B_MPEG2;
		else if(!strcmp(value, "B_QUICKTIME"))
			f.format = media_multistream_format::B_QUICKTIME;
		else
			f.format = atol(value);
	}
	else if(!strcmp(key, gKey_avg_bit_rate))
		f.avg_bit_rate = atof(value);
	else if(!strcmp(key, gKey_max_bit_rate))
		f.max_bit_rate = atof(value);
	else if(!strcmp(key, gKey_avg_chunk_size))
		f.avg_chunk_size = atol(value);
	else if(!strcmp(key, gKey_max_chunk_size))
		f.max_chunk_size = atol(value);
}

void import_multistream_flags_attribute(
	uint32& flags,
	const char* key,
	const char* value,
	ImportContext& context) {

	if(!atol(value))
		return;

	if(!strcmp(key, gKey_header_has_flags))
		flags |= media_multistream_format::B_HEADER_HAS_FLAGS;
	else if(!strcmp(key, gKey_clean_buffers))
		flags |= media_multistream_format::B_CLEAN_BUFFERS;
	else if(!strcmp(key, gKey_homogenous_buffers))
		flags |= media_multistream_format::B_HOMOGENOUS_BUFFERS;
}

void import_multistream_vid_info_attribute(
	media_multistream_format::vid_info& f,
	const char* key,
	const char* value,
	ImportContext& context) {

	if(!strcmp(key, gKey_frame_rate))
		f.frame_rate = atof(value);
	else if(!strcmp(key, gKey_width))
		f.width = atol(value);
	else if(!strcmp(key, gKey_height))
		f.height = atol(value);
	else if(!strcmp(key, gKey_space))
		import_color_space(value, f.space);
	else if(!strcmp(key, gKey_sampling_rate))
		f.sampling_rate = atof(value);
	else if(!strcmp(key, gKey_channel_count))
		f.channel_count = atol(value);
	else if(!strcmp(key, gKey_sample_format)) {
		if(!strcmp(value, "B_UNDEFINED_SAMPLES"))
			f.sample_format = B_UNDEFINED_SAMPLES;
		else if(!strcmp(value, "B_LINEAR_SAMPLES"))
			f.sample_format = B_LINEAR_SAMPLES;
		else if(!strcmp(value, "B_FLOAT_SAMPLES"))
			f.sample_format = B_FLOAT_SAMPLES;
		else if(!strcmp(value, "B_MULAW_SAMPLES"))
			f.sample_format = B_MULAW_SAMPLES;
	}
	else if(!strcmp(key, gKey_byte_order)) {
		if(!strcmp(value, "B_MEDIA_BIG_ENDIAN"))
			f.byte_order = B_MEDIA_BIG_ENDIAN;
		else if(!strcmp(value, "B_MEDIA_LITTLE_ENDIAN"))
			f.byte_order = B_MEDIA_LITTLE_ENDIAN;
	}
}

void import_multistream_avi_info_attribute(
	media_multistream_format::avi_info& f,
	const char* key,
	const char* value,
	ImportContext& context) {
	
	if(!strcmp(key, gKey_us_per_frame))
		f.us_per_frame = atol(value);
	else if(!strcmp(key, gKey_width))
		f.width = atol(value);
	else if(!strcmp(key, gKey_height))
		f.height = atol(value);
}

void import_encoded_audio_attribute(
	media_encoded_audio_format& f,
	const char* key,
	const char* value,
	ImportContext& context) {

	if(!strcmp(key, gKey_encoding)) {
		if(!strcmp(value, "B_ANY"))
			f.encoding = media_encoded_audio_format::B_ANY;
	}
	else if(!strcmp(key, gKey_bit_rate))
		f.bit_rate = atof(value);
	else if(!strcmp(key, gKey_frame_size))
		f.frame_size = atol(value);
}
			
void import_encoded_video_attribute(
	media_encoded_video_format& f,
	const char* key,
	const char* value,
	ImportContext& context) {
	
	if(!strcmp(key, gKey_encoding)) {
		if(!strcmp(value, "B_ANY"))
			f.encoding = media_encoded_video_format::B_ANY;
	}
	else if(!strcmp(key, gKey_avg_bit_rate))
		f.avg_bit_rate = atof(value);
	else if(!strcmp(key, gKey_max_bit_rate))
		f.max_bit_rate = atof(value);
	else if(!strcmp(key, gKey_frame_size))
		f.frame_size = atol(value);
	else if(!strcmp(key, gKey_forward_history))
		f.forward_history = atol(value);
	else if(!strcmp(key, gKey_backward_history))
		f.backward_history = atol(value);
}

// -------------------------------------------------------- //

void MediaFormatIO::xmlImportBegin(
	ImportContext&		context) {

	// initialize format
	if(!strcmp(context.element(), s_raw_audio_tag)) {
		m_format.type = B_MEDIA_RAW_AUDIO;
		m_format.u.raw_audio = media_raw_audio_format::wildcard;
	}
	else if(!strcmp(context.element(), s_raw_video_tag)) {
		m_format.type = B_MEDIA_RAW_VIDEO;
		m_format.u.raw_video = media_raw_video_format::wildcard;
	}
	else if(!strcmp(context.element(), s_multistream_tag)) {
		m_format.type = B_MEDIA_MULTISTREAM;
		m_format.u.multistream = media_multistream_format::wildcard;
	}
	else if(!strcmp(context.element(), s_encoded_audio_tag)) {
		m_format.type = B_MEDIA_ENCODED_AUDIO;
		m_format.u.encoded_audio = media_encoded_audio_format::wildcard;
	}
	else if(!strcmp(context.element(), s_encoded_video_tag)) {
		m_format.type = B_MEDIA_ENCODED_VIDEO;
		m_format.u.encoded_video = media_encoded_video_format::wildcard;
	}
	else
		context.reportError("Bad element mapping?  MediaFormatIO can't cope.");
}
	
void MediaFormatIO::xmlImportAttribute(
	const char*					key,
	const char*					value,
	ImportContext&		context) {
	switch(m_format.type) {
		case B_MEDIA_RAW_AUDIO:
			import_raw_audio_attribute(
				m_format.u.raw_audio, key, value, context);

#if B_BEOS_VERSION > B_BEOS_VERSION_4_5
			import_multi_audio_info_attribute(
				m_format.u.raw_audio, key, value, context);
#endif
			break;
			
		case B_MEDIA_RAW_VIDEO:
			import_raw_video_attribute(
				m_format.u.raw_video, key, value, context);
			break;

		case B_MEDIA_MULTISTREAM:
			import_multistream_attribute(
				m_format.u.multistream, key, value, context);
			break;
			
		case B_MEDIA_ENCODED_AUDIO:
			import_encoded_audio_attribute(
				m_format.u.encoded_audio, key, value, context);
			break;
			
		case B_MEDIA_ENCODED_VIDEO:
			import_encoded_video_attribute(
				m_format.u.encoded_video, key, value, context);
			break;
		
		default:
			context.reportError("MediaFormatIO: bad type.");
	}
}

void MediaFormatIO::xmlImportContent(
	const char*					data,
	uint32						length,
	ImportContext&		context) {}
		
void MediaFormatIO::xmlImportChild(
	IPersistent*			child,
	ImportContext&		context) {

	MediaFormatIO* childAsFormat = dynamic_cast<MediaFormatIO*>(child);
	if(m_format.type == B_MEDIA_ENCODED_AUDIO) {
		if(!childAsFormat || childAsFormat->m_format.type != B_MEDIA_RAW_AUDIO)
			context.reportError("Expected a raw_audio_format.");
		m_format.u.encoded_audio.output = childAsFormat->m_format.u.raw_audio;
	}
	else if(m_format.type == B_MEDIA_ENCODED_VIDEO) {
		if(!childAsFormat || childAsFormat->m_format.type != B_MEDIA_RAW_VIDEO)
			context.reportError("Expected a raw_video_format.");
		m_format.u.encoded_video.output = childAsFormat->m_format.u.raw_video;
	}
	else {
		// +++++ should this be an error?
		context.reportWarning("MediaFormatIO: Unexpected child element.");
	}
	delete child;
}
		
void MediaFormatIO::xmlImportComplete(
	ImportContext&		context) {
	
	// +++++ validity checks?
	
	m_complete = true;
}
		
void MediaFormatIO::xmlImportChildBegin(
	const char*					name,
	ImportContext&		context) {
	
	if(!strcmp(name, s_video_display_info_tag)) {
		if(m_format.type != B_MEDIA_RAW_VIDEO)
			context.reportError("MediaFormatIO: unexpected element.");
	}
	else if(!strcmp(name, s_multistream_flags_tag)) {
		if(m_format.type != B_MEDIA_MULTISTREAM)
			context.reportError("MediaFormatIO: unexpected element.");
	}
	else if(!strcmp(name, s_multistream_vid_info_tag)) {
		if(m_format.type != B_MEDIA_MULTISTREAM ||
			m_format.u.multistream.format != media_multistream_format::B_VID)
			context.reportError("MediaFormatIO: unexpected element.");
	}
	else if(!strcmp(name, s_multistream_avi_info_tag)) {
		if(m_format.type != B_MEDIA_MULTISTREAM ||
			m_format.u.multistream.format != media_multistream_format::B_AVI)
			context.reportError("MediaFormatIO: unexpected element.");
	}
	else if(!strcmp(name, s_media_type_tag)) {
		if(m_format.type != B_MEDIA_MULTISTREAM ||
			m_format.u.multistream.format != media_multistream_format::B_AVI)
			context.reportError("MediaFormatIO: unexpected element.");
	}
}

void MediaFormatIO::xmlImportChildAttribute(
	const char*					key,
	const char*					value,
	ImportContext&		context) {

	if(!strcmp(context.element(), s_video_display_info_tag))
		import_video_display_info_attribute(
			m_format.u.raw_video.display, key, value, context);

	else if(!strcmp(context.element(), s_multistream_flags_tag	))
		import_multistream_flags_attribute(
			m_format.u.multistream.flags, key, value, context);

	else if(!strcmp(context.element(), s_multistream_vid_info_tag	))
		import_multistream_vid_info_attribute(
			m_format.u.multistream.u.vid, key, value, context);

	else if(!strcmp(context.element(), s_multistream_avi_info_tag	))
		import_multistream_avi_info_attribute(
			m_format.u.multistream.u.avi, key, value, context);
			
	else
		context.reportError("MediaFormatIO: bad child element.");
}

void MediaFormatIO::xmlImportChildContent(
	const char*					data,
	uint32						length,
	ImportContext&		context) {

	if(!strcmp(context.element(), s_media_type_tag)) {
		m_mediaType.Append(data, length);
	}
}
		
void MediaFormatIO::xmlImportChildComplete(
	const char*					name,
	ImportContext&		context) {

	if(!strcmp(context.element(), s_media_type_tag)) {
		import_media_type_content(
			m_format.u.multistream.u.avi,
			m_mediaType.String(),	context);

		m_mediaType = "";
	}
}


// END -- MediaFormatIO.cpp --
