/*
 * Copyright 2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "NotAvailableStringView.h"

#include <ControlLook.h>

#include "Logger.h"


NotAvailableStringView::NotAvailableStringView(const char* name, const char* text)
	:
	BView(name, B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW),
	fText(text)
{
}


NotAvailableStringView::~NotAvailableStringView()
{
}


BSize
NotAvailableStringView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
		// can be any size
}


BAlignment
NotAvailableStringView::LayoutAlignment()
{
	return BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE);
}


void
NotAvailableStringView::SetText(const char* text)
{
	if (fText != text) {
		fText = text;
		Invalidate();
	}
}


const char*
NotAvailableStringView::Text() const
{
	return fText;
}


void
NotAvailableStringView::Draw(BRect updateRect)
{
	rgb_color highColor = HighColor();
	be_control_look->DrawLabel(this, fText, Bounds(), updateRect, ViewColor(),
		BControlLook::B_DISABLED, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE), &highColor);
}
