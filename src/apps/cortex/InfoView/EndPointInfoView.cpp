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


// EndPointInfoView.cpp

#include "EndPointInfoView.h"
// InfoView
#include "InfoWindowManager.h"
// Support
#include "MediaIcon.h"
#include "MediaString.h"

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor (public)
// -------------------------------------------------------- //

EndPointInfoView::EndPointInfoView(
	const media_input &input)
	: InfoView(input.name, "Media input", 0),
	  m_output(false),
	  m_port(input.destination.port),
	  m_id(input.destination.id) {
	D_METHOD(("EndPointInfoView::EndPointInfoView(input)\n"));

	setSideBarWidth(be_plain_font->StringWidth(" Destination ") 
					+ 2 * InfoView::M_H_MARGIN);

	// add "Source" field
	addField("Source", MediaString::getStringFor(input.source));

	// add "Destination" field
	addField("Destination", MediaString::getStringFor(input.destination));
	
	// add a separator field
	addField("", "");

	// add "Media Type" field
	addField("Media type", MediaString::getStringFor(input.format.type));

	_addFormatFields(input.format);
}

EndPointInfoView::EndPointInfoView(
	const media_output &output)
	: InfoView(output.name, "Media output", 0),
	  m_output(true),
	  m_port(output.source.port),
	  m_id(output.source.id) {
	D_METHOD(("EndPointInfoView::EndPointInfoView(output)\n"));

	setSideBarWidth(be_plain_font->StringWidth(" Destination ") 
					+ 2 * InfoView::M_H_MARGIN);

	// add "Source" field
	addField("Source", MediaString::getStringFor(output.source));

	// add "Destination" field
	addField("Destination", MediaString::getStringFor(output.destination));
	
	// add a separator field
	addField("", "");

	// add "Media Type" field
	addField("Media type", MediaString::getStringFor(output.format.type));

	_addFormatFields(output.format);
}

EndPointInfoView::~EndPointInfoView()
{
	D_METHOD(("EndPointInfoView::~EndPointInfoView()\n"));
}

// -------------------------------------------------------- //
// *** BView implementation (public)
// -------------------------------------------------------- //

void EndPointInfoView::DetachedFromWindow() {
	D_METHOD(("EndPointInfoView::DetachedFromWindow()\n"));

	InfoWindowManager *manager = InfoWindowManager::Instance();
	if (manager) {
		if (m_output) {
			BMessage message(InfoWindowManager::M_OUTPUT_WINDOW_CLOSED);
			message.AddInt32("source_port", m_port);
			message.AddInt32("source_id", m_id);
			manager->PostMessage(&message);
		}
		else {
			BMessage message(InfoWindowManager::M_INPUT_WINDOW_CLOSED);
			message.AddInt32("destination_port", m_port);
			message.AddInt32("destination_id", m_id);
			manager->PostMessage(&message);
		}
	}
}

// -------------------------------------------------------- //
// *** internal operations (private)
// -------------------------------------------------------- //

