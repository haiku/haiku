/*
 * Copyright 2006-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "VideoView.h"

#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <Region.h>
#include <Screen.h>
#include <WindowScreen.h>

#include "Settings.h"
#include "SubtitleBitmap.h"


enum {
	MSG_INVALIDATE = 'ivdt'
};


VideoView::VideoView(BRect frame, const char* name, uint32 resizeMask)
	:
	BView(frame, name, resizeMask, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE
		| B_PULSE_NEEDED),
	fVideoFrame(Bounds()),
	fOverlayMode(false),
	fIsPlaying(false),
	fIsFullscreen(false),
	fLastMouseMove(system_time()),

	fSubtitleBitmap(new SubtitleBitmap),
	fSubtitleFrame(),
	fSubtitleMaxButtom(Bounds().bottom),
	fHasSubtitle(false),
	fSubtitleChanged(false),

	fGlobalSettingsListener(this)
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

//SetSubTitle("<b><i>This</i></b> is a <font color=\"#00ff00\">test</font>!"
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

	if (fHasSubtitle)
		_DrawSubtitle();
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
		case MSG_INVALIDATE:
		{
			BRect dirty;
			if (message->FindRect("dirty", &dirty) == B_OK)
				Invalidate(dirty);
			break;
		}
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
					B_OVERLAY_FILTER_HORIZONTAL | B_OVERLAY_FILTER_VERTICAL);
				if (ret == B_OK) {
					fOverlayKeyColor = key;
					SetLowColor(key);
					snooze(20000);
					FillRect(fVideoFrame, B_SOLID_LOW);
					Sync();
					// use overlay from here on
					_SetOverlayMode(true);

					// update restrictions
					overlay_restrictions restrictions;
					if (bitmap->GetOverlayRestrictions(&restrictions) == B_OK)
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
			_SetOverlayMode(false);
			ClearViewOverlay();
			SetViewColor(B_TRANSPARENT_COLOR);
		}
		if (!fOverlayMode) {
			if (fSubtitleChanged) {
				_LayoutSubtitle();
				Invalidate(fVideoFrame | fSubtitleFrame);
			} else if (fHasSubtitle
				&& fVideoFrame.Intersects(fSubtitleFrame)) {
				Invalidate(fVideoFrame);
			} else
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
	_SetOverlayMode(false);
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
	_LayoutSubtitle();
}


void
VideoView::SetSubTitle(const char* text)
{
	BRect oldSubtitleFrame = fSubtitleFrame;

	if (text == NULL || text[0] == '\0') {
		fHasSubtitle = false;
		fSubtitleChanged = true;
	} else {
		fHasSubtitle = true;
		// If the subtitle frame still needs to be invalidated during
		// normal playback, make sure we don't unset the fSubtitleChanged
		// flag. It will be reset after drawing the subtitle once.
		fSubtitleChanged = fSubtitleBitmap->SetText(text) || fSubtitleChanged;
		if (fSubtitleChanged)
			_LayoutSubtitle();
	}

	if (!fIsPlaying && Window() != NULL) {
		// If we are playing, the new subtitle will be displayed,
		// or the old one removed from screen, as soon as the next
		// frame is shown. Otherwise we need to invalidate manually.
		// But we are not in the window thread and we shall not lock
		// it or we may dead-locks.
		BMessage message(MSG_INVALIDATE);
		message.AddRect("dirty", oldSubtitleFrame | fSubtitleFrame);
		Window()->PostMessage(&message);
	}
}


void
VideoView::SetSubTitleMaxBottom(float bottom)
{
	if (bottom == fSubtitleMaxButtom)
		return;

	fSubtitleMaxButtom = bottom;

	BRect oldSubtitleFrame = fSubtitleFrame;
	_LayoutSubtitle();
	Invalidate(fSubtitleFrame | oldSubtitleFrame);
}


// #pragma mark -


void
VideoView::_DrawBitmap(const BBitmap* bitmap)
{
	SetDrawingMode(B_OP_COPY);
	uint32 options = B_WAIT_FOR_RETRACE;
	if (fUseBilinearScaling)
		options |= B_FILTER_BITMAP_BILINEAR;

	DrawBitmapAsync(bitmap, bitmap->Bounds(), fVideoFrame, options);
}


void
VideoView::_DrawSubtitle()
{
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	DrawBitmapAsync(fSubtitleBitmap->Bitmap(), fSubtitleFrame.LeftTop());

	// Unless the subtitle frame intersects the video frame, we don't have
	// to draw the subtitle again.
	fSubtitleChanged = false;
}


void
VideoView::_AdoptGlobalSettings()
{
	mpSettings settings;
	Settings::Default()->Get(settings);

	fUseOverlays = settings.useOverlays;
	fUseBilinearScaling = settings.scaleBilinear;

	switch (settings.subtitleSize) {
		case mpSettings::SUBTITLE_SIZE_SMALL:
			fSubtitleBitmap->SetCharsPerLine(45.0);
			break;
		case mpSettings::SUBTITLE_SIZE_MEDIUM:
			fSubtitleBitmap->SetCharsPerLine(36.0);
			break;
		case mpSettings::SUBTITLE_SIZE_LARGE:
			fSubtitleBitmap->SetCharsPerLine(32.0);
			break;
	}

	fSubtitlePlacement = settings.subtitlePlacement;

	_LayoutSubtitle();
	Invalidate();
}


void
VideoView::_SetOverlayMode(bool overlayMode)
{
	fOverlayMode = overlayMode;
	fSubtitleBitmap->SetOverlayMode(overlayMode);
}


void
VideoView::_LayoutSubtitle()
{
	if (!fHasSubtitle)
		return;

	const BBitmap* subtitleBitmap = fSubtitleBitmap->Bitmap();
	if (subtitleBitmap == NULL)
		return;

	fSubtitleFrame = subtitleBitmap->Bounds();

	BPoint offset;
	offset.x = (fVideoFrame.left + fVideoFrame.right
		- fSubtitleFrame.Width()) / 2;
	switch (fSubtitlePlacement) {
		default:
		case mpSettings::SUBTITLE_PLACEMENT_BOTTOM_OF_VIDEO:
			offset.y = min_c(fSubtitleMaxButtom, fVideoFrame.bottom)
				- fSubtitleFrame.Height();
			break;
		case mpSettings::SUBTITLE_PLACEMENT_BOTTOM_OF_SCREEN:
		{
			// Center between video and screen bottom, if there is still
			// enough room.
			float centeredOffset = (fVideoFrame.bottom + fSubtitleMaxButtom
				- fSubtitleFrame.Height()) / 2;
			float maxOffset = fSubtitleMaxButtom - fSubtitleFrame.Height();
			offset.y = min_c(centeredOffset, maxOffset);
			break;
		}
	}

	fSubtitleFrame.OffsetTo(offset);
}


