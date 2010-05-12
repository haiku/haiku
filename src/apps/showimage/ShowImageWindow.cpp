/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Copyright 2004-2005 yellowTAB GmbH. All Rights Reserverd.
 * Copyright 2006 Bernd Korz. All Rights Reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 *		yellowTAB GmbH
 *		Bernd Korz
 */

#include "ShowImageWindow.h"

#include <new>
#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <PrintJob.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>
#include <SupportDefs.h>
#include <TranslationDefs.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <stdlib.h> // for bs_printf()

#include "EntryMenuItem.h"
#include "ResizerWindow.h"
#include "ShowImageApp.h"
#include "ShowImageConstants.h"
#include "ShowImageStatusView.h"
#include "ShowImageView.h"


// #pragma mark -- ShowImageWindow::RecentDocumentsMenu

class ShowImageWindow::RecentDocumentsMenu : public BMenu {
public:
			RecentDocumentsMenu(const char* title,
				menu_layout layout = B_ITEMS_IN_COLUMN);
	bool	AddDynamicItem(add_state addState);

private:
	void	UpdateRecentDocumentsMenu();
};


ShowImageWindow::RecentDocumentsMenu::RecentDocumentsMenu(const char* title,
	menu_layout layout)
	:
	BMenu(title, layout)
{
}


bool
ShowImageWindow::RecentDocumentsMenu::AddDynamicItem(add_state addState)
{
	if (addState != B_INITIAL_ADD)
		return false;

	while (CountItems() > 0)
		delete RemoveItem(0L);

	BMenuItem* item;
	BMessage list, *msg;
	entry_ref ref;
	char name[B_FILE_NAME_LENGTH];

	be_roster->GetRecentDocuments(&list, 20, NULL, kApplicationSignature);
	for (int i = 0; list.FindRef("refs", i, &ref) == B_OK; i++) {
		BEntry entry(&ref);
		if (entry.Exists() && entry.GetName(name) == B_OK) {
			msg = new BMessage(B_REFS_RECEIVED);
			msg->AddRef("refs", &ref);
			item = new EntryMenuItem(&ref, name, msg, 0, 0);
			AddItem(item);
			item->SetTarget(be_app, NULL);
		}
	}

	return false;
}


//	#pragma mark -- ShowImageWindow


// BMessage field names used in Save messages
const char* kTypeField = "be:type";
const char* kTranslatorField = "be:translator";


