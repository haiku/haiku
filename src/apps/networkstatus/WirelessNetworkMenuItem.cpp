/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "WirelessNetworkMenuItem.h"

#include <string.h>

#include <Catalog.h>
#include <String.h>

#include "RadioView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WirelessNetworkMenuItem"


WirelessNetworkMenuItem::WirelessNetworkMenuItem(wireless_network network,
	BMessage* message)
	:
	BMenuItem(network.name, message),
	fNetwork(network)
{
	// Append authentication mode to label
	BString label = B_TRANSLATE("%name% (%authenticationMode%)");
	label.Replace("%name%", network.name, 1);
	label.Replace("%authenticationMode%",
		AuthenticationName(network.authentication_mode), 1);

	SetLabel(label.String());
}


WirelessNetworkMenuItem::~WirelessNetworkMenuItem()
{
}


BString
WirelessNetworkMenuItem::AuthenticationName(int32 mode)
{
	switch (mode) {
		default:
		case B_NETWORK_AUTHENTICATION_NONE:
			return B_TRANSLATE_CONTEXT("open", "Open network");
			break;
		case B_NETWORK_AUTHENTICATION_WEP:
			return B_TRANSLATE_CONTEXT("WEP", "WEP protected network");
			break;
		case B_NETWORK_AUTHENTICATION_WPA:
			return B_TRANSLATE_CONTEXT("WPA", "WPA protected network");
			break;
		case B_NETWORK_AUTHENTICATION_WPA2:
			return B_TRANSLATE_CONTEXT("WPA2", "WPA2 protected network");
			break;
		case B_NETWORK_AUTHENTICATION_EAP:
			return B_TRANSLATE_CONTEXT("EAP", "EAP protected network");
			break;
	}
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

	RadioView::Draw(Menu(), bounds, fNetwork.signal_strength,
		RadioView::DefaultMax());
}


/*static*/ int
WirelessNetworkMenuItem::CompareSignalStrength(const BMenuItem* a,
	const BMenuItem* b)
{
	WirelessNetworkMenuItem* aItem = (WirelessNetworkMenuItem*)a;
	WirelessNetworkMenuItem* bItem = (WirelessNetworkMenuItem*)b;

	wireless_network aNetwork = aItem->Network();
	wireless_network bNetwork = bItem->Network();

	if (aNetwork.signal_strength == bNetwork.signal_strength)
		return strncasecmp(aNetwork.name, bNetwork.name, 32);

	return bNetwork.signal_strength - aNetwork.signal_strength;
}
