/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun <host.haiku@gmx.de>
 */
#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H


#include <View.h>


class BCheckBox;
class BMessage;
class BPopUpMenu;
class BRadioButton;


class SettingsView : public BView {
	public:
						SettingsView(BRect frame);
		virtual 		~SettingsView();

		virtual void 	AttachedToWindow();
		virtual void	MessageReceived(BMessage *message);

	private:
		void 			_InitView();

	private:
		BCheckBox		*fShow24Hours;
		BCheckBox		*fShowSeconds;
		BRadioButton 	*fGmtTime;
		BRadioButton 	*fLocalTime;

		BCheckBox		*fYearMonthDay;
		BCheckBox		*fDayMonthYear;
		BCheckBox		*fMonthDayYear;
		BCheckBox		*fStartWeekMonday;

		BPopUpMenu 		*fDateSeparator;

		bool			fInitialized;
};

#endif	// SETTINGS_VIEW_H
