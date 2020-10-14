/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2020, Panagiotis Vasilopoulos <hello@alwayslivid.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "BootPromptWindow.h"

#include <new>
#include <stdio.h>

#include <Alert.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Directory.h>
#include <Entry.h>
#include <Font.h>
#include <FindDirectory.h>
#include <File.h>
#include <FormattingConventions.h>
#include <IconUtils.h>
#include <IconView.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <Menu.h>
#include <MutableLocaleRoster.h>
#include <ObjectList.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <StringItem.h>
#include <StringView.h>
#include <TextView.h>
#include <UnicodeChar.h>

#include "BootPrompt.h"
#include "Keymap.h"
#include "KeymapNames.h"


using BPrivate::MutableLocaleRoster;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BootPromptWindow"


namespace BPrivate {
	void ForceUnloadCatalog();
};


static const char* kLanguageKeymapMappings[] = {
	// While there is a "Dutch" keymap, it apparently has not been widely
	// adopted, and the US-International keymap is common
	"Dutch", "US-International",

	// Cyrillic keymaps are not usable alone, as latin alphabet is required to
	// use Terminal. So we stay in US international until the user has a chance
	// to set up KeymapSwitcher.
	"Belarusian", "US-International",
	"Russian", "US-International",
	"Ukrainian", "US-International",

	// Turkish has two layouts, we must pick one
	"Turkish", "Turkish (Type-Q)",
};
static const size_t kLanguageKeymapMappingsSize
	= sizeof(kLanguageKeymapMappings) / sizeof(kLanguageKeymapMappings[0]);


class LanguageItem : public BStringItem {
public:
	LanguageItem(const char* label, const char* language)
		:
		BStringItem(label),
		fLanguage(language)
	{
	}

	~LanguageItem()
	{
	}

	const char* Language() const
	{
		return fLanguage.String();
	}

	void DrawItem(BView* owner, BRect frame, bool complete)
	{
		BStringItem::DrawItem(owner, frame, true/*complete*/);
	}

private:
			BString				fLanguage;
};


static int
compare_void_list_items(const void* _a, const void* _b)
{
	static BCollator collator;

	LanguageItem* a = *(LanguageItem**)_a;
	LanguageItem* b = *(LanguageItem**)_b;

	return collator.Compare(a->Text(), b->Text());
}


static int
compare_void_menu_items(const void* _a, const void* _b)
{
	static BCollator collator;

	BMenuItem* a = *(BMenuItem**)_a;
	BMenuItem* b = *(BMenuItem**)_b;

	return collator.Compare(a->Label(), b->Label());
}


// #pragma mark -