ShowImageWindow::ShowImageWindow(const entry_ref* ref,
	const BMessenger& trackerMessenger)
	:
	BWindow(BRect(5, 24, 250, 100), "", B_DOCUMENT_WINDOW, 0),
	fSavePanel(NULL),
	fBar(NULL),
	fOpenMenu(NULL),
	fBrowseMenu(NULL),
	fGoToPageMenu(NULL),
	fSlideShowDelay(NULL),
	fImageView(NULL),
	fStatusView(NULL),
	fModified(false),
	fFullScreen(false),
	fShowCaption(true),
	fPrintSettings(NULL),
	fResizerWindowMessenger(NULL),
	fResizeItem(NULL),
	fHeight(0),
	fWidth(0)
{
	_LoadSettings();

	// create menu bar
	fBar = new BMenuBar(BRect(0, 0, Bounds().right, 1), "menu_bar");
	AddMenus(fBar);
	AddChild(fBar);

	BRect viewFrame = Bounds();
	viewFrame.top = fBar->Bounds().Height() + 1;
	viewFrame.right -= B_V_SCROLL_BAR_WIDTH;
	viewFrame.bottom -= B_H_SCROLL_BAR_HEIGHT;

	// create the image view
	fImageView = new ShowImageView(viewFrame, "image_view", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED);
	// wrap a scroll view around the view
	BScrollView* scrollView = new BScrollView("image_scroller", fImageView,
		B_FOLLOW_ALL, 0, false, false, B_PLAIN_BORDER);
	AddChild(scrollView);

	const int32 kstatusWidth = 190;
	BRect rect;
	rect = Bounds();
	rect.top	= viewFrame.bottom + 1;
	rect.left 	= viewFrame.left + kstatusWidth;
	rect.right	= viewFrame.right + 1;
	rect.bottom += 1;
	BScrollBar* horizontalScrollBar = new BScrollBar(rect, "hscroll",
		fImageView, 0, 150, B_HORIZONTAL);
	AddChild(horizontalScrollBar);

	rect.left = 0;
	rect.right = kstatusWidth - 1;
	rect.bottom -= 1;
	fStatusView = new ShowImageStatusView(rect, "status_view", B_FOLLOW_BOTTOM,
		B_WILL_DRAW);
	AddChild(fStatusView);

	rect = Bounds();
	rect.top	= viewFrame.top - 1;
	rect.left	= viewFrame.right + 1;
	rect.bottom	= viewFrame.bottom + 1;
	rect.right	+= 1;
	BScrollBar* verticalScrollBar = new BScrollBar(rect, "vscroll", fImageView,
		0, 150, B_VERTICAL);
	AddChild(verticalScrollBar);

	SetSizeLimits(250, 100000, 100, 100000);

	// finish creating the window
	fImageView->SetImage(ref);
	fImageView->SetTrackerMessenger(trackerMessenger);

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "LoadAlerts"

	if (InitCheck() != B_OK) {
		BAlert* alert;
		alert = new BAlert(
			B_TRANSLATE("ShowImage"),
			B_TRANSLATE("Could not load image! Either the file or an image "
				"translator for it does not exist."),
			B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
		alert->Go();

		// quit if file could not be opened
		Quit();
		return;
	}

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Menus"

	// add View menu here so it can access ShowImageView methods
	BMenu* menu = new BMenu(B_TRANSLATE("View"));
	_BuildViewMenu(menu, false);
	fBar->AddItem(menu);
	_MarkMenuItem(fBar, MSG_DITHER_IMAGE, fImageView->GetDither());
	UpdateTitle();

	SetPulseRate(100000);
		// every 1/10 second; ShowImageView needs it for marching ants

	WindowRedimension(fImageView->GetBitmap());
	fImageView->MakeFocus(true); // to receive KeyDown messages
	Show();

	// Tell application object to query the clipboard
	// and tell this window if it contains interesting data or not
	be_app_messenger.SendMessage(B_CLIPBOARD_CHANGED);
}


ShowImageWindow::~ShowImageWindow()
{
	delete fResizerWindowMessenger;
}


status_t
ShowImageWindow::InitCheck()
{
	if (!fImageView || fImageView->GetBitmap() == NULL)
		return B_ERROR;

	return B_OK;
}


void
ShowImageWindow::UpdateTitle()
{
	BString path;
	fImageView->GetPath(&path);
	SetTitle(path.String());
}


void
ShowImageWindow::BuildContextMenu(BMenu* menu)
{
	_BuildViewMenu(menu, true);
}


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Menus"

void
ShowImageWindow::_BuildViewMenu(BMenu* menu, bool popupMenu)
{
	_AddItemMenu(menu, B_TRANSLATE("Slide show"), MSG_SLIDE_SHOW, 0, 0, this);
	_MarkMenuItem(menu, MSG_SLIDE_SHOW, fImageView->SlideShowStarted());
	BMenu* delayMenu = new BMenu(B_TRANSLATE("Slide delay"));
	if (fSlideShowDelay == NULL)
		fSlideShowDelay = delayMenu;

	delayMenu->SetRadioMode(true);
	// Note: ShowImage loads images in window thread so it becomes unresponsive
	//		 if slide show delay is too short! (Especially if loading the image
	//		 takes as long as or longer than the slide show delay). Should load
	//		 in background thread!
	_AddDelayItem(delayMenu, B_TRANSLATE("3 seconds"), 3);
	_AddDelayItem(delayMenu, B_TRANSLATE("4 seconds"), 4);
	_AddDelayItem(delayMenu, B_TRANSLATE("5 seconds"), 5);
	_AddDelayItem(delayMenu, B_TRANSLATE("6 seconds"), 6);
	_AddDelayItem(delayMenu, B_TRANSLATE("7 seconds"), 7);
	_AddDelayItem(delayMenu, B_TRANSLATE("8 seconds"), 8);
	_AddDelayItem(delayMenu, B_TRANSLATE("9 seconds"), 9);
	_AddDelayItem(delayMenu, B_TRANSLATE("10 seconds"), 10);
	_AddDelayItem(delayMenu, B_TRANSLATE("20 seconds"), 20);
	menu->AddItem(delayMenu);

	menu->AddSeparatorItem();

	_AddItemMenu(menu, B_TRANSLATE("Original size"),
		MSG_ORIGINAL_SIZE, '1', 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Zoom in"), MSG_ZOOM_IN, '+', 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Zoom out"), MSG_ZOOM_OUT, '-', 0, this);

	menu->AddSeparatorItem();

	_AddItemMenu(menu, B_TRANSLATE("High-quality zooming"),
		MSG_SCALE_BILINEAR, 0, 0, this);

	menu->AddSeparatorItem();

	_AddItemMenu(menu, B_TRANSLATE("Shrink to window"),
		MSG_SHRINK_TO_WINDOW, 0, 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Zoom to window"),
		MSG_ZOOM_TO_WINDOW, 0, 0, this);

	menu->AddSeparatorItem();

	_AddItemMenu(menu, B_TRANSLATE("Full screen"),
		MSG_FULL_SCREEN, B_ENTER, 0, this);
	_MarkMenuItem(menu, MSG_FULL_SCREEN, fFullScreen);

	_AddItemMenu(menu, B_TRANSLATE("Show caption in full screen mode"),
		MSG_SHOW_CAPTION, 0, 0, this);
	_MarkMenuItem(menu, MSG_SHOW_CAPTION, fShowCaption);

	_MarkMenuItem(menu, MSG_SCALE_BILINEAR, fImageView->GetScaleBilinear());

	bool shrink, zoom, enabled;

	shrink = fImageView->GetShrinkToBounds();
	zoom = fImageView->GetZoomToBounds();
	_MarkMenuItem(menu, MSG_SHRINK_TO_WINDOW, shrink);
	_MarkMenuItem(menu, MSG_ZOOM_TO_WINDOW, zoom);

	enabled = !(shrink || zoom);
	_EnableMenuItem(menu, MSG_ORIGINAL_SIZE, enabled);
	_EnableMenuItem(menu, MSG_ZOOM_IN, enabled);
	_EnableMenuItem(menu, MSG_ZOOM_OUT, enabled);

	if (popupMenu) {
		menu->AddSeparatorItem();
		_AddItemMenu(menu, B_TRANSLATE("Use as background" B_UTF8_ELLIPSIS),
			MSG_DESKTOP_BACKGROUND, 0, 0, this);
	}
}


