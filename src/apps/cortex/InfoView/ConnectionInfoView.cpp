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


// ConnectionInfoView.cpp

#include "ConnectionInfoView.h"
// InfoView
#include "InfoWindowManager.h"
// Support
#include "MediaIcon.h"
#include "MediaString.h"
// NodeManager
#include "Connection.h"

// MediaKit
#include <MediaDefs.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor (public)
// -------------------------------------------------------- //

ConnectionInfoView::ConnectionInfoView(
	const Connection &connection)
	: InfoView("Connection", "", 0),
	  m_source(connection.source()),
	  m_destination(connection.destination())
{
	D_METHOD(("ConnectionInfoView::ConnectionInfoView()\n"));

	setSideBarWidth(be_plain_font->StringWidth(" Destination ")
					+ 2 * InfoView::M_H_MARGIN);
	media_input input;
	media_output output;
	if (connection.getOutput(&output) == B_OK) {
		// add "Source" field
		BString s;
		s << output.name;
		if (s.Length() > 0)
			s << " ";
		s << "(" << MediaString::getStringFor(output.source) << ")";
		addField("Source", s);
	}
	if (connection.getInput(&input) == B_OK) {
		// add "Destination" field
		BString s;
		s << input.name;
		if (s.Length() > 0)
			s << " ";
		s << "(" << MediaString::getStringFor(input.destination) << ")";
		addField("Destination", s);
	}

	// add a separator field
	addField("", "");

	// add "Media Type" field
	addField("Media Type", MediaString::getStringFor(connection.format().type));

	// add the format fields
	_addFormatFields(connection.format());
}

ConnectionInfoView::~ConnectionInfoView()
{
	D_METHOD(("ConnectionInfoView::~ConnectionInfoView()\n"));
}

// -------------------------------------------------------- //
// *** internal operations (private)
// -------------------------------------------------------- //

void ConnectionInfoView::_addFormatFields(
	const media_format &format) {
	D_METHOD(("ConnectionInfoView::_addFormatFields()\n"));

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
			addField("Sample Rate", s);
			// add "Channels" field
			s = MediaString::forAudioChannelCount(format.u.raw_audio.channel_count);
			addField("Channels", s);
			// add "Channel Mask" field
			s = MediaString::forAudioChannelMask(format.u.raw_audio.channel_mask);
			addField("Channel Mask", s);
			// add "Matrix Mask" field
			s = MediaString::forAudioMatrixMask(format.u.raw_audio.matrix_mask);
			addField("Matrix Mask", s);
			// add the "Byte Order" field
			s = MediaString::forAudioByteOrder(format.u.raw_audio.byte_order);
			addField("Byte Order", s);
			// add the "Buffer Size" field
			s = MediaString::forAudioBufferSize(format.u.raw_audio.buffer_size);
			addField("Buffer Size", s);
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
			addField("Field Rate", s);
			// add the "Orientation" field
			s = MediaString::forVideoOrientation(format.u.raw_video.orientation);
			addField("Orientation", s);
			// add the "Aspect Ratio" field
			s = MediaString::forVideoAspectRatio(format.u.raw_video.pixel_width_aspect,
												 format.u.raw_video.pixel_height_aspect);
			addField("Aspect Ratio", s);
			// add the "Active Lines" field
			s = MediaString::forVideoActiveLines(format.u.raw_video.first_active,
												 format.u.raw_video.last_active);
			addField("Active Lines", s);
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
			addField("Bit Rate", s);
			// add the "Frame Size" field
			s = MediaString::forAudioFrameSize(format.u.encoded_audio.frame_size);
			addField("Frame Size", s);
			break;
		}
		case B_MEDIA_ENCODED_VIDEO: {
			// adjust view properties
			setSideBarWidth(be_plain_font->StringWidth(" Frame Size ") + 2 * InfoView::M_H_MARGIN);
			BString s;
			// add the "Bit Rate" field
			s = MediaString::forVideoBitRate(format.u.encoded_video.avg_bit_rate,
											 format.u.encoded_video.max_bit_rate);
			addField("Bit Rate", s);
			// add the "Frame Size" field
			s = MediaString::forVideoFrameSize(format.u.encoded_video.frame_size);
			addField("Frame Size", s);
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
			addField("Bit Rate", s);
			// add the "Chunk Size" field
			s = MediaString::forMultistreamChunkSize(format.u.multistream.avg_chunk_size,
													 format.u.multistream.max_chunk_size);
			addField("Chunk Size", s);
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

// -------------------------------------------------------- //
// *** BView implementation (public)
// -------------------------------------------------------- //

void ConnectionInfoView::DetachedFromWindow() {
	D_METHOD(("ConnectionInfoView::DetachedFromWindow()\n"));

	InfoWindowManager *manager = InfoWindowManager::Instance();
	if (manager) {
		BMessage message(InfoWindowManager::M_CONNECTION_WINDOW_CLOSED);
		message.AddInt32("source_port", m_source.port);
		message.AddInt32("source_id", m_source.id);
		message.AddInt32("destination_port", m_destination.port);
		message.AddInt32("destination_id", m_destination.id);
		manager->PostMessage(&message);
	}
}

// END -- ConnectionInfoView.cpp --
