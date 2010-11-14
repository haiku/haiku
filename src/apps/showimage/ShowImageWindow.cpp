/*
 * Copyright 2003-2010, Haiku, Inc. All Rights Reserved.
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
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ShowImageWindow.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>

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
#include <RecentItems.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>
#include <SupportDefs.h>
#include <TranslationDefs.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>

#include "ImageCache.h"
#include "ProgressWindow.h"
#include "ShowImageApp.h"
#include "ShowImageConstants.h"
#include "ShowImageStatusView.h"
#include "ShowImageView.h"


// BMessage field names used in Save messages
const char* kTypeField = "be:type";
const char* kTranslatorField = "be:translator";


// message constants
enum {
	MSG_CAPTURE_MOUSE			= 'mCPM',
	MSG_CHANGE_FOCUS			= 'mCFS',
	MSG_WINDOW_QUIT				= 'mWQT',
	MSG_OUTPUT_TYPE				= 'BTMN',
	MSG_SAVE_PANEL				= 'mFSP',
	MSG_CLEAR_SELECT			= 'mCSL',
	MSG_SELECT_ALL				= 'mSAL',
	MSG_SELECTION_MODE			= 'mSLM',
	MSG_PAGE_FIRST				= 'mPGF',
	MSG_PAGE_LAST				= 'mPGL',
	MSG_PAGE_NEXT				= 'mPGN',
	MSG_PAGE_PREV				= 'mPGP',
	MSG_GOTO_PAGE				= 'mGTP',
	MSG_ZOOM_IN					= 'mZIN',
	MSG_ZOOM_OUT				= 'mZOU',
	MSG_SCALE_BILINEAR			= 'mSBL',
	MSG_DESKTOP_BACKGROUND		= 'mDBG',
	MSG_ROTATE_90				= 'mR90',
	MSG_ROTATE_270				= 'mR27',
	MSG_FLIP_LEFT_TO_RIGHT		= 'mFLR',
	MSG_FLIP_TOP_TO_BOTTOM		= 'mFTB',
	MSG_SLIDE_SHOW_DELAY		= 'mSSD',
	MSG_FULL_SCREEN				= 'mFSC',
	MSG_SHOW_CAPTION			= 'mSCP',
	MSG_PAGE_SETUP				= 'mPSU',
	MSG_PREPARE_PRINT			= 'mPPT',
	kMsgFitToWindow				= 'mFtW',
	kMsgOriginalSize			= 'mOSZ',
	kMsgStretchToWindow			= 'mStW'
};


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


//	#pragma mark -- ShowImageWindow


ShowImageWindow::ShowImageWindow(const entry_ref& ref,
	const BMessenger& trackerMessenger)
	:
	BWindow(BRect(5, 24, 250, 100), "", B_DOCUMENT_WINDOW, 0),
	fNavigator(ref, trackerMessenger),
	fSavePanel(NULL),
	fBar(NULL),
	fBrowseMenu(NULL),
	fGoToPageMenu(NULL),
	fSlideShowDelay(NULL),
	fImageView(NULL),
	fStatusView(NULL),
	fProgressWindow(new ProgressWindow()),
	fModified(false),
	fFullScreen(false),
	fShowCaption(true),
	fPrintSettings(NULL)
{
	_ApplySettings();

	// create menu bar
	fBar = new BMenuBar(BRect(0, 0, Bounds().right, 1), "menu_bar");
	_AddMenus(fBar);
	AddChild(fBar);

	BRect viewFrame = Bounds();
	viewFrame.top = fBar->Bounds().Height() + 1;
	viewFrame.right -= B_V_SCROLL_BAR_WIDTH;
	viewFrame.bottom -= B_H_SCROLL_BAR_HEIGHT;

	// create the image view
	fImageView = new ShowImageView(viewFrame, "image_view", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED
			| B_FRAME_EVENTS);
	// wrap a scroll view around the view
	BScrollView* scrollView = new BScrollView("image_scroller", fImageView,
		B_FOLLOW_ALL, 0, false, false, B_PLAIN_BORDER);
	AddChild(scrollView);

	const int32 kstatusWidth = 190;
	BRect rect;
	rect = Bounds();
	rect.top = viewFrame.bottom + 1;
	rect.left = viewFrame.left + kstatusWidth;
	rect.right = viewFrame.right + 1;
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
	rect.top = viewFrame.top - 1;
	rect.left = viewFrame.right + 1;
	rect.bottom = viewFrame.bottom + 1;
	rect.right += 1;
	BScrollBar* verticalScrollBar = new BScrollBar(rect, "vscroll", fImageView,
		0, 150, B_VERTICAL);
	AddChild(verticalScrollBar);

	SetSizeLimits(250, 100000, 100, 100000);

	// finish creating the window
	if (_LoadImage() != B_OK) {
		_LoadError(ref);
		Quit();
		return;
	}

	// add View menu here so it can access ShowImageView methods
	BMenu* menu = new BMenu(B_TRANSLATE_WITH_CONTEXT("View", "Menus"));
	_BuildViewMenu(menu, false);
	fBar->AddItem(menu);

	SetPulseRate(100000);
		// every 1/10 second; ShowImageView needs it for marching ants

	_MarkMenuItem(menu, MSG_SELECTION_MODE,
		fImageView->IsSelectionModeEnabled());

	// Tell application object to query the clipboard
	// and tell this window if it contains interesting data or not
	be_app_messenger.SendMessage(B_CLIPBOARD_CHANGED);

	// The window will be shown on screen automatically
	Run();
}


ShowImageWindow::~ShowImageWindow()
{
	fProgressWindow->Lock();
	fProgressWindow->Quit();
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
		kMsgOriginalSize, '1', 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Fit to window"),
		kMsgFitToWindow, '0', 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Zoom in"), MSG_ZOOM_IN, '+', 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Zoom out"), MSG_ZOOM_OUT, '-', 0, this);

	menu->AddSeparatorItem();

	if (!popupMenu || fFullScreen) {
		_AddItemMenu(menu, B_TRANSLATE("High-quality zooming"),
			MSG_SCALE_BILINEAR, 0, 0, this);
		_AddItemMenu(menu, B_TRANSLATE("Stretch to window"),
			kMsgStretchToWindow, 0, 0, this);

		menu->AddSeparatorItem();
	}

	_AddItemMenu(menu, B_TRANSLATE("Full screen"),
		MSG_FULL_SCREEN, B_ENTER, 0, this);
	_MarkMenuItem(menu, MSG_FULL_SCREEN, fFullScreen);

	_AddItemMenu(menu, B_TRANSLATE("Show caption in full screen mode"),
		MSG_SHOW_CAPTION, 0, 0, this);
	_MarkMenuItem(menu, MSG_SHOW_CAPTION, fShowCaption);

	_MarkMenuItem(menu, MSG_SCALE_BILINEAR, fImageView->ScaleBilinear());
	_MarkMenuItem(menu, kMsgStretchToWindow, fImageView->StretchesToBounds());

	if (popupMenu) {
		menu->AddSeparatorItem();
		_AddItemMenu(menu, B_TRANSLATE("Use as background" B_UTF8_ELLIPSIS),
			MSG_DESKTOP_BACKGROUND, 0, 0, this);
	}
}


void
ShowImageWindow::_AddMenus(BMenuBar* bar)
{
	BMenu* menu = new BMenu(B_TRANSLATE("File"));

	// Add recent files to "Open File" entry as sub-menu.
	BMenuItem* item = new BMenuItem(BRecentFilesList::NewFileListMenu(
		B_TRANSLATE("Open"B_UTF8_ELLIPSIS), NULL, NULL, be_app, 10, true,
		NULL, kApplicationSignature), new BMessage(MSG_FILE_OPEN));
	item->SetShortcut('O', 0);
	item->SetTarget(be_app);
	menu->AddItem(item);
	menu->AddSeparatorItem();

	BMenu* menuSaveAs = new BMenu(B_TRANSLATE("Save as" B_UTF8_ELLIPSIS),
		B_ITEMS_IN_COLUMN);
	BTranslationUtils::AddTranslationItems(menuSaveAs, B_TRANSLATOR_BITMAP);
		// Fill Save As submenu with all types that can be converted
		// to from the Be bitmap image format
	menu->AddItem(menuSaveAs);
	_AddItemMenu(menu, B_TRANSLATE("Close"), B_QUIT_REQUESTED, 'W', 0, this);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Page setup" B_UTF8_ELLIPSIS),
		MSG_PAGE_SETUP, 0, 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Print" B_UTF8_ELLIPSIS),
		MSG_PREPARE_PRINT, 'P', 0, this);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("About ShowImage" B_UTF8_ELLIPSIS),
		B_ABOUT_REQUESTED, 0, 0, be_app);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Quit"), B_QUIT_REQUESTED, 'Q', 0, be_app);
	bar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Edit"));
	_AddItemMenu(menu, B_TRANSLATE("Undo"), B_UNDO, 'Z', 0, this, false);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Copy"), B_COPY, 'C', 0, this, false);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Selection Mode"), MSG_SELECTION_MODE, 0, 0,
		this);
	_AddItemMenu(menu, B_TRANSLATE("Clear selection"),
		MSG_CLEAR_SELECT, 0, 0, this, false);
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
	_AddItemMenu(menu, B_TRANSLATE("Use as background" B_UTF8_ELLIPSIS),
		MSG_DESKTOP_BACKGROUND, 0, 0, this);

	bar->AddItem(menu);
}


BMenuItem*
ShowImageWindow::_AddItemMenu(BMenu* menu, const char* label, uint32 what,
	char shortcut, uint32 modifier, const BHandler* target, bool enabled)
{
	BMenuItem* item = new BMenuItem(label, new BMessage(what), shortcut,
		modifier);
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
ShowImageWindow::_ResizeWindowToImage()
{
	BBitmap* bitmap = fImageView->Bitmap();
	BScreen screen;
	if (bitmap == NULL || !screen.IsValid())
		return;

	// TODO: use View::GetPreferredSize() instead?
	BRect r(bitmap->Bounds());
	float width = r.Width() + B_V_SCROLL_BAR_WIDTH;
	float height = r.Height() + 1 + fBar->Frame().Height()
		+ B_H_SCROLL_BAR_HEIGHT;

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
ShowImageWindow::Zoom(BPoint origin, float width, float height)
{
	_ToggleFullScreen();
}


void
ShowImageWindow::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		uint32 type;
		int32 count;
		status_t status = message->GetInfo("refs", &type, &count);
		if (status == B_OK && type == B_REF_TYPE) {
			message->what = B_REFS_RECEIVED;
			be_app->PostMessage(message);
		}
	}

	switch (message->what) {
		case kMsgImageCacheImageLoaded:
		{
			fProgressWindow->Stop();

			bool first = fImageView->Bitmap() == NULL;
			entry_ref ref;
			message->FindRef("ref", &ref);
			if (!first && ref != fNavigator.CurrentRef()) {
				// ignore older images
				break;
			}

			status_t status = fImageView->SetImage(message);
			if (status != B_OK) {
				_LoadError(ref);

				// quit if file could not be opened
				if (first)
					Quit();
				break;
			}

			fImageType = message->FindString("type");
			fNavigator.SetTo(ref, message->FindInt32("page"),
				message->FindInt32("pageCount"));

			if (first || (!fImageView->StretchesToBounds() && !fFullScreen)) {
				_ResizeWindowToImage();
				fImageView->FitToBounds();
			}
			if (first) {
				fImageView->MakeFocus(true);
					// to receive key messages
				Show();
			}
			break;
		}

		case kMsgImageCacheProgressUpdate:
		{
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK
				&& ref == fNavigator.CurrentRef()) {
				message->what = kMsgProgressUpdate;
				fProgressWindow->PostMessage(message);
			}
			break;
		}

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

		case MSG_UPDATE_STATUS:
		{
			int32 pages = fNavigator.PageCount();
			int32 currentPage = fNavigator.CurrentPage();

			bool enable = pages > 1 ? true : false;
			_EnableMenuItem(fBar, MSG_PAGE_FIRST, enable);
			_EnableMenuItem(fBar, MSG_PAGE_LAST, enable);
			_EnableMenuItem(fBar, MSG_PAGE_NEXT, enable);
			_EnableMenuItem(fBar, MSG_PAGE_PREV, enable);
			fGoToPageMenu->SetEnabled(enable);

			_EnableMenuItem(fBar, MSG_FILE_NEXT, fNavigator.HasNextFile());
			_EnableMenuItem(fBar, MSG_FILE_PREV, fNavigator.HasPreviousFile());

			if (fGoToPageMenu->CountItems() != pages) {
				// Only rebuild the submenu if the number of
				// pages is different

				while (fGoToPageMenu->CountItems() > 0) {
					// Remove all page numbers
					delete fGoToPageMenu->RemoveItem(0L);
				}

				for (int32 i = 1; i <= pages; i++) {
					// Fill Go To page submenu with an entry for each page
					BMessage* goTo = new BMessage(MSG_GOTO_PAGE);
					goTo->AddInt32("page", i);

					char shortcut = 0;
					if (i < 10)
						shortcut = '0' + i;

					BString strCaption;
					strCaption << i;

					BMenuItem* item = new BMenuItem(strCaption.String(), goTo,
						B_SHIFT_KEY, shortcut);
					if (currentPage == i)
						item->SetMarked(true);
					fGoToPageMenu->AddItem(item);
				}
			} else {
				// Make sure the correct page is marked
				BMenuItem* currentItem = fGoToPageMenu->ItemAt(currentPage - 1);
				if (currentItem != NULL && !currentItem->IsMarked())
					currentItem->SetMarked(true);
			}

			_UpdateStatusText(message);

			BPath path(fImageView->Image());
			SetTitle(path.Path());
			break;
		}

		case MSG_UPDATE_STATUS_TEXT:
		{
			_UpdateStatusText(message);
			break;
		}

		case MSG_SELECTION:
		{
			// The view sends this message when a selection is
			// made or the selection is cleared so that the window
			// can update the state of the appropriate menu items
			bool enable;
			if (message->FindBool("has_selection", &enable) == B_OK) {
				_EnableMenuItem(fBar, B_COPY, enable);
				_EnableMenuItem(fBar, MSG_CLEAR_SELECT, enable);
			}
			break;
		}

		case MSG_UNDO_STATE:
		{
			bool enable;
			if (message->FindBool("can_undo", &enable) == B_OK)
				_EnableMenuItem(fBar, B_UNDO, enable);
			break;
		}

		case B_UNDO:
			fImageView->Undo();
			break;

		case B_COPY:
			fImageView->CopySelectionToClipboard();
			break;

		case MSG_SELECTION_MODE:
			fImageView->SetSelectionMode(_ToggleMenuItem(MSG_SELECTION_MODE));
			break;

		case MSG_CLEAR_SELECT:
			fImageView->ClearSelection();
			break;

		case MSG_SELECT_ALL:
			fImageView->SelectAll();
			break;

		case MSG_PAGE_FIRST:
			if (_ClosePrompt() && fNavigator.FirstPage())
				_LoadImage();
			break;

		case MSG_PAGE_LAST:
			if (_ClosePrompt() && fNavigator.LastPage())
				_LoadImage();
			break;

		case MSG_PAGE_NEXT:
			if (_ClosePrompt() && fNavigator.NextPage())
				_LoadImage();
			break;

		case MSG_PAGE_PREV:
			if (_ClosePrompt() && fNavigator.PreviousPage())
				_LoadImage();
			break;

		case MSG_GOTO_PAGE:
		{
			if (!_ClosePrompt())
				break;

			int32 newPage;
			if (message->FindInt32("page", &newPage) != B_OK)
				break;

			int32 currentPage = fNavigator.CurrentPage();
			int32 pages = fNavigator.PageCount();

			// TODO: use radio mode instead!
			if (newPage > 0 && newPage <= pages) {
				BMenuItem* currentItem = fGoToPageMenu->ItemAt(currentPage - 1);
				BMenuItem* newItem = fGoToPageMenu->ItemAt(newPage - 1);
				if (currentItem != NULL && newItem != NULL) {
					currentItem->SetMarked(false);
					newItem->SetMarked(true);
					if (fNavigator.GoToPage(newPage))
						_LoadImage();
				}
			}
			break;
		}

		case kMsgFitToWindow:
			fImageView->FitToBounds();
			break;

		case kMsgStretchToWindow:
			fImageView->SetStretchToBounds(
				_ToggleMenuItem(kMsgStretchToWindow));
			break;

		case MSG_FILE_PREV:
			if (_ClosePrompt() && fNavigator.PreviousFile())
				_LoadImage(false);
			break;

		case MSG_FILE_NEXT:
			if (_ClosePrompt() && fNavigator.NextFile())
				_LoadImage();
			break;

		case kMsgDeleteCurrentFile:
		{
			if (fNavigator.MoveFileToTrash())
				_LoadImage();
			else
				PostMessage(B_QUIT_REQUESTED);
			break;
		}

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

		case MSG_SLIDE_SHOW:
		{
			BMenuItem* item = fBar->FindItem(message->what);
			if (!item)
				break;
			if (item->IsMarked()) {
				item->SetMarked(false);
				fImageView->StopSlideShow();
			} else if (_ClosePrompt()) {
				item->SetMarked(true);
				fImageView->StartSlideShow();
			}
			break;
		}

		case MSG_SLIDE_SHOW_DELAY:
		{
			float value;
			if (message->FindFloat("value", &value) == B_OK) {
				fImageView->SetSlideShowDelay(value);
				// in case message is sent from popup menu
				_MarkSlideShowDelay(value);
			}
			break;
		}

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

		case kMsgOriginalSize:
			fImageView->SetZoom(1.0);
			break;

		case MSG_SCALE_BILINEAR:
			fImageView->SetScaleBilinear(_ToggleMenuItem(message->what));
			break;

		case MSG_DESKTOP_BACKGROUND:
		{
			BMessage message(B_REFS_RECEIVED);
			message.AddRef("refs", fImageView->Image());
			// This is used in the Backgrounds code for scaled placement
			message.AddInt32("placement", 'scpl');
			be_roster->Launch("application/x-vnd.haiku-backgrounds", &message);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ShowImageWindow::_UpdateStatusText(const BMessage* message)
{
	BString status;
	if (fImageView->Bitmap() != NULL) {
		BRect bounds = fImageView->Bitmap()->Bounds();
		status << bounds.IntegerWidth() + 1
			<< "x" << bounds.IntegerHeight() + 1 << ", " << fImageType;
	}

	BString text;
	if (message != NULL && message->FindString("status", &text) == B_OK
		&& text.Length() > 0) {
		status << ", " << text;
	}

	fStatusView->Update(fNavigator.CurrentRef(), status);
}


void
ShowImageWindow::_LoadError(const entry_ref& ref)
{
	// TODO: give a better error message!
	BAlert* alert = new BAlert(B_TRANSLATE("ShowImage"),
		B_TRANSLATE_WITH_CONTEXT("Could not load image! Either the "
			"file or an image translator for it does not exist.",
			"LoadAlerts"),
		B_TRANSLATE_WITH_CONTEXT("OK", "Alerts"), NULL, NULL,
		B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
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


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ClosePrompt"


bool
ShowImageWindow::_ClosePrompt()
{
	if (!fModified)
		return true;

	int32 count = fNavigator.PageCount();
	int32 page = fNavigator.CurrentPage();
	BString prompt;

	if (count > 1) {
		bs_printf(&prompt,
			B_TRANSLATE("The document '%s' (page %d) has been changed. Do you "
				"want to close the document?"),
			fImageView->Image()->name, page);
	} else {
		bs_printf(&prompt,
			B_TRANSLATE("The document '%s' has been changed. Do you want to "
				"close the document?"),
			fImageView->Image()->name);
	}

	BAlert* alert = new BAlert(B_TRANSLATE("Close document"), prompt.String(),
		B_TRANSLATE("Cancel"), B_TRANSLATE("Close"));
	if (alert->Go() == 0) {
		// Cancel
		return false;
	}

	// Close
	fModified = false;
	return true;
}


status_t
ShowImageWindow::_LoadImage(bool forward)
{
	BMessenger us(this);
	status_t status = ImageCache::Default().RetrieveImage(
		fNavigator.CurrentRef(), fNavigator.CurrentPage(), &us);
	if (status != B_OK)
		return status;

	fProgressWindow->Start(this);

	// Preload previous/next images - two in the navigation direction, one
	// in the opposite direction.

	entry_ref nextRef = fNavigator.CurrentRef();
	if (_PreloadImage(forward, nextRef))
		_PreloadImage(forward, nextRef);

	entry_ref previousRef = fNavigator.CurrentRef();
	_PreloadImage(!forward, previousRef);

	return B_OK;
}


bool
ShowImageWindow::_PreloadImage(bool forward, entry_ref& ref)
{
	entry_ref currentRef = ref;
	if ((forward && !fNavigator.GetNextFile(currentRef, ref))
		|| (!forward && !fNavigator.GetPreviousFile(currentRef, ref)))
		return false;

	return ImageCache::Default().RetrieveImage(ref) == B_OK;
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

	MoveTo(frame.left, frame.top);
	ResizeTo(frame.Width(), frame.Height());

	fImageView->SetHideIdlingCursor(fFullScreen);
	fImageView->SetShowCaption(fFullScreen && fShowCaption);
	fImageView->FitToBounds();
}


void
ShowImageWindow::_ApplySettings()
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
	BPrintJob printJob(fImageView->Image()->name);
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
		BPrintJob printJob(fImageView->Image()->name);
		if (printJob.ConfigJob() == B_OK)
			fPrintSettings = printJob.Settings();
	}

	fPrintOptions.SetBounds(fImageView->Bitmap()->Bounds());
	fPrintOptions.SetWidth(fImageView->Bitmap()->Bounds().Width() + 1);

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

	BPrintJob printJob(fImageView->Image()->name);
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

		BBitmap* bitmap = fImageView->Bitmap();
		float imageWidth = bitmap->Bounds().Width() + 1.0;
		float imageHeight = bitmap->Bounds().Height() + 1.0;

		float width;
		switch (fPrintOptions.Option()) {
			case PrintOptions::kFitToPage: {
				float w1 = printableRect.Width()+1;
				float w2 = imageWidth * (printableRect.Height() + 1)
					/ imageHeight;
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


bool
ShowImageWindow::QuitRequested()
{
	if (fSavePanel) {
		// Don't allow this window to be closed if a save panel is open
		return false;
	}

	return _ClosePrompt();
}