BootPromptWindow::BootPromptWindow()
	:
	BWindow(BRect(0, 0, 530, 400), "",
		B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE,
		B_ALL_WORKSPACES),
	fDefaultKeymapItem(NULL)
{
	SetSizeLimits(450, 16384, 350, 16384);

	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	fInfoTextView = new BTextView("");
	fInfoTextView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fInfoTextView->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
	fInfoTextView->MakeEditable(false);
	fInfoTextView->MakeSelectable(false);
	fInfoTextView->MakeResizable(false);

	BResources* res = BApplication::AppResources();
	size_t size = 0;
	const uint8_t* data;

	BBitmap desktopIcon(BRect(0, 0, 23, 23), B_RGBA32);
	data = (const uint8_t*)res->LoadResource('VICN', "Desktop", &size);
	BIconUtils::GetVectorIcon(data, size, &desktopIcon);

	BBitmap installerIcon(BRect(0, 0, 23, 23), B_RGBA32);
	data = (const uint8_t*)res->LoadResource('VICN', "Installer", &size);
	BIconUtils::GetVectorIcon(data, size, &installerIcon);

	fDesktopButton = new BButton("", new BMessage(MSG_BOOT_DESKTOP));
	fDesktopButton->SetTarget(be_app);
	fDesktopButton->MakeDefault(true);
	fDesktopButton->SetIcon(&desktopIcon);

	fInstallerButton = new BButton("", new BMessage(MSG_RUN_INSTALLER));
	fInstallerButton->SetTarget(be_app);
	fInstallerButton->SetIcon(&installerIcon);

	data = (const uint8_t*)res->LoadResource('VICN', "Language", &size);
	IconView* languageIcon = new IconView(B_LARGE_ICON);
	languageIcon->SetIcon(data, size, B_LARGE_ICON);

	data = (const uint8_t*)res->LoadResource('VICN', "Keymap", &size);
	IconView* keymapIcon = new IconView(B_LARGE_ICON);
	keymapIcon->SetIcon(data, size, B_LARGE_ICON);

	fLanguagesLabelView = new BStringView("languagesLabel", "");
	fLanguagesLabelView->SetFont(be_bold_font);
	fLanguagesLabelView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNSET));

	fKeymapsMenuLabel = new BStringView("keymapsLabel", "");
	fKeymapsMenuLabel->SetFont(be_bold_font);
	fKeymapsMenuLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNSET));
	// Make sure there is enough space to display the text even in verbose
	// locales, to avoid width changes on language changes
	float labelWidth = fKeymapsMenuLabel->StringWidth("Disposition du clavier")
		+ 16;
	fKeymapsMenuLabel->SetExplicitMinSize(BSize(labelWidth, B_SIZE_UNSET));

	fLanguagesListView = new BListView();
	BScrollView* languagesScrollView = new BScrollView("languagesScroll",
		fLanguagesListView, B_WILL_DRAW, false, true);

	// Carefully designed to not exceed the 640x480 resolution with a 12pt font.
	float width = 640 * be_plain_font->Size() / 12 - (labelWidth + 64);
	float height = be_plain_font->Size() * 23;
	fInfoTextView->SetExplicitMinSize(BSize(width, height));
	fInfoTextView->SetExplicitMaxSize(BSize(width, B_SIZE_UNSET));

	// Make sure the language list view is always wide enough to show the
	// largest language
	fLanguagesListView->SetExplicitMinSize(
		BSize(fLanguagesListView->StringWidth("Português (Brasil)"),
		height));

	fKeymapsMenuField = new BMenuField("", "", new BMenu(""));
	fKeymapsMenuField->Menu()->SetLabelFromMarked(true);

	_InitCatalog(true);
	_PopulateLanguages();
	_PopulateKeymaps();

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_WINDOW_SPACING)
		.AddGroup(B_VERTICAL, 0)
			.SetInsets(0, 0, 0, B_USE_SMALL_SPACING)
			.AddGroup(B_HORIZONTAL)
				.Add(languageIcon)
				.Add(fLanguagesLabelView)
				.SetInsets(0, 0, 0, B_USE_SMALL_SPACING)
			.End()
			.Add(languagesScrollView)
			.AddGroup(B_HORIZONTAL)
				.Add(keymapIcon)
				.Add(fKeymapsMenuLabel)
				.SetInsets(0, B_USE_DEFAULT_SPACING, 0,
					B_USE_SMALL_SPACING)
			.End()
			.Add(fKeymapsMenuField)
		.End()
		.AddGroup(B_VERTICAL)
			.SetInsets(0)
			.Add(fInfoTextView)
			.AddGroup(B_HORIZONTAL)
				.SetInsets(0)
				.AddGlue()
				.Add(fInstallerButton)
				.Add(fDesktopButton)
			.End()
		.End();

	fLanguagesListView->MakeFocus();

	// Force the info text view to use a reasonable size
	fInfoTextView->SetText("x\n\n\n\n\n\n\n\n\n\n\n\n\n\nx");
	ResizeToPreferred();

	_UpdateStrings();
	CenterOnScreen();
	Show();
}


void
BootPromptWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_LANGUAGE_SELECTED:
			if (LanguageItem* item = static_cast<LanguageItem*>(
					fLanguagesListView->ItemAt(
						fLanguagesListView->CurrentSelection(0)))) {
				BMessage preferredLanguages;
				preferredLanguages.AddString("language", item->Language());
				MutableLocaleRoster::Default()->SetPreferredLanguages(
					&preferredLanguages);
				_InitCatalog(true);
				_UpdateKeymapsMenu();

				// Select default keymap by language
				BLanguage language(item->Language());
				BMenuItem* keymapItem = _KeymapItemForLanguage(language);
				if (keymapItem != NULL) {
					keymapItem->SetMarked(true);
					_ActivateKeymap(keymapItem->Message());
				}
			}
			// Calling it here is a cheap way of preventing the user to have
			// no item selected. Always the current item will be selected.
			_UpdateStrings();
			break;

		case MSG_KEYMAP_SELECTED:
			_ActivateKeymap(message);
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


