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
//	File Name:		Box.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BBox objects group views together and draw a border
//                  around them.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <string.h>

// System Includes -------------------------------------------------------------
#include <Box.h>
#include <Errors.h>
#include <Message.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BBox::BBox(BRect frame, const char *name, uint32 resizingMode, uint32 flags,
		   border_style border)
	:	BView(frame, name, resizingMode, flags),
		fLabel(NULL),
		fStyle(border),
		fLabelView(NULL)
{
	fBounds = Bounds();

	SetFont(be_bold_font);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
//------------------------------------------------------------------------------
BBox::~BBox()
{
	if (fLabel)
		delete fLabel;
}
//------------------------------------------------------------------------------
BBox::BBox(BMessage *archive)
	:	BView(archive)
{
	fBounds = Bounds();
	fLabelView = NULL;

	const char *string;

	if (archive->FindString("_label", &string) != B_OK)
		fLabel = NULL;
	else
		SetLabel(string);

	int32 anInt32;

	if (archive->FindInt32("_style", &anInt32) != B_OK)
		fStyle = B_FANCY_BORDER;
	else
		fStyle = (border_style)anInt32;
}
//------------------------------------------------------------------------------
BArchivable *BBox::Instantiate(BMessage *archive)
{
	if ( validate_instantiation ( archive, "BBox" ) )
		return new BBox ( archive );
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BBox::Archive(BMessage *archive, bool deep) const
{
	status_t err = BView::Archive(archive, deep);

	if (err != B_OK)
		return err;

	if (fLabel)
		 err = archive->AddString("_label",fLabel);

	if (err != B_OK)
		return err;

	if (fStyle != B_FANCY_BORDER)
		err = archive->AddInt32("_style", fStyle);

	return err;
}
//------------------------------------------------------------------------------
void BBox::SetBorder(border_style border)
{
	fStyle = border;

	Invalidate();
}
//------------------------------------------------------------------------------
border_style BBox::Border() const
{
	return fStyle;
}
//------------------------------------------------------------------------------
void BBox::SetLabel(const char *string)
{ 
	if (fLabel)
		delete fLabel;

	fLabel = strdup(string);

	// Update fBounds
	fBounds = Bounds();

	if (fLabel)
	{
		font_height fh;
		GetFontHeight(&fh);

		fBounds.top = (float)ceil((fh.ascent + fh.descent) / 2.0f);
	}

	Invalidate();
}
//------------------------------------------------------------------------------
status_t BBox::SetLabel(BView *viewLabel)
{
	if (viewLabel)
	{
		if (fLabelView)
			RemoveChild(fLabelView);

		fLabelView = viewLabel;
		AddChild(fLabelView);
		
		if(fLabel)
		{
			delete fLabel;
			fLabel = NULL;
		}
	}

	// Update fBounds
	fBounds = Bounds();

	if (fLabelView)
		fBounds.top = (float)ceil(fLabelView->Frame().Height() / 2.0f);

	Invalidate();

	return B_OK;
}
//------------------------------------------------------------------------------
const char *BBox::Label() const
{
	return fLabel;
}
//------------------------------------------------------------------------------
BView *BBox::LabelView() const
{
	return fLabelView;
}
//------------------------------------------------------------------------------
void BBox::Draw(BRect updateRect)
{
	switch (fStyle)
	{
		case B_FANCY_BORDER:
			DrawFancy();
			break;

		case B_PLAIN_BORDER:
			DrawPlain();
			break;

		default:
			break;
	}
	
	if (fLabel)
	{
		font_height fh;
		GetFontHeight(&fh);

		SetHighColor(ViewColor());

		FillRect(BRect(6.0f, 1.0f, 12.0f + StringWidth(fLabel),
			(float)ceil(fh.ascent + fh.descent))/*, B_SOLID_LOW*/);

		SetHighColor(0, 0, 0);
		DrawString(fLabel, BPoint(10.0f, (float)ceil(fh.ascent - fh.descent) + 1.0f ));
	}
}
//------------------------------------------------------------------------------
void BBox::AttachedToWindow()
{
	if (Parent())
	{
		SetViewColor(Parent()->ViewColor());
		SetLowColor(Parent()->ViewColor());
	}
}
//------------------------------------------------------------------------------
void BBox::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void BBox::AllAttached()
{
	BView::AllAttached();
}
//------------------------------------------------------------------------------
void BBox::AllDetached()
{
	BView::AllDetached();
}
//------------------------------------------------------------------------------
void BBox::FrameResized(float width, float height)
{
	fBounds.right = Bounds().right;
	fBounds.bottom = Bounds().bottom;
}
//------------------------------------------------------------------------------
void BBox::MessageReceived(BMessage *message)
{
	BView::MessageReceived(message);
}
//------------------------------------------------------------------------------
void BBox::MouseDown(BPoint point)
{
	BView::MouseDown(point);
}
//------------------------------------------------------------------------------
void BBox::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}
//------------------------------------------------------------------------------
void BBox::WindowActivated(bool active)
{
	BView::WindowActivated(active);
}
//------------------------------------------------------------------------------
void BBox::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BView::MouseMoved(point, transit, message);
}
//------------------------------------------------------------------------------
void BBox::FrameMoved(BPoint newLocation)
{
	BView::FrameMoved(newLocation);
}
//------------------------------------------------------------------------------
BHandler *BBox::ResolveSpecifier(BMessage *message, int32 index,
								 BMessage *specifier, int32 what,
								 const char *property)
{
	return BView::ResolveSpecifier(message, index, specifier, what, property);
}
//------------------------------------------------------------------------------
void BBox::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}
//------------------------------------------------------------------------------
void BBox::GetPreferredSize(float *width, float *height)
{
	BRect rect(0,0,99,99);

	if (Parent())
	{
		rect = Parent()->Bounds();
		rect.InsetBy(10,10);
	}
	
	*width = rect.Width();
	*height = rect.Height();
}
//------------------------------------------------------------------------------
void BBox::MakeFocus(bool focused)
{
	BView::MakeFocus(focused);
}
//------------------------------------------------------------------------------
status_t BBox::GetSupportedSuites(BMessage *message)
{
	return BView::GetSupportedSuites(message);
}
//------------------------------------------------------------------------------
status_t BBox::Perform(perform_code d, void *arg)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BBox::_ReservedBox1() {}
void BBox::_ReservedBox2() {}
//------------------------------------------------------------------------------
BBox &BBox::operator=(const BBox &)
{
	return *this;
}
//------------------------------------------------------------------------------
void BBox::InitObject(BMessage *data)
{
}
//------------------------------------------------------------------------------
void BBox::DrawPlain()
{
	BRect rect = fBounds;

	rect.top--;
	SetHighColor(tint_color(ViewColor(), B_LIGHTEN_MAX_TINT));
	StrokeLine(BPoint(rect.left, rect.bottom), BPoint(rect.left, rect.top));
	StrokeLine(BPoint(rect.left+1.0f, rect.top), BPoint(rect.right, rect.top));
	SetHighColor(tint_color(ViewColor(), B_DARKEN_3_TINT));
	StrokeLine(BPoint(rect.left+1.0f, rect.bottom), BPoint(rect.right, rect.bottom));
	StrokeLine(BPoint(rect.right, rect.bottom), BPoint(rect.right, rect.top+1.0f));
}
//------------------------------------------------------------------------------
void BBox::DrawFancy()
{
	BRect rect = fBounds;

	SetHighColor(tint_color(ViewColor(), B_LIGHTEN_MAX_TINT));
	rect.left++;
	rect.top++;
	StrokeRect(rect);
	SetHighColor(tint_color(ViewColor(), B_DARKEN_3_TINT));
	rect.OffsetBy(-1,-1);
	StrokeRect(rect);
}
//------------------------------------------------------------------------------
void BBox::ClearAnyLabel()
{
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
