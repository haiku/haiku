/*
 * Copyright (C) 1998-1999 Be Incorporated. All rights reseved.
 * Distributed under the terms of the Be Sample Code license.
 *
 * Copyright (C) 2001-2008 Stephan Aßmus. All rights reserved.
 * Distributed under the terms of the MIT license.
 */

/*----------------------------------------------------------
PURPOSE:
gui class for displaying stereo audio peak level info

FEATURES:
- uses a bitmap, but not a view accepting one, to redraw
without flickering.
- can be configured to use it's own message runner instead
of the windows current pulse (in case you have text views in
the window as well, which use a slow pulse for cursor blinking)
- if used with own message runner, refresh delay is configurable
- can be used in a dynamic liblayout gui

USAGE:
To display the peak level of your streaming audio, just
calculate the local maximum of both channels within your
audio buffer and call SetMax() once for every buffer. The
PeakView will take care of the rest.
min = 0.0
max = 1.0
----------------------------------------------------------*/
#ifndef PEAK_VIEW_H
#define PEAL_VIEW_H


#include <View.h>

class BBitmap;
class BMessageRunner;

class PeakView : public BView {
public:
								PeakView(const char* name,
									bool useGlobalPulse = true,
									bool displayLabels = true);
	virtual						~PeakView();

	// BView interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				MouseDown(BPoint where);
	virtual	void				Draw(BRect area);
	virtual	void				FrameResized(float width, float height);
	virtual	void				Pulse();
	virtual	BSize				MinSize();

	// PeakView
			bool				IsValid() const;
			void				SetPeakRefreshDelay(bigtime_t delay);
			void				SetPeakNotificationWhat(uint32 what);

			void				SetChannelCount(uint32 count);
			void				SetMax(float max, uint32 channel);

private:
			BRect				_BackBitmapFrame() const;
			void				_ResizeBackBitmap(int32 width, int32 channels);
			void				_UpdateBackBitmap();
			void				_RenderSpan(uint8* span, uint32 width,
									float current, float peak, bool overshot);
			void				_DrawBitmap();

private:
			bool				fUseGlobalPulse;
			bool				fDisplayLabels;
			bool				fPeakLocked;

			bigtime_t			fRefreshDelay;
			BMessageRunner*		fPulse;

			struct ChannelInfo {
				float			current_max;
				float			last_max;
				bigtime_t		last_overshot_time;
			};

			ChannelInfo*		fChannelInfos;
			uint32				fChannelCount;
			bool				fGotData;

			BBitmap*			fBackBitmap;
			font_height			fFontHeight;

			uint32				fPeakNotificationWhat;
};

#endif // PEAK_VIEW_H
