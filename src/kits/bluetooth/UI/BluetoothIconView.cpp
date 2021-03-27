/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */

#include <BluetoothIconView.h>

#include <stdio.h>

namespace Bluetooth {

BBitmap* 	BluetoothIconView::fBitmap = NULL;
int32		BluetoothIconView::fRefCount = 0;

BluetoothIconView::BluetoothIconView()
	:
	BView(BRect(0, 0, 80, 80), "", B_FOLLOW_ALL, B_WILL_DRAW)
{
	if (fRefCount == 0) {
		fBitmap = new BBitmap(BRect(0, 0, 64, 64), 0, B_RGBA32);

		uint8* tempIcon;
		size_t tempSize;

		BMimeType mime("application/x-vnd.Haiku-bluetooth_server");
		mime.GetIcon(&tempIcon, &tempSize);

		BIconUtils::GetVectorIcon(tempIcon, tempSize, fBitmap);

		fRefCount++;
	} else {
		fRefCount++;
	}

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
}


BluetoothIconView::~BluetoothIconView()
{
	fRefCount--;

	if (fRefCount <= 0)
		delete fBitmap;
}


void
BluetoothIconView::Draw(BRect rect)
{
	this->DrawBitmap(fBitmap);
}

}
