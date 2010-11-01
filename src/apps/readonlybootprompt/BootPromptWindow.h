/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BOOT_PROMPT_WINDOW_H
#define BOOT_PROMPT_WINDOW_H

#include <Catalog.h>
#include <Message.h>
#include <Window.h>

class BButton;
class BListView;
class BStringView;
class BTextView;


class BootPromptWindow : public BWindow {
public:
								BootPromptWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_InitCatalog(bool saveSettings);
			void				_UpdateStrings();
			void				_PopulateLanguages();
			void				_PopulateKeymaps();
			void				_StoreKeymap() const;
			status_t			_GetCurrentKeymapRef(entry_ref& ref) const;

private:
			BView*				fInfoTextView;
			BStringView*		fLanguagesLabelView;
			BListView*			fLanguagesListView;
			BStringView*		fKeymapsLabelView;
			BListView*			fKeymapsListView;
			BButton*			fDesktopButton;
			BButton*			fInstallerButton;

			BMessage			fInstalledLanguages;
};

#endif // BOOT_PROMPT_WINDOW_H
