/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "BootPromptWindow.h"

#include <stdio.h>

#include <Button.h>
#include <ControlLook.h>
#include <Directory.h>
#include <Entry.h>
#include <Font.h>
#include <FindDirectory.h>
#include <File.h>
#include <GroupLayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <MutableLocaleRoster.h>
#include <Path.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <StringItem.h>
#include <StringView.h>
#include <TextView.h>
#include <UnicodeChar.h>

#include "BootPrompt.h"
#include "Keymap.h"
#include "KeymapListItem.h"


using BPrivate::gMutableLocaleRoster;


enum {
	MSG_LANGUAGE_SELECTED	= 'lngs',
	MSG_KEYMAP_SELECTED		= 'kmps'
};

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "BootPromptWindow"


class LanguageItem : public BStringItem {
public:
	LanguageItem(const char* label, const char* language)
		:
		BStringItem(label),
		fLanguage(language)
	{
	}

	const char* Language() const
	{
		return fLanguage.String();
	}

private:
	BString fLanguage;
};


static int
compare_void_list_items(const void* _a, const void* _b)
{
	static BCollator collator;

	LanguageItem* a = *(LanguageItem**)_a;
	LanguageItem* b = *(LanguageItem**)_b;

	return collator.Compare(a->Text(), b->Text());
}


BootPromptWindow::BootPromptWindow()
	:
	BWindow(BRect(0, 0, 450, 380), "",
		B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE
			| B_AUTO_UPDATE_SIZE_LIMITS)
{
	// Get the list of all known languages (suffice to do it only once)
	be_locale_roster->GetAvailableLanguages(&fInstalledLanguages);

	fInfoTextView = new BTextView("info", be_plain_font, NULL, B_WILL_DRAW);
	fInfoTextView->SetInsets(10, 10, 10, 10);
	fInfoTextView->MakeEditable(false);
	fInfoTextView->MakeSelectable(false);

	BScrollView* infoScrollView = new BScrollView("infoScroll",
		fInfoTextView, B_WILL_DRAW, false, true);

	fDesktopButton = new BButton("", new BMessage(MSG_BOOT_DESKTOP));
	fDesktopButton->SetTarget(be_app);
	fDesktopButton->MakeDefault(true);

	fInstallerButton = new BButton("", new BMessage(MSG_RUN_INSTALLER));
	fInstallerButton->SetTarget(be_app);

	fLanguagesLabelView = new BStringView("languagesLabel", "");
	fLanguagesLabelView->SetFont(be_bold_font);
	fLanguagesLabelView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNSET));

	fKeymapsLabelView = new BStringView("keymapsLabel", "");
	fKeymapsLabelView->SetFont(be_bold_font);
	fKeymapsLabelView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNSET));

	fLanguagesListView = new BListView();
	BScrollView* languagesScrollView = new BScrollView("languagesScroll",
		fLanguagesListView, B_WILL_DRAW, false, true);

	fKeymapsListView = new BListView();
	BScrollView* keymapsScrollView = new BScrollView("keymapsScroll",
		fKeymapsListView, B_WILL_DRAW, false, true);

	_InitCatalog(false);
	_UpdateStrings();

	float spacing = be_control_look->DefaultItemSpacing();

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(BGroupLayoutBuilder(B_VERTICAL, spacing)
			.Add(infoScrollView)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, spacing)
				.Add(BGroupLayoutBuilder(B_VERTICAL, spacing)
					.Add(fLanguagesLabelView)
					.Add(languagesScrollView)
				)
				.Add(BGroupLayoutBuilder(B_VERTICAL, spacing)
					.Add(fKeymapsLabelView)
					.Add(keymapsScrollView)
				)
			)
			.SetInsets(spacing, spacing, spacing, spacing)
		)
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, spacing)
			.AddGlue()
			.Add(fInstallerButton)
			.Add(fDesktopButton)
			.SetInsets(spacing, spacing, spacing, spacing)
		)
	);

	CenterOnScreen();
	Show();
	Lock();

	// This call in the first _PopulateLanguages/Keymaps had no effect yet,
	// since the list wasn't attached to the window yet.
	fLanguagesListView->ScrollToSelection();
	fKeymapsListView->ScrollToSelection();

	Unlock();
}


