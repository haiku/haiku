/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Locale.h"
#include "LocaleWindow.h"

#include <iostream>

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <OutlineListView.h>
#include <Screen.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TabView.h>

#include <ICUWrapper.h>

#include <unicode/locid.h>
#include <unicode/datefmt.h>

#include "TimeFormatSettingsView.h"


#define TR_CONTEXT "Locale Preflet Window"
#define MAX_DRAG_HEIGHT		200.0
#define ALPHA				170
#define TEXT_OFFSET			5.0


class LanguageListItem: public BStringItem
{
	public:
		LanguageListItem(const char* text, const char* code)
		:
		BStringItem(text),
		fLanguageCode(code)
		{}

		LanguageListItem(const LanguageListItem& other)
			:
			BStringItem(other.Text()),
			fLanguageCode(other.fLanguageCode)
		{}

		~LanguageListItem() {};

		const inline BString LanguageCode() { return fLanguageCode; }

	private:
		const BString fLanguageCode;
};


static int
compare_list_items(const void* _a, const void* _b)
{
	LanguageListItem* a = *(LanguageListItem**)_a;
	LanguageListItem* b = *(LanguageListItem**)_b;
	return strcasecmp(a->Text(), b->Text());
}


class LanguageListView: public BOutlineListView
{
	public:
		LanguageListView(const char* name, list_view_type type)
			:
			BOutlineListView(name, type),
			fMsgPrefLanguagesChanged(new BMessage(kMsgPrefLanguagesChanged))
		{}

		~LanguageListView() { delete fMsgPrefLanguagesChanged; }

		bool InitiateDrag(BPoint point, int32 index, bool wasSelected);
		void MouseMoved(BPoint where, uint32 transit, const BMessage* msg);
		void MoveItems(BList& items, int32 index);
		void MoveItemFrom(BOutlineListView* origin, int32 index, int32 dropSpot = 0);
		void AttachedToWindow()
		{
			BOutlineListView::AttachedToWindow();
			ScrollToSelection();
		}

		void MessageReceived (BMessage* message)
		{
			if (message->what == 'DRAG') {
				// Someone just dropped something on us
				LanguageListView* list = NULL;
				if (message->FindPointer("list", (void**)&list) == B_OK) {
					// It comes from a list
					if (list == this) {
						// It comes from ourselves : move the item around in the list
						int32 count = CountItems();
						if (fDropIndex < 0 || fDropIndex > count)
							fDropIndex = count;

						BList items;
						int32 index;
						for (int32 i = 0; message->FindInt32("index", i, &index)
							 	== B_OK; i++)
							if (BListItem* item = FullListItemAt(index))
								items.AddItem((void*)item);

						if (items.CountItems() > 0) {
							// There is something to move
							LanguageListItem* parent =
								static_cast<LanguageListItem*>(Superitem(
									static_cast<LanguageListItem*>(
										items.FirstItem())));
							if (parent) {
								// item has a parent - it should then stay
								// below it
								if (Superitem(FullListItemAt(fDropIndex - 1))
										== parent || FullListItemAt(fDropIndex - 1) == parent)
									MoveItems(items, fDropIndex);
							} else {
								// item is top level and should stay so.
								if (Superitem(FullListItemAt(fDropIndex - 1)) == NULL)
									MoveItems(items, fDropIndex);
								else {
									int itemCount = CountItemsUnder(
										FullListItemAt(fDropIndex), true);
									MoveItems(items, FullListIndexOf(
										Superitem(FullListItemAt(fDropIndex - 1))+itemCount));
								}
							}
						}
						fDropIndex = -1;
					} else {
						// It comes from another list : move it here
						int32 count = CountItems();
						if (fDropIndex < 0 || fDropIndex > count)
							fDropIndex = count;

						// ensure we always drop things at top-level and not
						// in the middle of another outline
						if (Superitem(FullListItemAt(fDropIndex))) {
							// Item has a parent
							fDropIndex = FullListIndexOf(Superitem(FullListItemAt(fDropIndex)));
						}
						
						// Item is now a top level one - we must insert just below its last child
						fDropIndex += CountItemsUnder(FullListItemAt(fDropIndex),false)  + 1;

						int32 index;
						for (int32 i = 0; message->FindInt32("index", i, &index)
								== B_OK; i++) {
							MoveItemFrom(list,index,fDropIndex);
							fDropIndex++;
						}

						fDropIndex = -1;
					}
					Invoke(fMsgPrefLanguagesChanged);
				}
			} else BOutlineListView::MessageReceived(message);
		}
	private:
		int32		fDropIndex;
		BMessage*	fMsgPrefLanguagesChanged;
};


