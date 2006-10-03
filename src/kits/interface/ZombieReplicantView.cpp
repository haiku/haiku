/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */

#include <Alert.h>
#include <Message.h>
#include <MimeType.h>

#include <ZombieReplicantView.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>


const static rgb_color kZombieColor = {220, 220, 220, 255};


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
_BZombieReplicantView_::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_ABOUT_REQUESTED:
		{
			const char *addOn = NULL;
			char description[B_MIME_TYPE_LENGTH];

			if (fArchive->FindString("add_on", &addOn) == B_OK) {
				BMimeType type(addOn);
				type.GetShortDescription(description);
			}

			char error[1024];
			snprintf(error, sizeof(error),
				"Can't create the \"%s\" replicant because the library is in the Trash. (%s)",
				description, strerror(fError));
			
			BAlert *alert = new (nothrow) BAlert("Error", error, "OK", NULL, NULL,
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


void
_BZombieReplicantView_::SetArchive(BMessage *archive)
{
	fArchive = archive;
}
