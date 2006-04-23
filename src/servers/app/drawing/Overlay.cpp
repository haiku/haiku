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
Overlay::Show()
{
	if (fOverlayToken == NULL) {
		fOverlayToken = fHWInterface.AcquireOverlayChannel();
		if (fOverlayToken == NULL)
			return;
	}

	fHWInterface.ShowOverlay(this);
}


void
Overlay::Hide()
{
	if (fOverlayToken == NULL)
		return;

	fHWInterface.HideOverlay(this);
}


void
Overlay::SetView(const BRect& source, const BRect& destination)
{
	fSource = source;
	fDestination = destination;

	if (fOverlayToken == NULL)
		return;

	fHWInterface.UpdateOverlay(this);
}

