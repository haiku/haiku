/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */


#include "SubtitleBitmap.h"

#include <stdio.h>

#include <Bitmap.h>
#include <TextView.h>

#include "StackBlurFilter.h"


SubtitleBitmap::SubtitleBitmap()
	:
	fBitmap(NULL),
	fTextView(new BTextView("offscreen text")),
	fShadowTextView(new BTextView("offscreen text shadow")),
	fCharsPerLine(36),
	fUseSoftShadow(true),
	fOverlayMode(false)
{
	fTextView->SetStylable(true);
	fTextView->MakeEditable(false);
	fTextView->SetWordWrap(false);
	fTextView->SetAlignment(B_ALIGN_CENTER);

	fShadowTextView->SetStylable(true);
	fShadowTextView->MakeEditable(false);
	fShadowTextView->SetWordWrap(false);
	fShadowTextView->SetAlignment(B_ALIGN_CENTER);
}


SubtitleBitmap::~SubtitleBitmap()
{
	delete fBitmap;
	delete fTextView;
	delete fShadowTextView;
}


bool
SubtitleBitmap::SetText(const char* text)
{
	if (text == fText)
		return false;

	fText = text;

	_GenerateBitmap();
	return true;
}


void
SubtitleBitmap::SetVideoBounds(BRect bounds)
{
	if (bounds == fVideoBounds)
		return;

	fVideoBounds = bounds;

	fUseSoftShadow = true;
	_GenerateBitmap();
}


void
SubtitleBitmap::SetOverlayMode(bool overlayMode)
{
	if (overlayMode == fOverlayMode)
		return;

	fOverlayMode = overlayMode;

	_GenerateBitmap();
}


void
SubtitleBitmap::SetCharsPerLine(float charsPerLine)
{
	if (charsPerLine == fCharsPerLine)
		return;

	fCharsPerLine = charsPerLine;

	fUseSoftShadow = true;
	_GenerateBitmap();
}


const BBitmap*
SubtitleBitmap::Bitmap() const
{
	return fBitmap;
}


void
SubtitleBitmap::_GenerateBitmap()
{
	if (!fVideoBounds.IsValid())
		return;

	delete fBitmap;

	BRect bounds;
	float outlineRadius;
	_InsertText(bounds, outlineRadius, fOverlayMode);

	bigtime_t startTime = 0;
	if (!fOverlayMode && fUseSoftShadow)
		startTime = system_time();

	fBitmap = new BBitmap(bounds, B_BITMAP_ACCEPTS_VIEWS, B_RGBA32);
	memset(fBitmap->Bits(), 0, fBitmap->BitsLength());

	if (fBitmap->Lock()) {
		fBitmap->AddChild(fShadowTextView);
		fShadowTextView->ResizeTo(bounds.Width(), bounds.Height());

		fShadowTextView->SetViewColor(0, 0, 0, 0);
		fShadowTextView->SetDrawingMode(B_OP_ALPHA);
		fShadowTextView->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

		fShadowTextView->PushState();
		fShadowTextView->Draw(bounds);
		fShadowTextView->PopState();

		if (!fOverlayMode && fUseSoftShadow) {
			fShadowTextView->Sync();
			StackBlurFilter filter;
			filter.Filter(fBitmap, outlineRadius * 2);
		}

		fShadowTextView->RemoveSelf();

		fBitmap->AddChild(fTextView);
		fTextView->ResizeTo(bounds.Width(), bounds.Height());
		if (!fOverlayMode && fUseSoftShadow)
			fTextView->MoveTo(-outlineRadius / 2, -outlineRadius / 2);
		else
			fTextView->MoveTo(0, 0);

		fTextView->SetViewColor(0, 0, 0, 0);
		fTextView->SetDrawingMode(B_OP_ALPHA);
		fTextView->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

		fTextView->PushState();
		fTextView->Draw(bounds);
		fTextView->PopState();

		fTextView->Sync();
		fTextView->RemoveSelf();

		fBitmap->Unlock();
	}

	if (!fOverlayMode && fUseSoftShadow && system_time() - startTime > 10000)
		fUseSoftShadow = false;
}


struct ParseState {
	ParseState(rgb_color color)
		:
		color(color),
		bold(false),
		italic(false),
		underlined(false),

		previous(NULL)
	{
	}

	ParseState(ParseState* previous)
		:
		color(previous->color),
		bold(previous->bold),
		italic(previous->italic),
		underlined(previous->underlined),

		previous(previous)
	{
	}

	rgb_color	color;
	bool		bold;
	bool		italic;
	bool		underlined;

	ParseState*	previous;
};