void
ShowImageWindow::AddMenus(BMenuBar* bar)
{
	BMenu* menu = new BMenu(B_TRANSLATE("File"));
	fOpenMenu = new RecentDocumentsMenu(B_TRANSLATE("Open"));
	menu->AddItem(fOpenMenu);
	fOpenMenu->Superitem()->SetTrigger('O');
	fOpenMenu->Superitem()->SetMessage(new BMessage(MSG_FILE_OPEN));
	fOpenMenu->Superitem()->SetTarget(be_app);
	fOpenMenu->Superitem()->SetShortcut('O', 0);
	menu->AddSeparatorItem();
	BMenu *pmenuSaveAs = new BMenu(B_TRANSLATE("Save as" B_UTF8_ELLIPSIS),
		B_ITEMS_IN_COLUMN);
	BTranslationUtils::AddTranslationItems(pmenuSaveAs, B_TRANSLATOR_BITMAP);
		// Fill Save As submenu with all types that can be converted
		// to from the Be bitmap image format
	menu->AddItem(pmenuSaveAs);
	_AddItemMenu(menu, B_TRANSLATE("Close"), B_QUIT_REQUESTED, 'W', 0, this);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Page setup" B_UTF8_ELLIPSIS),
		MSG_PAGE_SETUP, 0, 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Print" B_UTF8_ELLIPSIS),
		MSG_PREPARE_PRINT, 'P', 0, this);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("About ShowImage" B_UTF8_ELLIPSIS),
		B_ABOUT_REQUESTED, 0, 0, 	be_app);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Quit"), B_QUIT_REQUESTED, 'Q', 0, be_app);
	bar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Edit"));
	_AddItemMenu(menu, B_TRANSLATE("Undo"), B_UNDO, 'Z', 0, this, false);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Cut"), B_CUT, 'X', 0, this, false);
	_AddItemMenu(menu, B_TRANSLATE("Copy"), B_COPY, 'C', 0, this, false);
	_AddItemMenu(menu, B_TRANSLATE("Paste"), B_PASTE, 'V', 0, this, false);
	_AddItemMenu(menu, B_TRANSLATE("Clear"),
		MSG_CLEAR_SELECT, 0, 0, this, false);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Select all"),
		MSG_SELECT_ALL, 'A', 0, this);
	bar->AddItem(menu);

	menu = fBrowseMenu = new BMenu(B_TRANSLATE("Browse"));
	_AddItemMenu(menu, B_TRANSLATE("First page"),
		MSG_PAGE_FIRST, B_LEFT_ARROW, B_SHIFT_KEY, this);
	_AddItemMenu(menu, B_TRANSLATE("Last page"),
		MSG_PAGE_LAST, B_RIGHT_ARROW, B_SHIFT_KEY, this);
	_AddItemMenu(menu, B_TRANSLATE("Previous page"),
		MSG_PAGE_PREV, B_LEFT_ARROW, 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Next page"),
		MSG_PAGE_NEXT, B_RIGHT_ARROW, 0, this);
	fGoToPageMenu = new BMenu(B_TRANSLATE("Go to page"));
	fGoToPageMenu->SetRadioMode(true);
	menu->AddItem(fGoToPageMenu);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Previous file"),
		MSG_FILE_PREV, B_UP_ARROW, 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Next file"),
		MSG_FILE_NEXT, B_DOWN_ARROW, 0, this);
	bar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Image"));
	_AddItemMenu(menu, B_TRANSLATE("Rotate clockwise"),
		MSG_ROTATE_90, 'R', 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Rotate counterclockwise"),
		MSG_ROTATE_270, 'R', B_SHIFT_KEY, this);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Flip left to right"),
		MSG_FLIP_LEFT_TO_RIGHT, 0, 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Flip top to bottom"),
		MSG_FLIP_TOP_TO_BOTTOM, 0, 0, this);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Invert colors"), MSG_INVERT, 0, 0, this);
	menu->AddSeparatorItem();
	fResizeItem = _AddItemMenu(menu, B_TRANSLATE("Resize" B_UTF8_ELLIPSIS),
		MSG_OPEN_RESIZER_WINDOW, 0, 0, this);
	bar->AddItem(menu);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Use as background" B_UTF8_ELLIPSIS),
		MSG_DESKTOP_BACKGROUND, 0, 0, this);
}


BMenuItem*
ShowImageWindow::_AddItemMenu(BMenu* menu, const char* label, uint32 what,
	const char shortcut, uint32 modifier, const BHandler* target, bool enabled)
{
	BMenuItem* item = new BMenuItem(label, new BMessage(what), shortcut, modifier);
	menu->AddItem(item);

	item->SetTarget(target);
	item->SetEnabled(enabled);

	return item;
}


