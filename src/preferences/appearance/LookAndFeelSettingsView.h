/*
 *  Copyright 2010-2012 Haiku, Inc. All rights reserved.
 *  Distributed under the terms of the MIT license.
 *
 *  Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ryan Leavengood <leavengood@gmail.com>
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


class LookAndFeelSettingsView : public BView {
public:
								LookAndFeelSettingsView(const char* name);
	virtual						~LookAndFeelSettingsView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			void				SetDefaults();
			void				Revert();
			bool				IsDefaultable();
			bool				IsRevertable();

private:
			void				_SetDecor(const BString& name);
			void				_SetDecor(BPrivate::DecorInfo* decorInfo);

			void				_BuildDecorMenu();
			void				_AdoptToCurrentDecor();
			void				_AdoptInterfaceToCurrentDecor();
			bool				_GetDoubleScrollbarArrowsSetting();
			void				_SetDoubleScrollbarArrowsSetting(bool value);

private:
			DecorInfoUtility	fDecorUtility;

			BButton*			fDecorInfoButton;
			BMenuField*			fDecorMenuField;
			BPopUpMenu*			fDecorMenu;
			BCheckBox*			fDoubleScrollbarArrowsCheckBox;

			BString				fSavedDecor;
			BString				fCurrentDecor;
			bool				fSavedDoubleArrowsValue;
};

#endif // LOOK_AND_FEEL_SETTINGS_VIEW_H