void EndPointInfoView::_addFormatFields(
	const media_format &format)
{
	D_METHOD(("EndPointInfoView::_addFormatFields()\n"));

	switch (format.type) {
		case B_MEDIA_RAW_AUDIO: {
			// adjust view properties
			setSideBarWidth(be_plain_font->StringWidth(" Sample Rate ") + 2 * InfoView::M_H_MARGIN);
			BString s;
			// add "Format" field
			s = MediaString::forAudioFormat(format.u.raw_audio.format,
											format.u.raw_audio.valid_bits);
			addField("Format", s);
			// add "Sample Rate" field
			s = MediaString::forAudioFrameRate(format.u.raw_audio.frame_rate);
			addField("Sample rate", s);
			// add "Channels" field
			s = MediaString::forAudioChannelCount(format.u.raw_audio.channel_count);
			addField("Channels", s);
			// add "Channel Mask" field
			s = MediaString::forAudioChannelMask(format.u.raw_audio.channel_mask);
			addField("Channel mask", s);
			// add "Matrix Mask" field
			s = MediaString::forAudioMatrixMask(format.u.raw_audio.matrix_mask);
			addField("Matrix mask", s);
			// add the "Byte Order" field
			s = MediaString::forAudioByteOrder(format.u.raw_audio.byte_order);
			addField("Byte order", s);
			// add the "Buffer Size" field
			s = MediaString::forAudioBufferSize(format.u.raw_audio.buffer_size);
			addField("Buffer size", s);
			break;
		}
		case B_MEDIA_RAW_VIDEO: {
			// adjust view properties
			setSideBarWidth(be_plain_font->StringWidth(" Video Data Between ") + 2 * InfoView::M_H_MARGIN);
			BString s;
			// add the "Format" field
			s = MediaString::forVideoFormat(format.u.raw_video.display.format);
			addField("Format", s);
			// add the "Resolution" field
			s = MediaString::forVideoResolution(format.u.raw_video.display.line_width,
												format.u.raw_video.display.line_count);
			addField("Resolution", s);
			// add the "Field Rate" field
			s = MediaString::forVideoFieldRate(format.u.raw_video.field_rate,
											   format.u.raw_video.interlace);
			addField("Field rate", s);
			// add the "Orientation" field
			s = MediaString::forVideoOrientation(format.u.raw_video.orientation);
			addField("Orientation", s);
			// add the "Aspect Ratio" field
			s = MediaString::forVideoAspectRatio(format.u.raw_video.pixel_width_aspect,
												 format.u.raw_video.pixel_height_aspect);
			addField("Aspect ratio", s);
			// add the "Active Lines" field
			s = MediaString::forVideoActiveLines(format.u.raw_video.first_active,
												 format.u.raw_video.last_active);
			addField("Active lines", s);
			// add the "Offset" field
			s = MediaString::forVideoOffset(format.u.raw_video.display.pixel_offset,
											format.u.raw_video.display.line_offset);			
			addField("Offset", s);
			break;
		}
		case B_MEDIA_ENCODED_AUDIO: {
			// adjust view properties
			setSideBarWidth(be_plain_font->StringWidth(" Frame Size ") + 2 * InfoView::M_H_MARGIN);
			BString s;
			// add the "Bit Rate" field
			s = MediaString::forAudioBitRate(format.u.encoded_audio.bit_rate);
			addField("Bit rate", s);
			// add the "Frame Size" field
			s = MediaString::forAudioFrameSize(format.u.encoded_audio.frame_size);
			addField("Frame size", s);
			break;
		}
		case B_MEDIA_ENCODED_VIDEO: {
			// adjust view properties
			setSideBarWidth(be_plain_font->StringWidth(" Frame Size ") + 2 * InfoView::M_H_MARGIN);
			BString s;
			// add the "Bit Rate" field
			s = MediaString::forVideoBitRate(format.u.encoded_video.avg_bit_rate,
											 format.u.encoded_video.max_bit_rate);
			addField("Bit rate", s);
			// add the "Frame Size" field
			s = MediaString::forVideoFrameSize(format.u.encoded_video.frame_size);
			addField("Frame size", s);
			// add the "History" field
			s = MediaString::forVideoHistory(format.u.encoded_video.forward_history,
											 format.u.encoded_video.backward_history);
			addField("History", s);
			break;
		}
		case B_MEDIA_MULTISTREAM: {
			// adjust view properties
			setSideBarWidth(be_plain_font->StringWidth(" Chunk Size ") + 2 * InfoView::M_H_MARGIN);
			BString s;
			// add the "Format" field
			s = MediaString::forMultistreamFormat(format.u.multistream.format);
			addField("Format", s);
			// add the "Bit Rate" field
			s = MediaString::forMultistreamBitRate(format.u.multistream.avg_bit_rate,
												   format.u.multistream.max_bit_rate);
			addField("Bit rate", s);
			// add the "Chunk Size" field
			s = MediaString::forMultistreamChunkSize(format.u.multistream.avg_chunk_size,
													 format.u.multistream.max_chunk_size);
			addField("Chunk size", s);
			// add the "Flags" field
			s = MediaString::forMultistreamFlags(format.u.multistream.flags);
			addField("Flags", s);
			break;
		}
		default: {
			// add no fields
		}
	}
}

// END -- EndPointInfoView.cpp --
