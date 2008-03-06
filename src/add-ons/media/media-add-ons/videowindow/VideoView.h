/*
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef __VIDEO_VIEW_H
#define __VIDEO_VIEW_H


#include <View.h>


class BMediaAddOn;
class VideoNode;


class VideoView : public BView
{
public:
					VideoView(BRect frame, const char *name, uint32 resizeMask, uint32 flags, VideoNode *node);
					~VideoView();

	void			RemoveVideoDisplay();
	void			RemoveOverlay();

	VideoNode *		Node();

	bool			IsOverlaySupported();

	void			OverlayLockAcquire();
	void			OverlayLockRelease();

	void			OverlayScreenshotPrepare();
	void			OverlayScreenshotCleanup();

	void			DrawFrame();

private:
	void			AttachedToWindow();
	void			MessageReceived(BMessage *msg);
	void			Draw(BRect updateRect);

private:
	VideoNode *		fVideoNode;
	bool			fOverlayActive;
	rgb_color		fOverlayKeyColor;
};

#endif
