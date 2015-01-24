/*
 *  Copyright 2010-2015 Haiku, Inc. All rights reserved.
 *  Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		Ryan Leavengood, leavengood@gmail.com
 *		John Scipione, jscipione@gmail.com
 */
#ifndef LOOK_AND_FEEL_SETTINGS_VIEW_H
#define LOOK_AND_FEEL_SETTINGS_VIEW_H


#include <DecorInfo.h>
#include <String.h>
#include <View.h>


class BButton;
class BCheckBox;
class BMenuField;
class BPopUpMenu;
class FakeScrollBar;

class LookAndFeelSettingsView : public BView {
public:
								LookAndFeelSettingsView(const char* name);
	virtual						~LookAndFeelSettingsView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			bool				IsDefaultable();
			void				SetDefaults();

			bool				IsRevertable();
			void				Revert();

private:
			void				_SetDecor(const BString& name);
			void				_SetDecor(BPrivate::DecorInfo* decorInfo);

			void				_BuildDecorMenu();
			void				_AdoptToCurrentDecor();
			void				_AdoptInterfaceToCurrentDecor();

			bool				_DoubleScrollBarArrows();
			void				_SetDoubleScrollBarArrows(bool doubleArrows);

			int32				_ScrollBarKnobStyle();
			void				_SetScrollBarKnobStyle(int32 knobStyle);

private:
			DecorInfoUtility	fDecorUtility;

			BButton*			fDecorInfoButton;
			BMenuField*			fDecorMenuField;
			BPopUpMenu*			fDecorMenu;
			BString				fSavedDecor;
			BString				fCurrentDecor;

			FakeScrollBar*		fArrowStyleSingle;
			FakeScrollBar*		fArrowStyleDouble;

			FakeScrollBar*		fKnobStyleNone;
			FakeScrollBar*		fKnobStyleDots;
			FakeScrollBar*		fKnobStyleLines;

			int32				fSavedKnobStyleValue;
			bool				fSavedDoubleArrowsValue;
};


#endif // LOOK_AND_FEEL_SETTINGS_VIEW_H
