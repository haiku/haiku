/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef TOUCHPAD_PREF_VIEW_H
#define TOUCHPAD_PREF_VIEW_H

#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <GroupView.h>
#include <Invoker.h>
#include <StringView.h>
#include <Slider.h>
#include <View.h>

#include "InputTouchpadPref.h"
#include "touchpad_settings.h"

#include <Debug.h>

#if DEBUG
#	define LOG(text...) PRINT((text))
#else
#	define LOG(text...)
#endif

const uint SCROLL_AREA_CHANGED = '&sac';
const uint SCROLL_CONTROL_CHANGED = '&scc';
const uint TAP_CONTROL_CHANGED = '&tcc';
const uint DEFAULT_SETTINGS = '&dse';
const uint REVERT_SETTINGS = '&rse';
const uint PADBLOCK_TIME_CHANGED = '&ptc';

class DeviceListView;


//! Shows a touchpad
class TouchpadView : public BView, public BInvoker {
public:
							TouchpadView(BRect frame);
	virtual					~TouchpadView();
	virtual void			Draw(BRect updateRect);
	virtual void			MouseDown(BPoint point);
	virtual void			MouseUp(BPoint point);
	virtual void			MouseMoved(BPoint point, uint32 transit,
								const BMessage* dragMessage);

	virtual void			AttachedToWindow();
	virtual void			GetPreferredSize(float* width, float* height);

			void			SetValues(float rightRange, float bottomRange);
			float			GetRightScrollRatio()
								{ return 1 - fXScrollRange / fPadRect.Width(); }
			float			GetBottomScrollRatio()
								{ return 1
									- fYScrollRange / fPadRect.Height(); }
private:
	virtual void 			DrawSliders();

			BRect			fPrefRect;
			BRect			fPadRect;
			BRect			fXScrollDragZone;
			float			fXScrollRange;
			float			fOldXScrollRange;
			BRect			fYScrollDragZone;
			float			fYScrollRange;
			float			fOldYScrollRange;

			bool			fXTracking;
			bool			fYTracking;
			BView*			fOffScreenView;
			BBitmap*		fOffScreenBitmap;
};


class TouchpadPrefView : public BGroupView {
public:
							TouchpadPrefView(BInputDevice* dev);
	virtual					~TouchpadPrefView();
	virtual	void			MessageReceived(BMessage* message);
	virtual	void			AttachedToWindow();
	virtual	void			DetachedFromWindow();
			void			SetupView();

			void			SetValues(touchpad_settings *settings);
private:
			TouchpadPref	fTouchpadPref;
			TouchpadView*	fTouchpadView;
			BCheckBox*		fTwoFingerBox;
			BCheckBox*		fTwoFingerHorizontalBox;
			BSlider*		fScrollStepXSlider;
			BSlider*		fScrollStepYSlider;
			BSlider*		fScrollAccelSlider;
			BSlider*		fPadBlockerSlider;
			BSlider*		fTapSlider;
			BButton*		fDefaultButton;
			BButton*		fRevertButton;
};

#endif	// TOUCHPAD_PREF_VIEW_H
