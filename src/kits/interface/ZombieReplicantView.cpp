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
#include <String.h>
#include <SystemCatalog.h>

#include "ZombieReplicantView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

using BPrivate::gSystemCatalog;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ZombieReplicantView"

#undef B_TRANSLATE
#define B_TRANSLATE(str) \
	gSystemCatalog.GetString(B_TRANSLATE_MARK(str), "ZombieReplicantView")


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
			BString error;
			if (fArchive->FindString("add_on", &addOn) == B_OK) {
				char description[B_MIME_TYPE_LENGTH] = "";
				BMimeType type(addOn);
				type.GetShortDescription(description);
				error = B_TRANSLATE("Cannot create the replicant for "
						"\"%description\".\n%error");
				error.ReplaceFirst("%description", description);
			} else
				error = B_TRANSLATE("Cannot locate the application for the "
					"replicant. No application signature supplied.\n%error");

			error.ReplaceFirst("%error", strerror(fError));

			BAlert* alert = new (std::nothrow) BAlert(B_TRANSLATE("Error"),
				error.String(), B_TRANSLATE("OK"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
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
