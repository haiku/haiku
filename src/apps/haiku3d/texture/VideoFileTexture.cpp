/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */

#include "VideoFileTexture.h"

#include <Bitmap.h>
#include <Entry.h>
#include <MediaFile.h>
#include <MediaTrack.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <string.h>
#include <stdio.h>


VideoFileTexture::VideoFileTexture(const char* fileName)
	:
	Texture(),
	fMediaFile(NULL),
	fVideoTrack(NULL),
	fVideoBitmap(NULL)
{
	_Load(fileName);
}


VideoFileTexture::~VideoFileTexture()
{
	delete fVideoBitmap;

	if (fMediaFile != NULL) {
		fMediaFile->ReleaseAllTracks();
		fMediaFile->CloseFile();
		delete fMediaFile;
	}
}


void
VideoFileTexture::_Load(const char* fileName)
{
	BEntry entry(fileName);
	entry_ref ref;
	entry.GetRef(&ref);

	fMediaFile = new BMediaFile(&ref);
	status_t err = fMediaFile->InitCheck();
	if (err != B_OK) {
		printf("cannot contruct BMediaFile object -- %s\n", strerror(err));
		return;
	}

	int32 trackCount = fMediaFile->CountTracks();

	for (int32 i = 0; i < trackCount; i++) {
		BMediaTrack* track = fMediaFile->TrackAt(i);
		if (track == NULL) {
			printf("cannot contruct BMediaTrack object\n");
			return;
		}

		// get the encoded format
		media_format format;
		err = track->EncodedFormat(&format);
		if (err != B_OK) {
			printf("BMediaTrack::EncodedFormat error -- %s\n", strerror(err));
			return;
		}

		if (format.type == B_MEDIA_ENCODED_VIDEO) {
			fVideoTrack = track;
			// allocate a bitmap large enough to contain the decoded frame.
			BRect bounds(0.0, 0.0,
				format.u.encoded_video.output.display.line_width - 1.0,
				format.u.encoded_video.output.display.line_count - 1.0);
			fVideoBitmap = new BBitmap(bounds, B_RGB32);

			// specifiy the decoded format. we derive this information from
			// the encoded format.
			format = media_format();
			format.u.raw_video.last_active = (int32) (bounds.Height() - 1.0);
			format.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
			format.u.raw_video.pixel_width_aspect = 1;
			format.u.raw_video.pixel_height_aspect = 3;
			format.u.raw_video.display.format = fVideoBitmap->ColorSpace();
			format.u.raw_video.display.line_width = (int32) bounds.Width();
			format.u.raw_video.display.line_count = (int32) bounds.Height();
			format.u.raw_video.display.bytes_per_row
				= fVideoBitmap->BytesPerRow();

			err = fVideoTrack->DecodedFormat(&format);
			if (err != B_OK) {
				printf("error with BMediaTrack::DecodedFormat() -- %s\n",
					strerror(err));
				return;
			}

			// Create Texture
			glGenTextures(1, &fId);
			glBindTexture(GL_TEXTURE_2D, fId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, 4,
				(int) fVideoBitmap->Bounds().Width() + 1,
				(int) fVideoBitmap->Bounds().Height() + 1,
				0, GL_BGRA, GL_UNSIGNED_BYTE, fVideoBitmap->Bits());
		}
	}
}


void
VideoFileTexture::Update(float /*dt*/) {
	// TODO loop
	int64 frameCount = 0;
	media_header mh;
	status_t err
		= fVideoTrack->ReadFrames(fVideoBitmap->Bits(), &frameCount, &mh);
	if (err) {
		printf("BMediaTrack::ReadFrames error -- %s\n", strerror(err));
		return;
	}

	glBindTexture(GL_TEXTURE_2D, fId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
		(int)fVideoBitmap->Bounds().Width() + 1,
		(int)fVideoBitmap->Bounds().Height() + 1,
		GL_BGRA, GL_UNSIGNED_BYTE, fVideoBitmap->Bits());
}

