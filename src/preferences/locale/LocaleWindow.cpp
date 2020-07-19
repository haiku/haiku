/*
 * Copyright 2005-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009-2010, Adrien Destugues <pulkomandy@gmail.com>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LocaleWindow.h"

#include <iostream>

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <FormattingConventions.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MutableLocaleRoster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <StringView.h>
#include <TabView.h>
#include <UnicodeChar.h>

#include "FormatSettingsView.h"
#include "LocalePreflet.h"
#include "LanguageListView.h"


using BPrivate::MutableLocaleRoster;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Locale Preflet Window"


static const uint32 kMsgLanguageInvoked = 'LaIv';
static const uint32 kMsgLanguageDragged = 'LaDr';
static const uint32 kMsgPreferredLanguageInvoked = 'PLIv';
static const uint32 kMsgPreferredLanguageDragged = 'PLDr';
static const uint32 kMsgPreferredLanguageDeleted = 'PLDl';
static const uint32 kMsgConventionsSelection = 'csel';
static const uint32 kMsgDefaults = 'dflt';

static const uint32 kMsgPreferredLanguagesChanged = 'lang';


static int
compare_typed_list_items(const BListItem* _a, const BListItem* _b)
{
	static BCollator collator;

	LanguageListItem* a = (LanguageListItem*)_a;
	LanguageListItem* b = (LanguageListItem*)_b;

	return collator.Compare(a->Text(), b->Text());
}


// #pragma mark -

LocaleWindow::LocaleWindow()
	:
	BWindow(BRect(0, 0, 0, 0), B_TRANSLATE_SYSTEM_NAME("Locale"), B_TITLED_WINDOW,
		B_QUIT_ON_WINDOW_CLOSE | B_ASYNCHRONOUS_CONTROLS
			| B_AUTO_UPDATE_SIZE_LIMITS),
	fInitialConventionsItem(NULL),
	fDefaultConventionsItem(NULL)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	float spacing = be_control_look->DefaultItemSpacing();

	BTabView* tabView = new BTabView("tabview", B_WIDTH_FROM_WIDEST);
	tabView->SetBorder(B_NO_BORDER);

	BGroupView* languageTab = new BGroupView(B_TRANSLATE("Language"),
		B_HORIZONTAL, spacing);

	// first list: available languages
	fLanguageListView = new LanguageListView("available",
		B_MULTIPLE_SELECTION_LIST);
	BScrollView* scrollView = new BScrollView("scroller", fLanguageListView,
		B_WILL_DRAW | B_FRAME_EVENTS, true, true);

	fLanguageListView->SetInvocationMessage(new BMessage(kMsgLanguageInvoked));
	fLanguageListView->SetDragMessage(new BMessage(kMsgLanguageDragged));
	fLanguageListView->SetGlobalDropTargetIndicator(true);

	BFont font;
	fLanguageListView->GetFont(&font);

	// Fill the language list from the LocaleRoster data
	BMessage availableLanguages;
	if (BLocaleRoster::Default()->GetAvailableLanguages(&availableLanguages)
			== B_OK) {
		BString currentID;
		LanguageListItem* currentToplevelItem = NULL;

		for (int i = 0; availableLanguages.FindString("language", i, &currentID)
				== B_OK; i++) {
			// Now get the human-readable, native name for each language
			BString name;
			BLanguage currentLanguage(currentID.String());
			currentLanguage.GetNativeName(name);

			int nameLength = name.CountChars();
			bool hasGlyphs[nameLength];
			font.GetHasGlyphs(name.String(), nameLength, hasGlyphs);
			for (int32 i = 0; i < nameLength; ++i) {
				if (!hasGlyphs[i]) {
					// replace by name translated to current language
					currentLanguage.GetName(name);
					break;
				}
			}

			LanguageListItem* item;
			if (currentLanguage.IsCountrySpecific()) {
				item = new LanguageListItemWithFlag(name, currentID.String(),
					currentLanguage.Code(), currentLanguage.CountryCode());
			} else {
				item = new LanguageListItem(name, currentID.String(),
					currentLanguage.Code());
			}

			if (currentLanguage.IsCountrySpecific()
				&& currentToplevelItem != NULL
				&& currentToplevelItem->Code() == item->Code()) {
				fLanguageListView->AddUnder(item, currentToplevelItem);
			} else if (currentLanguage.ScriptCode() != NULL
				&& currentToplevelItem != NULL
				&& currentToplevelItem->Code() == item->Code()) {
				// This is a script for some language, skip it and add the
				// country-specific variants to the parent directly
				delete item;
			} else {
				// This is a generic language, add it at top-level
				fLanguageListView->AddItem(item);
				item->SetExpanded(false);
				currentToplevelItem = item;
			}
		}

		fLanguageListView->FullListSortItems(compare_typed_list_items);
	} else {
		BAlert* alert = new BAlert("Error",
			B_TRANSLATE("Unable to find the available languages! You can't "
				"use this preflet!"),
			B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
	}

	// Second list: active languages
	fPreferredListView = new LanguageListView("preferred",
		B_MULTIPLE_SELECTION_LIST);
	BScrollView* scrollViewEnabled = new BScrollView("scroller",
		fPreferredListView, B_WILL_DRAW | B_FRAME_EVENTS, true, true);

	fPreferredListView->SetInvocationMessage(
		new BMessage(kMsgPreferredLanguageInvoked));
	fPreferredListView->SetDeleteMessage(
		new BMessage(kMsgPreferredLanguageDeleted));
	fPreferredListView->SetDragMessage(
		new BMessage(kMsgPreferredLanguageDragged));

	BLayoutBuilder::Group<>(languageTab)
		.AddGroup(B_VERTICAL)
			.Add(new BStringView("", B_TRANSLATE("Available languages")))
			.Add(scrollView)
			.End()
		.AddGroup(B_VERTICAL)
			.Add(new BStringView("", B_TRANSLATE("Preferred languages")))
			.Add(scrollViewEnabled)
			.End()
		.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
			B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING);

	BView* formattingTab = new BView(B_TRANSLATE("Formatting"), B_WILL_DRAW);
	formattingTab->SetLayout(new BGroupLayout(B_VERTICAL, 0));

	fConventionsListView = new LanguageListView("formatting",
		B_SINGLE_SELECTION_LIST);
	scrollView = new BScrollView("scroller", fConventionsListView,
		B_WILL_DRAW | B_FRAME_EVENTS, true, true);
	fConventionsListView->SetSelectionMessage(
		new BMessage(kMsgConventionsSelection));

	// get all available formatting conventions (by language)
	BFormattingConventions initialConventions;
	BLocale::Default()->GetFormattingConventions(&initialConventions);
	BString conventionsID;
	fInitialConventionsItem = NULL;
	LanguageListItem* currentToplevelItem = NULL;
	for (int i = 0;
		availableLanguages.FindString("language", i, &conventionsID) == B_OK;
		i++) {
		BFormattingConventions conventions(conventionsID);
		BString conventionsName;
		conventions.GetName(conventionsName);

		LanguageListItem* item;
		if (conventions.AreCountrySpecific()) {
			item = new LanguageListItemWithFlag(conventionsName, conventionsID,
				conventions.LanguageCode(), conventions.CountryCode());
		} else {
			item = new LanguageListItem(conventionsName, conventionsID,
				conventions.LanguageCode());
		}
		if (!strcmp(conventionsID, "en_US"))
			fDefaultConventionsItem = item;
		if (conventions.AreCountrySpecific()
			&& currentToplevelItem != NULL
			&& currentToplevelItem->Code() == item->Code()) {
			if (!strcmp(conventionsID, initialConventions.ID())) {
				fConventionsListView->Expand(currentToplevelItem);
				fInitialConventionsItem = item;
			}
			fConventionsListView->AddUnder(item, currentToplevelItem);
		} else {
			// This conventions-item isn't country-specific, add it at top-level
			fConventionsListView->AddItem(item);
			item->SetExpanded(false);
			currentToplevelItem = item;
			if (!strcmp(conventionsID, initialConventions.ID()))
				fInitialConventionsItem = item;
		}
	}

	fConventionsListView->FullListSortItems(compare_typed_list_items);
	if (fInitialConventionsItem != NULL) {
		fConventionsListView->Select(fConventionsListView->IndexOf(
			fInitialConventionsItem));
	}

	fConventionsListView->SetExplicitMinSize(BSize(21 * be_plain_font->Size(),
		B_SIZE_UNSET));

	fFormatView = new FormatSettingsView();

	formattingTab->AddChild(BLayoutBuilder::Group<>(B_HORIZONTAL, spacing)
		.AddGroup(B_VERTICAL, 3)
			.Add(scrollView)
			.End()
		.Add(fFormatView)
		.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
			B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING));

	tabView->AddTab(languageTab);
	tabView->AddTab(formattingTab);

	BButton* button
		= new BButton(B_TRANSLATE("Defaults"), new BMessage(kMsgDefaults));

	fRevertButton
		= new BButton(B_TRANSLATE("Revert"), new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0, B_USE_DEFAULT_SPACING, 0, B_USE_WINDOW_SPACING)
		.Add(tabView)
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
			.Add(button)
			.Add(fRevertButton)
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_WINDOW_SPACING, 0)
			.AddGlue();

	_Refresh(true);
	_SettingsReverted();
	CenterOnScreen();
}


LocaleWindow::~LocaleWindow()
{
}


void
LocaleWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_LOCALE_CHANGED:
			fFormatView->MessageReceived(message);
			break;

		case kMsgDefaults:
			_Defaults();
			break;

		case kMsgRevert:
		{
			_Revert();
			fFormatView->Revert();
			fConventionsListView->DeselectAll();
			if (fInitialConventionsItem != NULL) {
				BListItem* superitem
					= fConventionsListView->Superitem(fInitialConventionsItem);
				if (superitem != NULL)
					superitem->SetExpanded(true);
				fConventionsListView->Select(fConventionsListView->IndexOf(
						fInitialConventionsItem));
				fConventionsListView->ScrollToSelection();
			}
			_SettingsReverted();
			break;
		}

		case kMsgSettingsChanged:
			_SettingsChanged();
			break;

		case kMsgLanguageDragged:
		{
			void* target = NULL;
			if (message->FindPointer("drop_target", &target) != B_OK
				|| target != fPreferredListView)
				break;

			// Add from available languages to preferred languages
			int32 dropIndex;
			if (message->FindInt32("drop_index", &dropIndex) != B_OK)
				dropIndex = fPreferredListView->CountItems();

			int32 i = 0;
			for (int32 index = 0;
					message->FindInt32("index", i, &index) == B_OK; i++) {
				LanguageListItem* item = static_cast<LanguageListItem*>(
					fLanguageListView->ItemAt(index));
				_InsertPreferredLanguage(item, dropIndex++);
			}

			fPreferredListView->Select(dropIndex - i, dropIndex - 1);
			break;
		}

		case kMsgLanguageInvoked:
		{
			int32 index = 0;
			for (int32 i = 0; message->FindInt32("index", i, &index) == B_OK;
					i++) {
				LanguageListItem* item = static_cast<LanguageListItem*>(
					fLanguageListView->ItemAt(index));
				_InsertPreferredLanguage(item);
			}
			break;
		}

		case kMsgPreferredLanguageDragged:
		{
			void* target = NULL;
			if (fPreferredListView->CountItems() == 1
				|| message->FindPointer("drop_target", &target) != B_OK)
				break;

			if (target == fPreferredListView) {
				// change ordering
				int32 dropIndex = message->FindInt32("drop_index");
				int32 i = 0;
				for (int32 index = 0;
						message->FindInt32("index", i, &index) == B_OK;
						i++, dropIndex++) {
					if (dropIndex > index) {
						dropIndex--;
						index -= i;
					}
					BListItem* item = fPreferredListView->RemoveItem(index);
					fPreferredListView->AddItem(item, dropIndex);
				}

				fPreferredListView->Select(dropIndex - i, dropIndex - 1);
				_PreferredLanguagesChanged();
				break;
			}

			// supposed to fall through - remove item
		}
		case kMsgPreferredLanguageDeleted:
		case kMsgPreferredLanguageInvoked:
		{
			if (fPreferredListView->CountItems() == 1)
				break;

			// Remove from preferred languages
			int32 index = 0;
			for (int32 i = 0; message->FindInt32("index", i, &index) == B_OK;
					i++) {
				delete fPreferredListView->RemoveItem(index - i);

				if (message->what == kMsgPreferredLanguageDeleted) {
					int32 count = fPreferredListView->CountItems();
					fPreferredListView->Select(
						index < count ? index : count - 1);
				}
			}

			_PreferredLanguagesChanged();
			break;
		}

		case kMsgConventionsSelection:
		{
			// Country selection changed.
			// Get the new selected country from the ListView and send it to the
			// main app event handler.
			void* listView;
			if (message->FindPointer("source", &listView) != B_OK)
				break;

			BListView* conventionsList = static_cast<BListView*>(listView);
			if (conventionsList == NULL)
				break;

			LanguageListItem* item = static_cast<LanguageListItem*>
				(conventionsList->ItemAt(conventionsList->CurrentSelection()));
			if (item == NULL)
				break;

			BFormattingConventions conventions(item->ID());
			MutableLocaleRoster::Default()->SetDefaultFormattingConventions(
				conventions);

			_SettingsChanged();
			fFormatView->Refresh();
			break;
		}

		case kMsgFilesystemTranslationChanged:
		{
			int32 value = message->FindInt32("be:value");
			MutableLocaleRoster::Default()->SetFilesystemTranslationPreferred(
				value == B_CONTROL_ON);

			BAlert* alert = new BAlert(B_TRANSLATE("Locale"),
				B_TRANSLATE("Deskbar and Tracker need to be restarted for this "
				"change to take effect. Would you like to restart them now?"),
				B_TRANSLATE("Cancel"), B_TRANSLATE("Restart"), NULL,
				B_WIDTH_FROM_WIDEST, B_IDEA_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			alert->Go(new BInvoker(new BMessage(kMsgRestartTrackerAndDeskbar),
				NULL, be_app));
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
LocaleWindow::QuitRequested()
{
	return true;
}


void
LocaleWindow::Show()
{
	BWindow::Show();

	Lock();
	if (IsLocked()) {
		fConventionsListView->ScrollToSelection();
		Unlock();
	}
}


void
LocaleWindow::_SettingsChanged()
{
	fRevertButton->SetEnabled(fFormatView->IsReversible() || _IsReversible());
}


void
LocaleWindow::_SettingsReverted()
{
	fRevertButton->SetEnabled(false);
}


bool
LocaleWindow::_IsReversible() const
{
	BMessage preferredLanguages;
	BLocaleRoster::Default()->GetPreferredLanguages(&preferredLanguages);

	return !preferredLanguages.HasSameData(fInitialPreferredLanguages);
}


void
LocaleWindow::_PreferredLanguagesChanged()
{
	BMessage preferredLanguages;
	int index = 0;
	while (index < fPreferredListView->CountItems()) {
		LanguageListItem* item = static_cast<LanguageListItem*>(
			fPreferredListView->ItemAt(index));
		if (item != NULL)
			preferredLanguages.AddString("language", item->ID());
		index++;
	}
	MutableLocaleRoster::Default()->SetPreferredLanguages(&preferredLanguages);

	_EnableDisableLanguages();
}


void
LocaleWindow::_EnableDisableLanguages()
{
	DisableUpdates();

	for (int32 i = 0; i < fLanguageListView->FullListCountItems(); i++) {
		LanguageListItem* item = static_cast<LanguageListItem*>(
			fLanguageListView->FullListItemAt(i));

		bool enable = fPreferredListView->ItemForLanguageID(item->ID()) == NULL;
		if (item->IsEnabled() != enable) {
			item->SetEnabled(enable);

			int32 visibleIndex = fLanguageListView->IndexOf(item);
			if (visibleIndex >= 0) {
				if (!enable)
					fLanguageListView->Deselect(visibleIndex);
				fLanguageListView->InvalidateItem(visibleIndex);
			}
		}
	}

	EnableUpdates();
}


void
LocaleWindow::_Refresh(bool setInitial)
{
	BMessage preferredLanguages;
	BLocaleRoster::Default()->GetPreferredLanguages(&preferredLanguages);
	if (setInitial)
		fInitialPreferredLanguages = preferredLanguages;

	_SetPreferredLanguages(preferredLanguages);
}


void
LocaleWindow::_Revert()
{
	_SetPreferredLanguages(fInitialPreferredLanguages);
}


void
LocaleWindow::_SetPreferredLanguages(const BMessage& languages)
{
	DisableUpdates();

	// Delete all existing items
	for (int32 index = fPreferredListView->CountItems(); index-- > 0; ) {
		delete fPreferredListView->ItemAt(index);
	}
	fPreferredListView->MakeEmpty();

	BString languageID;
	for (int32 index = 0;
		languages.FindString("language", index, &languageID) == B_OK; index++) {
		int32 listIndex;
		LanguageListItem* item = fLanguageListView->ItemForLanguageID(
			languageID.String(), &listIndex);
		if (item != NULL) {
			// We found the item we were looking for, now copy it to
			// the other list
			fPreferredListView->AddItem(new LanguageListItem(*item));
		}
	}

	_EnableDisableLanguages();
	EnableUpdates();
}


void
LocaleWindow::_InsertPreferredLanguage(LanguageListItem* item, int32 atIndex)
{
	if (item == NULL || fPreferredListView->ItemForLanguageID(
			item->ID().String()) != NULL)
		return;

	if (atIndex == -1)
		atIndex = fPreferredListView->CountItems();

	BLanguage language(item->Code());
	LanguageListItem* baseItem
		= fPreferredListView->ItemForLanguageCode(language.Code(), &atIndex);

	DisableUpdates();

	fPreferredListView->AddItem(new LanguageListItem(*item), atIndex);

	// Replace other languages sharing the same base
	if (baseItem != NULL) {
		fPreferredListView->RemoveItem(baseItem);
		delete baseItem;
	}

	_PreferredLanguagesChanged();

	EnableUpdates();
}


void
LocaleWindow::_Defaults()
{
	BMessage preferredLanguages;
	preferredLanguages.AddString("language", "en");
	MutableLocaleRoster::Default()->SetPreferredLanguages(&preferredLanguages);
	_SetPreferredLanguages(preferredLanguages);

	BFormattingConventions conventions("en_US");
	MutableLocaleRoster::Default()->SetDefaultFormattingConventions(
		conventions);

	fConventionsListView->DeselectAll();
	if (fDefaultConventionsItem != NULL) {
		BListItem* superitem
			= fConventionsListView->Superitem(fDefaultConventionsItem);
		if (superitem != NULL && !superitem->IsExpanded())
			fConventionsListView->Expand(superitem);
		fConventionsListView->Select(fConventionsListView->IndexOf(
				fDefaultConventionsItem));
		fConventionsListView->ScrollToSelection();
	}
}
