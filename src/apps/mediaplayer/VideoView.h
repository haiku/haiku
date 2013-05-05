/*
 * Copyright 2006-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef VIDEO_VIEW_H
#define VIDEO_VIEW_H


#include <View.h>

#include "ListenerAdapter.h"
#include "VideoTarget.h"


enum {
	M_HIDE_FULL_SCREEN_CONTROLS = 'hfsc'
};


class SubtitleBitmap;


class VideoView : public BView, public VideoTarget {
public:
								VideoView(BRect frame, const char* name,
									uint32 resizeMask);
	virtual						~VideoView();

	// BView interface
	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Pulse();
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage = NULL);

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
			void				SetFullscreen(bool fullScreen);
			void				SetFullscreenControlsVisible(bool visible);
			void				SetVideoFrame(const BRect& frame);

			void				SetSubTitle(const char* text);
			void				SetSubTitleMaxBottom(float bottom);

private:
			void				_DrawBitmap(const BBitmap* bitmap);
			void				_DrawSubtitle();
			void				_AdoptGlobalSettings();
			void				_SetOverlayMode(bool overlayMode);
			void				_LayoutSubtitle();

private:
			BRect				fVideoFrame;
			bool				fOverlayMode;
			overlay_restrictions fOverlayRestrictions;
			rgb_color			fOverlayKeyColor;
			bool				fIsPlaying;
			bool				fIsFullscreen;
			bool				fFullscreenControlsVisible;
			bool				fFirstPulseAfterFullscreen;
			uint8				fSendHideCounter;
			bigtime_t			fLastMouseMove;

			SubtitleBitmap*		fSubtitleBitmap;
			BRect				fSubtitleFrame;
			float				fSubtitleMaxButtom;
			bool				fHasSubtitle;
			bool				fSubtitleChanged;

			// Settings values:
			ListenerAdapter		fGlobalSettingsListener;
			bool				fUseOverlays;
			bool				fUseBilinearScaling;
			uint32				fSubtitlePlacement;
};

#endif // VIDEO_VIEW_H
