/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PropertyItemView.h"

#include <stdio.h>

#include <Message.h>
#include <Window.h>

#include "support_ui.h"
#include "ui_defines.h"

#include "CommonPropertyIDs.h"
#include "Property.h"
#include "PropertyEditorFactory.h"
#include "PropertyEditorView.h"
#include "PropertyListView.h"

// constructor
PropertyItemView::PropertyItemView(Property* property)
	: BView(BRect(0.0, 0.0, 10.0, 10.0), "property item",
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
			B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	  fParent(NULL),
	  fEditorView(/*factory->*/EditorFor(property)),
	  	// NOTE: can be NULL if property is NULL or unkown
	  fSelected(false),
	  fEnabled(true),
	  fLabelWidth(0.0)
{
	if (fEditorView) {
		AddChild(fEditorView);
		fEditorView->SetItemView(this);
	}
}

// destructor
PropertyItemView::~PropertyItemView()
{
}

// Draw
void
PropertyItemView::Draw(BRect updateRect)
{
	const Property* property = GetProperty();
	if (property && fParent) {
		BRect b(Bounds());

		// just draw background and label
		rgb_color labelColor = LowColor();
		if (fEnabled)
			labelColor = tint_color(labelColor, B_DARKEN_MAX_TINT);
		else
			labelColor = tint_color(labelColor, B_DISABLED_LABEL_TINT);

		SetHighColor(labelColor);
		BFont font;
		GetFont(&font);

		BString truncated(name_for_id(property->Identifier()));
		font.TruncateString(&truncated, B_TRUNCATE_MIDDLE, fLabelWidth - 10.0);

		font_height fh;
		font.GetHeight(&fh);

		FillRect(BRect(b.left, b.top, b.left + fLabelWidth, b.bottom), B_SOLID_LOW);
		DrawString(truncated.String(), BPoint(b.left + 5.0,
											  floorf(b.top + b.Height() / 2.0
												  		   + fh.ascent / 2.0)));

		// draw a "separator" line behind the label
		SetHighColor(tint_color(LowColor(), B_DARKEN_1_TINT));
		StrokeLine(BPoint(b.left + fLabelWidth - 1.0, b.top),
				   BPoint(b.left + fLabelWidth - 1.0, b.bottom), B_SOLID_HIGH);
	}
}

// FrameResized
void
PropertyItemView::FrameResized(float width, float height)
{
	if (fEditorView) {
		fEditorView->MoveTo(fLabelWidth, 0.0);
		fEditorView->ResizeTo(width - fLabelWidth, height);
		fEditorView->FrameResized(fEditorView->Bounds().Width(),
								 fEditorView->Bounds().Height());
	}
}

// MakeFocus
void
PropertyItemView::MakeFocus(bool focused)
{
	if (fEditorView)
		fEditorView->MakeFocus(focused);
}

// MouseDown
void
PropertyItemView::MouseDown(BPoint where)
{
	if (fParent) {
		// select ourself
		fParent->Select(this);
		if (fEditorView)
			fEditorView->MakeFocus(true);

		if (BMessage* message = Window()->CurrentMessage()) {
			int32 clicks;
			if (message->FindInt32("clicks", &clicks) >= B_OK) {
				if (clicks >= 2)
					fParent->DoubleClicked(this);
				else
					fParent->Clicked(this);
			}
		}
	}
}

// MouseUp
void
PropertyItemView::MouseUp(BPoint where)
{
}

// MouseMoved
void
PropertyItemView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
}

// PreferredHeight
float
PropertyItemView::PreferredHeight() const
{
	font_height fh;
	GetFontHeight(&fh);

	float height = floorf(4.0 + fh.ascent + fh.descent);
	if (fEditorView)
		height = max_c(height, fEditorView->PreferredHeight());

	return height;
}

// PreferredLabelWidth
float
PropertyItemView::PreferredLabelWidth() const
{
	float width = 0.0;
	if (const Property* property = GetProperty())
		width = ceilf(StringWidth(name_for_id(property->Identifier())) + 10.0);
	return width;
}

// SetLabelWidth
void
PropertyItemView::SetLabelWidth(float width)
{
	if (width < 0.0)
		width = 0.0;
/*	if (fEditorView && width > Bounds().Width() - fEditorView->Bounds().Width())
		width = Bounds().Width() - fEditorView->Bounds().Width();
	else if (width > Bounds().Width())
		width = Bounds().Width();*/

	fLabelWidth = width;
}

// SetSelected
void
PropertyItemView::SetSelected(bool selected)
{
	fSelected = selected;
	_UpdateLowColor();
}

// SetEnabled
void
PropertyItemView::SetEnabled(bool enabled)
{
	if (fEnabled != enabled) {
		fEnabled = enabled;
		fEditorView->SetEnabled(fEnabled);
	}
}

// IsFocused
bool
PropertyItemView::IsFocused() const
{
	if (fEditorView)
		return fEditorView->IsFocused();
	return false;
}

// GetProperty
Property*
PropertyItemView::GetProperty() const
{
	if (fEditorView)
		return fEditorView->GetProperty();
	return NULL;
}

// SetProperty
bool
PropertyItemView::AdoptProperty(Property* property)
{
	if (fEditorView && fEditorView->AdoptProperty(property)) {
		_UpdateLowColor();
		return true;
	}
	return false;
}

// SetListView
void
PropertyItemView::SetListView(PropertyListView* parent)
{
	fParent = parent;
	_UpdateLowColor();
}

// UpdateObject
void
PropertyItemView::UpdateObject()
{
	if (fParent && fEditorView) {
		if (const Property* p = fEditorView->GetProperty())
			fParent->UpdateObject(p->Identifier());
	}
}

// #pragma mark -

// _UpdateLowColor
void
PropertyItemView::_UpdateLowColor()
{
	rgb_color lowColor = fParent ? fParent->LowColor() : kWhite;
	if (fSelected)
		lowColor = tint_color(lowColor, B_DARKEN_2_TINT);

	if (lowColor != LowColor()) {
		SetLowColor(lowColor);
		Invalidate();
	}
}

