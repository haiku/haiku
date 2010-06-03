/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mikael Konradson, mikael.konradson@gmail.com
 */
#ifndef CONTROL_VIEW_H
#define CONTROL_VIEW_H


#include <View.h>


class BButton;
class BCheckBox;
class BHandler;
class BLooper;
class BMenu;
class BMenuField;
class BMessageRunner;
class BMessenger;
class BSlider;
class BTextControl;


class ControlView : public BView {
	public:
		ControlView(BRect rect);
		virtual ~ControlView();

		virtual void AttachedToWindow();
		virtual void Draw(BRect updateRect);
		virtual void MessageReceived(BMessage* message);
		void SetTarget(BHandler* handler);

	private:
		void _AddFontMenu(BRect rect);
		void _UpdateFontmenus(bool setInitialfont = false);
		void _DeselectOldItems();

		void _UpdateAndSendFamily(const BMessage* message);
		void _UpdateAndSendStyle(const BMessage* message);

		BMessenger*		fMessenger;
		BMessageRunner*	fMessageRunner;
		BTextControl*	fTextControl;
		BMenuField*		fFontMenuField;
		BSlider*		fFontsizeSlider;
		BSlider*		fShearSlider;
		BSlider*		fRotationSlider;
		BSlider*		fSpacingSlider;
		BSlider*		fOutlineSlider;
		BCheckBox*		fAliasingCheckBox;
		BCheckBox*		fBoundingboxesCheckBox;
		BButton*		fCyclingFontButton;
		BMenu*			fFontFamilyMenu;
		BMenu*			fDrawingModeMenu;
		bool 			fCycleFonts;
		int32 			fFontStyleindex;
};

#endif	// CONTROL_VIEW_H

