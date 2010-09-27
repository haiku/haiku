/*
 * Copyright 2006-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "VideoView.h"

#include <stdio.h>

#include <Bitmap.h>

#include <Application.h>
#include <Region.h>
#include <WindowScreen.h>

#include "Settings.h"
#include "SubtitleBitmap.h"


VideoView::VideoView(BRect frame, const char* name, uint32 resizeMask)
	:
	BView(frame, name, resizeMask, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE
		| B_PULSE_NEEDED),
	fVideoFrame(Bounds()),
	fOverlayMode(false),
	fIsPlaying(false),
	fIsFullscreen(false),
	fLastMouseMove(system_time()),
	fGlobalSettingsListener(this),
	fSubtitleBitmap(new SubtitleBitmap),
	fHasSubtitle(false)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetHighColor(0, 0, 0);

	// create some hopefully sensible default overlay restrictions
	// they will be adjusted when overlays are actually used
	fOverlayRestrictions.min_width_scale = 0.25;
	fOverlayRestrictions.max_width_scale = 8.0;
	fOverlayRestrictions.min_height_scale = 0.25;
	fOverlayRestrictions.max_height_scale = 8.0;

	Settings::Default()->AddListener(&fGlobalSettingsListener);
	_AdoptGlobalSettings();

//SetSubtitle("<b><i>This</i></b> is a <font color=\"#00ff00\">test</font>!"
//	"\nWith a <i>short</i> line and a <b>long</b> line.");
}


VideoView::~VideoView()
{
	Settings::Default()->RemoveListener(&fGlobalSettingsListener);
	delete fSubtitleBitmap;
}


void
VideoView::Draw(BRect updateRect)
{
	BRegion outSideVideoRegion(updateRect);

	if (LockBitmap()) {
		if (const BBitmap* bitmap = GetBitmap()) {
			outSideVideoRegion.Exclude(fVideoFrame);
			if (!fOverlayMode)
				_DrawBitmap(bitmap);
			else
				FillRect(fVideoFrame & updateRect, B_SOLID_LOW);
		}
		UnlockBitmap();
	}

	if (outSideVideoRegion.CountRects() > 0)
		FillRegion(&outSideVideoRegion);
}


void
VideoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_OBJECT_CHANGED:
			// received from fGlobalSettingsListener
			// TODO: find out which object, if we ever watch more than
			// the global settings instance...
			_AdoptGlobalSettings();
			break;
		default:
			BView::MessageReceived(message);
	}
}


void
VideoView::Pulse()
{
	if (!fIsFullscreen || !fIsPlaying)
		return;

	bigtime_t now = system_time();
	if (now - fLastMouseMove > 1500000) {
		fLastMouseMove = now;
		// take care of disabling the screen saver
		BPoint where;
		uint32 buttons;
		GetMouse(&where, &buttons, false);
		if (buttons == 0) {
			BMessage message(M_HIDE_FULL_SCREEN_CONTROLS);
			message.AddPoint("where", where);
			Window()->PostMessage(&message, Window());

			ConvertToScreen(&where);
			set_mouse_position((int32)where.x, (int32)where.y);
		}
	}
}


void
VideoView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	fLastMouseMove = system_time();
}


// #pragma mark -


void
VideoView::SetBitmap(const BBitmap* bitmap)
{
	VideoTarget::SetBitmap(bitmap);
	// Attention: Don't lock the window, if the bitmap is NULL. Otherwise
	// we're going to deadlock when the window tells the node manager to
	// stop the nodes (Window -> NodeManager -> VideoConsumer -> VideoView
	// -> Window).
	if (!bitmap || LockLooperWithTimeout(10000) != B_OK)
		return;

	if (LockBitmap()) {
		if (fOverlayMode
			|| (bitmap->Flags() & B_BITMAP_WILL_OVERLAY) != 0) {
			if (!fOverlayMode) {
				// init overlay
				rgb_color key;
				status_t ret = SetViewOverlay(bitmap, bitmap->Bounds(),
					fVideoFrame, &key, B_FOLLOW_ALL,
					B_OVERLAY_FILTER_HORIZONTAL
					| B_OVERLAY_FILTER_VERTICAL);
				if (ret == B_OK) {
					fOverlayKeyColor = key;
					SetLowColor(key);
					snooze(20000);
					FillRect(fVideoFrame, B_SOLID_LOW);
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
					FillRect(fVideoFrame);
					Sync();
				}
			} else {
				// transfer overlay channel
				rgb_color key;
				SetViewOverlay(bitmap, bitmap->Bounds(), fVideoFrame,
					&key, B_FOLLOW_ALL, B_OVERLAY_FILTER_HORIZONTAL
						| B_OVERLAY_FILTER_VERTICAL
						| B_OVERLAY_TRANSFER_CHANNEL);
			}
		} else if (fOverlayMode
			&& (bitmap->Flags() & B_BITMAP_WILL_OVERLAY) == 0) {
			fOverlayMode = false;
			ClearViewOverlay();
			SetViewColor(B_TRANSPARENT_COLOR);
		}
		if (!fOverlayMode) {
			if (fHasSubtitle)
				Invalidate(fVideoFrame);
			else
				_DrawBitmap(bitmap);
		}

		UnlockBitmap();
	}
	UnlockLooper();
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
VideoView::UseOverlays() const
{
	return fUseOverlays;
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


void
VideoView::SetPlaying(bool playing)
{
	fIsPlaying = playing;
}


void
VideoView::SetFullscreen(bool fullScreen)
{
	fIsFullscreen = fullScreen;
}


void
VideoView::SetVideoFrame(const BRect& frame)
{
	if (fVideoFrame == frame)
		return;

	BRegion invalid(fVideoFrame | frame);
	invalid.Exclude(frame);
	Invalidate(&invalid);

	fVideoFrame = frame;

	fSubtitleBitmap->SetVideoBounds(fVideoFrame.OffsetToCopy(B_ORIGIN));
}


void
VideoView::SetSubTitle(const char* text)
{
	if (text == NULL || text[0] == '\0') {
		fHasSubtitle = false;
	} else {
		fHasSubtitle = true;
		fSubtitleBitmap->SetText(text);
	}
	// TODO: Make smarter and invalidate only previous subtitle bitmap
	// region;
	if (!fIsPlaying && LockLooper()) {
		Invalidate();
		UnlockLooper();
	}
}


// #pragma mark -


void
VideoView::_DrawBitmap(const BBitmap* bitmap)
{
	SetDrawingMode(B_OP_COPY);
	uint32 options = fUseBilinearScaling ? B_FILTER_BITMAP_BILINEAR : 0;
	DrawBitmap(bitmap, bitmap->Bounds(), fVideoFrame, options);

	if (fHasSubtitle) {
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	
		const BBitmap* subtitleBitmap = fSubtitleBitmap->Bitmap();
		BPoint offset;
		offset.x = (fVideoFrame.left + fVideoFrame.right
			- subtitleBitmap->Bounds().Width()) / 2;
		offset.y = fVideoFrame.bottom - subtitleBitmap->Bounds().Height();
		DrawBitmap(subtitleBitmap, offset);
	}
}


void
VideoView::_AdoptGlobalSettings()
{
	mpSettings settings = Settings::CurrentSettings();
		// thread safe

	fUseOverlays = settings.useOverlays;
	fUseBilinearScaling = settings.scaleBilinear;

	if (!fIsPlaying && !fOverlayMode)
		Invalidate();
}

