//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		StatusBar.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BStatusBar displays a "percentage-of-completion" gauge.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <string.h>

// System Includes -------------------------------------------------------------
#include <StatusBar.h>
#include <Message.h>
#include <Errors.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BStatusBar::BStatusBar(BRect frame, const char *name, const char *label,
					   const char *trailingLabel)
	:	BView ( frame, name, 0, B_WILL_DRAW ),
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
	fLabel = strdup(label);
	fTrailingLabel = strdup(trailingLabel);
	
	fBarColor.red = 50;
	fBarColor.green = 150;
	fBarColor.blue = 255;
	fBarColor.alpha = 255;
}
//------------------------------------------------------------------------------
BStatusBar::BStatusBar(BMessage *archive)
	:	BView(archive),
		fTrailingWidth(-1.0f),
		fEraseText(-1.0f),
		fEraseTrailingText(-1.0f),
		fCustomBarHeight(false)
{
	if (archive->FindFloat("_high", &fBarHeight) != B_OK)
		fBarHeight = -1.0f;

	const void *ptr;

	if (archive->FindData("_bcolor", B_INT32_TYPE, &ptr, NULL ) != B_OK)
	{
		fBarColor.red = 50;
		fBarColor.green = 150;
		fBarColor.blue = 255;
		fBarColor.alpha = 255;
	}
	else
		memcpy(&fBarColor, ptr, sizeof(rgb_color));
	
	if (archive->FindFloat("_val", &fCurrent) != B_OK)
		fCurrent = 0.0f;

	if (archive->FindFloat("_max", &fMax) != B_OK)
		fMax = 100.0f; 
	
	const char *string;

	if (archive->FindString("_text", &string) != B_OK)
		fText = NULL;
	else
		fText = strdup(string);
		
	if (archive->FindString("_ttext", &string) != B_OK)
		fTrailingText = NULL;
	else
		fTrailingText = strdup(string);

	if (archive->FindString("_label", &string) != B_OK)
		fLabel = NULL;
	else
		fLabel = strdup(string);

	if ( archive->FindString("_tlabel", &string) != B_OK)
		fTrailingLabel = NULL;
	else
		fTrailingLabel = strdup(string);
}
//------------------------------------------------------------------------------
BStatusBar::~BStatusBar()
{
	if (fLabel)
		delete fLabel;

	if (fTrailingLabel)
		delete fTrailingLabel;

	if (fText)
		delete fText;

	if (fTrailingText)
		delete fTrailingText;
}
//------------------------------------------------------------------------------
BArchivable *BStatusBar::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BStatusBar"))
		return new BStatusBar(archive);

	return NULL;
}
//------------------------------------------------------------------------------
status_t BStatusBar::Archive(BMessage *archive, bool deep) const
{
	status_t err = BView::Archive(archive, deep);
	
	if (err != B_OK)
		return err;

	if (fBarHeight != 16.0f)
		err = archive->AddFloat("_high", fBarHeight);
		
	if (err != B_OK)
		return err;

	// TODO: Should we compare the color with (50, 150, 255) ?
	err = archive->AddData("_bcolor", B_INT32_TYPE, &fBarColor, sizeof( int32 ));
	
	if (err != B_OK)
		return err;
	
	if (fCurrent != 0.0f)
		err = archive->AddFloat("_val", fCurrent);
		
	if (err != B_OK)
		return err;

	if (fMax != 100.0f )
		err = archive->AddFloat("_max", fMax); 
		
	if (err != B_OK)
		return err;
	
	if (fText )
		err = archive->AddString("_text", fText);
		
	if (err != B_OK)
		return err;
	
	if (fTrailingText)
		err = archive->AddString("_ttext", fTrailingText);
		
	if (err != B_OK)
		return err;

	if (fLabel)
		err = archive->AddString("_label", fLabel);
		
	if (err != B_OK)
		return err;

	if (fTrailingLabel)
		err = archive->AddString ("_tlabel", fTrailingLabel);

	return err;
}
//------------------------------------------------------------------------------
void BStatusBar::AttachedToWindow()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(Frame().Width(), height);

	if (Parent())
		SetViewColor(Parent ()->ViewColor());
}
//------------------------------------------------------------------------------
void BStatusBar::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
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
			BView::MessageReceived ( message );
	}
}
//------------------------------------------------------------------------------
void BStatusBar::Draw(BRect updateRect)
{
	float width = Frame().Width();

	if (fLabel)
	{
		font_height fh;

		GetFontHeight(&fh);
		
		SetHighColor(0, 0, 0);
		DrawString(fLabel, BPoint(0.0f, (float)ceil(fh.ascent) + 1.0f));
	}

	BRect rect(0.0f, 14.0f, width, 14.0f + fBarHeight);

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

	if (fCurrent != 0.0f)
	{
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
//------------------------------------------------------------------------------
void BStatusBar::SetBarColor(rgb_color color)
{
	memcpy(&fBarColor, &color, sizeof(rgb_color));

	Invalidate();
}
//------------------------------------------------------------------------------
void BStatusBar::SetBarHeight(float height)
{
	BRect frame = Frame();

	fBarHeight = height;
	fCustomBarHeight = true;
	ResizeTo(frame.Width(), fBarHeight + 16);
}
//------------------------------------------------------------------------------
void BStatusBar::SetText ( const char *string )
{
	if (fText)
		delete fText;

	fText = strdup(string);

	Invalidate();
}
//------------------------------------------------------------------------------
void BStatusBar::SetTrailingText(const char *string)
{
	if (fTrailingText)
		delete fTrailingText;

	fTrailingText = strdup(string);

	Invalidate();
}
//------------------------------------------------------------------------------
void BStatusBar::SetMaxValue(float max)
{
	fMax = max;

	Invalidate();
}
//------------------------------------------------------------------------------
void BStatusBar::Update(float delta, const char *text, const char *trailingText)
{
	fCurrent += delta;

	if (fText)
		delete fText;

	fText = strdup(text);

	if (fTrailingText)
		delete fTrailingText;

	fTrailingText = strdup(trailingText);

	Invalidate();
}
//------------------------------------------------------------------------------
void BStatusBar::Reset(const char *label, const char *trailingLabel)
{
	if (fLabel)
		delete fLabel;

	fLabel = strdup(label);

	if (fTrailingLabel)
		delete fTrailingLabel;

	fTrailingLabel = strdup(trailingLabel);

	fCurrent = 0.0f;

	Invalidate();
}
float BStatusBar::CurrentValue() const
{
	return fCurrent;
}
//------------------------------------------------------------------------------
float BStatusBar::MaxValue() const
{
	return fMax;
}
//------------------------------------------------------------------------------
rgb_color BStatusBar::BarColor() const
{
	return fBarColor;
}
//------------------------------------------------------------------------------
float BStatusBar::BarHeight() const
{
	if (!fCustomBarHeight && fBarHeight == -1.0f)
	{
		font_height fh;
		GetFontHeight(&fh);
		((BStatusBar*)this)->fBarHeight = fh.ascent + fh.descent + 6.0f;
	}
	
	return fBarHeight;
}
//------------------------------------------------------------------------------
const char *BStatusBar::Text() const
{
	return fText;
}
//------------------------------------------------------------------------------
const char *BStatusBar::TrailingText() const
{
	return fTrailingText;
}
//------------------------------------------------------------------------------
const char *BStatusBar::Label() const
{
	return fLabel;
}
//------------------------------------------------------------------------------
const char *BStatusBar::TrailingLabel() const
{
	return fTrailingLabel;
}
//------------------------------------------------------------------------------
void BStatusBar::MouseDown(BPoint point)
{
	BView::MouseDown(point);
}
//------------------------------------------------------------------------------
void BStatusBar::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}
//------------------------------------------------------------------------------
void BStatusBar::WindowActivated(bool state)
{
	BView::WindowActivated(state);
}
//------------------------------------------------------------------------------
void BStatusBar::MouseMoved(BPoint point, uint32 transit,
							const BMessage *message)
{
	BView::MouseMoved(point, transit, message);
}
//------------------------------------------------------------------------------
void BStatusBar::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void BStatusBar::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}
//------------------------------------------------------------------------------
void BStatusBar::FrameResized(float new_width, float new_height)
{
	BView::FrameResized(new_width, new_height);
}
//------------------------------------------------------------------------------
BHandler *BStatusBar::ResolveSpecifier(BMessage *message, int32 index,
									   BMessage *specifier,
									   int32 what, const char *property)
{
	return BView::ResolveSpecifier(message, index, specifier, what, property);
}
//------------------------------------------------------------------------------
void BStatusBar::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}
//------------------------------------------------------------------------------
void BStatusBar::GetPreferredSize(float *width, float *height)
{
	font_height fh;
	GetFontHeight(&fh);
	
	*width = 0.0f;
	if (Label() && TrailingLabel())
		*width += 3.0f;
	if (Label())
		*width += (float)ceil(StringWidth(Label())) + 2.0f;
	if (TrailingLabel())
		*width += (float)ceil(StringWidth(TrailingLabel())) + 2.0f;
	if (Text())
		*width += 3.0f;
	if (TrailingText())
		*width += 3.0f;
	*height = fh.ascent + fh.descent + 5.0f + BarHeight();
}
//------------------------------------------------------------------------------
void BStatusBar::MakeFocus(bool state)
{
	BView::MakeFocus(state);
}
//------------------------------------------------------------------------------
void BStatusBar::AllAttached()
{
	BView::AllAttached();
}
//------------------------------------------------------------------------------
void BStatusBar::AllDetached()
{
	BView::AllDetached();
}
//------------------------------------------------------------------------------
status_t BStatusBar::GetSupportedSuites(BMessage *data)
{
	return BView::GetSupportedSuites(data);
}
//------------------------------------------------------------------------------
status_t BStatusBar::Perform(perform_code d, void *arg)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BStatusBar::_ReservedStatusBar1() {}
void BStatusBar::_ReservedStatusBar2() {}
void BStatusBar::_ReservedStatusBar3() {}
void BStatusBar::_ReservedStatusBar4() {}

//------------------------------------------------------------------------------
BStatusBar &BStatusBar::operator=(const BStatusBar &)
{
	return *this;
}
//------------------------------------------------------------------------------
void BStatusBar::InitObject(const char *l, const char *aux_l)
{
	// TODO:
}
//------------------------------------------------------------------------------
void BStatusBar::SetTextData(char **pp, const char *str)
{
	// TODO:
}
//------------------------------------------------------------------------------
void BStatusBar::FillBar(BRect r)
{
	// TODO:
}
//------------------------------------------------------------------------------
void BStatusBar::Resize()
{
	// TODO:
}
//------------------------------------------------------------------------------
void BStatusBar::_Draw(BRect updateRect, bool bar_only)
{
	// TODO:
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
