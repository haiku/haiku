//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ZombieReplicantView.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Class for Zombie replicants
//------------------------------------------------------------------------------

#include <Alert.h>
#include <Message.h>
#include <MimeType.h>

#include <ZombieReplicantView.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const static rgb_color kZombieColor = {220, 220, 220, 255};

_BZombieReplicantView_::_BZombieReplicantView_(BRect frame, status_t error)
	:	BBox(frame, "<Zombie>", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fError = error;

	BFont font(be_bold_font);
	font.SetSize(9.0f); // TODO
	SetFont(&font);
	SetViewColor(kZombieColor);
}


_BZombieReplicantView_::~_BZombieReplicantView_()
{
}


void
_BZombieReplicantView_::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_ABOUT_REQUESTED:
		{
			const char *add_on = NULL;
			char description[B_MIME_TYPE_LENGTH];

			if (fArchive->FindString("add_on", &add_on) == B_OK) {
				BMimeType type(add_on);
				type.GetShortDescription(description);
			}

			char error[1024];

			sprintf(error, "Can't create the \"%s\" replicant because the library is in the Trash. (%s)",
				description, strerror(fError));

			(new BAlert("Error", error, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();

			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


void
_BZombieReplicantView_::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	font_height fh;

	GetFontHeight(&fh);

	DrawChar('?', BPoint(bounds.Width() / 2.0f - StringWidth("?") / 2.0f,
		bounds.Height() / 2.0f - fh.ascent / 2.0f));

	BBox::Draw(updateRect);
}


void
_BZombieReplicantView_::MouseDown(BPoint)
{
}


void
_BZombieReplicantView_::SetArchive(BMessage *archive)
{
	fArchive = archive;
}