BMenuItem*
ShowImageWindow::_AddDelayItem(BMenu* menu, const char* label, float value)
{
	BMessage* message = new BMessage(MSG_SLIDE_SHOW_DELAY);
	message->AddFloat("value", value);

	BMenuItem* item = new BMenuItem(label, message, 0);
	item->SetTarget(this);

	bool marked = fImageView->GetSlideShowDelay() == value;
	if (marked)
		item->SetMarked(true);

	menu->AddItem(item);
	return item;
}


void
ShowImageWindow::WindowRedimension(BBitmap* pbitmap)
{
	BScreen screen;
	if (!screen.IsValid())
		return;

	BRect r(pbitmap->Bounds());
	float width = r.Width() + 2 * PEN_SIZE + B_V_SCROLL_BAR_WIDTH;
	float height = r.Height() + 2 * PEN_SIZE + 1 + fBar->Frame().Height() +
		B_H_SCROLL_BAR_HEIGHT;

	BRect frame = screen.Frame();
	const float windowBorder = 5;
	// dimensions so that window does not reach outside of screen
	float maxWidth = frame.Width() + 1 - windowBorder - Frame().left;
	float maxHeight = frame.Height() + 1 - windowBorder - Frame().top;

	// We have to check size limits manually, otherwise
	// menu bar will be too short for small images.

	float minW, maxW, minH, maxH;
	GetSizeLimits(&minW, &maxW, &minH, &maxH);
	if (maxWidth > maxW)
		maxWidth = maxW;
	if (maxHeight > maxH)
		maxHeight = maxH;
	if (width < minW)
		width = minW;
	if (height < minH)
		height = minH;

	if (width > maxWidth)
		width = maxWidth;
	if (height > maxHeight)
		height = maxHeight;

	ResizeTo(width, height);
}


void
ShowImageWindow::FrameResized(float width, float height)
{
	BWindow::FrameResized(width, height);
}


bool
ShowImageWindow::_ToggleMenuItem(uint32 what)
{
	bool marked = false;
	BMenuItem* item = fBar->FindItem(what);
	if (item != NULL) {
		marked = !item->IsMarked();
		item->SetMarked(marked);
	}
	return marked;
}


void
ShowImageWindow::_EnableMenuItem(BMenu* menu, uint32 what, bool enable)
{
	BMenuItem* item = menu->FindItem(what);
	if (item && item->IsEnabled() != enable)
		item->SetEnabled(enable);
}


void
ShowImageWindow::_MarkMenuItem(BMenu* menu, uint32 what, bool marked)
{
	BMenuItem* item = menu->FindItem(what);
	if (item && item->IsMarked() != marked)
		item->SetMarked(marked);
}


void
ShowImageWindow::_MarkSlideShowDelay(float value)
{
	const int32 n = fSlideShowDelay->CountItems();
	float v;
	for (int32 i = 0; i < n; i ++) {
		BMenuItem* item = fSlideShowDelay->ItemAt(i);
		if (item) {
			if (item->Message()->FindFloat("value", &v) == B_OK && v == value) {
				if (!item->IsMarked())
					item->SetMarked(true);
				return;
			}
		}
	}
}


void
ShowImageWindow::_ResizeToWindow(bool shrink, uint32 what)
{
	bool enabled = _ToggleMenuItem(what);
	if (shrink)
		fImageView->SetShrinkToBounds(enabled);
	else
		fImageView->SetZoomToBounds(enabled);

	enabled = !(fImageView->GetShrinkToBounds() || fImageView->GetZoomToBounds());
	_EnableMenuItem(fBar, MSG_ORIGINAL_SIZE, enabled);
	_EnableMenuItem(fBar, MSG_ZOOM_IN, enabled);
	_EnableMenuItem(fBar, MSG_ZOOM_OUT, enabled);
}


