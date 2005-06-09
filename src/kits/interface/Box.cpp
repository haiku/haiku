//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
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
#include <stdlib.h>
#include <string.h>

#include <Box.h>
#include <Message.h>


BBox::BBox(BRect frame, const char *name, uint32 resizingMode, uint32 flags,
		   border_style border)
	:	BView(frame, name, resizingMode, flags | B_FRAME_EVENTS),
		fStyle(border)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	InitObject();
}


BBox::~BBox()
{
	ClearAnyLabel();
}


BBox::BBox(BMessage *archive)
	:	BView(archive)
{
	InitObject(archive);

	const char *string;

	if (archive->FindString("_label", &string) == B_OK)
		SetLabel(string);

	bool aBool;
	int32 anInt32;

	if (archive->FindBool("_style", &aBool) == B_OK)
		fStyle = aBool ? B_FANCY_BORDER : B_PLAIN_BORDER;
	else if (archive->FindInt32("_style", &anInt32) == B_OK)
		fStyle = (border_style)anInt32;

	if (archive->FindBool("_lblview", &aBool) == B_OK)
		fLabelView = ChildAt(0);
}


BArchivable *
BBox::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BBox"))
		return new BBox(archive);
	else
		return NULL;
}


status_t
BBox::Archive(BMessage *archive, bool deep) const
{
	BView::Archive(archive, deep);

	if (fLabel)
		 archive->AddString("_label", fLabel);

	if (fLabelView)
		 archive->AddBool("_lblview", true);

	if (fStyle != B_FANCY_BORDER)
		archive->AddInt32("_style", fStyle);

	return B_OK;
}


void
BBox::SetBorder(border_style border)
{
	fStyle = border;

	if (Window() != NULL && LockLooper()) {
		Invalidate();
		UnlockLooper();
	}
}


border_style
BBox::Border() const
{
	return fStyle;
}


void
BBox::SetLabel(const char *string)
{ 
	ClearAnyLabel();

	if (string) {
		// Update fBounds
		fBounds = Bounds();
		font_height fh;
		GetFontHeight(&fh);

		fBounds.top = (float)ceil((fh.ascent + fh.descent) / 2.0f);

		fLabel = strdup(string);
	}

	if (Window())
		Invalidate();
}


status_t
BBox::SetLabel(BView *viewLabel)
{
	ClearAnyLabel();

	if (viewLabel) {
		// Update fBounds
		fBounds = Bounds();

		fBounds.top = (float)ceil(viewLabel->Bounds().Height() / 2.0f);

		fLabelView = viewLabel;
		fLabelView->MoveTo(10.0f, 0.0f);
		AddChild(fLabelView, ChildAt(0));
	}

	if (Window())
		Invalidate();

	return B_OK;
}


const char *
BBox::Label() const
{
	return fLabel;
}


BView *
BBox::LabelView() const
{
	return fLabelView;
}


void
BBox::Draw(BRect updateRect)
{
	switch (fStyle) {
		case B_FANCY_BORDER:
			DrawFancy();
			break;

		case B_PLAIN_BORDER:
			DrawPlain();
			break;

		default:
			break;
	}
	
	if (fLabel) {
		font_height fh;
		GetFontHeight(&fh);

		SetHighColor(ViewColor());

		FillRect(BRect(6.0f, 1.0f, 12.0f + StringWidth(fLabel),
			(float)ceil(fh.ascent + fh.descent))/*, B_SOLID_LOW*/);

		SetHighColor(0, 0, 0);
		DrawString(fLabel, BPoint(10.0f, (float)ceil(fh.ascent - fh.descent)
			+ 1.0f));
	}
}


void BBox::AttachedToWindow()
{
	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
		SetLowColor(Parent()->ViewColor());
	}
}


void
BBox::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BBox::AllAttached()
{
	BView::AllAttached();
}


void
BBox::AllDetached()
{
	BView::AllDetached();
}


void
BBox::FrameResized(float width, float height)
{
	BRect bounds(Bounds());

	// TODO: only invalidate the part that really need redrawing!

	fBounds.right = bounds.right;
	fBounds.bottom = bounds.bottom;

	Invalidate();
}


