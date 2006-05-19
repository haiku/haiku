/*
 * Copyright 2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "HWInterface.h"
#include "Overlay.h"

#include <BitmapPrivate.h>

#include <InterfaceDefs.h>


Overlay::Overlay(HWInterface& interface)
	:
	fHWInterface(interface),
	fOverlayBuffer(NULL),
	fClientData(NULL),
	fOverlayToken(NULL)
{
	fSemaphore = create_sem(1, "overlay lock");
	fColor.SetColor(21, 16, 21, 16);
		// TODO: whatever fine color we want to use here...

	fWindow.offset_top = 0;
	fWindow.offset_left = 0;
	fWindow.offset_right = 0;
	fWindow.offset_bottom = 0;

	fWindow.flags = B_OVERLAY_COLOR_KEY;
}


Overlay::~Overlay()
{
	fHWInterface.ReleaseOverlayChannel(fOverlayToken);
	fHWInterface.FreeOverlayBuffer(fOverlayBuffer);

	delete_sem(fSemaphore);
}


status_t
Overlay::InitCheck() const
{
	return fSemaphore >= B_OK ? B_OK : fSemaphore;
}


void
Overlay::SetOverlayData(const overlay_buffer* overlayBuffer,
	overlay_token token, overlay_client_data* clientData)
{
	fOverlayBuffer = overlayBuffer;
	fOverlayToken = token;

	fClientData = clientData;
	fClientData->lock = fSemaphore;
	fClientData->buffer = (uint8*)fOverlayBuffer->buffer;
}


void
Overlay::SetFlags(uint32 flags)
{
	if (flags & B_OVERLAY_FILTER_HORIZONTAL)
		fWindow.flags |= B_OVERLAY_HORIZONTAL_FILTERING;
	if (flags & B_OVERLAY_FILTER_VERTICAL)
		fWindow.flags |= B_OVERLAY_VERTICAL_FILTERING;
	if (flags & B_OVERLAY_MIRROR)
		fWindow.flags |= B_OVERLAY_HORIZONTAL_MIRRORING;
}


void
Overlay::TakeOverToken(Overlay* other)
{
	overlay_token token = other->OverlayToken();
	if (token == NULL)
		return;

	fOverlayToken = token;
	//other->fOverlayToken = NULL;
}


const overlay_buffer*
Overlay::OverlayBuffer() const
{
	return fOverlayBuffer;
}


overlay_client_data*
Overlay::ClientData() const
{
	return fClientData;
}


overlay_token
Overlay::OverlayToken() const
{
	return fOverlayToken;
}


void
Overlay::Hide()
{
	if (fOverlayToken == NULL)
		return;

	fHWInterface.HideOverlay(this);
}


void
Overlay::SetColorSpace(uint32 colorSpace)
{
	if ((fWindow.flags & B_OVERLAY_COLOR_KEY) == 0)
		return;

	uint8 colorShift = 0, greenShift = 0, alphaShift = 0;
	rgb_color colorKey = fColor.GetColor32();

	switch (colorSpace) {
		case B_CMAP8:
			colorKey.red = 0xff;
			colorKey.green = 0xff;
			colorKey.blue = 0xff;
			colorKey.alpha = 0xff;
			break;
		case B_RGB15:
			greenShift = colorShift = 3;
			alphaShift = 7;
			break;
		case B_RGB16:
			colorShift = 3;
			greenShift = 2;
			alphaShift = 8;
			break;
	}

	fWindow.red.value = colorKey.red >> colorShift;
	fWindow.green.value = colorKey.green >> greenShift;
	fWindow.blue.value = colorKey.blue >> colorShift;
	fWindow.alpha.value = colorKey.alpha >> alphaShift;
	fWindow.red.mask = 0xff >> colorShift;
	fWindow.green.mask = 0xff >> greenShift;
	fWindow.blue.mask = 0xff >> colorShift;
	fWindow.alpha.mask = 0xff >> alphaShift;
}


void
Overlay::Configure(const BRect& source, const BRect& destination)
{
	if (fOverlayToken == NULL) {
		fOverlayToken = fHWInterface.AcquireOverlayChannel();
		if (fOverlayToken == NULL)
			return;
	}

	fView.h_start = (uint16)source.left;
	fView.v_start = (uint16)source.top;
	fView.width = (uint16)source.IntegerWidth() + 1;
	fView.height = (uint16)source.IntegerHeight() + 1;

	fWindow.h_start = (int16)destination.left;
	fWindow.v_start = (int16)destination.top;
	fWindow.width = (uint16)destination.IntegerWidth() + 1;
	fWindow.height = (uint16)destination.IntegerHeight() + 1;

	fHWInterface.ConfigureOverlay(this);
}

