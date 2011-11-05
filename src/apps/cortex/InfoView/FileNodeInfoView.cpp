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


// FileNodeInfoView.cpp

#include "FileNodeInfoView.h"
#include "MediaIcon.h"
#include "MediaString.h"
#include "NodeRef.h"

#include <MediaFile.h>
#include <MediaNode.h>
#include <MediaRoster.h>
#include <MediaTrack.h>
#include <TimeCode.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor (public)
// -------------------------------------------------------- //

FileNodeInfoView::FileNodeInfoView(
	const NodeRef *ref)
	: LiveNodeInfoView(ref)
{
	D_METHOD(("FileNodeInfoView::FileNodeInfoView()\n"));

	// adjust view properties
	setSideBarWidth(be_plain_font->StringWidth(" File Format ") + 2 * InfoView::M_H_MARGIN);
	setSubTitle("Live File-Interface Node");

	// if a ref is set for this file-interface display some info
	// thru MediaFile and set the title appropriatly
	BString title;
	BString s;
	entry_ref nodeFile;
	status_t error;
	error = BMediaRoster::Roster()->GetRefFor(ref->node(), &nodeFile);
	if (!error)
	{
		BMediaFile file(&nodeFile);
		if (file.InitCheck() == B_OK)
		{
			// add separator field
			addField("", "");
			
			// add "File Format" fields
			media_file_format format;
			if (file.GetFileFormatInfo(&format) == B_OK)
			{
				s = "";
				s << format.pretty_name << " (" << format.mime_type << ")";
				addField("File Format", s);
			}
			
			// add "Copyright" field
			const char *copyRight = file.Copyright();
			if (copyRight)
			{
				s = copyRight;
				addField("Copyright", s);
			}
	
			// add "Tracks" list
			if (file.CountTracks() > 0)
			{
				addField("Tracks", "");
				for (int32 i = 0; i < file.CountTracks(); i++)
				{
					BString label;
					label << "(" << i + 1 << ")";
					BMediaTrack *track = file.TrackAt(i);
					
					// add format
					media_format format;
					if (track->EncodedFormat(&format) == B_OK)
					{
						s = MediaString::getStringFor(format, false);
					}
					
					if ((format.type == B_MEDIA_ENCODED_AUDIO)
					 || (format.type == B_MEDIA_ENCODED_VIDEO))
					{
						// add codec
						media_codec_info codec;
						if (track->GetCodecInfo(&codec) == B_OK)
						{
							s << "\n- Codec: " << codec.pretty_name;
							if (codec.id > 0)
							{
								s << " (ID: " << codec.id << ")";
							}
						}
					}

					// add duration
					bigtime_t duration = track->Duration();
					int hours, minutes, seconds, frames;
					us_to_timecode(duration, &hours, &minutes, &seconds, &frames);
					char buffer[64];
					sprintf(buffer, "%02d:%02d:%02d:%02d", hours, minutes, seconds, frames);
					s << "\n- Duration: " << buffer;
					
					// add quality
					float quality;
					if (track->GetQuality(&quality) == B_OK)
					{
						s << "\n- Quality: " << quality;
					}
					addField(label, s);
				}
			}
		}
		// set title
		BEntry entry(&nodeFile);
		char fileName[B_FILE_NAME_LENGTH];
		entry.GetName(fileName);
		title = fileName;
	}
	else
	{
		// set title
		title = ref->name();
		title += " (no file)";
	}
	setTitle(title);
}

FileNodeInfoView::~FileNodeInfoView()
{
	D_METHOD(("FileNodeInfoView::~FileNodeInfoView()\n"));
}

// END -- FileNodeInfoView.cpp --
