/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#include "TestResultItem.h"

const float kDistance = 5;

TestResultItem::TestResultItem(const char* name, BRect bitmapSize)
	: fName(name)
	, fBitmapSize(bitmapSize)
	, fOk(true)
	, fDirectBitmap(NULL)
	, fOriginalBitmap(NULL)
	, fArchivedBitmap(NULL)
{
}

TestResultItem::~TestResultItem()
{
	delete fDirectBitmap;
	fDirectBitmap = NULL;
	delete fOriginalBitmap;
	fOriginalBitmap = NULL;
	delete fArchivedBitmap;
	fArchivedBitmap = NULL;	
}
	
void 
TestResultItem::DrawItem(BView *owner, BRect itemRect, bool drawEverthing)
{
	owner->SetDrawingMode(B_OP_COPY);
	
	owner->PushState();
	if (IsSelected()) {
		rgb_color lowColor = owner->LowColor();
		owner->SetHighColor(tint_color(lowColor, B_DARKEN_2_TINT));
	} else if (fOk) {
		// green background color on success
		owner->SetHighColor(200, 255, 200);
	} else {
		// red background color on failure
		owner->SetHighColor(255, 200, 200);
	}
	owner->FillRect(itemRect);
	owner->PopState();

	itemRect.InsetBy(1, 1);
	
	owner->MovePenTo(itemRect.left+1, itemRect.top+1);
	if (fDirectBitmap != NULL) {
		owner->DrawBitmap(fDirectBitmap);
	}
	owner->MovePenBy(fBitmapSize.Width() + kDistance, 0);

	if (fOriginalBitmap != NULL) {
		owner->DrawBitmap(fOriginalBitmap);
	}
	owner->MovePenBy(fBitmapSize.Width() + kDistance, 0);
	
	if (fArchivedBitmap != NULL) {
		owner->DrawBitmap(fArchivedBitmap);
	}
	owner->MovePenBy(fBitmapSize.Width() + kDistance, 0);
	
	owner->DrawBitmap(fDirectBitmap);
	owner->SetDrawingMode(B_OP_SUBTRACT);
	owner->DrawBitmap(fOriginalBitmap);

	owner->MovePenBy(fBitmapSize.Width() + kDistance, 0);

	owner->SetDrawingMode(B_OP_OVER);	
	BFont font;
	owner->GetFont(&font);
	// vertically center text 
	float baseLine = itemRect.top + (itemRect.IntegerHeight() / 2 + font.Size() / 2);
	owner->MovePenTo(owner->PenLocation().x, baseLine);
	owner->DrawString(fName.String());

	if (fErrorMessage.Length() == 0)
		return;

	owner->PushState();	
	font.SetFace(B_ITALIC_FACE);
	owner->SetFont(&font);
	owner->SetHighColor(255, 0, 0);
	owner->MovePenBy(kDistance, 0);
	owner->DrawString(fErrorMessage.String());
	owner->PopState();
}


void 
TestResultItem::Update(BView *owner, const BFont *font)
{
	BListItem::Update(owner, font);
	float width = 0.0;
	float height = 0.0;

	width += font->StringWidth(fName.String());
	width += kDistance;
	width += font->StringWidth(fErrorMessage.String());

	width += 3 * kDistance;
	width += 3 * fBitmapSize.Width();

	height = fBitmapSize.Height();	

	// border of two pixels
	width += 4;
	height += 4;

	if (width > Width())
		SetWidth(width);

	if (height > Height())
		SetHeight(height);
}


// HeaderListItem
HeaderListItem::HeaderListItem(const char* label1, const char* label2,
					const char* label3, const char* label4, const char* label5,
					const char* label6, BRect rect)
	:
	fRect(rect)
{
	fLabels[0] = label1;
	fLabels[1] = label2;
	fLabels[2] = label3;
	fLabels[3] = label4;
	fLabels[4] = label5;
	fLabels[5] = label6;
}


/* virtual */
void
HeaderListItem::DrawItem(BView *owner, BRect itemRect, bool drawEverthing)
{
	owner->SetDrawingMode(B_OP_COPY);

	owner->PushState();
	if (IsSelected()) {
		rgb_color lowColor = owner->LowColor();
		owner->SetHighColor(tint_color(lowColor, B_DARKEN_2_TINT));
		owner->FillRect(itemRect);
	}

	owner->PopState();

	itemRect.InsetBy(0, 1);
	owner->StrokeRect(itemRect);

	itemRect.InsetBy(1, 0);

	owner->SetDrawingMode(B_OP_OVER);

	BFont font;
	owner->GetFont(&font);
	float baseLine = itemRect.top + (itemRect.IntegerHeight() / 2 + font.Size() / 2);

	for (int32 c = 0; c < sizeof(fLabels) / sizeof(fLabels[0]); c++) {
		owner->MovePenTo(itemRect.left + 1 + (fRect.Width() + kDistance) * c, baseLine);
		owner->DrawString(fLabels[c]);
	}
}


/* virtual */
void
HeaderListItem::Update(BView *owner, const BFont *font)
{
	BListItem::Update(owner, font);
	float width = 0.0;
	float height = 0.0;

	for (int32 c = 0; c < sizeof(fLabels) / sizeof(fLabels[0]); c++) {
		width += font->StringWidth(fLabels[c].String());
	}

	width += kDistance * (sizeof(fLabels) / sizeof(fLabels[0]) - 1);

	height = fRect.Height();

	// border of two pixels
	width += 4;
	height += 4;

	if (width > Width())
		SetWidth(width);
	if (height > Height())
		SetHeight(height);
}