void
LanguageListView::MoveItems(BList& items, int32 index)
{
	// TODO : only allow moving top level item around other top level
	// or sublevels within the same top level

	DeselectAll();
	// we remove the items while we look at them, the insertion index is
	// decreaded when the items index is lower, so that we insert at the right
	// spot after removal
	BList removedItems;
	int32 count = items.CountItems();
	// We loop in the reverse way so we can remove childs before their parents
	for (int32 i = count - 1; i >= 0; i--) {
		BListItem* item = (BListItem*)items.ItemAt(i);
		int32 removeIndex = IndexOf(item);
		// TODO : remove all childs before removing the item itself, or else
		// they will be lost forever
		if (RemoveItem(item) && removedItems.AddItem((void*)item, 0)) {
			if (removeIndex < index)
				index--;
		}
		// else ??? -> blow up
	}
	for (int32 i = 0;
		 BListItem* item = (BListItem*)removedItems.ItemAt(i); i++) {
		if (AddItem(item, index)) {
			// after we're done, the newly inserted items will be selected
			Select(index, true);
			// next items will be inserted after this one
			index++;
		} else
			delete item;
	}
}


void
LanguageListView::MoveItemFrom(BOutlineListView* origin, int32 index,
	int32 dropSpot)
{
	// Check that the node we are going to move is a top-level one.
	// If not, we want his parent instead
	LanguageListItem* itemToMove = static_cast<LanguageListItem*>(
		origin->Superitem(origin->FullListItemAt(index)));
	if (itemToMove == NULL) {
		itemToMove = static_cast<LanguageListItem*>(
			origin->FullListItemAt(index));
	} else
		index = origin->FullListIndexOf(itemToMove);

	int itemCount = origin->CountItemsUnder(itemToMove, true);
	LanguageListItem* newItem = new LanguageListItem(*itemToMove);
	this->AddItem(newItem, dropSpot);
	newItem->SetExpanded(itemToMove->IsExpanded());

	for (int i = 0; i < itemCount ; i++) {
		LanguageListItem* subItem = static_cast<LanguageListItem*>(
			origin->ItemUnderAt(itemToMove, true, i));
		this->AddUnder(new LanguageListItem(*subItem),newItem);
	}
	origin->RemoveItem(index);
		// This will also remove the children
}


