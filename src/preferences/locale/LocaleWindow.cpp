/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Locale.h"
#include "LocaleWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ListView.h>
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
#define MAX_DRAG_HEIGHT		200.0
#define ALPHA				170
#define TEXT_OFFSET			5.0

// Language lists management


// This is a language item. It's a BStringItem but we also want to keep the
// language code and not only the displayName
class LanguageListItem: public BStringItem
{
	public:
		LanguageListItem(const char* display, const char* code)
			: BStringItem(display)
			, LanguageCode(code)
		{}

		~LanguageListItem() {};

		const inline BString getLanguageCode() { return LanguageCode; }
		void Draw(BView *owner, BRect frame);
	private:
		const BString LanguageCode;
};


void
LanguageListItem::Draw(BView *owner, BRect frame)
{
	owner->SetLowColor(255, 255, 255, 255);
	owner->FillRect(frame, B_SOLID_LOW);
	// label
	owner->SetHighColor(0, 0, 0, 255);
	font_height fh;
	owner->GetFontHeight(&fh);
	const char* text = Text();
	BString truncatedString(text);
	owner->TruncateString(&truncatedString, B_TRUNCATE_MIDDLE,
		frame.Width() - TEXT_OFFSET - 4.0);
	float height = frame.Height();
	float textHeight = fh.ascent + fh.descent;
	BPoint textPoint;
	textPoint.x = frame.left + TEXT_OFFSET;
	textPoint.y = frame.top
		+ ceilf(height / 2.0 - textHeight / 2.0 + fh.ascent);
	owner->DrawString(truncatedString.String(), textPoint);
}


// This is a language list. Basically, a drag-n-drop enabled list.
class LanguageListView: public BListView
{
	public:
		LanguageListView(const char* name, list_view_type type)
			: BListView(name, type)
		{}

		bool InitiateDrag(BPoint point, int32 index, bool wasSelected);
		void MouseMoved(BPoint where, uint32 transit, const BMessage *msg);
		void MoveItems(BList& items, int32 index);
		void AttachedToWindow()
		{
			BListView::AttachedToWindow();
			ScrollToSelection();
		}

		void MessageReceived (BMessage* message)
		{
			if (message->what == 'DRAG') {
				LanguageListView *list = NULL;
				if (message->FindPointer("list", (void **)&list) == B_OK) {
					if (list == this) {
						int32 count = CountItems();
						if (fDropIndex < 0 || fDropIndex > count)
							fDropIndex = count;

						// gather all the items from the BMessage
						BList items;
						int32 index;
						for (int32 i = 0; message->FindInt32("index", i, &index)
							 	== B_OK; i++)
							if (BListItem* item = ItemAt(index))
								items.AddItem((void*)item);

						// handle them
						if (items.CountItems() > 0) {
							MoveItems(items, fDropIndex);
						}
						fDropIndex = -1;
					} else {
						int32 count = CountItems();
						if (fDropIndex < 0 || fDropIndex > count)
							fDropIndex = count;

						// gather all the items from the BMessage
						int32 index;
						for (int32 i = 0; message->FindInt32("index", i, &index)
								== B_OK; i++)
							if (BListItem* item = list->RemoveItem(index)) {
								AddItem(item, fDropIndex);
								fDropIndex++;
							}

						fDropIndex = -1;
					}
				}
			} else BListView::MessageReceived(message);
		}
	private:
		int32 fDropIndex;
};


