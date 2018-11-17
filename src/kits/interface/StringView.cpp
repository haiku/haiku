/*
 * Copyright 2001-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Frans van Nispen (xlr8@tref.nl)
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


//!	BStringView draws a non-editable text string.


#include <StringView.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <LayoutUtils.h>
#include <Message.h>
#include <PropertyInfo.h>
#include <StringList.h>
#include <View.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>


static property_info sPropertyList[] = {
	{
		"Text",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_STRING_TYPE }
	},
	{
		"Alignment",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_INT32_TYPE }
	},

	{ 0 }
};


BStringView::BStringView(BRect frame, const char* name, const char* text,
	uint32 resizingMode, uint32 flags)
	:
	BView(frame, name, resizingMode, flags | B_FULL_UPDATE_ON_RESIZE),
	fText(text ? strdup(text) : NULL),
	fTruncation(B_NO_TRUNCATION),
	fAlign(B_ALIGN_LEFT),
	fPreferredSize(text ? _StringWidth(text) : 0.0, -1)
{
}


BStringView::BStringView(const char* name, const char* text, uint32 flags)
	:
	BView(name, flags | B_FULL_UPDATE_ON_RESIZE),
	fText(text ? strdup(text) : NULL),
	fTruncation(B_NO_TRUNCATION),
	fAlign(B_ALIGN_LEFT),
	fPreferredSize(text ? _StringWidth(text) : 0.0, -1)
{
}


BStringView::BStringView(BMessage* archive)
	:
	BView(archive),
	fText(NULL),
	fTruncation(B_NO_TRUNCATION),
	fPreferredSize(0, -1)
{
	fAlign = (alignment)archive->GetInt32("_align", B_ALIGN_LEFT);
	fTruncation = (uint32)archive->GetInt32("_truncation", B_NO_TRUNCATION);

	const char* text = archive->GetString("_text", NULL);

	SetText(text);
	SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE);
}


BStringView::~BStringView()
{
	free(fText);
}


// #pragma mark - Archiving methods


BArchivable*
BStringView::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data, "BStringView"))
		return NULL;

	return new BStringView(data);
}


status_t
BStringView::Archive(BMessage* data, bool deep) const
{
	status_t status = BView::Archive(data, deep);

	if (status == B_OK && fText)
		status = data->AddString("_text", fText);
	if (status == B_OK && fTruncation != B_NO_TRUNCATION)
		status = data->AddInt32("_truncation", fTruncation);
	if (status == B_OK)
		status = data->AddInt32("_align", fAlign);

	return status;
}


// #pragma mark - Hook methods


void
BStringView::AttachedToWindow()
{
	if (HasDefaultColors())
		SetHighUIColor(B_PANEL_TEXT_COLOR);

	BView* parent = Parent();

	if (parent != NULL) {
		float tint = B_NO_TINT;
		color_which which = parent->ViewUIColor(&tint);

		if (which != B_NO_COLOR) {
			SetViewUIColor(which, tint);
			SetLowUIColor(which, tint);
		} else {
			SetViewColor(parent->ViewColor());
			SetLowColor(ViewColor());
		}
	}

	if (ViewColor() == B_TRANSPARENT_COLOR)
		AdoptSystemColors();
}


void
BStringView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BStringView::AllAttached()
{
	BView::AllAttached();
}


void
BStringView::AllDetached()
{
	BView::AllDetached();
}


// #pragma mark - Layout methods


void
BStringView::MakeFocus(bool focus)
{
	BView::MakeFocus(focus);
}


void
BStringView::GetPreferredSize(float* _width, float* _height)
{
	_ValidatePreferredSize();

	if (_width)
		*_width = fPreferredSize.width;

	if (_height)
		*_height = fPreferredSize.height;
}


BSize
BStringView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(),
		_ValidatePreferredSize());
}


BSize
BStringView::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		_ValidatePreferredSize());
}


BSize
BStringView::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		_ValidatePreferredSize());
}


void
BStringView::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);

	// Resize the width only for B_ALIGN_LEFT (if its large enough already, that is)
	if (Bounds().Width() > width && Alignment() != B_ALIGN_LEFT)
		width = Bounds().Width();

	BView::ResizeTo(width, height);
}


BAlignment
BStringView::LayoutAlignment()
{
	return BLayoutUtils::ComposeAlignment(ExplicitAlignment(),
		BAlignment(fAlign, B_ALIGN_MIDDLE));
}


// #pragma mark - More hook methods


void
BStringView::FrameMoved(BPoint newPosition)
{
	BView::FrameMoved(newPosition);
}


void
BStringView::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);
}


void
BStringView::Draw(BRect updateRect)
{
	if (!fText)
		return;

	if (LowUIColor() == B_NO_COLOR)
		SetLowColor(ViewColor());

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	BRect bounds = Bounds();

	BStringList lines;
	BString(fText).Split("\n", false, lines);
	for (int i = 0; i < lines.CountStrings(); i++) {
		const char* text = lines.StringAt(i).String();
		float width = StringWidth(text);
		BString truncated;
		if (fTruncation != B_NO_TRUNCATION && width > bounds.Width()) {
			// The string needs to be truncated
			// TODO: we should cache this
			truncated = lines.StringAt(i);
			TruncateString(&truncated, fTruncation, bounds.Width());
			text = truncated.String();
			width = StringWidth(text);
		}

		float y = (bounds.top + bounds.bottom - ceilf(fontHeight.descent))
			- ceilf(fontHeight.ascent + fontHeight.descent + fontHeight.leading)
				* (lines.CountStrings() - i - 1);
		float x;
		switch (fAlign) {
			case B_ALIGN_RIGHT:
				x = bounds.Width() - width;
				break;

			case B_ALIGN_CENTER:
				x = (bounds.Width() - width) / 2.0;
				break;

			default:
				x = 0.0;
				break;
		}

		DrawString(text, BPoint(x, y));
	}
}


void
BStringView::MessageReceived(BMessage* message)
{
	if (message->what == B_GET_PROPERTY || message->what == B_SET_PROPERTY) {
		int32 index;
		BMessage specifier;
		int32 form;
		const char* property;
		if (message->GetCurrentSpecifier(&index, &specifier, &form, &property)
				!= B_OK) {
			BView::MessageReceived(message);
			return;
		}

		BMessage reply(B_REPLY);
		bool handled = false;
		if (strcmp(property, "Text") == 0) {
			if (message->what == B_GET_PROPERTY) {
				reply.AddString("result", fText);
				handled = true;
			} else {
				const char* text;
				if (message->FindString("data", &text) == B_OK) {
					SetText(text);
					reply.AddInt32("error", B_OK);
					handled = true;
				}
			}
		} else if (strcmp(property, "Alignment") == 0) {
			if (message->what == B_GET_PROPERTY) {
				reply.AddInt32("result", (int32)fAlign);
				handled = true;
			} else {
				int32 align;
				if (message->FindInt32("data", &align) == B_OK) {
					SetAlignment((alignment)align);
					reply.AddInt32("error", B_OK);
					handled = true;
				}
			}
		}

		if (handled) {
			message->SendReply(&reply);
			return;
		}
	}

	BView::MessageReceived(message);
}


void
BStringView::MouseDown(BPoint point)
{
	BView::MouseDown(point);
}


void
BStringView::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}


void
BStringView::MouseMoved(BPoint point, uint32 transit, const BMessage* msg)
{
	BView::MouseMoved(point, transit, msg);
}


// #pragma mark -


void
BStringView::SetText(const char* text)
{
	if ((text && fText && !strcmp(text, fText)) || (!text && !fText))
		return;

	free(fText);
	fText = text ? strdup(text) : NULL;

	float newStringWidth = _StringWidth(fText);
	if (fPreferredSize.width != newStringWidth) {
		fPreferredSize.width = newStringWidth;
		InvalidateLayout();
	}

	Invalidate();
}


const char*
BStringView::Text() const
{
	return fText;
}


void
BStringView::SetAlignment(alignment flag)
{
	fAlign = flag;
	Invalidate();
}


alignment
BStringView::Alignment() const
{
	return fAlign;
}


void
BStringView::SetTruncation(uint32 truncationMode)
{
	if (fTruncation != truncationMode) {
		fTruncation = truncationMode;
		Invalidate();
	}
}


uint32
BStringView::Truncation() const
{
	return fTruncation;
}


BHandler*
BStringView::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	BPropertyInfo propInfo(sPropertyList);
	if (propInfo.FindMatch(message, 0, specifier, form, property) >= B_OK)
		return this;

	return BView::ResolveSpecifier(message, index, specifier, form, property);
}


status_t
BStringView::GetSupportedSuites(BMessage* data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t status = data->AddString("suites", "suite/vnd.Be-string-view");
	if (status != B_OK)
		return status;

	BPropertyInfo propertyInfo(sPropertyList);
	status = data->AddFlat("messages", &propertyInfo);
	if (status != B_OK)
		return status;

	return BView::GetSupportedSuites(data);
}


void
BStringView::SetFont(const BFont* font, uint32 mask)
{
	BView::SetFont(font, mask);

	fPreferredSize.width = _StringWidth(fText);

	Invalidate();
	InvalidateLayout();
}


void
BStringView::LayoutInvalidated(bool descendants)
{
	// invalidate cached preferred size
	fPreferredSize.height = -1;
}


// #pragma mark - Perform


status_t
BStringView::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BStringView::MinSize();
			return B_OK;

		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BStringView::MaxSize();
			return B_OK;

		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BStringView::PreferredSize();
			return B_OK;

		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BStringView::LayoutAlignment();
			return B_OK;

		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BStringView::HasHeightForWidth();
			return B_OK;

		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BStringView::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}

		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BStringView::SetLayout(data->layout);
			return B_OK;
		}

		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BStringView::LayoutInvalidated(data->descendants);
			return B_OK;
		}

		case PERFORM_CODE_DO_LAYOUT:
		{
			BStringView::DoLayout();
			return B_OK;
		}
	}

	return BView::Perform(code, _data);
}


// #pragma mark - FBC padding methods


void BStringView::_ReservedStringView1() {}
void BStringView::_ReservedStringView2() {}
void BStringView::_ReservedStringView3() {}


// #pragma mark - Private methods


BStringView&
BStringView::operator=(const BStringView&)
{
	// Assignment not allowed (private)
	return *this;
}


BSize
BStringView::_ValidatePreferredSize()
{
	if (fPreferredSize.height < 0) {
		// height
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		int32 lines = 1;
		char* temp = fText ? strchr(fText, '\n') : NULL;
		while (temp != NULL) {
			temp = strchr(temp + 1, '\n');
			lines++;
		};

		fPreferredSize.height = ceilf(fontHeight.ascent + fontHeight.descent
			+ fontHeight.leading) * lines;

		ResetLayoutInvalidation();
	}

	return fPreferredSize;
}


float
BStringView::_StringWidth(const char* text)
{
	if(text == NULL)
		return 0.0f;

	float maxWidth = 0.0f;
	BStringList lines;
	BString(fText).Split("\n", false, lines);
	for (int i = 0; i < lines.CountStrings(); i++) {
		float width = StringWidth(lines.StringAt(i));
		if (maxWidth < width)
			maxWidth = width;
	}
	return maxWidth;
}


extern "C" void
B_IF_GCC_2(InvalidateLayout__11BStringViewb,
	_ZN11BStringView16InvalidateLayoutEb)(BView* view, bool descendants)
{
	perform_data_layout_invalidated data;
	data.descendants = descendants;

	view->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}