static bool
find_next_tag(const BString& string, int32& tagPos, int32& tagLength,
	ParseState*& state)
{
	static const char* kTags[] = {
		"<b>", "</b>",
		"<i>", "</i>",
		"<u>", "</u>",
		"<font color=\"#", "</font>"
	};
	static const int32 kTagCount = sizeof(kTags) / sizeof(const char*);

	int32 startPos = tagPos;
	tagPos = string.Length();
	tagLength = 0;

	// Find the next tag closest from the current position
	// This way of doing it allows broken input with overlapping tags even.
	BString tag;
	for (int32 i = 0; i < kTagCount; i++) {
		int32 nextTag = string.IFindFirst(kTags[i], startPos);
		if (nextTag >= startPos && nextTag < tagPos) {
			tagPos = nextTag;
			tag = kTags[i];
		}
	}

	if (tag.Length() == 0)
		return false;

	// Tag found, ParseState will change.
	tagLength = tag.Length();
	if (tag == "<b>") {
		state = new ParseState(state);
		state->bold = true;
	} else if (tag == "<i>") {
		state = new ParseState(state);
		state->italic = true;
	} else if (tag == "<u>") {
		state = new ParseState(state);
		state->underlined = true;
	} else if (tag == "<font color=\"#") {
		state = new ParseState(state);
		char number[16];
		snprintf(number, sizeof(number), "0x%.6s",
			string.String() + tagPos + tag.Length());
		int colorInt;
		if (sscanf(number, "%x", &colorInt) == 1) {
			state->color.red = (colorInt & 0xff0000) >> 16;
			state->color.green = (colorInt & 0x00ff00) >> 8;
			state->color.blue = (colorInt & 0x0000ff);
			// skip 'RRGGBB">' part, too
			tagLength += 8;
		}
	} else if (tag == "</b>" || tag == "</i>" || tag == "</u>"
		|| tag == "</font>") {
		// Closing tag, pop state
		if (state->previous != NULL) {
			ParseState* oldState = state;
			state = state->previous;
			delete oldState;
		}
	}
	return true;
}


static void
apply_state(BTextView* textView, const ParseState* state, BFont font,
	bool changeColor)
{
	uint16 face = 0;
	if (state->bold || state->italic || state->underlined) {
		if (state->bold)
			face |= B_BOLD_FACE;
		if (state->italic)
			face |= B_ITALIC_FACE;
		// NOTE: This is probably not supported by the app_server (perhaps
		// it is if the font contains a specific underline face).
		if (state->underlined)
			face |= B_UNDERSCORE_FACE;
	} else
		face = B_REGULAR_FACE;
	font.SetFace(face);
	if (changeColor)
		textView->SetFontAndColor(&font, B_FONT_ALL, &state->color);
	else
		textView->SetFontAndColor(&font, B_FONT_ALL, NULL);
}


static void
parse_text(const BString& string, BTextView* textView, const BFont& font,
	const rgb_color& color, bool changeColor)
{
	ParseState rootState(color);
		// Colors may change, but alpha channel will be preserved

	ParseState* state = &rootState;

	int32 pos = 0;
	while (pos < string.Length()) {
		int32 nextPos = pos;
		int32 tagLength;
		bool stateChanged = find_next_tag(string, nextPos, tagLength, state);
		if (nextPos > pos) {
			// Insert text between last and next tags
			BString subString;
			string.CopyInto(subString, pos, nextPos - pos);
			textView->Insert(subString.String());
		}
		pos = nextPos + tagLength;
		if (stateChanged)
			apply_state(textView, state, font, changeColor);
	}

	// Cleanup states in case the input text had non-matching tags.
	while (state->previous != NULL) {
		ParseState* oldState = state;
		state = state->previous;
		delete oldState;
	}
}


void
SubtitleBitmap::_InsertText(BRect& textRect, float& outlineRadius,
	bool overlayMode)
{
	BFont font(be_plain_font);
	float fontSize = ceilf((fVideoBounds.Width() * 0.9) / fCharsPerLine);
	outlineRadius = ceilf(fontSize / 28.0);
	font.SetSize(fontSize);

	rgb_color shadow;
	shadow.red = 0;
	shadow.green = 0;
	shadow.blue = 0;
	shadow.alpha = 200;

	rgb_color color;
	color.red = 255;
	color.green = 255;
	color.blue = 255;
	color.alpha = 240;

	textRect = fVideoBounds;
	textRect.OffsetBy(outlineRadius, outlineRadius);

	fTextView->SetText(NULL);
	fTextView->SetFontAndColor(&font, B_FONT_ALL, &color);

	fTextView->Insert(" ");
	parse_text(fText, fTextView, font, color, true);

	font.SetFalseBoldWidth(outlineRadius);
	fShadowTextView->ForceFontAliasing(overlayMode);
	fShadowTextView->SetText(NULL);
	fShadowTextView->SetFontAndColor(&font, B_FONT_ALL, &shadow);

	fShadowTextView->Insert(" ");
	parse_text(fText, fShadowTextView, font, shadow, false);

	// This causes the BTextView to calculate the layout of the text
	fTextView->SetTextRect(BRect(0, 0, 0, 0));
	fTextView->SetTextRect(textRect);
	fShadowTextView->SetTextRect(BRect(0, 0, 0, 0));
	fShadowTextView->SetTextRect(textRect);

	textRect = fTextView->TextRect();
	textRect.InsetBy(-outlineRadius, -outlineRadius);
	textRect.OffsetTo(B_ORIGIN);

	// Make sure the text rect really finishes behind the last line.
	// We don't want any accidental extra space.
	textRect.bottom = outlineRadius;
	int32 lineCount = fTextView->CountLines();
	for (int32 i = 0; i < lineCount; i++)
		textRect.bottom += fTextView->LineHeight(i);
	textRect.bottom += outlineRadius;
}



