/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */

/*! BStatusBar displays a "percentage-of-completion" gauge. */

#include <Debug.h>
#include <Message.h>
#include <StatusBar.h>

#include <stdlib.h>
#include <string.h>


BStatusBar::BStatusBar(BRect frame, const char *name, const char *label,
					   const char *trailingLabel)
	:	BView(frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		fLabel(NULL),
		fTrailingLabel(NULL),
		fText(NULL),
		fTrailingText(NULL),
		fMax(100.0f),
		fCurrent(0.0f),
		fBarHeight(-1.0f),
		fTrailingWidth(-1.0f),
		fEraseText(-1.0f),
		fEraseTrailingText(-1.0f),
		fCustomBarHeight(false)

{
	// TODO: Move initializer list and other stuff to InitObject
	InitObject(label, trailingLabel);
	
	fBarColor.set_to(50, 150, 255, 255);
}


BStatusBar::BStatusBar(BMessage *archive)
	:	BView(archive),
		fLabel(NULL),
		fTrailingLabel(NULL),
		fText(NULL),
		fTrailingText(NULL),
		fMax(100.0f),
		fCurrent(0.0f),
		fBarHeight(-1.0f),
		fTrailingWidth(-1.0f),
		fEraseText(-1.0f),
		fEraseTrailingText(-1.0f),
		fCustomBarHeight(false)
{
	const char *label = NULL;
	const char *trailingLabel = NULL;	
	archive->FindString("_label", &label);
	archive->FindString("_tlabel", &trailingLabel);
	
	InitObject(label, trailingLabel);
	
	if (archive->FindFloat("_high", &fBarHeight) != B_OK)
		fBarHeight = -1.0f;

	if (archive->FindInt32("_bcolor", (int32 *)&fBarColor) < B_OK)
		fBarColor.set_to(50, 150, 255, 255);
	
	if (archive->FindFloat("_val", &fCurrent) < B_OK)
		fCurrent = 0.0f;

	if (archive->FindFloat("_max", &fMax) < B_OK)
		fMax = 100.0f; 
	
	const char *string;

	if (archive->FindString("_text", &string) < B_OK)
		fText = NULL;
	else
		fText = strdup(string);
		
	if (archive->FindString("_ttext", &string) < B_OK)
		fTrailingText = NULL;
	else
		fTrailingText = strdup(string);

}


BStatusBar::~BStatusBar()
{
	free(fLabel);
	free(fTrailingLabel);
	free(fText);
	free(fTrailingText);
}


BArchivable *
BStatusBar::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BStatusBar"))
		return new BStatusBar(archive);

	return NULL;
}


status_t
BStatusBar::Archive(BMessage *archive, bool deep) const
{
	status_t err = BView::Archive(archive, deep);

	if (err < B_OK)
		return err;

	if (fBarHeight != 16.0f)
		err = archive->AddFloat("_high", fBarHeight);

	if (err < B_OK)
		return err;

	// DW: I'm pretty sure we don't need to compare the color with (50, 150, 255) ?
	err = archive->AddInt32("_bcolor", (const uint32 &)fBarColor);

	if (err < B_OK)
		return err;

	if (fCurrent != 0.0f)
		err = archive->AddFloat("_val", fCurrent);

	if (err < B_OK)
		return err;
			
	if (fMax != 100.0f )
		err = archive->AddFloat("_max", fMax);
      
	if (err < B_OK)
		return err;
 
	if (fText )
		err = archive->AddString("_text", fText);

	if (err < B_OK)
		return err;

	if (fTrailingText)
		err = archive->AddString("_ttext", fTrailingText);

	if (err < B_OK)
		return err;
			
	if (fLabel)
		err = archive->AddString("_label", fLabel);

	if (err < B_OK)
		return err;

	if (fTrailingLabel)
		err = archive->AddString ("_tlabel", fTrailingLabel);

	return err;
}


void
BStatusBar::AttachedToWindow()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(Frame().Width(), height);

	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
		SetLowColor(Parent()->ViewColor());
	}
}