void
ShowImageWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_MODIFIED:
			// If image has been modified due to a Cut or Paste
			fModified = true;
			break;

		case MSG_OUTPUT_TYPE:
			// User clicked Save As then choose an output format
			if (!fSavePanel)
				// If user doesn't already have a save panel open
				_SaveAs(message);
			break;

		case MSG_SAVE_PANEL:
			// User specified where to save the output image
			_SaveToFile(message);
			break;

		case B_CANCEL:
			delete fSavePanel;
			fSavePanel = NULL;
			break;

		case MSG_UPDATE_STATUS: {
			int32 pages = fImageView->PageCount();
			int32 curPage = fImageView->CurrentPage();

			bool enable = (pages > 1) ? true : false;
			_EnableMenuItem(fBar, MSG_PAGE_FIRST, enable);
			_EnableMenuItem(fBar, MSG_PAGE_LAST, enable);
			_EnableMenuItem(fBar, MSG_PAGE_NEXT, enable);
			_EnableMenuItem(fBar, MSG_PAGE_PREV, enable);
			fGoToPageMenu->SetEnabled(enable);

			_EnableMenuItem(fBar, MSG_FILE_NEXT, fImageView->HasNextFile());
			_EnableMenuItem(fBar, MSG_FILE_PREV, fImageView->HasPrevFile());

			if (fGoToPageMenu->CountItems() != pages) {
				// Only rebuild the submenu if the number of
				// pages is different

				while (fGoToPageMenu->CountItems() > 0)
					// Remove all page numbers
					delete fGoToPageMenu->RemoveItem(0L);

				for (int32 i = 1; i <= pages; i++) {
					// Fill Go To page submenu with an entry for each page
					BMessage* pgomsg = new BMessage(MSG_GOTO_PAGE);
					pgomsg->AddInt32("page", i);

					char shortcut = 0;
					if (i < 10) {
						shortcut = '0' + i;
					} else if (i == 10) {
						shortcut = '0';
					}

					BString strCaption;
					strCaption << i;

					BMenuItem* item = new BMenuItem(strCaption.String(), pgomsg,
						shortcut);
					if (curPage == i)
						item->SetMarked(true);
					fGoToPageMenu->AddItem(item);
				}
			} else {
				// Make sure the correct page is marked
				BMenuItem *pcurItem;
				pcurItem = fGoToPageMenu->ItemAt(curPage - 1);
				if (!pcurItem->IsMarked()) {
					pcurItem->SetMarked(true);
				}
			}

			// Disable the Invert menu item if the bitmap color space
			// is B_CMAP8. (B_CMAP8 is currently unsupported by the
			// invert algorithm)
			color_space colors = B_NO_COLOR_SPACE;
			message->FindInt32("colors", reinterpret_cast<int32 *>(&colors));
			_EnableMenuItem(fBar, MSG_INVERT, (colors != B_CMAP8));

			BString status;
			bool messageProvidesSize = false;
			if (message->FindInt32("width", &fWidth) >= B_OK
				&& message->FindInt32("height", &fHeight) >= B_OK) {
				status << fWidth << "x" << fHeight;
				messageProvidesSize = true;
			}

			BString str;
			if (message->FindString("status", &str) == B_OK && str.Length() > 0) {
				if (status.Length() > 0)
					status << ", ";
				status << str;
			}

			if (messageProvidesSize) {
				_UpdateResizerWindow(fWidth, fHeight);
				if (!fImageView->GetZoomToBounds()
					&& !fImageView->GetShrinkToBounds()
					&& !fFullScreen)
					WindowRedimension(fImageView->GetBitmap());
			}

			fStatusView->SetText(status);

			UpdateTitle();
		}	break;

		case MSG_UPDATE_STATUS_TEXT: {
			BString status;
			status << fWidth << "x" << fHeight;
			BString str;
			if (message->FindString("status", &str) == B_OK && str.Length() > 0) {
				status << ", " << str;
				fStatusView->SetText(status);
			}
		}	break;

		case MSG_SELECTION: {
			// The view sends this message when a selection is
			// made or the selection is cleared so that the window
			// can update the state of the appropriate menu items
			bool benable;
			if (message->FindBool("has_selection", &benable) == B_OK) {
				_EnableMenuItem(fBar, B_CUT, benable);
				_EnableMenuItem(fBar, B_COPY, benable);
				_EnableMenuItem(fBar, MSG_CLEAR_SELECT, benable);
			}
		}	break;

		case MSG_UNDO_STATE: {
			bool benable;
			if (message->FindBool("can_undo", &benable) == B_OK)
				_EnableMenuItem(fBar, B_UNDO, benable);
		}	break;

		case MSG_CLIPBOARD_CHANGED: {
			// The app sends this message after it examines the clipboard in
			// response to a B_CLIPBOARD_CHANGED message
			bool bdata;
			if (message->FindBool("data_available", &bdata) == B_OK)
				_EnableMenuItem(fBar, B_PASTE, bdata);
		}	break;

		case B_UNDO:
			fImageView->Undo();
			break;

		case B_CUT:
			fImageView->Cut();
			break;

		case B_COPY:
			fImageView->CopySelectionToClipboard();
			break;

		case B_PASTE:
			fImageView->Paste();
			break;

		case MSG_CLEAR_SELECT:
			fImageView->ClearSelection();
			break;

		case MSG_SELECT_ALL:
			fImageView->SelectAll();
			break;

		case MSG_PAGE_FIRST:
			if (_ClosePrompt())
				fImageView->FirstPage();
			break;

		case MSG_PAGE_LAST:
			if (_ClosePrompt())
				fImageView->LastPage();
			break;

		case MSG_PAGE_NEXT:
			if (_ClosePrompt())
				fImageView->NextPage();
			break;

		case MSG_PAGE_PREV:
			if (_ClosePrompt())
				fImageView->PrevPage();
			break;

		case MSG_GOTO_PAGE: {
			if (!_ClosePrompt())
				break;

			int32 newPage;
			if (message->FindInt32("page", &newPage) != B_OK)
				break;

			int32 curPage = fImageView->CurrentPage();
			int32 pages = fImageView->PageCount();

			if (newPage > 0 && newPage <= pages) {
				BMenuItem* pcurItem = fGoToPageMenu->ItemAt(curPage - 1);
				BMenuItem* pnewItem = fGoToPageMenu->ItemAt(newPage - 1);
				if (pcurItem && pnewItem) {
					pcurItem->SetMarked(false);
					pnewItem->SetMarked(true);
					fImageView->GoToPage(newPage);
				}
			}
		}	break;

		case MSG_DITHER_IMAGE:
			fImageView->SetDither(_ToggleMenuItem(message->what));
			break;

		case MSG_SHRINK_TO_WINDOW:
			_ResizeToWindow(true, message->what);
			break;

		case MSG_ZOOM_TO_WINDOW:
			_ResizeToWindow(false, message->what);
			break;

		case MSG_FILE_PREV:
			if (_ClosePrompt())
				fImageView->PrevFile();
			break;

		case MSG_FILE_NEXT:
			if (_ClosePrompt())
				fImageView->NextFile();
			break;

		case MSG_ROTATE_90:
			fImageView->Rotate(90);
			break;

		case MSG_ROTATE_270:
			fImageView->Rotate(270);
			break;

		case MSG_FLIP_LEFT_TO_RIGHT:
			fImageView->Flip(true);
			break;

		case MSG_FLIP_TOP_TO_BOTTOM:
			fImageView->Flip(false);
			break;

		case MSG_INVERT:
			fImageView->Invert();
			break;

		case MSG_SLIDE_SHOW: {
			BMenuItem* item = fBar->FindItem(message->what);
			if (!item)
				break;
			if (item->IsMarked()) {
				item->SetMarked(false);
				fResizeItem->SetEnabled(true);
				fImageView->StopSlideShow();
			} else if (_ClosePrompt()) {
				item->SetMarked(true);
				fResizeItem->SetEnabled(false);
				fImageView->StartSlideShow();
			}
		}	break;

		case MSG_SLIDE_SHOW_DELAY: {
			float value;
			if (message->FindFloat("value", &value) == B_OK) {
				fImageView->SetSlideShowDelay(value);
				// in case message is sent from popup menu
				_MarkSlideShowDelay(value);
			}
		}	break;

		case MSG_FULL_SCREEN:
			_ToggleFullScreen();
			break;

		case MSG_EXIT_FULL_SCREEN:
			if (fFullScreen)
				_ToggleFullScreen();
			break;

		case MSG_SHOW_CAPTION: {
			fShowCaption = _ToggleMenuItem(message->what);
			ShowImageSettings* settings = my_app->Settings();

			if (settings->Lock()) {
				settings->SetBool("ShowCaption", fShowCaption);
				settings->Unlock();
			}
			if (fFullScreen)
				fImageView->SetShowCaption(fShowCaption);
		}	break;

		case MSG_PAGE_SETUP:
			_PageSetup();
			break;

		case MSG_PREPARE_PRINT:
			_PrepareForPrint();
			break;

		case MSG_PRINT:
			_Print(message);
			break;

		case MSG_ZOOM_IN:
			fImageView->ZoomIn();
			break;

		case MSG_ZOOM_OUT:
			fImageView->ZoomOut();
			break;

		case MSG_ORIGINAL_SIZE:
			fImageView->SetZoom(1.0);
			break;

		case MSG_SCALE_BILINEAR:
			fImageView->SetScaleBilinear(_ToggleMenuItem(message->what));
			break;

		case MSG_OPEN_RESIZER_WINDOW: {
			if (fImageView->GetBitmap() != NULL) {
				BRect rect = fImageView->GetBitmap()->Bounds();
				_OpenResizerWindow(rect.IntegerWidth()+1, rect.IntegerHeight()+1);
			}
		}	break;

		case MSG_RESIZE: {
			int w = message->FindInt32("w");
			int h = message->FindInt32("h");
			fImageView->ResizeImage(w, h);
		} break;

		case MSG_RESIZER_WINDOW_QUIT:
			delete fResizerWindowMessenger;
			fResizerWindowMessenger = NULL;
			break;

		case MSG_DESKTOP_BACKGROUND: {
			BMessage message(B_REFS_RECEIVED);
			message.AddRef("refs", fImageView->Image());
			// This is used in the Backgrounds code for scaled placement
			message.AddInt32("placement", 'scpl');
			be_roster->Launch("application/x-vnd.haiku-backgrounds", &message);
		}	break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ShowImageWindow::_SaveAs(BMessage* message)
{
	// Read the translator and output type the user chose
	translator_id outTranslator;
	uint32 outType;
	if (message->FindInt32(kTranslatorField,
			reinterpret_cast<int32 *>(&outTranslator)) != B_OK
		|| message->FindInt32(kTypeField,
			reinterpret_cast<int32 *>(&outType)) != B_OK)
		return;

	// Add the chosen translator and output type to the
	// message that the save panel will send back
	BMessage panelMsg(MSG_SAVE_PANEL);
	panelMsg.AddInt32(kTranslatorField, outTranslator);
	panelMsg.AddInt32(kTypeField, outType);

	// Create save panel and show it
	BMessenger target(this);
	fSavePanel = new (std::nothrow) BFilePanel(B_SAVE_PANEL,
		&target, NULL, 0, false, &panelMsg);
	if (!fSavePanel)
		return;

	fSavePanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
	fSavePanel->Show();
}