bool
BootPromptWindow::QuitRequested()
{
	// If the Deskbar is not running, then FirstBootPrompt is
	// is the only thing visible on the screen and that we won't
	// have anything else to show. In that case, it would make
	// sense to reboot the machine instead, but doing so without
	// a warning could be confusing.
	//
	// Rebooting is managed by BootPrompt.cpp.

	BAlert* alert = new(std::nothrow) BAlert(
		B_TRANSLATE_SYSTEM_NAME("Quit Haiku"),
		B_TRANSLATE("Are you sure you want to close this window? This will "
			"restart your system!"),
		B_TRANSLATE("Cancel"), B_TRANSLATE("Restart system"), NULL,
		B_WIDTH_AS_USUAL, B_STOP_ALERT);

	// If there is not enough memory to create the alert here, we may as
	// well try to reboot. There probably isn't much else to do anyway.
	if (alert != NULL) {
		alert->SetShortcut(0, B_ESCAPE);

		if (alert->Go() == 0) {
			// User doesn't want to exit after all
			return false;
		}
	}

	// If deskbar is running, don't actually reboot: we are in test mode
	// (probably run by a developer manually).
	if (!be_roster->IsRunning(kDeskbarSignature))
		be_app->PostMessage(MSG_REBOOT_REQUESTED);

	return true;
}


void
BootPromptWindow::_InitCatalog(bool saveSettings)
{
	// Initilialize the Locale Kit
	BPrivate::ForceUnloadCatalog();

	if (!saveSettings)
		return;

	BMessage settings;
	BString language;
	if (BLocaleRoster::Default()->GetCatalog()->GetLanguage(&language) == B_OK)
		settings.AddString("language", language.String());

	MutableLocaleRoster::Default()->SetPreferredLanguages(&settings);

	BFormattingConventions conventions(language.String());
	MutableLocaleRoster::Default()->SetDefaultFormattingConventions(
		conventions);
}


void
BootPromptWindow::_UpdateStrings()
{
#ifdef HAIKU_DISTRO_COMPATIBILITY_OFFICIAL
	BString name("Haiku");
#else
	BString name("*Distroname*");
#endif

	BString text(B_TRANSLATE("Welcome to %distroname%!"));
	text.ReplaceFirst("%distroname%", name);
	SetTitle(text);

	text = B_TRANSLATE_COMMENT(
		"Thank you for trying out %distroname%! We hope you'll like it!\n\n"
		"Please select your preferred language and keymap. Both settings can "
		"also be changed later when running %distroname%.\n\n"

		"Do you wish to install %distroname% now, or try it out first?",

		"For other languages, a note could be added: \""
		"Note: Localization of Haiku applications and other components is "
		"an on-going effort. You will frequently encounter untranslated "
		"strings, but if you like, you can join in the work at "
		"<www.haiku-os.org>.\"");
	text.ReplaceAll("%distroname%", name);
	fInfoTextView->SetText(text);

	text = B_TRANSLATE("Try out %distroname%");
	text.ReplaceFirst("%distroname%", name);
	fDesktopButton->SetLabel(text);

	text = B_TRANSLATE("Install %distroname%");
	text.ReplaceFirst("%distroname%", name);
	fInstallerButton->SetLabel(text);

	fLanguagesLabelView->SetText(B_TRANSLATE("Language"));
	fKeymapsMenuLabel->SetText(B_TRANSLATE("Keymap"));
	if (fKeymapsMenuField->Menu()->FindMarked() == NULL)
		fKeymapsMenuField->MenuItem()->SetLabel(B_TRANSLATE("Custom"));
}