void
BStatusBar::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case B_UPDATE_STATUS_BAR:
		{
			float delta;
			const char *text = NULL, *trailing_text = NULL;

			message->FindFloat("delta", &delta);
			message->FindString("text", &text);
			message->FindString("trailing_text", &trailing_text);

			Update(delta, text, trailing_text);

			break;
		}

		case B_RESET_STATUS_BAR: 
		{
			const char *label = NULL, *trailing_label = NULL;

			message->FindString("label", &label);
			message->FindString("trailing_label", &trailing_label);

			Reset(label, trailing_label);

			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
BStatusBar::Draw(BRect updateRect)
{
	float width = Frame().Width();
	font_height fh;
	GetFontHeight(&fh);
	
	SetHighColor(0, 0, 0);
	MovePenTo(2.0f, (float)ceil(fh.ascent) + 1.0f);

	if (fLabel)
		DrawString(fLabel);
	if (fText)
		DrawString(fText);
		
	if (fTrailingText) {
		if (fTrailingLabel) {
			MovePenTo(width - StringWidth(fTrailingText) -
			StringWidth(fTrailingLabel) - 2.0f,
			(float)ceil(fh.ascent) + 1.0f);
			DrawString(fTrailingText);
			DrawString(fTrailingLabel);
		} else {
			MovePenTo(width - StringWidth(fTrailingText) - 2.0f,
			(float)ceil(fh.ascent) + 1.0f);
			DrawString(fTrailingText);
		}
		
	} else if (fTrailingLabel) {
		MovePenTo(width - StringWidth(fTrailingLabel) - 2.0f,
			(float)ceil(fh.ascent) + 1.0f);
		DrawString(fTrailingLabel);
	}

	BRect rect(0.0f, (float)ceil(fh.ascent) + 4.0f, width,
		(float)ceil(fh.ascent) + 4.0f + fBarHeight);

	// First bevel
	SetHighColor(tint_color(ui_color ( B_PANEL_BACKGROUND_COLOR ), B_DARKEN_1_TINT));
	StrokeLine(BPoint(rect.left, rect.bottom), BPoint(rect.left, rect.top));
	StrokeLine(BPoint(rect.right, rect.top));

	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
	StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), BPoint(rect.right, rect.bottom));
	StrokeLine(BPoint(rect.right, rect.top + 1.0f));

	rect.InsetBy(1.0f, 1.0f);
	
	// Second bevel
	SetHighColor(tint_color(ui_color ( B_PANEL_BACKGROUND_COLOR ), B_DARKEN_4_TINT));
	StrokeLine(BPoint(rect.left, rect.bottom), BPoint(rect.left, rect.top));
	StrokeLine(BPoint(rect.right, rect.top));

	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), BPoint(rect.right, rect.bottom));
	StrokeLine(BPoint(rect.right, rect.top + 1.0f));

	rect.InsetBy(1.0f, 1.0f);

	// Filling
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_MAX_TINT));
	FillRect(rect);

	if (fCurrent != 0.0f) {
		rect.right = rect.left + (float)ceil(fCurrent * (width - 4) / fMax),

		// Bevel
		SetHighColor(tint_color(fBarColor, B_LIGHTEN_2_TINT));
		StrokeLine(BPoint(rect.left, rect.bottom), BPoint(rect.left, rect.top));
		StrokeLine(BPoint(rect.right, rect.top));

		SetHighColor(tint_color(fBarColor, B_DARKEN_2_TINT));
		StrokeLine(BPoint(rect.left, rect.bottom), BPoint(rect.right, rect.bottom));
		StrokeLine(BPoint(rect.right, rect.top));

		rect.InsetBy(1.0f, 1.0f);

		// Filling
		SetHighColor(fBarColor);
		FillRect(rect);
	}
}


void
BStatusBar::SetBarColor(rgb_color color)
{
	fBarColor = color;

	Invalidate();
}


void
BStatusBar::SetBarHeight(float height)
{
	BRect frame = Frame();

	fBarHeight = height;
	fCustomBarHeight = true;
	ResizeTo(frame.Width(), fBarHeight + 16);
}


void
BStatusBar::SetText(const char *string)
{
	SetTextData(&fText, string);

	Invalidate();
}


void
BStatusBar::SetTrailingText(const char *string)
{
	SetTextData(&fTrailingText, string);

	Invalidate();
}


void
BStatusBar::SetMaxValue(float max)
{
	fMax = max;

	Invalidate();
}