void
ShowImageWindow::_SaveToFile(BMessage* message)
{
	// Read in where the file should be saved
	entry_ref dirRef;
	if (message->FindRef("directory", &dirRef) != B_OK)
		return;

	const char* filename;
	if (message->FindString("name", &filename) != B_OK)
		return;

	// Read in the translator and type to be used
	// to save the output image
	translator_id outTranslator;
	uint32 outType;
	if (message->FindInt32(kTranslatorField,
			reinterpret_cast<int32 *>(&outTranslator)) != B_OK
		|| message->FindInt32(kTypeField,
			reinterpret_cast<int32 *>(&outType)) != B_OK)
		return;

	// Find the translator_format information needed to
	// write a MIME attribute for the image file
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	const translation_format* outFormat = NULL;
	int32 outCount = 0;
	if (roster->GetOutputFormats(outTranslator, &outFormat, &outCount) != B_OK
		|| outCount < 1)
		return;

	int32 i;
	for (i = 0; i < outCount; i++) {
		if (outFormat[i].group == B_TRANSLATOR_BITMAP && outFormat[i].type == outType)
			break;
	}
	if (i == outCount)
		return;

	// Write out the image file
	BDirectory dir(&dirRef);
	fImageView->SaveToFile(&dir, filename, NULL, &outFormat[i]);
}


