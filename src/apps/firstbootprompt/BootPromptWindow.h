/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2020, Panagiotis Vasilopoulos <hello@alwayslivid.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BOOT_PROMPT_WINDOW_H
#define BOOT_PROMPT_WINDOW_H


#include <Window.h>


enum {
	MSG_LANGUAGE_SELECTED	= 'lngs',
	MSG_KEYMAP_SELECTED	= 'kmps',
};


class BButton;
class BLanguage;
class BListView;
class BMenuItem;
class BMenuField;
class BStringView;
class BTextView;


class BootPromptWindow : public BWindow {
public:
								BootPromptWindow();

			bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

private:
			void				_InitCatalog(bool saveSettings);
			void				_UpdateStrings();
			void				_UpdateKeymapsMenu();
			void				_PopulateLanguages();
			void				_PopulateKeymaps();
			void				_ActivateKeymap(const BMessage* message) const;
			status_t			_GetCurrentKeymapRef(entry_ref& ref) const;
			BMenuItem*			_KeymapItemForLanguage(
									BLanguage& language) const;

private:
			BTextView*			fInfoTextView;
			BStringView*		fLanguagesLabelView;
			BStringView*		fKeymapsMenuLabel;
			BListView*			fLanguagesListView;
			BMenuField*			fKeymapsMenuField;
			BMenuItem*			fDefaultKeymapItem;
			BButton*			fDesktopButton;
			BButton*			fInstallerButton;
};


#endif // BOOT_PROMPT_WINDOW_H
