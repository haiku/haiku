/*
 * Copyright 2005-2010, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LOCALE_WINDOW_H
#define LOCALE_WINDOW_H


#include <Window.h>


static const uint32 kMsgRevert = 'revt';


class BButton;
class BListView;
class FormatView;
class LanguageListItem;
class LanguageListView;


class LocaleWindow : public BWindow {
public:
								LocaleWindow();
	virtual						~LocaleWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			void				SettingsChanged();
			void				SettingsReverted();

private:
			void				_PreferredLanguagesChanged();
			void				_EnableDisableLanguages();
			void				_UpdatePreferredFromLocaleRoster();
			void				_InsertPreferredLanguage(LanguageListItem* item,
									int32 atIndex = -1);
			void				_Defaults();

			BButton*			fRevertButton;
			LanguageListView*	fLanguageListView;
			LanguageListView*	fPreferredListView;
			FormatView*			fFormatView;
};


#endif	// LOCALE_WINDOW_H

