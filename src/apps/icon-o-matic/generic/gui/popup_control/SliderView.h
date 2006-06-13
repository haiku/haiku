/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#ifndef SLIDER_VIEW_H
#define SLIDER_VIEW_H

#include <String.h>

#include "PopupView.h"

class BFont;
class PopupSlider;

class SliderView : public PopupView {
 public:
								SliderView(PopupSlider* target,
										   int32 min,
										   int32 max,
										   int32 value,
										   const char* formatString);
	virtual						~SliderView();

								// MView
	virtual	minimax				layoutprefs();
	virtual	BRect				layout(BRect frame);

								// BView
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* message);
	virtual	void				MouseUp(BPoint where);

								// BHandler
	virtual	void				MessageReceived(BMessage* message);

								// SliderView
			void				SetValue(int32 value);
			int32				Value() const;
			void				SetMin(int32 min);
			int32				Min() const;
			void				SetMax(int32 max);
			int32				Max() const;

			void				SetFormatString(const char* formatString);
			const char*			FormatString() const;

			void				SetDragOffset(float offset);

			float				ButtonOffset();

	static	void				GetSliderButtonDimensions(int32 max,
														  const char* formatString,
														  BFont* font,
														  float& width,
														  float& height);
	static	void				DrawSliderButton(BView* into, BRect frame,
												 int32 value,
												 const char* formatString,
												 bool enabled);


 private:
			int32				_ValueAt(float h);

			PopupSlider*		fTarget;
			BString				fFormatString;

			int32				fMin;
			int32				fMax;
			int32				fValue;

			BRect				fButtonRect;
			float				fDragOffset;
};


#endif	// SLIDER_VIEW_H
