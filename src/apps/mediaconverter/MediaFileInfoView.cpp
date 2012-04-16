// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#include "MediaFileInfoView.h"

#include <Alert.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Locale.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <String.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaConverter-FileInfo"


const float kSpacing = 5.0f;


MediaFileInfoView::MediaFileInfoView()
	:
	BView("MediaFileInfoView", B_WILL_DRAW | B_SUPPORTS_LAYOUT),
	fMinMaxValid(false),
	fRef(),
	fMediaFile(NULL)
{
	SetFont(be_plain_font);
}


MediaFileInfoView::~MediaFileInfoView()
{
}


void 
MediaFileInfoView::Draw(BRect /*update*/)
{
	_ValidateMinMax();

	_SetFontFace(B_BOLD_FACE);

	font_height fh;
	GetFontHeight(&fh);
	BPoint labelStart(kSpacing, fh.ascent + fh.leading + 1);

	if (fMediaFile == NULL) {
		DrawString(B_TRANSLATE("No file selected"), labelStart);
		return;
	}

	// draw filename
	DrawString(fRef.name, labelStart);
	labelStart.y += fLineHeight + kSpacing;
	BPoint infoStart(labelStart.x + fMaxLabelWidth + kSpacing, labelStart.y);
	
	// draw labels
	DrawString(B_TRANSLATE("Audio:"), labelStart);
	labelStart.y += fLineHeight * 2;

	DrawString(B_TRANSLATE("Video:"), labelStart);
	labelStart.y += fLineHeight * 2;

	DrawString(B_TRANSLATE("Duration:"), labelStart);
	labelStart.y += fLineHeight * 2;

	// draw audio/video/duration info
	_SetFontFace(B_REGULAR_FACE);
	
	BString* infoStrings[5] = {&fInfo.audio.format, &fInfo.audio.details,
		&fInfo.video.format, &fInfo.video.details, &fInfo.duration};
	for (int32 i = 0; i < 5; i++) {
		DrawString(*infoStrings[i], infoStart);
		infoStart.y += fLineHeight;
	}
}


BSize
MediaFileInfoView::MinSize()
{
	_ValidateMinMax();
	return fMinSize;
}


BSize
MediaFileInfoView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, fMinSize.height);
}


BSize
MediaFileInfoView::PreferredSize()
{
	_ValidateMinMax();
	return fMinSize;
}


BAlignment
MediaFileInfoView::LayoutAlignment()
{
	return BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP);
}


void
MediaFileInfoView::InvalidateLayout(bool /*children*/)
{
	fMinMaxValid = false;
	BView::InvalidateLayout();
}


void
MediaFileInfoView::SetFont(const BFont* font, uint32 mask)
{
	BView::SetFont(font, mask);
	if (mask == B_FONT_FACE)
		return;

	fLineHeight = _LineHeight();
	BFont bold(font);
	bold.SetFace(B_BOLD_FACE);
	fMaxLabelWidth = 0;

	BString labels[] = {B_TRANSLATE("Video:"), B_TRANSLATE("Duration:"), B_TRANSLATE("Audio:")};
	int32 labelCount = sizeof(labels) / sizeof(BString);
	fMaxLabelWidth = _MaxLineWidth(labels, labelCount, bold);

	fNoFileLabelWidth = ceilf(bold.StringWidth(B_TRANSLATE("No file selected")));
	InvalidateLayout();
}


void 
MediaFileInfoView::AttachedToWindow()
{
	rgb_color c = Parent()->LowColor();
	SetViewColor(c);
	SetLowColor(c);
}


void 
MediaFileInfoView::Update(BMediaFile* file, entry_ref* ref)
{
	if (fMediaFile == file)
		return;

	fMediaFile = file;

	if (file != NULL && ref != NULL) {
		fRef = *ref;
		status_t ret = fInfo.LoadInfo(file);
		if (ret != B_OK) {
			BString error(B_TRANSLATE("An error has occurred reading the "
				"file info.\n\nError : "));
			error << strerror(ret);
			BAlert* alert = new BAlert(
				B_TRANSLATE("File Error"), error.String(),
				B_TRANSLATE("OK"));
			alert->Go(NULL);
		}
	} else {
		fRef = entry_ref();
	}
	InvalidateLayout();
	Invalidate();
}


float
MediaFileInfoView::_LineHeight()
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	return ceilf(fontHeight.ascent + fontHeight.descent + fontHeight.leading);
}


float
MediaFileInfoView::_MaxLineWidth(BString* strings, int32 count,
	const BFont& font)
{
	float width = 0;
	for (int32 i = 0; i < count; i++)
		width = max_c(font.StringWidth(strings[i]), width);

	return ceilf(width);
}


void
MediaFileInfoView::_ValidateMinMax()
{
	if (fMinMaxValid)
		return;

	BFont font;
	GetFont(&font);

	BFont bold(font);
	bold.SetFace(B_BOLD_FACE);
	fMinSize.Set(0, 0);

	if (fMediaFile == NULL) {
		fMinSize.width =  fNoFileLabelWidth + kSpacing * 2;
		fMinSize.height = fLineHeight + kSpacing;
		return;
	}

	fMinSize.height += fLineHeight + kSpacing + 1;
	fMinSize.width = ceilf(bold.StringWidth(fRef.name));

	BString strings[5] = {fInfo.audio.format, fInfo.audio.details,
		fInfo.video.format, fInfo.video.details, fInfo.duration};
	float maxInfoWidth = _MaxLineWidth(strings, 5, font);

	fMinSize.width = max_c(fMinSize.width, fMaxLabelWidth
		+ maxInfoWidth + kSpacing);
	fMinSize.width += kSpacing;

	fMinSize.height += fLineHeight * 5 + 2 * kSpacing;
		// 5 lines of info, w/ spacing above and below (not between lines)

	ResetLayoutInvalidation();
	fMinMaxValid = true;
}


void
MediaFileInfoView::_SetFontFace(uint16 face)
{
	BFont font;
	font.SetFace(face);
	SetFont(&font, B_FONT_FACE);
}