void
BootPromptWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_LANGUAGE_SELECTED:
			if (BListItem* item = fLanguagesListView->ItemAt(
				fLanguagesListView->CurrentSelection(0))) {
				LanguageItem* languageItem = static_cast<LanguageItem*>(item);
				BMessage preferredLanguages;
				preferredLanguages.AddString("language",
					languageItem->Language());
				gMutableLocaleRoster->SetPreferredLanguages(
					&preferredLanguages);
				_InitCatalog(true);
			}
			// Calling it here is a cheap way of preventing the user to have
			// no item selected. Always the current item will be selected.
			_UpdateStrings();
			break;

		case MSG_KEYMAP_SELECTED:
			_StoreKeymap();
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


namespace BPrivate {
	void ForceUnloadCatalog();
};


void
BootPromptWindow::_InitCatalog(bool saveSettings)
{
	// Initilialize the Locale Kit
	BPrivate::ForceUnloadCatalog();

	if (!saveSettings)
		return;

	BMessage settings;
	BString language;
	if (be_locale_roster->GetCatalog()->GetLanguage(&language) == B_OK) {
		settings.AddString("language", language.String());
	}

	gMutableLocaleRoster->SetPreferredLanguages(&settings);

	BCountry country(language.String(), language.ToUpper());
	gMutableLocaleRoster->SetDefaultCountry(country);
}


void
BootPromptWindow::_UpdateStrings()
{
	SetTitle(B_TRANSLATE("Welcome to Haiku!"));

	const char* infoText = B_TRANSLATE(
		"Do you wish to run the Installer or continue booting to the "
		"Desktop? You can also select your preferred language and keyboard "
		"layout from the list below.\n\n"

		"Note: Localization of Haiku applications and other components is "
		"an on-going effort. You will frequently encounter untranslated "
		"strings, but if you like, you can join in the work at "
		"<www.haiku-os.org>."
	);

	fInfoTextView->SetText(infoText);

	fDesktopButton->SetLabel(B_TRANSLATE("Desktop (Live-CD)"));
	fInstallerButton->SetLabel(B_TRANSLATE("Run Installer"));

	fLanguagesLabelView->SetText(B_TRANSLATE("Language"));
	fKeymapsLabelView->SetText(B_TRANSLATE("Keymap"));

	_PopulateLanguages();
	_PopulateKeymaps();
}


void
BootPromptWindow::_PopulateLanguages()
{
	// Disable sending the selection message while we change the selection.
	fLanguagesListView->SetSelectionMessage(NULL);

	// Clear the list view first
	while (BListItem* item = fLanguagesListView->RemoveItem(
			fLanguagesListView->CountItems() - 1)) {
		delete item;
	}

	// Get current first preferred language of the user
	BMessage preferredLanguages;
	be_locale_roster->GetPreferredLanguages(&preferredLanguages);
	const char* firstPreferredLanguage;
	if (preferredLanguages.FindString("language", &firstPreferredLanguage)
			!= B_OK) {
		// Fall back to built-in language of this application.
		firstPreferredLanguage = "en";
	}

	BMessage installedCatalogs;
	be_locale_roster->GetInstalledCatalogs(&installedCatalogs,
		"x-vnd.Haiku-ReadOnlyBootPrompt");

	BFont font;
	fLanguagesListView->GetFont(&font);

	// Try to instantiate a BCatalog for each language, it will only work
	// for translations of this application. So the list of languages will be
	//  limited to catalogs written for this application, which is on purpose!

	const char* languageID;
	LanguageItem* currentItem = NULL;
	for (int32 i = 0; installedCatalogs.FindString("language", i, &languageID)
			== B_OK; i++) {
		BLanguage* language;
		if (be_locale_roster->GetLanguage(languageID, &language) == B_OK) {
			BString name;
			language->GetNativeName(name);

			// TODO: the following block fails to detect a couple of language
			// names as containing glyphs we can't render. Why's that?
			bool hasGlyphs[name.CountChars()];
			font.GetHasGlyphs(name.String(), name.CountChars(), hasGlyphs);
			for (int32 i = 0; i < name.CountChars(); ++i) {
				if (!hasGlyphs[i]) {
					// replace by name translated to current language
					language->GetName(name);
					break;
				}
			}

			LanguageItem* item = new LanguageItem(name.String(),
				languageID);
			fLanguagesListView->AddItem(item);
			// Select this item if it is the first preferred language
			if (strcmp(firstPreferredLanguage, languageID) == 0)
				currentItem = item;

			delete language;
		} else
			printf("failed to get BLanguage for %s\n", languageID);
	}

	fLanguagesListView->SortItems(compare_void_list_items);
	if (currentItem != NULL)
		fLanguagesListView->Select(fLanguagesListView->IndexOf(currentItem));
	fLanguagesListView->ScrollToSelection();

	// Re-enable sending the selection message.
	fLanguagesListView->SetSelectionMessage(
		new BMessage(MSG_LANGUAGE_SELECTED));
}


