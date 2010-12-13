/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "WirelessNetworkMenuItem.h"

#include "RadioView.h"


WirelessNetworkMenuItem::WirelessNetworkMenuItem(const char* name,
	int32 signalQuality, bool encrypted, BMessage* message)
	:
	BMenuItem(name, message),
	fQuality(signalQuality),
	fIsEncrypted(encrypted)
{
}


WirelessNetworkMenuItem::~WirelessNetworkMenuItem()
{
}


void
WirelessNetworkMenuItem::SetSignalQuality(int32 quality)
{
	fQuality = quality;
}


void
WirelessNetworkMenuItem::DrawContent()
{
	DrawRadioIcon();
	BMenuItem::DrawContent();
}


void
WirelessNetworkMenuItem::Highlight(bool isHighlighted)
{
	BMenuItem::Highlight(isHighlighted);
}


void
WirelessNetworkMenuItem::GetContentSize(float* width, float* height)
{
	BMenuItem::GetContentSize(width, height);
	*width += *height + 4;
}


void
WirelessNetworkMenuItem::DrawRadioIcon()
{
	BRect bounds = Frame();
	bounds.left = bounds.right - 4 - bounds.Height();
	bounds.right -= 4;
	bounds.bottom -= 2;

	RadioView::Draw(Menu(), bounds, fQuality, RadioView::DefaultMax());
}
