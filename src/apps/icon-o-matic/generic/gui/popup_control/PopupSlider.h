/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#ifndef POPUP_SLIDER_H
#define POPUP_SLIDER_H

#include <String.h>

#include "MDividable.h"

#include "PopupControl.h"

class MDividable;
class SliderView;

class PopupSlider : public PopupControl,
					public MDividable {
 public:
								PopupSlider(const char* name = NULL,
											const char* label = NULL,
											BMessage* model = NULL,
											BHandler* target = NULL,
											int32 min = 0,
											int32 max = 100,
											int32 value = 0,
											const char* formatString = "%ld");
	virtual						~PopupSlider();

								// MView
	virtual	minimax				layoutprefs();
	virtual	BRect				layout(BRect frame);

								// BHandler
	virtual	void				MessageReceived(BMessage* message);

								// BView
	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

								// PopupControl
	virtual	void				PopupShown();
	virtual	void				PopupHidden(bool canceled);

								// PopupSlider
			void				SetValue(int32 value);
			int32				Value() const;
			void				SetEnabled(bool enabled);
			bool				IsEnabled() const;
			void				TriggerValueChanged(const BMessage* message) const;
			bool				IsTracking() const;
								// override this to take some action
	virtual	void				ValueChanged(int32 newValue);
	virtual	void				DrawSlider(BRect frame, bool enabled);
	virtual	float				Scale(float ratio) const;
	virtual	float				DeScale(float ratio) const;

			void				SetMessage(BMessage* message);
			const BMessage*		Message() const
									{ return fModel; }
			void				SetPressedMessage(BMessage* message);
			void				SetReleasedMessage(BMessage* message);

	virtual	void				SetMin(int32 min);
			int32				Min() const;

	virtual	void				SetMax(int32 max);
			int32				Max() const;

	virtual	void				SetLabel(const char* label);
			const char*			Label() const;

								// support for MDividable
	virtual	float				LabelWidth();

// TODO: change design to implement these features:
								// you can override this function
								// to have costum value strings
	virtual	const char*			StringForValue(int32 value);
								// but you should override this
								// as well to make sure the width
								// of the slider is calculated properly
	virtual	float				MaxValueStringWidth();

	virtual	void				SetFormatString(const char* formatString);
			const char*			FormatString() const;

 protected:
			BRect				SliderFrame() const
									{ return fSliderButtonRect; }


 private:
			float				_MinLabelWidth() const;

			SliderView*			fSlider;
			BMessage*			fModel;
			BMessage*			fPressModel;
			BMessage*			fReleaseModel;
			BHandler*			fTarget;
			BString				fLabel;
			BRect				fSliderButtonRect;
			bool				fEnabled;
			bool				fTracking;
};
/*
class PercentSlider : public PopupSlider {
 public:

	virtual	const char*			StringForValue(int32 value);
	virtual	float				MaxValueStringWidth();

};
*/

#endif	// POPUP_SLIDER_H