void
BBox::MessageReceived(BMessage *message)
{
	BView::MessageReceived(message);
}


void
BBox::MouseDown(BPoint point)
{
	BView::MouseDown(point);
}


void
BBox::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}


void
BBox::WindowActivated(bool active)
{
	BView::WindowActivated(active);
}


void
BBox::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BView::MouseMoved(point, transit, message);
}


void
BBox::FrameMoved(BPoint newLocation)
{
	BView::FrameMoved(newLocation);
}


BHandler *
BBox::ResolveSpecifier(BMessage *message, int32 index,
								 BMessage *specifier, int32 what,
								 const char *property)
{
	return BView::ResolveSpecifier(message, index, specifier, what, property);
}


void
BBox::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


void
BBox::GetPreferredSize(float *width, float *height)
{
/*	BRect rect(0,0,99,99);

	if (Parent())
	{
		rect = Parent()->Bounds();
		rect.InsetBy(10,10);
	}
	
	*width = rect.Width();
	*height = rect.Height();*/

	BView::GetPreferredSize(width, height);
}


void
BBox::MakeFocus(bool focused)
{
	BView::MakeFocus(focused);
}


status_t
BBox::GetSupportedSuites(BMessage *message)
{
	return BView::GetSupportedSuites(message);
}


status_t
BBox::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}


void BBox::_ReservedBox1() {}
void BBox::_ReservedBox2() {}


BBox &
BBox::operator=(const BBox &)
{
	return *this;
}


void
BBox::InitObject(BMessage *data)
{
	fLabel = NULL;
	fBounds = Bounds();
	fLabelView = NULL;

	int32 flags = 0;

	BFont font(be_bold_font);

	if (!data || !data->HasString("_fname"))
		flags = B_FONT_FAMILY_AND_STYLE;

	if (!data || !data->HasFloat("_fflt"))
		flags |= B_FONT_SIZE;

	if (flags != 0)
		SetFont(&font, flags);
}


void
BBox::DrawPlain()
{
	BRect r = fBounds;

	rgb_color light = tint_color(ViewColor(), B_LIGHTEN_MAX_TINT);
	rgb_color shadow = tint_color(ViewColor(), B_DARKEN_3_TINT);

	BeginLineArray(4);
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), light);
		AddLine(BPoint(r.left + 1.0f, r.top),
				BPoint(r.right, r.top), light);
		AddLine(BPoint(r.left + 1.0f, r.bottom),
				BPoint(r.right, r.bottom), shadow);
		AddLine(BPoint(r.right, r.bottom - 1.0f),
				BPoint(r.right, r.top + 1.0f), shadow);
	EndLineArray();
}


void
BBox::DrawFancy()
{
	BRect r = fBounds;

	rgb_color light = tint_color(ViewColor(), B_LIGHTEN_MAX_TINT);
	rgb_color shadow = tint_color(ViewColor(), B_DARKEN_3_TINT);

	BeginLineArray(8);
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), shadow);
		AddLine(BPoint(r.left + 1.0f, r.top),
				BPoint(r.right, r.top), shadow);
		AddLine(BPoint(r.left + 1.0f, r.bottom),
				BPoint(r.right, r.bottom), light);
		AddLine(BPoint(r.right, r.bottom - 1.0f),
				BPoint(r.right, r.top + 1.0f), light);

		r.InsetBy(1.0, 1.0);

		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), light);
		AddLine(BPoint(r.left + 1.0f, r.top),
				BPoint(r.right, r.top), light);
		AddLine(BPoint(r.left + 1.0f, r.bottom),
				BPoint(r.right, r.bottom), shadow);
		AddLine(BPoint(r.right, r.bottom - 1.0f),
				BPoint(r.right, r.top + 1.0f), shadow);
	EndLineArray();
}


void
BBox::ClearAnyLabel()
{
	fBounds.top = 0;
	
	if (fLabel) {
		free(fLabel);
		fLabel = NULL;
	} else if (fLabelView) {
		fLabelView->RemoveSelf();
		delete fLabelView;
		fLabelView = NULL;
	}
}
