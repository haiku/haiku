/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */

#ifndef COLORSTEPVIEW_H
#define COLORSTEPVIEW_H

#include <Bitmap.h>
#include <Control.h>
#include <ObjectList.h>
#include <Slider.h>
#include <String.h>

#include "DriverInterface.h"

const uint32 kSteppingChanged = '&spc';

struct performance_step {
	float		cpu_usage;	// upper limit
	rgb_color	color;
};


typedef BObjectList<performance_step> PerformanceList;


class ColorStepView : public BControl
{
	public:
							ColorStepView(BRect frame);
							~ColorStepView();
		virtual void		AttachedToWindow();
		virtual void		DetachedFromWindow();
		virtual void		FrameResized(float w,float h);
		
		virtual void		GetPreferredSize(float *width, float *height);
		virtual void		Draw(BRect updateRect);
		virtual void		MessageReceived(BMessage *message);
		virtual void		SetEnabled(bool enabled);
		
		void				SetFrequencys(StateList *list);
		PerformanceList*	GetPerformanceSteps();
		static BString		CreateFrequencyString(uint16 frequency);
		
		void				SetSliderPosition(float pos) {
								fSlider->SetPosition(pos);
								fSliderPosition = pos;
								_CalculatePerformanceSteps();
							}
		float				GetSliderPosition() {	return fSliderPosition;	}
		
		static float		UsageOfStep(int32 step, int32 nSteps, float base);
		
	private:
		void				_InitView();
		void				_CalculatePerformanceSteps();
		float				_PositonStep(int32 step);
		void				_ColorStep(int32 step, rgb_color &color);
		
		BSlider*			fSlider;
		
		float				fSliderPosition;
		int32				fNSteps;
		rgb_color			fLowFreqColor;
		rgb_color			fHighFreqColor;
		
		BView*				fOffScreenView;
		BBitmap*			fOffScreenBitmap;
		
		BString				fMinFrequencyLabel;
		BString				fMaxFrequencyLabel;
		
		PerformanceList*	fPerformanceList;
		StateList*			fStateList;
};


#endif
