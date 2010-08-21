/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */

#include <Alert.h>
#include <Message.h>
#include <MimeType.h>

#include "ZombieReplicantView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>



_BZombieReplicantView_::_BZombieReplicantView_(BRect frame, status_t error)
	:
	BBox(frame, "<Zombie>", B_FOLLOW_NONE, B_WILL_DRAW),
	fError(error)
{
	BFont font(be_bold_font);
	font.SetSize(9.0f); // TODO
	SetFont(&font);
	SetViewColor(kZombieColor);
}


_BZombieReplicantView_::~_BZombieReplicantView_()
{
}


void
_BZombieReplicantView_::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_ABOUT_REQUESTED:
		{
			const char* addOn = NULL;
			char error[1024];
			if (fArchive->FindString("add_on", &addOn) == B_OK) {
				char description[B_MIME_TYPE_LENGTH] = "";				
				BMimeType type(addOn);
				type.GetShortDescription(description);
				snprintf(error, sizeof(error),
					"Cannot create the replicant for \"%s\". (%s)",
				description, strerror(fError));
			} else {
				snprintf(error, sizeof(error),
					"Cannot locate the application for the replicant. "
					"No application signature supplied. (%s)", strerror(fError));
			}

						
			BAlert* alert = new (std::nothrow) BAlert("Error", error, "OK", NULL, NULL,
								B_WIDTH_AS_USUAL, B_STOP_ALERT);
			if (alert != NULL)
				alert->Go();

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


status_t
_BZombieReplicantView_::Archive(BMessage* archive, bool) const
{
	*archive = *fArchive;
	
	return B_OK;
}


void
_BZombieReplicantView_::SetArchive(BMessage* archive)
{
	fArchive = archive;
}