void
BStatusBar::Update(float delta, const char *text, const char *trailingText)
{
	fCurrent += delta;

	if (fCurrent > fMax)
		fCurrent = fMax;

	// Passing NULL for the text or trailingText argument retains the previous
	// text or trailing text string.
	if (text)
		SetTextData(&fText, text);

	if (trailingText)
		SetTextData(&fTrailingText, trailingText);

	Invalidate();
}


void
BStatusBar::Reset(const char *label, const char *trailingLabel)
{
	// Reset replaces the label and trailing label with copies of the
	// strings passed as arguments. If either argument is NULL, the
	// label or trailing label will be deleted and erased.
	SetTextData(&fLabel, label);
	SetTextData(&fTrailingLabel, trailingLabel);

	// Reset deletes and erases any text or trailing text
	SetTextData(&fText, NULL);
	SetTextData(&fTrailingText, NULL);

	fCurrent = 0.0f;
	fMax = 100.0f;

	Invalidate();
}


float
BStatusBar::CurrentValue() const
{
	return fCurrent;
}


float
BStatusBar::MaxValue() const
{
	return fMax;
}


rgb_color
BStatusBar::BarColor() const
{
	return fBarColor;
}


float
BStatusBar::BarHeight() const
{
	if (!fCustomBarHeight && fBarHeight == -1.0f) {
		font_height fh;
		GetFontHeight(&fh);
		const_cast<BStatusBar *>(this)->fBarHeight = fh.ascent + fh.descent + 6.0f;
	}
	
	return fBarHeight;
}


const char *
BStatusBar::Text() const
{
	return fText;
}


const char *
BStatusBar::TrailingText() const
{
	return fTrailingText;
}


const char *
BStatusBar::Label() const
{
	return fLabel;
}


const char *
BStatusBar::TrailingLabel() const
{
	return fTrailingLabel;
}


void
BStatusBar::MouseDown(BPoint point)
{
	BView::MouseDown(point);
}


void
BStatusBar::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}


void
BStatusBar::WindowActivated(bool state)
{
	BView::WindowActivated(state);
}


void
BStatusBar::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BView::MouseMoved(point, transit, message);
}


void
BStatusBar::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BStatusBar::FrameMoved(BPoint newPosition)
{
	BView::FrameMoved(newPosition);
}


void
BStatusBar::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);
}


BHandler *
BStatusBar::ResolveSpecifier(BMessage *message, int32 index,
									   BMessage *specifier,
									   int32 what, const char *property)
{
	return BView::ResolveSpecifier(message, index, specifier, what, property);
}


void
BStatusBar::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


void
BStatusBar::GetPreferredSize(float* _width, float* _height)
{
	if (_width) {
		*_width = (fLabel ? (float)ceil(StringWidth(fLabel)) : 0.0f)
			+ (fTrailingLabel ? (float)ceil(StringWidth(fTrailingLabel)) : 0.0f)
			+ 7.0f;
	}

	if (_height) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		*_height = ceil(fontHeight.ascent + fontHeight.descent) + 5.0f + BarHeight();
	}
}


void
BStatusBar::MakeFocus(bool state)
{
	BView::MakeFocus(state);
}


void
BStatusBar::AllAttached()
{
	BView::AllAttached();
}


void
BStatusBar::AllDetached()
{
	BView::AllDetached();
}


status_t
BStatusBar::GetSupportedSuites(BMessage *data)
{
	return BView::GetSupportedSuites(data);
}


status_t
BStatusBar::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}


void BStatusBar::_ReservedStatusBar1() {}
void BStatusBar::_ReservedStatusBar2() {}
void BStatusBar::_ReservedStatusBar3() {}
void BStatusBar::_ReservedStatusBar4() {}


BStatusBar &
BStatusBar::operator=(const BStatusBar &)
{
	return *this;
}


void
BStatusBar::InitObject(const char *label, const char *trailingLabel)
{
	SetTextData(&fLabel, label);
	SetTextData(&fTrailingLabel, trailingLabel);
}

	
void
BStatusBar::SetTextData(char **dest, const char *source)
{
	ASSERT(dest != NULL);
	
	if (*dest != NULL) {
		free(*dest);
		*dest = NULL;
	}

	if (source != NULL && source[0] != '\0')
		*dest = strdup(source);
}


void
BStatusBar::FillBar(BRect rect)
{
	// TODO:
}


void
BStatusBar::Resize()
{
	// TODO:
}


void
BStatusBar::_Draw(BRect updateRect, bool barOnly)
{
	// TODO:
}