// This is temporary solution for building BString with printf like format.
// will be removed in the future.
static void
bs_printf(BString* string, const char* format, ...)
{
	va_list ap;
	char* buf;

	va_start(ap, format);
	vasprintf(&buf, format, ap);
	string->SetTo(buf);
	free(buf);
	va_end(ap);
}


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ClosePrompt"

bool
ShowImageWindow::_ClosePrompt()
{
	if (!fModified)
		return true;

	int32 page, count;
	count = fImageView->PageCount();
	page = fImageView->CurrentPage();
	BString prompt, name;
	fImageView->GetName(&name);

	if (count > 1) {
		bs_printf(&prompt,
			B_TRANSLATE("The document '%s' (page %d) has been changed. Do you "
				"want to close the document?"),
			name.String(), page);
	} else {
		bs_printf(&prompt,
			B_TRANSLATE("The document '%s' has been changed. Do you want to "
				"close the document?"),
			name.String());
	}

	BAlert* pAlert = new BAlert(B_TRANSLATE("Close document"), prompt.String(),
		B_TRANSLATE("Cancel"), B_TRANSLATE("Close"));
	if (pAlert->Go() == 0) {
		// Cancel
		return false;
	} else {
		// Close
		fModified = false;
		return true;
	}
}


void
ShowImageWindow::_ToggleFullScreen()
{
	BRect frame;
	fFullScreen = !fFullScreen;
	if (fFullScreen) {
		BScreen screen;
		fWindowFrame = Frame();
		frame = screen.Frame();
		frame.top -= fBar->Bounds().Height()+1;
		frame.right += B_V_SCROLL_BAR_WIDTH;
		frame.bottom += B_H_SCROLL_BAR_HEIGHT;
		frame.InsetBy(-1, -1); // PEN_SIZE in ShowImageView

		SetFlags(Flags() | B_NOT_RESIZABLE | B_NOT_MOVABLE);

		Activate();
			// make the window frontmost
	} else {
		frame = fWindowFrame;

		SetFlags(Flags() & ~(B_NOT_RESIZABLE | B_NOT_MOVABLE));
	}

	fImageView->SetFullScreen(fFullScreen);
	fImageView->SetShowCaption(fFullScreen && fShowCaption);
	MoveTo(frame.left, frame.top);
	ResizeTo(frame.Width(), frame.Height());
}


void
ShowImageWindow::_LoadSettings()
{
	ShowImageSettings* settings = my_app->Settings();

	if (settings->Lock()) {
		fShowCaption = settings->GetBool("ShowCaption", fShowCaption);
		fPrintOptions.SetBounds(BRect(0, 0, 1023, 767));

		int32 op = settings->GetInt32("PO:Option", fPrintOptions.Option());
		fPrintOptions.SetOption((enum PrintOptions::Option)op);

		float f = settings->GetFloat("PO:ZoomFactor", fPrintOptions.ZoomFactor());
		fPrintOptions.SetZoomFactor(f);

		f = settings->GetFloat("PO:DPI", fPrintOptions.DPI());
		fPrintOptions.SetDPI(f);

		f = settings->GetFloat("PO:Width", fPrintOptions.Width());
		fPrintOptions.SetWidth(f);

		f = settings->GetFloat("PO:Height", fPrintOptions.Height());
		fPrintOptions.SetHeight(f);

		settings->Unlock();
	}
}


void
ShowImageWindow::_SavePrintOptions()
{
	ShowImageSettings* settings = my_app->Settings();

	if (settings->Lock()) {
		settings->SetInt32("PO:Option", fPrintOptions.Option());
		settings->SetFloat("PO:ZoomFactor", fPrintOptions.ZoomFactor());
		settings->SetFloat("PO:DPI", fPrintOptions.DPI());
		settings->SetFloat("PO:Width", fPrintOptions.Width());
		settings->SetFloat("PO:Height", fPrintOptions.Height());
		settings->Unlock();
	}
}