void
LanguageListView::MoveItems(BList& items, int32 index)
{
	DeselectAll();
	// we remove the items while we look at them, the insertion index is
	// decreaded when the items index is lower, so that we insert at the right
	// spot after removal
	BList removedItems;
	int32 count = items.CountItems();
	for (int32 i = 0; i < count; i++) {
		BListItem* item = (BListItem*)items.ItemAt(i);
		int32 removeIndex = IndexOf(item);
		if (RemoveItem(item) && removedItems.AddItem((void*)item)) {
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


bool
LanguageListView::InitiateDrag(BPoint point, int32 index, bool)
{
	bool success = false;
	BListItem* item = ItemAt(CurrentSelection(0));
	if (!item) {
		// workarround a timing problem
		Select(index);
		item = ItemAt(index);
	}
	if (item) {
		// create drag message
		BMessage msg('DRAG');
		msg.AddPointer("list",(void*)(this));
		int32 index;
		for (int32 i = 0; (index = CurrentSelection(i)) >= 0; i++)
			msg.AddInt32("index", index);
		// figure out drag rect
		float width = Bounds().Width();
		BRect dragRect(0.0, 0.0, width, -1.0);
		// figure out, how many items fit into our bitmap
		int32 numItems;
		bool fade = false;
		for (numItems = 0; BListItem* item = ItemAt(CurrentSelection(numItems));
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
			if (BView *v = new BView(dragBitmap->Bounds(), "helper",
									 B_FOLLOW_NONE, B_WILL_DRAW)) {
				dragBitmap->AddChild(v);
				dragBitmap->Lock();
				BRect itemBounds(dragRect) ;
				itemBounds.bottom = 0.0;
				// let all selected items, that fit into our drag_bitmap, draw
				for (int32 i = 0; i < numItems; i++) {
					int32 index = CurrentSelection(i);
					LanguageListItem* item
						= static_cast<LanguageListItem*>(ItemAt(index));
					itemBounds.bottom = itemBounds.top + ceilf(item->Height());
					if (itemBounds.bottom > dragRect.bottom)
						itemBounds.bottom = dragRect.bottom;
					item->Draw(v, itemBounds);
					itemBounds.top = itemBounds.bottom + 1.0;
				}
				// make a black frame arround the edge
				v->SetHighColor(0, 0, 0, 255);
				v->StrokeRect(v->Bounds());
				v->Sync();
	
				uint8 *bits = (uint8 *)dragBitmap->Bits();
				int32 height = (int32)dragBitmap->Bounds().Height() + 1;
				int32 width = (int32)dragBitmap->Bounds().Width() + 1;
				int32 bpr = dragBitmap->BytesPerRow();
	
				if (fade) {
					for (int32 y = 0; y < height - ALPHA / 2;
							y++, bits += bpr) {
						uint8 *line = bits + 3;
						for (uint8 *end = line + 4 * width; line < end;
								line += 4)
							*line = ALPHA;
					}
					for (int32 y = height - ALPHA / 2; y < height;
							y++, bits += bpr) {
						uint8 *line = bits + 3;
						for (uint8 *end = line + 4 * width; line < end;
								line += 4)
							*line = (height - y) << 1;
					}
				} else {
					for (int32 y = 0; y < height; y++, bits += bpr) {
						uint8 *line = bits + 3;
						for (uint8 *end = line + 4 * width; line < end;
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
LanguageListView::MouseMoved(BPoint where, uint32 transit, const BMessage *msg)
{
	if (msg && (msg->what == 'DRAG')) {
		switch (transit) {
			case B_ENTERED_VIEW:
			case B_INSIDE_VIEW: {
				// set drop target through virtual function
				// offset where by half of item height
				BRect r = ItemFrame(0);
				where.y += r.Height() / 2.0;
	
				int32 index = IndexOf(where);
				if (index < 0)
					index = CountItems();
				if (fDropIndex != index) {
					fDropIndex = index;
					if (fDropIndex >= 0) {
						int32 count = CountItems();
						if (fDropIndex == count) {
							BRect r;
							if (ItemAt(count - 1)) {
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
		BListView::MouseMoved(where, transit, msg);
	}
}


const static uint32 kMsgSelectLanguage = 'slng';
const static uint32 kMsgDefaults = 'dflt';
const static uint32 kMsgRevert = 'revt';


LocaleWindow::LocaleWindow(BRect rect)
	: BWindow(rect, "Locale", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
			| B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	// Buttons at the bottom

	BButton *button = new BButton(TR("Defaults"), new BMessage(kMsgDefaults));

	fRevertButton = new BButton(TR("Revert"), new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);

	// Tabs
	BTabView *tabView = new BTabView("tabview");

	// Language tab
	BView *tab = new BView(TR("Language"), B_WILL_DRAW);
	//tab->SetViewColor(tabView->ViewColor());
	tab->SetLayout(new BGroupLayout(B_VERTICAL, 0));
	tabView->AddTab(tab);

	{
		// first list: available languages
		LanguageListView *listView = new LanguageListView("available",
			B_MULTIPLE_SELECTION_LIST);
		BScrollView *scrollView = new BScrollView("scroller", listView,
			0, false, true, B_FANCY_BORDER);

		// Fill the language list from the LocaleRoster data
		BMessage installedLanguages;
		if (be_locale_roster->GetInstalledLanguages(&installedLanguages)
				== B_OK) {

			BString currentLanguage;
			for (int i = 0; installedLanguages.FindString("langs",
					i, &currentLanguage) == B_OK; i++) {

				// Now get an human-readable, loacalized name for each language
				// TODO: sort them using collators.
				Locale currentLocale
					= Locale::createFromName(currentLanguage.String());
				UnicodeString languageFullName;
				BString str;
				BStringByteSink bbs(&str);
				currentLocale.getDisplayName(languageFullName);
				languageFullName.toUTF8(bbs);
				LanguageListItem* si
					= new LanguageListItem(str, currentLanguage.String());
				listView->AddItem(si);
			}

		} else {
			BAlert* myAlert = new BAlert("Error",
				TR("Unable to find the available languages! You can't use this "
					"preflet!"),
				TR("Ok"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
			myAlert->Go();
		}

		// Second list: active languages
		fPreferredListView = new LanguageListView("preferred",
			B_MULTIPLE_SELECTION_LIST);
		BScrollView *scrollViewEnabled = new BScrollView("scroller",
			fPreferredListView, 0, false, true, B_FANCY_BORDER);

		// get the preferred languages from the Settings. Move them here from
		// the other list.
		BMessage msg;
		be_locale_roster->GetPreferredLanguages(&msg);
		BString langCode;
		for (int index = 0; msg.FindString("language", index, &langCode)
					== B_OK;
				index++) {
			for (int listPos = 0; LanguageListItem* lli
					= static_cast<LanguageListItem*>(listView->ItemAt(listPos));
					listPos++) {
				if (langCode == lli->getLanguageCode()) {
					fPreferredListView->AddItem(lli);
					listView->RemoveItem(lli);
				}
			}
		}

		tab->AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.Add(BGroupLayoutBuilder(B_VERTICAL, 10)
				.Add(new BStringView("", TR("Available languages")))
				.Add(scrollView)
			)
			.Add(BGroupLayoutBuilder(B_VERTICAL, 10)
				.Add(new BStringView("", TR("Preferred languages")))
				.Add(scrollViewEnabled)
			)
		);

	}

	// Country tab
	tab = new BView(TR("Country"), B_WILL_DRAW);
	//tab->SetViewColor(tabView->ViewColor());
	tab->SetLayout(new BGroupLayout(B_VERTICAL, 0));
	tabView->AddTab(tab);

	{
		BListView* listView = new BListView("country", B_SINGLE_SELECTION_LIST);
		BScrollView *scrollView = new BScrollView("scroller",
			listView, 0, false, true, B_FANCY_BORDER);

		BMessage* msg = new BMessage('csel');
		listView->SetSelectionMessage(msg);

		
		// get all available countries from ICU
		// Use DateFormat::getAvailableLocale so we get only the one we can
		// use. Maybe check the NumberFormat one and see if there is more.
		int32_t localeCount;
		const Locale* currentLocale
			= Locale::getAvailableLocales(localeCount);
		BCountry* defaultCountry;
		be_locale_roster->GetDefaultCountry(&defaultCountry);

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

		fTimeFormatSettings
			= new TimeFormatSettingsView(defaultCountry);

		tab->AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 5)
			.Add(scrollView)
			.Add(new BScrollView("advanced", fTimeFormatSettings, 0,
				false, true, B_NO_BORDER))
		);

		listView->ScrollToSelection();
	}

	// check if the window is on screen
	rect = BScreen().Frame();
	rect.right -= 20;
	rect.bottom -= 20;

	BPoint position = Frame().LeftTop();
	if (!rect.Contains(position)) {
		// center window on screen as it doesn't fit on the saved position
		position.x = (rect.Width() - Bounds().Width()) / 2;
		position.y = (rect.Height() - Bounds().Height()) / 2;
	}
	MoveTo(position);

	// Layout management
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 3)
		.Add(tabView)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 3)
			.Add(button)
			.Add(fRevertButton)
			.AddGlue()
		)
		.SetInsets(5, 5, 5, 5)
	);
}


bool
LocaleWindow::QuitRequested()
{
	BMessage update(kMsgSettingsChanged);
	update.AddRect("window_frame", Frame());
	int index = 0;
	while (index < fPreferredListView->CountItems()) {
		update.AddString("language", static_cast<LanguageListItem*>
			(fPreferredListView->ItemAt(index))->getLanguageCode());
		index++;
	}
	// TODO also save Country tab settings
	be_app_messenger.SendMessage(&update);

	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	return true;
}


void
LocaleWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgDefaults:
			// reset default settings
			break;

		case kMsgRevert:
			// revert to last settings
			break;

		case 'csel':
		{
			// Country selection changed.
			// Get the new selected country from the ListView and send it to the
			// main app event handler.
			void* ptr;
			message->FindPointer("source", &ptr);
			BListView* countryList = static_cast<BListView*>(ptr);
			LanguageListItem* lli = static_cast<LanguageListItem*>
				(countryList->ItemAt(countryList->CurrentSelection()));
			BMessage* newMessage = new BMessage(kMsgSettingsChanged);
			newMessage->AddString("country",lli->getLanguageCode());
			be_app_messenger.SendMessage(newMessage);

			BCountry* country = new BCountry(lli->getLanguageCode());
			fTimeFormatSettings->SetCountry(country);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