void
BootPromptWindow::_PopulateLanguages()
{
	// TODO: detect language/country from IP address

	// Get current first preferred language of the user
	BMessage preferredLanguages;
	BLocaleRoster::Default()->GetPreferredLanguages(&preferredLanguages);
	const char* firstPreferredLanguage;
	if (preferredLanguages.FindString("language", &firstPreferredLanguage)
			!= B_OK) {
		// Fall back to built-in language of this application.
		firstPreferredLanguage = "en";
	}

	BMessage installedCatalogs;
	BLocaleRoster::Default()->GetAvailableCatalogs(&installedCatalogs,
		"x-vnd.Haiku-FirstBootPrompt");

	BFont font;
	fLanguagesListView->GetFont(&font);

	// Try to instantiate a BCatalog for each language, it will only work
	// for translations of this application. So the list of languages will be
	// limited to catalogs written for this application, which is on purpose!

	const char* languageID;
	LanguageItem* currentItem = NULL;
	for (int32 i = 0; installedCatalogs.FindString("language", i, &languageID)
			== B_OK; i++) {
		BLanguage* language;
		if (BLocaleRoster::Default()->GetLanguage(languageID, &language)
				== B_OK) {
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
			fprintf(stderr, "failed to get BLanguage for %s\n", languageID);
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
BootPromptWindow::_UpdateKeymapsMenu()
{
	BMenu *menu = fKeymapsMenuField->Menu();
	BMenuItem* item;
	BList itemsList;

	// Recreate keymapmenu items list, since BMenu could not sort its items.
	while ((item = menu->ItemAt(0)) != NULL) {
		BMessage* message = item->Message();
		entry_ref ref;
		message->FindRef("ref", &ref);
		item-> SetLabel(B_TRANSLATE_NOCOLLECT_ALL((ref.name),
		"KeymapNames", NULL));
		itemsList.AddItem(item);
		menu->RemoveItem((int32)0);
	}
	itemsList.SortItems(compare_void_menu_items);
	fKeymapsMenuField->Menu()->AddList(&itemsList, 0);
}


void
BootPromptWindow::_PopulateKeymaps()
{
	// Get the name of the current keymap, so we can mark the correct entry
	// in the list view.
	BString currentName;
	entry_ref currentRef;
	if (_GetCurrentKeymapRef(currentRef) == B_OK) {
		BNode node(&currentRef);
		node.ReadAttrString("keymap:name", &currentName);
	}

	// TODO: common keymaps!
	BPath path;
	if (find_directory(B_SYSTEM_DATA_DIRECTORY, &path) != B_OK
		|| path.Append("Keymaps") != B_OK) {
		return;
	}

	// US-International is the default keymap, if we could not found a
	// matching one
	BString usInternational("US-International");

	// Populate the menu
	BDirectory directory;
	if (directory.SetTo(path.Path()) == B_OK) {
		entry_ref ref;
		BList itemsList;
		while (directory.GetNextRef(&ref) == B_OK) {
			BMessage* message = new BMessage(MSG_KEYMAP_SELECTED);
			message->AddRef("ref", &ref);
			BMenuItem* item =
				new BMenuItem(B_TRANSLATE_NOCOLLECT_ALL((ref.name),
				"KeymapNames", NULL), message);
			itemsList.AddItem(item);
			if (currentName == ref.name)
				item->SetMarked(true);

			if (usInternational == ref.name)
				fDefaultKeymapItem = item;
		}
		itemsList.SortItems(compare_void_menu_items);
		fKeymapsMenuField->Menu()->AddList(&itemsList, 0);
	}
}


void
BootPromptWindow::_ActivateKeymap(const BMessage* message) const
{
	entry_ref ref;
	if (message == NULL || message->FindRef("ref", &ref) != B_OK)
		return;

	// Load and use the new keymap
	Keymap keymap;
	if (keymap.Load(ref) != B_OK) {
		fprintf(stderr, "Failed to load new keymap file (%s).\n", ref.name);
		return;
	}

	// Get entry_ref to the Key_map file in the user settings.
	entry_ref currentRef;
	if (_GetCurrentKeymapRef(currentRef) != B_OK) {
		fprintf(stderr, "Failed to get ref to user keymap file.\n");
		return;
	}

	if (keymap.Save(currentRef) != B_OK) {
		fprintf(stderr, "Failed to save new keymap file (%s).\n", ref.name);
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


BMenuItem*
BootPromptWindow::_KeymapItemForLanguage(BLanguage& language) const
{
	BLanguage english("en");
	BString name;
	if (language.GetName(name, &english) != B_OK)
		return fDefaultKeymapItem;

	// Check special mappings first
	for (size_t i = 0; i < kLanguageKeymapMappingsSize; i += 2) {
		if (!strcmp(name, kLanguageKeymapMappings[i])) {
			name = kLanguageKeymapMappings[i + 1];
			break;
		}
	}

	BMenu* menu = fKeymapsMenuField->Menu();
	for (int32 i = 0; i < menu->CountItems(); i++) {
		BMenuItem* item = menu->ItemAt(i);
		BMessage* message = item->Message();

		entry_ref ref;
		if (message->FindRef("ref", &ref) == B_OK
			&& name == ref.name)
			return item;
	}

	return fDefaultKeymapItem;
}
