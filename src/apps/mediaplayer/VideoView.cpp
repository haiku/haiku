/*
 * Copyright © 2006-2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "VideoView.h"

#include <stdio.h>

#include <Bitmap.h>


VideoView::VideoView(BRect frame, const char* name, uint32 resizeMask)
	: BView(frame, name, resizeMask, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	  fOverlayMode(false)
{
	SetViewColor(B_TRANSPARENT_COLOR);
		// might be reset to overlay key color if overlays are used
	SetHighColor(0, 0, 0);

	// create some hopefully sensible default overlay restrictions
	fOverlayRestrictions.min_width_scale = 0.25;
	fOverlayRestrictions.max_width_scale = 8.0;
	fOverlayRestrictions.min_height_scale = 0.25;
	fOverlayRestrictions.max_height_scale = 8.0;
}


VideoView::~VideoView()
{
}


void
VideoView::Draw(BRect updateRect)
{
	bool fillBlack = true;

	if (LockBitmap()) {
		BRect r(Bounds());
		if (const BBitmap* bitmap = GetBitmap()) {
			fillBlack = false;
			if (!fOverlayMode)
				DrawBitmap(bitmap, bitmap->Bounds(), r);
		}
		UnlockBitmap();
	}

	if (fillBlack)
		FillRect(updateRect);
}


void
VideoView::SetBitmap(const BBitmap* bitmap)
{
	VideoTarget::SetBitmap(bitmap);
	// Attention: Don't lock the window, if the bitmap is NULL. Otherwise
	// we're going to deadlock when the window tells the node manager to
	// stop the nodes (Window -> NodeManager -> VideoConsumer -> VideoView
	// -> Window).
	if (bitmap && LockLooperWithTimeout(10000) == B_OK) {
		if (LockBitmap()) {
//			if (fOverlayMode || bitmap->Flags() & B_BITMAP_WILL_OVERLAY) {
			if (fOverlayMode || bitmap->ColorSpace() == B_YCbCr422) {
				if (!fOverlayMode) {
					// init overlay
					rgb_color key;
					status_t ret = SetViewOverlay(bitmap, bitmap->Bounds(),
						Bounds(), &key, B_FOLLOW_ALL,
						B_OVERLAY_FILTER_HORIZONTAL
						| B_OVERLAY_FILTER_VERTICAL);
					if (ret == B_OK) {
						fOverlayKeyColor = key;
						SetViewColor(key);
						SetLowColor(key);
						snooze(20000);
						FillRect(Bounds(), B_SOLID_LOW);
						Sync();
						// use overlay from here on
						fOverlayMode = true;

						// update restrictions
						overlay_restrictions restrictions;
						if (bitmap->GetOverlayRestrictions(&restrictions)
								== B_OK)
							fOverlayRestrictions = restrictions;
					} else {
						// try again next time
						// synchronous draw
						FillRect(Bounds());
						Sync();
					}
				} else {
					// transfer overlay channel
					rgb_color key;
					SetViewOverlay(bitmap, bitmap->Bounds(), Bounds(),
						&key, B_FOLLOW_ALL, B_OVERLAY_FILTER_HORIZONTAL
							| B_OVERLAY_FILTER_VERTICAL
							| B_OVERLAY_TRANSFER_CHANNEL);
				}
			} else if (fOverlayMode && bitmap->ColorSpace() != B_YCbCr422) {
				fOverlayMode = false;
				ClearViewOverlay();
				SetViewColor(B_TRANSPARENT_COLOR);
			}
			if (!fOverlayMode)
				DrawBitmap(bitmap, bitmap->Bounds(), Bounds());

			UnlockBitmap();
		}
		UnlockLooper();
	}
}


void
VideoView::GetOverlayScaleLimits(float* minScale, float* maxScale) const
{
	*minScale = max_c(fOverlayRestrictions.min_width_scale,
		fOverlayRestrictions.min_height_scale);
	*maxScale = max_c(fOverlayRestrictions.max_width_scale,
		fOverlayRestrictions.max_height_scale);
}


void
VideoView::OverlayScreenshotPrepare()
{
	// TODO: Do nothing if the current bitmap is in RGB color space
	// and no overlay. Otherwise, convert current bitmap to RGB color
	// space an draw it in place of the normal display.
}


void
VideoView::OverlayScreenshotCleanup()
{
	// TODO: Do nothing if the current bitmap is in RGB color space
	// and no overlay. Otherwise clean view area with overlay color.
}


bool
VideoView::IsOverlayActive()
{
	bool active = false;
	if (LockBitmap()) {
		active = fOverlayMode;
		UnlockBitmap();
	}
	return active;
}


void
VideoView::DisableOverlay()
{
	if (!fOverlayMode)
		return;

	FillRect(Bounds());
	Sync();

	ClearViewOverlay();
	snooze(20000);
	Sync();
	fOverlayMode = false;
}

