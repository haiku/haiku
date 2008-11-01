#ifndef TouchpadPrefView_h
#define TouchpadPrefView_h

#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <Invoker.h>
#include <Slider.h>
#include <View.h>

#include "TouchpadPref.h"
#include "touchpad_settings.h"

#define DEBUG 1
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


// draw a touchpad
class TouchpadView : public BView, public BInvoker
{
	public:
						TouchpadView(BRect frame);
						~TouchpadView();
		virtual void 	Draw(BRect updateRect);
		virtual void	MouseDown(BPoint point);
		virtual void	MouseUp(BPoint point);
		virtual void	MouseMoved(BPoint point, uint32 transit, const BMessage *message);
		
		virtual void	AttachedToWindow();
		virtual void	GetPreferredSize(float *width, float *height);
		
		void			SetValues(float rightRange, float bottomRange);
		float			GetRightScrollRatio(){ return (1 - fXScrollRange / fPadRect.Width()); };
		float			GetBottomScrollRatio(){ return (1 - fYScrollRange / fPadRect.Height()); };
	private:
	
		virtual void 	DrawSliders();
				BRect	fPrefRect;
				BRect	fPadRect;
				BRect	fXScrollDragZone;
				BRect	fXScrollDragZone1;
				BRect	fXScrollDragZone2;
				float	fXScrollRange;
				float	fOldXScrollRange;
				BRect	fYScrollDragZone;
				BRect	fYScrollDragZone1;
				BRect	fYScrollDragZone2;
				float	fYScrollRange;
				float	fOldYScrollRange;
				
				bool	fXTracking;
				bool	fYTracking;
				BView	*fOffScreenView;
				BBitmap *fOffScreenBitmap;
};


class TouchpadPrefView : public BView
{
	public:
							TouchpadPrefView(BRect frame, const char *name);
							~TouchpadPrefView();
		virtual	void		MessageReceived(BMessage *msg);
		virtual void		AttachedToWindow();
		virtual void		DetachedFromWindow(void);
				void		SetupView();
				
				void		SetValues(touchpad_settings *settings);
				
	private:
		// visual the touchpad
		TouchpadView*		fTouchpadView;
		BCheckBox*			fTwoFingerBox;
		BCheckBox*			fMultiFingerBox;
		BSlider*			fScrollStepXSlider;
		BSlider*			fScrollStepYSlider;
		BSlider*			fScrollAccelSlider;
		BSlider*			fTapSlider;
		BButton*			fDefaultButton;
		BButton*			fRevertButton;
		
		TouchpadPref		fTouchpadPref;
};


#endif