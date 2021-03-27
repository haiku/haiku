/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */

#include <ConnectionView.h>
#include <BluetoothIconView.h>

namespace Bluetooth
{

ConnectionView::ConnectionView(BRect frame, BString device, BString address)
	:
	BView(frame, "ConnectionView", 0, B_PULSE_NEEDED)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fIcon = new BluetoothIconView();

	strMessage = "A new connection is incoming..";

	fMessage = new BStringView(frame, "", strMessage, B_FOLLOW_LEFT);
	fMessage->SetAlignment(B_ALIGN_LEFT);

	fDeviceLabel = new BStringView(frame, "", "Device Name:", B_FOLLOW_LEFT);
	fDeviceLabel->SetFont(be_bold_font);

	fDeviceText = new BStringView(frame, "", device, B_FOLLOW_RIGHT);
	fDeviceText->SetAlignment(B_ALIGN_RIGHT);

	fAddressLabel = new BStringView(frame, "", "MAC Address:", B_FOLLOW_LEFT);
	fAddressLabel->SetFont(be_bold_font);

	fAddressText = new BStringView(frame, "", address, B_FOLLOW_RIGHT);
	fAddressText->SetAlignment(B_ALIGN_RIGHT);

	AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 0)
			.Add(BGroupLayoutBuilder(B_VERTICAL, 8)
				.Add(fIcon)
			)
			.Add(BGroupLayoutBuilder(B_VERTICAL, 0)
				.Add(fMessage)
				.AddGlue()
				.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
					.Add(fDeviceLabel)
					.AddGlue()
					.Add(fDeviceText)
				)
				.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
					.Add(fAddressLabel)
					.AddGlue()
					.Add(fAddressText)
				)
				.AddGlue()
			)
			.AddGlue()
			.SetInsets(8, 8, 8, 8)
	);
}


void
ConnectionView::Pulse()
{
	static int pulses = 0;

	pulses++;

	if (pulses >= 5) {
		Window()->PostMessage(B_QUIT_REQUESTED);
	} else {
		strMessage += ".";
		fMessage->SetText(strMessage);
	}
}

}