bool
ShowImageWindow::_PageSetup()
{
	BString name;
	fImageView->GetName(&name);
	BPrintJob printJob(name.String());
	if (fPrintSettings != NULL)
		printJob.SetSettings(new BMessage(*fPrintSettings));

	status_t status = printJob.ConfigPage();
	if (status == B_OK) {
		delete fPrintSettings;
		fPrintSettings = printJob.Settings();
	}

	return status == B_OK;
}


void
ShowImageWindow::_PrepareForPrint()
{
	if (fPrintSettings == NULL) {
		BString name;
		fImageView->GetName(&name);

		BPrintJob printJob("");
		if (printJob.ConfigJob() == B_OK)
			fPrintSettings = printJob.Settings();
	}

	fPrintOptions.SetBounds(fImageView->GetBitmap()->Bounds());
	fPrintOptions.SetWidth(fImageView->GetBitmap()->Bounds().Width() + 1);

	new PrintOptionsWindow(BPoint(Frame().left + 30, Frame().top + 50),
		&fPrintOptions, this);
}


void
ShowImageWindow::_Print(BMessage* msg)
{
	status_t st;
	if (msg->FindInt32("status", &st) != B_OK || st != B_OK)
		return;

	_SavePrintOptions();

	BString name;
	fImageView->GetName(&name);

	BPrintJob printJob(name.String());
	if (fPrintSettings)
		printJob.SetSettings(new BMessage(*fPrintSettings));

	if (printJob.ConfigJob() == B_OK) {
		delete fPrintSettings;
		fPrintSettings = printJob.Settings();

		// first/lastPage is unused for now
		int32 firstPage = printJob.FirstPage();
		int32 lastPage = printJob.LastPage();
		BRect printableRect = printJob.PrintableRect();

		if (firstPage < 1)
			firstPage = 1;
		if (lastPage < firstPage)
			lastPage = firstPage;

		BBitmap* bitmap = fImageView->GetBitmap();
		float imageWidth = bitmap->Bounds().Width() + 1.0;
		float imageHeight = bitmap->Bounds().Height() + 1.0;

		float width;
		switch (fPrintOptions.Option()) {
			case PrintOptions::kFitToPage: {
				float w1 = printableRect.Width()+1;
				float w2 = imageWidth * (printableRect.Height() + 1) / imageHeight;
				if (w2 < w1)
					width = w2;
				else
					width = w1;
			}	break;
			case PrintOptions::kZoomFactor:
				width = imageWidth * fPrintOptions.ZoomFactor();
				break;
			case PrintOptions::kDPI:
				width = imageWidth * 72.0 / fPrintOptions.DPI();
				break;
			case PrintOptions::kWidth:
			case PrintOptions::kHeight:
				width = fPrintOptions.Width();
				break;

			default:
				// keep compiler silent; should not reach here
				width = imageWidth;
		}

		// TODO: eventually print large images on several pages
		printJob.BeginJob();
		fImageView->SetScale(width / imageWidth);
		// coordinates are relative to printable rectangle
		BRect bounds(bitmap->Bounds());
		printJob.DrawView(fImageView, bounds, BPoint(0, 0));
		fImageView->SetScale(1.0);
		printJob.SpoolPage();
		printJob.CommitJob();
	}
}


void
ShowImageWindow::_OpenResizerWindow(int32 width, int32 height)
{
	if (fResizerWindowMessenger == NULL) {
		// open window if it is not already opened
		BWindow* window = new ResizerWindow(this, width, height);
		fResizerWindowMessenger = new BMessenger(window);
		window->Show();
	} else {
		fResizerWindowMessenger->SendMessage(ResizerWindow::kActivateMsg);
	}
}


void
ShowImageWindow::_UpdateResizerWindow(int32 width, int32 height)
{
	if (fResizerWindowMessenger == NULL)
		return;

	BMessage updateMsg(ResizerWindow::kUpdateMsg);
	updateMsg.AddInt32("width", width);
	updateMsg.AddInt32("height", height);
	fResizerWindowMessenger->SendMessage(&updateMsg);
}


void
ShowImageWindow::_CloseResizerWindow()
{
	if (fResizerWindowMessenger == NULL)
		return;

	fResizerWindowMessenger->SendMessage(B_QUIT_REQUESTED);
	delete fResizerWindowMessenger;
	fResizerWindowMessenger = NULL;
}


bool
ShowImageWindow::QuitRequested()
{
	if (fSavePanel) {
		// Don't allow this window to be closed if a save panel is open
		return false;
	}

	bool quit = _ClosePrompt();

	if (quit) {
		_CloseResizerWindow();

		// tell the app to forget about this window
		be_app->PostMessage(MSG_WINDOW_QUIT);
	}

	return quit;
}


void
ShowImageWindow::ScreenChanged(BRect frame, color_space mode)
{
	fImageView->SetDither(mode == B_CMAP8);
}
