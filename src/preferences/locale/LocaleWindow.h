/*
 * Copyright 2005-2010, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LOCALE_WINDOW_H
#define LOCALE_WINDOW_H


#include <Message.h>
#include <Window.h>


static const uint32 kMsgRevert = 'revt';


class BButton;
class BCheckBox;
class BListView;
class FormatSettingsView;
class LanguageListItem;
class LanguageListView;


class LocaleWindow : public BWindow {
public:
								LocaleWindow();
	virtual						~LocaleWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
	virtual void				Show();

private:
			void				_SettingsChanged();
			void				_SettingsReverted();

			bool				_IsReversible() const;

			void				_Refresh(bool setInitial = false);
			void				_Revert();

			void				_SetPreferredLanguages(
									const BMessage& languages);
			void				_PreferredLanguagesChanged();
			void				_EnableDisableLanguages();
			void				_InsertPreferredLanguage(LanguageListItem* item,
									int32 atIndex = -1);
			void				_Defaults();

			BButton*			fRevertButton;
			LanguageListView*	fLanguageListView;
			LanguageListView*	fPreferredListView;
			LanguageListView*	fConventionsListView;
			FormatSettingsView*	fFormatView;
			LanguageListItem*	fInitialConventionsItem;
			LanguageListItem*	fDefaultConventionsItem;
			BMessage			fInitialPreferredLanguages;
};


#endif	// LOCALE_WINDOW_H

