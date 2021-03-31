/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */


#include <ConnectionIncoming.h>
#include <ConnectionView.h>

namespace Bluetooth
{

ConnectionIncoming::ConnectionIncoming(bdaddr_t address)
	:
	BWindow(BRect(600, 100, 1000, 180), "Incoming Connection..",
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
					// 400x80
{
	SetPulseRate(1 * 1000 * 1000);
		// 1 second
	fView = new ConnectionView(BRect(0, 0, 400, 80), "<unknown_device>",
		bdaddrUtils::ToString(address));
	AddChild(fView);
}


ConnectionIncoming::ConnectionIncoming(RemoteDevice* rDevice)
	:
	BWindow(BRect(600, 100, 1000, 180), "Incoming Connection",
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	SetPulseRate(1 * 1000 * 1000);
		// 1 second

	if (rDevice != NULL)
		fView = new ConnectionView(BRect(0, 0, 400, 80), rDevice->GetFriendlyName(),
					bdaddrUtils::ToString(rDevice->GetBluetoothAddress()));
	else
		fView = new ConnectionView(BRect(0, 0, 400, 80), "<unknown_device>",
					bdaddrUtils::ToString(bdaddrUtils::NullAddress()));

	AddChild(fView);
}


ConnectionIncoming::~ConnectionIncoming()
{
}


void
ConnectionIncoming::MessageReceived(BMessage* message)
{
}


bool
ConnectionIncoming::QuitRequested()
{
	return BWindow::QuitRequested();
}


} /* end namespace Bluetooth */
