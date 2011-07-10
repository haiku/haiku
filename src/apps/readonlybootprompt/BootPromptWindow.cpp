/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "BootPromptWindow.h"

#include <new>
#include <stdio.h>

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
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <Menu.h>
#include <MutableLocaleRoster.h>
#include <ObjectList.h>
#include <Path.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <StringItem.h>
#include <StringView.h>
#include <TextView.h>
#include <UnicodeChar.h>

#include "BootPrompt.h"
#include "Keymap.h"


using BPrivate::MutableLocaleRoster;


enum {
	MSG_LANGUAGE_SELECTED	= 'lngs',
	MSG_KEYMAP_SELECTED		= 'kmps'
};


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "BootPromptWindow"


namespace BPrivate {
	void ForceUnloadCatalog();
};


static const char* kLanguageKeymapMappings[] = {
	// While there is a "Dutch" keymap, it apparently has not been widely
	// adopted, and the US-International keymap is common
	"Dutch", "US-International"
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
		fIcon = new(std::nothrow) BBitmap(BRect(0, 0, 15, 15), B_RGBA32);
		if (fIcon != NULL
			&& (!fIcon->IsValid()
				|| BLocaleRoster::Default()->GetFlagIconForLanguage(fIcon,
					language) != B_OK)) {
			delete fIcon;
			fIcon = NULL;
		}
	}

	~LanguageItem()
	{
		delete fIcon;
	}

	const char* Language() const
	{
		return fLanguage.String();
	}

	void DrawItem(BView* owner, BRect frame, bool complete)
	{
		BStringItem::DrawItem(owner, frame, true/*complete*/);

		// Draw the icon
		if (fIcon != NULL) {
			frame.left = frame.right - kFlagWidth;
			BRect iconFrame(frame);
			iconFrame.Set(iconFrame.left, iconFrame.top + 1,
				iconFrame.left + kFlagWidth - 2,
				iconFrame.top + kFlagWidth - 1);

			owner->SetDrawingMode(B_OP_OVER);
			owner->DrawBitmap(fIcon, iconFrame);
			owner->SetDrawingMode(B_OP_COPY);
		}
	}

private:
	static	const int			kFlagWidth = 16;

			BString				fLanguage;
			BBitmap*			fIcon;
};


static int
compare_void_list_items(const void* _a, const void* _b)
{
	static BCollator collator;

	LanguageItem* a = *(LanguageItem**)_a;
	LanguageItem* b = *(LanguageItem**)_b;

	return collator.Compare(a->Text(), b->Text());
}


// #pragma mark -


BootPromptWindow::BootPromptWindow()
	:
	BWindow(BRect(0, 0, 530, 400), "",
		B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE),
	fDefaultKeymapItem(NULL)
{
	SetSizeLimits(450, 16384, 350, 16384);

	fInfoTextView = new BTextView("");
	fInfoTextView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fInfoTextView->MakeEditable(false);
	fInfoTextView->MakeSelectable(false);

	fDesktopButton = new BButton("", new BMessage(MSG_BOOT_DESKTOP));
	fDesktopButton->SetTarget(be_app);
	fDesktopButton->MakeDefault(true);

	fInstallerButton = new BButton("", new BMessage(MSG_RUN_INSTALLER));
	fInstallerButton->SetTarget(be_app);

	fLanguagesLabelView = new BStringView("languagesLabel", "");
	fLanguagesLabelView->SetFont(be_bold_font);
	fLanguagesLabelView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNSET));

	fLanguagesListView = new BListView();
	fLanguagesListView->SetFlags(
		fLanguagesListView->Flags() | B_FULL_UPDATE_ON_RESIZE);
		// Our ListItem rendering depends on the width of the view, so
		// we need a full update
	BScrollView* languagesScrollView = new BScrollView("languagesScroll",
		fLanguagesListView, B_WILL_DRAW, false, true);

	fKeymapsMenuField = new BMenuField("", "", new BMenu(""));
	fKeymapsMenuField->Menu()->SetLabelFromMarked(true);

	_InitCatalog(false);
	_PopulateLanguages();
	_PopulateKeymaps();

	float spacing = be_control_look->DefaultItemSpacing();

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BLayoutBuilder::Group<>(B_VERTICAL, 0)
		.AddGroup(B_HORIZONTAL, spacing)
			.AddGroup(B_VERTICAL, spacing)
				.AddGroup(B_VERTICAL, spacing)
					.Add(fLanguagesLabelView)
					.Add(languagesScrollView)
				.End()
				.AddGrid(0.f)
					.AddMenuField(fKeymapsMenuField, 0, 0)
				.End()
			.End()
			.Add(fInfoTextView)
			.SetInsets(spacing, spacing, spacing, spacing)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL, spacing)
			.AddGlue()
			.Add(fInstallerButton)
			.Add(fDesktopButton)
			.SetInsets(spacing, spacing, spacing, spacing)
		.End()
		.View()
	);

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

				// Select default keymap by language
				BLanguage language(item->Language());
				BMenuItem* item = _KeymapItemForLanguage(language);
				if (item != NULL) {
					item->SetMarked(true);
					_ActivateKeymap(item->Message());
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
	SetTitle(B_TRANSLATE("Welcome to Haiku!"));

	fInfoTextView->SetText(B_TRANSLATE_COMMENT(
		"Thank you for trying out Haiku! We hope you'll like it!\n\n"
		"You can select your preferred language and keyboard "
		"layout from the list on the left which will then be used instantly. "
		"You can easily change both settings from the Desktop later on on "
		"the fly.\n\n"

		"Do you wish to run the Installer or continue booting to the "
		"Desktop?\n",

		"For other languages, a note could be added: \""
		"Note: Localization of Haiku applications and other components is "
		"an on-going effort. You will frequently encounter untranslated "
		"strings, but if you like, you can join in the work at "
		"<www.haiku-os.org>.\""));

	fDesktopButton->SetLabel(B_TRANSLATE("Desktop (Live-CD)"));
	fInstallerButton->SetLabel(B_TRANSLATE("Run Installer"));

	fLanguagesLabelView->SetText(B_TRANSLATE("Language"));
	fKeymapsMenuField->SetLabel(B_TRANSLATE("Keymap"));
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
		"x-vnd.Haiku-ReadOnlyBootPrompt");

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
		while (directory.GetNextRef(&ref) == B_OK) {
			BMessage* message = new BMessage(MSG_KEYMAP_SELECTED);
			message->AddRef("ref", &ref);
			BMenuItem* item = new BMenuItem(ref.name, message);
			fKeymapsMenuField->Menu()->AddItem(item);

			if (currentName == ref.name)
				item->SetMarked(true);

			if (usInternational == ref.name)
				fDefaultKeymapItem = item;
		}
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
