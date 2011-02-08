/*
 * Copyright 2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include "PictureView.h"

#include <Bitmap.h>
#include <IconUtils.h>
#include <MimeType.h>
#include <TranslationUtils.h>

#include "PeopleApp.h"


const float kPictureMargin = 6.0;

PictureView::PictureView(float width, float height, const entry_ref* ref)
	:
	BView("pictureview", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fPicture(NULL),
	fOriginalPicture(NULL),
	fDefaultPicture(NULL),
	fRef(ref)
{
	SetViewColor(255, 255, 255);

	BSize size(width + 2 * kPictureMargin, height + 2 * kPictureMargin);
	SetExplicitMinSize(size);
	SetExplicitMaxSize(size);

	BMimeType mime;
	mime.SetType(B_PERSON_MIMETYPE);
	uint8* iconData;
	size_t iconDataSize;
	if (mime.GetIcon(&iconData, &iconDataSize) == B_OK) {
		fDefaultPicture = new BBitmap(BRect(0, 0, width - 1, height - 1),
			B_RGB32);
		if (fDefaultPicture->InitCheck() != B_OK
			|| BIconUtils::GetVectorIcon(iconData, iconDataSize,
				fDefaultPicture) != B_OK) {
			delete fDefaultPicture;
			fDefaultPicture = NULL;
		}
	}

	Update();
}


PictureView::~PictureView()
{
	delete fDefaultPicture;
	if (fPicture != fDefaultPicture)
		delete fPicture;
	if (fOriginalPicture != fPicture)
		delete fOriginalPicture;
}


bool
PictureView::HasChanged()
{
	return fOriginalPicture != fPicture;
}


void
PictureView::Revert()
{
	if (!HasChanged())
		return;

	if (fPicture != fDefaultPicture)
		delete fPicture;

	fPicture = fOriginalPicture != NULL ?
		fOriginalPicture : fDefaultPicture;

	Invalidate();
}


void
PictureView::Update()
{
	fOriginalPicture = BTranslationUtils::GetBitmap(fRef);
	if (fOriginalPicture != NULL)
		fPicture = fOriginalPicture;
	else
		fPicture = fDefaultPicture;

	Invalidate();
}


BBitmap*
PictureView::Picture()
{
	return fPicture != fDefaultPicture ? fPicture : NULL;
}


void
PictureView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			BBitmap* picture = BTranslationUtils::GetBitmap(&ref);
			if (picture == NULL)
				break;

			if (fPicture != fDefaultPicture
				&& fPicture != fOriginalPicture)
				delete fPicture;

			fPicture = picture;
			Invalidate();
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
PictureView::Draw(BRect updateRect)
{
	BRect rect = Bounds();
	if (fPicture != NULL) {
		if (fPicture == fDefaultPicture) {
			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
			SetHighColor(0, 0, 0, 24);
		}
 		DrawBitmapAsync(fPicture, fPicture->Bounds(),
			rect.InsetByCopy(kPictureMargin, kPictureMargin),
			B_FILTER_BITMAP_BILINEAR);
	}

	rgb_color black = {0, 0, 0, 255};
	SetHighColor(tint_color(black, B_LIGHTEN_2_TINT));
	StrokeRect(rect);
}
