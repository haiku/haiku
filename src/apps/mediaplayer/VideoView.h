/*
 * Copyright © 2006-2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef VIDEO_VIEW_H
#define VIDEO_VIEW_H


#include <View.h>

#include "ListenerAdapter.h"
#include "VideoTarget.h"


class VideoView : public BView, public VideoTarget {
public:
								VideoView(BRect frame, const char* name,
									uint32 resizeMask);
	virtual						~VideoView();

	// BView interface
	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);

	// VideoTarget interface
	virtual	void				SetBitmap(const BBitmap* bitmap);

	// VideoView
			void				GetOverlayScaleLimits(float* minScale,
									float* maxScale) const;

			void				OverlayScreenshotPrepare();
			void				OverlayScreenshotCleanup();

			bool				UseOverlays() const;
			bool				IsOverlayActive();
			void				DisableOverlay();

			void				SetPlaying(bool playing);

private:
			void				_DrawBitmap(const BBitmap* bitmap);
			void				_AdoptGlobalSettings();

			bool				fOverlayMode;
			overlay_restrictions fOverlayRestrictions;
			rgb_color			fOverlayKeyColor;
			bool				fIsPlaying;

			ListenerAdapter		fGlobalSettingsListener;
			bool				fUseOverlays;
			bool				fUseBilinearScaling;
};

#endif // VIDEO_VIEW_H