void
BootPromptWindow::_PopulateKeymaps()
{
	// Disable sending the selection message while we change the selection.
	fKeymapsListView->SetSelectionMessage(NULL);

	// Clean the list view first
	while (BListItem* item = fKeymapsListView->RemoveItem(
			fKeymapsListView->CountItems() - 1)) {
		delete item;
	}

	// Get the name of the current keymap, so we can mark the correct entry
	// in the list view.
	BString currentKeymapName;
	entry_ref ref;
	if (_GetCurrentKeymapRef(ref) == B_OK) {
		BNode node(&ref);
		node.ReadAttrString("keymap:name", &currentKeymapName);
	}

	// TODO: common keymaps!
	BPath path;
	if (find_directory(B_SYSTEM_DATA_DIRECTORY, &path) != B_OK
		|| path.Append("Keymaps") != B_OK) {
		return;
	}

	// Populate the list
	BDirectory directory;
	if (directory.SetTo(path.Path()) == B_OK) {
		while (directory.GetNextRef(&ref) == B_OK) {
			fKeymapsListView->AddItem(new KeymapListItem(ref));
			if (currentKeymapName == ref.name)
				fKeymapsListView->Select(fKeymapsListView->CountItems() - 1);
		}
	}

	fKeymapsListView->ScrollToSelection();

	// Re-enable sending the selection message.
	fKeymapsListView->SetSelectionMessage(
		new BMessage(MSG_KEYMAP_SELECTED));
}


void
BootPromptWindow::_StoreKeymap() const
{
	KeymapListItem* item = dynamic_cast<KeymapListItem*>(
		fKeymapsListView->ItemAt(fKeymapsListView->CurrentSelection(0)));
	if (item == NULL)
		return;

	// Load and use the new keymap
	Keymap keymap;
	if (keymap.Load(item->EntryRef()) != B_OK) {
		fprintf(stderr, "Failed to load new keymap file (%s).\n",
			item->EntryRef().name);
		return;
	}

	// Get entry_ref to the Key_map file in the user settings.
	entry_ref ref;
	if (_GetCurrentKeymapRef(ref) != B_OK) {
		fprintf(stderr, "Failed to get ref to user keymap file.\n");
		return;
	}

	if (keymap.Save(ref) != B_OK) {
		fprintf(stderr, "Failed to save new keymap file (%s).\n",
			item->EntryRef().name);
		return;
	}

	keymap.Use();
}


status_t
BootPromptWindow::_GetCurrentKeymapRef(entry_ref& ref) const
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append("Key_map") != B_OK) {
		return B_ERROR;
	}

	return get_ref_for_path(path.Path(), &ref);
}

