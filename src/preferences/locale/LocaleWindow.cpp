/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Locale.h"
#include "LocaleWindow.h"
#include "LanguageListView.h"

#include <iostream>

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TabView.h>

#include <ICUWrapper.h>

#include <unicode/locid.h>
#include <unicode/datefmt.h>

#include "TimeFormatSettingsView.h"


#define TR_CONTEXT "Locale Preflet Window"


static int
compare_list_items(const void* _a, const void* _b)
{
	LanguageListItem* a = *(LanguageListItem**)_a;
	LanguageListItem* b = *(LanguageListItem**)_b;
	return strcasecmp(a->Text(), b->Text());
}


static int
compare_typed_list_items(const BListItem* _a, const BListItem* _b)
{
	LanguageListItem* a = *(LanguageListItem**)_a;
	LanguageListItem* b = *(LanguageListItem**)_b;
	return strcasecmp(a->Text(), b->Text());
}


LocaleWindow::LocaleWindow()
	:
	BWindow(BRect(0, 0, 0, 0), "Locale", B_TITLED_WINDOW, B_NOT_RESIZABLE
		| B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
		| B_QUIT_ON_WINDOW_CLOSE),
	fMsgPrefLanguagesChanged(new BMessage(kMsgPrefLanguagesChanged))
{
	BCountry* defaultCountry;
	be_locale_roster->GetDefaultCountry(&defaultCountry);

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	BButton* button = new BButton(TR("Defaults"), new BMessage(kMsgDefaults));
	fRevertButton = new BButton(TR("Revert"), new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);

	BTabView* tabView = new BTabView("tabview");

	BView* languageTab = new BView(TR("Language"), B_WILL_DRAW);
	languageTab->SetLayout(new BGroupLayout(B_VERTICAL, 0));

	{
		// first list: available languages
		fLanguageListView = new LanguageListView("available",
			B_MULTIPLE_SELECTION_LIST);
		BScrollView* scrollView = new BScrollView("scroller", fLanguageListView,
			B_WILL_DRAW | B_FRAME_EVENTS, false, true);

		fLanguageListView->SetInvocationMessage(new BMessage(kMsgLangInvoked));

		// Fill the language list from the LocaleRoster data
		BMessage installedLanguages;
		if (be_locale_roster->GetInstalledLanguages(&installedLanguages)
				== B_OK) {

			BString currentLanguageCode;
			BString currentLanguageName;
			LanguageListItem* lastAddedLanguage = NULL;
			for (int i = 0; installedLanguages.FindString("langs",
					i, &currentLanguageCode) == B_OK; i++) {

				// Now get an human-readable, localized name for each language
				// TODO: sort them using collators.
				BLanguage* currentLanguage;
				be_locale_roster->GetLanguage(&currentLanguage,
					currentLanguageCode.String());

				currentLanguageName.Truncate(0);
				currentLanguage->GetName(&currentLanguageName);

				LanguageListItem* si = new LanguageListItem(currentLanguageName,
					currentLanguageCode.String());
				if (currentLanguage->IsCountry()) {
					fLanguageListView->AddUnder(si,lastAddedLanguage);
				} else {
					// This is a language without country, add it at top-level
					fLanguageListView->AddItem(si);
					si->SetExpanded(false);
					if (lastAddedLanguage != NULL) {
						fLanguageListView->SortItemsUnder(lastAddedLanguage,
							true, compare_typed_list_items);
					}
					lastAddedLanguage = si;
				}

				delete currentLanguage;
			}
			fLanguageListView->SortItemsUnder(lastAddedLanguage, true,
				compare_typed_list_items);

			fLanguageListView->SortItems(compare_list_items);
				// see previous comment on sort using collators

		} else {
			BAlert* myAlert = new BAlert("Error",
				TR("Unable to find the available languages! You can't use this "
					"preflet!"),
				TR("OK"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
			myAlert->Go();
		}

		// Second list: active languages
		fPreferredListView = new LanguageListView("preferred",
			B_MULTIPLE_SELECTION_LIST);
		BScrollView* scrollViewEnabled = new BScrollView("scroller",
			fPreferredListView, B_WILL_DRAW | B_FRAME_EVENTS, false, true);

		fPreferredListView
			->SetInvocationMessage(new BMessage(kMsgPrefLangInvoked));

		// get the preferred languages from the Settings. Move them here from
		// the other list.
		BMessage msg;
		be_locale_roster->GetPreferredLanguages(&msg);
		BString langCode;
		for (int index = 0;
			msg.FindString("language", index, &langCode) == B_OK; index++) {
			for (int listPos = 0; LanguageListItem* lli
					= static_cast<LanguageListItem*>
						(fLanguageListView->FullListItemAt(listPos));
					listPos++) {
				if (langCode == lli->LanguageCode()) {
					// We found the item we were looking for, now move it to
					// the other list along with all its children
					static_cast<LanguageListView*>(fPreferredListView)
						->MoveItemFrom(fLanguageListView,
							fLanguageListView->FullListIndexOf(lli),
							fLanguageListView->CountItems());
				}
			}
		}

		languageTab->AddChild(BLayoutBuilder::Group<>(B_HORIZONTAL, 10)
			.AddGroup(B_VERTICAL, 10)
				.Add(new BStringView("", TR("Available languages")))
				.Add(scrollView)
				.End()
			.AddGroup(B_VERTICAL, 10)
				.Add(new BStringView("", TR("Preferred languages")))
				.Add(scrollViewEnabled)
				.End()
			.View());
	}

	BView* countryTab = new BView(TR("Country"), B_WILL_DRAW);
	countryTab->SetLayout(new BGroupLayout(B_VERTICAL, 0));

	{
		BListView* listView = new BListView("country", B_SINGLE_SELECTION_LIST);
		BScrollView* scrollView = new BScrollView("scroller",
			listView, B_WILL_DRAW | B_FRAME_EVENTS, false, true);
		listView->SetSelectionMessage(new BMessage(kMsgCountrySelection));

		// get all available countries from ICU
		// Use DateFormat::getAvailableLocale so we get only the one we can
		// use. Maybe check the NumberFormat one and see if there is more.
		int32_t localeCount;
		const Locale* currentLocale
			= Locale::getAvailableLocales(localeCount);

		for (int index = 0; index < localeCount; index++)
		{
			UnicodeString countryFullName;
			BString str;
			BStringByteSink bbs(&str);
			currentLocale[index].getDisplayName(countryFullName);
			countryFullName.toUTF8(bbs);
			LanguageListItem* si
				= new LanguageListItem(str, currentLocale[index].getName());
			listView->AddItem(si);
			if (strcmp(currentLocale[index].getName(),
					defaultCountry->Code()) == 0)
				listView->Select(listView->CountItems() - 1);
		}

		// TODO: find a real solution intead of this hack
		listView->SetExplicitMinSize(BSize(300, B_SIZE_UNSET));

		fFormatView = new FormatView(defaultCountry);

		countryTab->AddChild(BLayoutBuilder::Group<>(B_HORIZONTAL, 5)
			.AddGroup(B_VERTICAL, 3)
				.Add(scrollView)
				.End()
			.Add(fFormatView)
			.View()
		);

		listView->ScrollToSelection();
	}

	tabView->AddTab(languageTab);
	tabView->AddTab(countryTab);

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, 3)
			.Add(tabView)
			.AddGroup(B_HORIZONTAL, 3)
				.Add(button)
				.Add(fRevertButton)
				.AddGlue()
				.End()
			.SetInsets(5, 5, 5, 5)
			.End();

	CenterOnScreen();
}


LocaleWindow::~LocaleWindow()
{
	delete fMsgPrefLanguagesChanged;
}


void
LocaleWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgDefaults:
			// TODO
			break;

		case kMsgRevert:
			// TODO
			break;

		case kMsgPrefLanguagesChanged:
		{
			BMessage update(kMsgSettingsChanged);
			int index = 0;
			while (index < fPreferredListView->FullListCountItems()) {
				// only include subitems : we can guess the superitem
				// from them anyway
				if (fPreferredListView->Superitem(
						fPreferredListView->FullListItemAt(index)) != NULL) {
					update.AddString("language", static_cast<LanguageListItem*>(
							fPreferredListView->FullListItemAt(index))
						->LanguageCode());
				}
				index++;
			}
			fLanguageListView->SortItems(compare_list_items);
			be_app_messenger.SendMessage(&update);
			break;
		}

		case kMsgCountrySelection:
		{
			// Country selection changed.
			// Get the new selected country from the ListView and send it to the
			// main app event handler.
			void* ptr;
			message->FindPointer("source", &ptr);
			BListView* countryList = static_cast<BListView*>(ptr);
			LanguageListItem* lli = static_cast<LanguageListItem*>
				(countryList->ItemAt(countryList->CurrentSelection()));
			BMessage newMessage(kMsgSettingsChanged);
			newMessage.AddString("country",lli->LanguageCode());
			be_app_messenger.SendMessage(&newMessage);

			BCountry* country = new BCountry(lli->LanguageCode());
			fFormatView->SetCountry(country);
			break;
		}

		case kMsgLangInvoked:
		{
			int32 index = 0;
			if (message->FindInt32("index", &index) == B_OK) {
				LanguageListItem* listItem
					= static_cast<LanguageListItem*>
						(fLanguageListView->RemoveItem(index));
				fPreferredListView->AddItem(listItem);
				fPreferredListView->Invoke(fMsgPrefLanguagesChanged);
			}
			break;
		}

		case kMsgPrefLangInvoked:
		{
			if (fPreferredListView->CountItems() == 1)
				break;

			int32 index = 0;
			if (message->FindInt32("index", &index) == B_OK) {
				LanguageListItem* listItem
					= static_cast<LanguageListItem*>
						(fPreferredListView->RemoveItem(index));
				fLanguageListView->AddItem(listItem);
				fLanguageListView->SortItems(compare_list_items);
					// see previous comment on sort using collators
				fPreferredListView->Invoke(fMsgPrefLanguagesChanged);
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
LocaleWindow::FrameMoved(BPoint newPosition)
{
	BMessage update(kMsgSettingsChanged);
	update.AddPoint("window_location", newPosition);
	be_app_messenger.SendMessage(&update);
}