bool
LanguageListView::InitiateDrag(BPoint point, int32 index, bool)
{
	bool success = false;
	BListItem* item = FullListItemAt(CurrentSelection(0));
	if (!item) {
		// workarround a timing problem
		Select(index);
		item = FullListItemAt(index);
	}
	if (item) {
		// create drag message
		BMessage msg('DRAG');
		msg.AddPointer("list",(void*)(this));
		int32 index;
		for (int32 i = 0; (index = FullListCurrentSelection(i)) >= 0; i++) {
			msg.AddInt32("index", index);
		}
		// figure out drag rect
		float width = Bounds().Width();
		BRect dragRect(0.0, 0.0, width, -1.0);
		// figure out, how many items fit into our bitmap
		int32 numItems;
		bool fade = false;
		for (numItems = 0; BListItem* item = FullListItemAt(CurrentSelection(numItems));
				numItems++) {
			dragRect.bottom += ceilf(item->Height()) + 1.0;
			if (dragRect.Height() > MAX_DRAG_HEIGHT) {
				fade = true;
				dragRect.bottom = MAX_DRAG_HEIGHT;
				numItems++;
				break;
			}
		}
		BBitmap* dragBitmap = new BBitmap(dragRect, B_RGB32, true);
		if (dragBitmap && dragBitmap->IsValid()) {
			if (BView* v = new BView(dragBitmap->Bounds(), "helper",
									 B_FOLLOW_NONE, B_WILL_DRAW)) {
				dragBitmap->AddChild(v);
				dragBitmap->Lock();
				BRect itemBounds(dragRect) ;
				itemBounds.bottom = 0.0;
				// let all selected items, that fit into our drag_bitmap, draw
				for (int32 i = 0; i < numItems; i++) {
					int32 index = FullListCurrentSelection(i);
					LanguageListItem* item
						= static_cast<LanguageListItem*>(FullListItemAt(index));
					itemBounds.bottom = itemBounds.top + ceilf(item->Height());
					if (itemBounds.bottom > dragRect.bottom)
						itemBounds.bottom = dragRect.bottom;
					item->DrawItem(v, itemBounds);
					itemBounds.top = itemBounds.bottom + 1.0;
				}
				// make a black frame arround the edge
				v->SetHighColor(0, 0, 0, 255);
				v->StrokeRect(v->Bounds());
				v->Sync();
	
				uint8* bits = (uint8*)dragBitmap->Bits();
				int32 height = (int32)dragBitmap->Bounds().Height() + 1;
				int32 width = (int32)dragBitmap->Bounds().Width() + 1;
				int32 bpr = dragBitmap->BytesPerRow();
	
				if (fade) {
					for (int32 y = 0; y < height - ALPHA / 2;
							y++, bits += bpr) {
						uint8* line = bits + 3;
						for (uint8* end = line + 4 * width; line < end;
								line += 4)
							*line = ALPHA;
					}
					for (int32 y = height - ALPHA / 2; y < height;
							y++, bits += bpr) {
						uint8* line = bits + 3;
						for (uint8* end = line + 4 * width; line < end;
								line += 4)
							*line = (height - y) << 1;
					}
				} else {
					for (int32 y = 0; y < height; y++, bits += bpr) {
						uint8* line = bits + 3;
						for (uint8* end = line + 4 * width; line < end;
								line += 4)
							*line = ALPHA;
					}
				}
				dragBitmap->Unlock();
			}
		} else {
			delete dragBitmap;
			dragBitmap = NULL;
		}
		if (dragBitmap)
			DragMessage(&msg, dragBitmap, B_OP_ALPHA, BPoint(0.0, 0.0));
		else
			DragMessage(&msg, dragRect.OffsetToCopy(point), this);

		success = true;
	}
	return success;
}


void
LanguageListView::MouseMoved(BPoint where, uint32 transit, const BMessage* msg)
{
	if (msg && (msg->what == 'DRAG')) {
		switch (transit) {
			case B_ENTERED_VIEW:
			case B_INSIDE_VIEW: {
				// set drop target through virtual function
				// offset where by half of item height
				BRect r = ItemFrame(0);
				where.y += r.Height() / 2.0;
	
				int32 index = FullListIndexOf(where);
				if (index < 0)
					index = FullListCountItems();
				if (fDropIndex != index) {
					fDropIndex = index;
					if (fDropIndex >= 0) {
						int32 count = FullListCountItems();
						if (fDropIndex == count) {
							BRect r;
							if (FullListItemAt(count - 1)) {
								r = ItemFrame(count - 1);
								r.top = r.bottom;
								r.bottom = r.top + 1.0;
							} else {
								r = Bounds();
								r.bottom--;
									// compensate for scrollbars moved slightly
									// out of window
							}
						} else {
							BRect r = ItemFrame(fDropIndex);
							r.top--;
							r.bottom = r.top + 1.0;
						}
					}
				}
				break;
			}
		}
	} else {
		BOutlineListView::MouseMoved(where, transit, msg);
	}
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

				// Now get an human-readable, loacalized name for each language
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
					lastAddedLanguage = si;
				}

				delete currentLanguage;
			}

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
		for (int index = 0; msg.FindString("language", index, &langCode)
					== B_OK;
				index++) {
			for (int listPos = 0; LanguageListItem* lli
					= static_cast<LanguageListItem*>
						(fLanguageListView->FullListItemAt(listPos));
					listPos++) {
				if (langCode == lli->LanguageCode()) {
					// We found the item we were looking for, now move it to
					// the other list along with all its children
					static_cast<LanguageListView*>(fPreferredListView)
						-> MoveItemFrom(fLanguageListView, fLanguageListView
							-> FullListIndexOf(lli));
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
					if (fPreferredListView->Superitem(fPreferredListView->
								FullListItemAt(index))
							!= NULL) {
						update.AddString("language",
							static_cast<LanguageListItem*>
								(fPreferredListView->FullListItemAt(index))
							-> LanguageCode());
					}
					index++;
				}
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
				fPreferredListView
					->Invoke(fMsgPrefLanguagesChanged);
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
				fPreferredListView
					->Invoke(fMsgPrefLanguagesChanged);
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

