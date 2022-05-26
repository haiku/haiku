/*
 * Copyright 2003-2014, Haiku, Inc. All Rights Reserved.
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
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "ShowImageWindow.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Button.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <ControlLook.h>
#include <DurationFormat.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageRunner.h>
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
#include "ToolBarIcons.h"


// BMessage field names used in Save messages
const char* kTypeField = "be:type";
const char* kTranslatorField = "be:translator";

const bigtime_t kDefaultSlideShowDelay = 3000000;
	// 3 seconds


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
	MSG_SHOW_CAPTION			= 'mSCP',
	MSG_PAGE_SETUP				= 'mPSU',
	MSG_PREPARE_PRINT			= 'mPPT',
	MSG_GET_INFO				= 'mGFI',
	MSG_SET_RATING				= 'mSRT',
	kMsgFitToWindow				= 'mFtW',
	kMsgOriginalSize			= 'mOSZ',
	kMsgStretchToWindow			= 'mStW',
	kMsgNextSlide				= 'mNxS',
	kMsgToggleToolBar			= 'mTTB',
	kMsgSlideToolBar			= 'mSTB',
	kMsgFinishSlidingToolBar	= 'mFST'
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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Menus"


ShowImageWindow::ShowImageWindow(BRect frame, const entry_ref& ref,
	const BMessenger& trackerMessenger)
	:
	BWindow(frame, "", B_DOCUMENT_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS),
	fNavigator(ref, trackerMessenger),
	fSavePanel(NULL),
	fBar(NULL),
	fBrowseMenu(NULL),
	fGoToPageMenu(NULL),
	fSlideShowDelayMenu(NULL),
	fToolBar(NULL),
	fImageView(NULL),
	fStatusView(NULL),
	fProgressWindow(new ProgressWindow()),
	fModified(false),
	fFullScreen(false),
	fShowCaption(true),
	fShowToolBar(true),
	fPrintSettings(NULL),
	fSlideShowRunner(NULL),
	fSlideShowDelay(kDefaultSlideShowDelay)
{
	_ApplySettings();

	SetLayout(new BGroupLayout(B_VERTICAL, 0));

	// create menu bar
	fBar = new BMenuBar("menu_bar");
	_AddMenus(fBar);
	float menuBarMinWidth = fBar->MinSize().width;
	AddChild(fBar);

	// Add a content view so the tool bar can be moved outside of the
	// visible portion without colliding with the menu bar.

	BView* contentView = new BView(BRect(), "content", B_FOLLOW_NONE, 0);
	contentView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	contentView->SetExplicitMinSize(BSize(250, 100));
	AddChild(contentView);

	// Create the tool bar
	BRect viewFrame = contentView->Bounds();
	viewFrame.right -= be_control_look->GetScrollBarWidth(B_VERTICAL);
	fToolBar = new BToolBar(viewFrame);

	// Add the tool icons.

//	fToolBar->AddAction(MSG_FILE_OPEN, be_app,
//		tool_bar_icon(kIconDocumentOpen), B_TRANSLATE("Open" B_UTF8_ELLIPSIS));
	fToolBar->AddAction(MSG_FILE_PREV, this,
		tool_bar_icon(kIconGoPrevious), B_TRANSLATE("Previous file"));
	fToolBar->AddAction(MSG_FILE_NEXT, this, tool_bar_icon(kIconGoNext),
		B_TRANSLATE("Next file"));
	BMessage* fullScreenSlideShow = new BMessage(MSG_SLIDE_SHOW);
	fullScreenSlideShow->AddBool("full screen", true);
	fToolBar->AddAction(fullScreenSlideShow, this,
		tool_bar_icon(kIconMediaMovieLibrary), B_TRANSLATE("Slide show"));
	fToolBar->AddSeparator();
	fToolBar->AddAction(MSG_SELECTION_MODE, this,
		tool_bar_icon(kIconDrawRectangularSelection),
		B_TRANSLATE("Selection mode"));
	fToolBar->AddSeparator();
	fToolBar->AddAction(kMsgOriginalSize, this,
		tool_bar_icon(kIconZoomOriginal), B_TRANSLATE("Original size"), NULL,
		true);
	fToolBar->AddAction(kMsgFitToWindow, this,
		tool_bar_icon(kIconZoomFitBest), B_TRANSLATE("Fit to window"));
	fToolBar->AddAction(MSG_ZOOM_IN, this, tool_bar_icon(kIconZoomIn),
		B_TRANSLATE("Zoom in"));
	fToolBar->AddAction(MSG_ZOOM_OUT, this, tool_bar_icon(kIconZoomOut),
		B_TRANSLATE("Zoom out"));
	fToolBar->AddSeparator();
	fToolBar->AddAction(MSG_PAGE_PREV, this, tool_bar_icon(kIconPagePrevious),
		B_TRANSLATE("Previous page"));
	fToolBar->AddAction(MSG_PAGE_NEXT, this, tool_bar_icon(kIconPageNext),
		B_TRANSLATE("Next page"));
	fToolBar->AddGlue();
	fToolBar->AddAction(MSG_FULL_SCREEN, this,
		tool_bar_icon(kIconViewWindowed), B_TRANSLATE("Leave full screen"));
	fToolBar->SetActionVisible(MSG_FULL_SCREEN, false);

	fToolBar->ResizeTo(viewFrame.Width(), fToolBar->MinSize().height);

	contentView->AddChild(fToolBar);

	if (fShowToolBar)
		viewFrame.top = fToolBar->Frame().bottom + 1;
	else
		fToolBar->Hide();

	fToolBarVisible = fShowToolBar;

	viewFrame.bottom = contentView->Bounds().bottom;
	viewFrame.bottom -= be_control_look->GetScrollBarWidth(B_HORIZONTAL);

	// create the image view
	fImageView = new ShowImageView(viewFrame, "image_view", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED
			| B_FRAME_EVENTS);
	// wrap a scroll view around the view
	fScrollView = new BScrollView("image_scroller", fImageView,
		B_FOLLOW_ALL, 0, true, true, B_PLAIN_BORDER);
	contentView->AddChild(fScrollView);

	fStatusView = new ShowImageStatusView(fScrollView);
	fScrollView->AddChild(fStatusView);

	// Update minimum window size
	float toolBarMinWidth = fToolBar->MinSize().width;
	SetSizeLimits(std::max(menuBarMinWidth, toolBarMinWidth), 100000, 100,
		100000);

	// finish creating the window
	if (_LoadImage() != B_OK) {
		_LoadError(ref);
		Quit();
		return;
	}

	// add View menu here so it can access ShowImageView methods
	BMenu* menu = new BMenu(B_TRANSLATE_CONTEXT("View", "Menus"));
	_BuildViewMenu(menu, false);
	fBar->AddItem(menu);

	fBar->AddItem(_BuildRatingMenu());

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

	_StopSlideShow();
}


void
ShowImageWindow::BuildContextMenu(BMenu* menu)
{
	_BuildViewMenu(menu, true);
}


void
ShowImageWindow::_BuildViewMenu(BMenu* menu, bool popupMenu)
{
	_AddItemMenu(menu, B_TRANSLATE("Slide show"), MSG_SLIDE_SHOW, 0, 0, this);
	_MarkMenuItem(menu, MSG_SLIDE_SHOW, fSlideShowRunner != NULL);
	BMenu* delayMenu = new BMenu(B_TRANSLATE("Slide delay"));
	if (fSlideShowDelayMenu == NULL)
		fSlideShowDelayMenu = delayMenu;

	delayMenu->SetRadioMode(true);

	int32 kDelays[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 20};
	for (uint32 i = 0; i < sizeof(kDelays) / sizeof(kDelays[0]); i++) {
		BString text;
		BDurationFormat format;
		text.Truncate(0);
		format.Format(text, 0, kDelays[i] * 1000000LL);

		_AddDelayItem(delayMenu, text.String(), kDelays[i] * 1000000LL);
	}
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

	if (!popupMenu) {
		_AddItemMenu(menu, B_TRANSLATE("Show tool bar"), kMsgToggleToolBar,
			'B', 0, this);
		_MarkMenuItem(menu, kMsgToggleToolBar,
			!fToolBar->IsHidden(fToolBar));
	}

	if (popupMenu) {
		menu->AddSeparatorItem();
		_AddItemMenu(menu, B_TRANSLATE("Use as background" B_UTF8_ELLIPSIS),
			MSG_DESKTOP_BACKGROUND, 0, 0, this);
	}
}


BMenu*
ShowImageWindow::_BuildRatingMenu()
{
	fRatingMenu = new BMenu(B_TRANSLATE("Rating"));
	for (int32 i = 1; i <= 10; i++) {
		BString label;
		label << i;
		BMessage* message = new BMessage(MSG_SET_RATING);
		message->AddInt32("rating", i);
		fRatingMenu->AddItem(new BMenuItem(label.String(), message));
	}
	// NOTE: We may want to encapsulate the Rating menu within a more
	// general "Attributes" menu.
	return fRatingMenu;
}


void
ShowImageWindow::_AddMenus(BMenuBar* bar)
{
	BMenu* menu = new BMenu(B_TRANSLATE("File"));

	// Add recent files to "Open File" entry as sub-menu.
	BMenuItem* item = new BMenuItem(BRecentFilesList::NewFileListMenu(
		B_TRANSLATE("Open" B_UTF8_ELLIPSIS), NULL, NULL, be_app, 10, true,
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
	_AddItemMenu(menu, B_TRANSLATE("Move to Trash"), kMsgDeleteCurrentFile, 'T', 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Get info" B_UTF8_ELLIPSIS),
		MSG_GET_INFO, 'I', 0, this);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Page setup" B_UTF8_ELLIPSIS),
		MSG_PAGE_SETUP, 0, 0, this);
	_AddItemMenu(menu, B_TRANSLATE("Print" B_UTF8_ELLIPSIS),
		MSG_PREPARE_PRINT, 'P', 0, this);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Quit"), B_QUIT_REQUESTED, 'Q', 0, be_app);
	bar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Edit"));
	_AddItemMenu(menu, B_TRANSLATE("Copy"), B_COPY, 'C', 0, this, false);
	menu->AddSeparatorItem();
	_AddItemMenu(menu, B_TRANSLATE("Selection mode"), MSG_SELECTION_MODE, 0, 0,
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
ShowImageWindow::_AddDelayItem(BMenu* menu, const char* label, bigtime_t delay)
{
	BMessage* message = new BMessage(MSG_SLIDE_SHOW_DELAY);
	message->AddInt64("delay", delay);

	BMenuItem* item = new BMenuItem(label, message, 0);
	item->SetTarget(this);

	if (delay == fSlideShowDelay)
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
	float width = r.Width() + be_control_look->GetScrollBarWidth(B_VERTICAL);
	float height = r.Height() + 1 + fBar->Frame().Height()
		+ be_control_look->GetScrollBarWidth(B_HORIZONTAL);

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
	fToolBar->SetActionPressed(what, marked);
	return marked;
}


void
ShowImageWindow::_EnableMenuItem(BMenu* menu, uint32 what, bool enable)
{
	BMenuItem* item = menu->FindItem(what);
	if (item && item->IsEnabled() != enable)
		item->SetEnabled(enable);
	fToolBar->SetActionEnabled(what, enable);
}


void
ShowImageWindow::_MarkMenuItem(BMenu* menu, uint32 what, bool marked)
{
	BMenuItem* item = menu->FindItem(what);
	if (item && item->IsMarked() != marked)
		item->SetMarked(marked);
	fToolBar->SetActionPressed(what, marked);
}


void
ShowImageWindow::_MarkSlideShowDelay(bigtime_t delay)
{
	const int32 count = fSlideShowDelayMenu->CountItems();
	for (int32 i = 0; i < count; i ++) {
		BMenuItem* item = fSlideShowDelayMenu->ItemAt(i);
		if (item != NULL) {
			bigtime_t itemDelay;
			if (item->Message()->FindInt64("delay", &itemDelay) == B_OK
				&& itemDelay == delay) {
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

			BitmapOwner* bitmapOwner = NULL;
			message->FindPointer("bitmapOwner", (void**)&bitmapOwner);

			bool first = fImageView->Bitmap() == NULL;
			entry_ref ref;
			message->FindRef("ref", &ref);
			if (!first && ref != fNavigator.CurrentRef()) {
				// ignore older images
				if (bitmapOwner != NULL)
					bitmapOwner->ReleaseReference();
				break;
			}

			int32 page = message->FindInt32("page");
			int32 pageCount = message->FindInt32("pageCount");
			if (!first && page != fNavigator.CurrentPage()) {
				// ignore older pages
				if (bitmapOwner != NULL)
					bitmapOwner->ReleaseReference();
				break;
			}

			status_t status = fImageView->SetImage(message);
			if (status != B_OK) {
				if (bitmapOwner != NULL)
					bitmapOwner->ReleaseReference();

				_LoadError(ref);

				// quit if file could not be opened
				if (first)
					Quit();
				break;
			}

			fImageType = message->FindString("type");
			fNavigator.SetTo(ref, page, pageCount);

			fImageView->FitToBounds();
			if (first) {
				fImageView->MakeFocus(true);
					// to receive key messages
				Show();
			}
			_UpdateRatingMenu();
			// Set width and height attributes of the currently showed file.
			// This should only be a temporary solution.
			_SaveWidthAndHeight();
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

			_EnableMenuItem(fBar, MSG_PAGE_FIRST,
				fNavigator.HasPreviousPage());
			_EnableMenuItem(fBar, MSG_PAGE_LAST, fNavigator.HasNextPage());
			_EnableMenuItem(fBar, MSG_PAGE_NEXT, fNavigator.HasNextPage());
			_EnableMenuItem(fBar, MSG_PAGE_PREV, fNavigator.HasPreviousPage());
			fGoToPageMenu->SetEnabled(pages > 1);

			_EnableMenuItem(fBar, MSG_FILE_NEXT, fNavigator.HasNextFile());
			_EnableMenuItem(fBar, MSG_FILE_PREV, fNavigator.HasPreviousFile());

			if (fGoToPageMenu->CountItems() != pages) {
				// Only rebuild the submenu if the number of
				// pages is different

				while (fGoToPageMenu->CountItems() > 0) {
					// Remove all page numbers
					delete fGoToPageMenu->RemoveItem((int32)0);
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
						shortcut, B_SHIFT_KEY);
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

		case B_COPY:
			fImageView->CopySelectionToClipboard();
			break;

		case MSG_SELECTION_MODE:
		{
			bool selectionMode = _ToggleMenuItem(MSG_SELECTION_MODE);
			fImageView->SetSelectionMode(selectionMode);
			if (!selectionMode)
				fImageView->ClearSelection();
			break;
		}

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
		case kMsgNextSlide:
			if (_ClosePrompt()) {
				if (!fNavigator.NextFile()) {
					// Wrap back around
					fNavigator.FirstFile();
				}
				_LoadImage();
			}
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

		case MSG_GET_INFO:
			_GetFileInfo(fNavigator.CurrentRef());
			break;

		case MSG_SLIDE_SHOW:
		{
			bool fullScreen = false;
			message->FindBool("full screen", &fullScreen);

			BMenuItem* item = fBar->FindItem(message->what);
			if (item == NULL)
				break;

			if (item->IsMarked()) {
				item->SetMarked(false);
				_StopSlideShow();
				fToolBar->SetActionPressed(MSG_SLIDE_SHOW, false);
			} else if (_ClosePrompt()) {
				item->SetMarked(true);
				if (!fFullScreen && fullScreen)
					_ToggleFullScreen();
				_StartSlideShow();
				fToolBar->SetActionPressed(MSG_SLIDE_SHOW, true);
			}
			break;
		}

		case kMsgStopSlideShow:
		{
			BMenuItem* item = fBar->FindItem(MSG_SLIDE_SHOW);
			if (item != NULL)
				item->SetMarked(false);

			_StopSlideShow();
			fToolBar->SetActionPressed(MSG_SLIDE_SHOW, false);
			break;
		}

		case MSG_SLIDE_SHOW_DELAY:
		{
			bigtime_t delay;
			if (message->FindInt64("delay", &delay) == B_OK) {
				_SetSlideShowDelay(delay);
				// in case message is sent from popup menu
				_MarkSlideShowDelay(delay);
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

		case MSG_SHOW_CAPTION:
		{
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

		case MSG_UPDATE_STATUS_ZOOM:
			fStatusView->SetZoom(fImageView->Zoom());
			break;

		case kMsgOriginalSize:
			if (message->FindInt32("behavior") == BButton::B_TOGGLE_BEHAVIOR) {
				bool force = (message->FindInt32("be:value") == B_CONTROL_ON);
				fImageView->ForceOriginalSize(force);
				if (!force)
					break;
			}
			fImageView->SetZoom(1.0);
			break;

		case MSG_SCALE_BILINEAR:
			fImageView->SetScaleBilinear(_ToggleMenuItem(message->what));
			break;

		case MSG_DESKTOP_BACKGROUND:
		{
			BMessage backgroundsMessage(B_REFS_RECEIVED);
			backgroundsMessage.AddRef("refs", fImageView->Image());
			// This is used in the Backgrounds code for scaled placement
			backgroundsMessage.AddInt32("placement", 'scpl');
			be_roster->Launch("application/x-vnd.haiku-backgrounds",
				&backgroundsMessage);
			break;
		}

		case MSG_SET_RATING:
		{
			int32 rating;
			if (message->FindInt32("rating", &rating) != B_OK)
				break;
			BFile file(&fNavigator.CurrentRef(), B_WRITE_ONLY);
			if (file.InitCheck() != B_OK)
				break;
			file.WriteAttr("Media:Rating", B_INT32_TYPE, 0, &rating,
				sizeof(rating));
			_UpdateRatingMenu();
			break;
		}

		case kMsgToggleToolBar:
		{
			fShowToolBar = _ToggleMenuItem(message->what);
			_SetToolBarVisible(fShowToolBar, true);

			ShowImageSettings* settings = my_app->Settings();

			if (settings->Lock()) {
				settings->SetBool("ShowToolBar", fShowToolBar);
				settings->Unlock();
			}
			break;
		}
		case kShowToolBarIfEnabled:
		{
			bool show;
			if (message->FindBool("show", &show) != B_OK)
				break;
			_SetToolBarVisible(fShowToolBar && show, true);
			break;
		}
		case kMsgSlideToolBar:
		{
			float offset;
			if (message->FindFloat("offset", &offset) == B_OK) {
				fToolBar->MoveBy(0, offset);
				fScrollView->ResizeBy(0, -offset);
				fScrollView->MoveBy(0, offset);
				UpdateIfNeeded();
				snooze(15000);
			}
			break;
		}
		case kMsgFinishSlidingToolBar:
		{
			float offset;
			bool show;
			if (message->FindFloat("offset", &offset) == B_OK
				&& message->FindBool("show", &show) == B_OK) {
				// Compensate rounding errors with the final placement
				fToolBar->MoveTo(fToolBar->Frame().left, offset);
				if (!show)
					fToolBar->Hide();
				BRect frame = fToolBar->Parent()->Bounds();
				frame.top = fToolBar->Frame().bottom + 1;
				fScrollView->MoveTo(fScrollView->Frame().left, frame.top);
				fScrollView->ResizeTo(fScrollView->Bounds().Width(),
					frame.Height() + 1);
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ShowImageWindow::_GetFileInfo(const entry_ref& ref)
{
	BMessage message('Tinf');
	BMessenger tracker("application/x-vnd.Be-TRAK");
	message.AddRef("refs", &ref);
	tracker.SendMessage(&message);
}


void
ShowImageWindow::_UpdateStatusText(const BMessage* message)
{
	BString frameText;
	if (fImageView->Bitmap() != NULL) {
		BRect bounds = fImageView->Bitmap()->Bounds();
		frameText << bounds.IntegerWidth() + 1
			<< "x" << bounds.IntegerHeight() + 1;
	}
	BString pages;
	if (fNavigator.PageCount() > 1)
		pages << fNavigator.CurrentPage() << "/" << fNavigator.PageCount();
	fStatusView->Update(fNavigator.CurrentRef(), frameText, pages, fImageType,
		fImageView->Zoom());
}


void
ShowImageWindow::_LoadError(const entry_ref& ref)
{
	// TODO: give a better error message!
	BAlert* alert = new BAlert(B_TRANSLATE_SYSTEM_NAME("ShowImage"),
		B_TRANSLATE_CONTEXT("Could not load image! Either the "
			"file or an image translator for it does not exist.",
			"LoadAlerts"),
		B_TRANSLATE_CONTEXT("OK", "Alerts"), NULL, NULL,
		B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
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

	// Retrieve save directory from settings;
	ShowImageSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		fSavePanel->SetPanelDirectory(
			settings->GetString("SaveDirectory", NULL));
		settings->Unlock();
	}

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
		if (outFormat[i].group == B_TRANSLATOR_BITMAP && outFormat[i].type
				== outType)
			break;
	}
	if (i == outCount)
		return;

	// Write out the image file
	BDirectory dir(&dirRef);
	fImageView->SaveToFile(&dir, filename, NULL, &outFormat[i]);

	// Store Save directory in settings;
	ShowImageSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		BPath path(&dirRef);
		settings->SetString("SaveDirectory", path.Path());
		settings->Unlock();
	}
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ClosePrompt"


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
	alert->SetShortcut(0, B_ESCAPE);

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
	// If the user triggered a _LoadImage while in a slide show,
	// make sure the new image is shown for the set delay:
	_ResetSlideShowDelay();

	BMessenger us(this);
	status_t status = my_app->DefaultCache().RetrieveImage(
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

	return my_app->DefaultCache().RetrieveImage(ref) == B_OK;
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
		frame.top -= fBar->Bounds().Height() + 1;
		frame.right += be_control_look->GetScrollBarWidth(B_VERTICAL);
		frame.bottom += be_control_look->GetScrollBarWidth(B_HORIZONTAL);

		SetFlags(Flags() | B_NOT_RESIZABLE | B_NOT_MOVABLE);

		Activate();
			// make the window frontmost
	} else {
		frame = fWindowFrame;

		SetFlags(Flags() & ~(B_NOT_RESIZABLE | B_NOT_MOVABLE));
	}

	fToolBar->SetActionVisible(MSG_FULL_SCREEN, fFullScreen);
	_SetToolBarVisible(!fFullScreen && fShowToolBar);
	_SetToolBarBorder(!fFullScreen);

	MoveTo(frame.left, frame.top);
	ResizeTo(frame.Width(), frame.Height());

	fImageView->SetHideIdlingCursor(fFullScreen);
	fImageView->SetShowCaption(fFullScreen && fShowCaption);

	Layout(false);
		// We need to manually relayout here, as the views are layouted
		// asynchronously, and FitToBounds() would still have the wrong size
	fImageView->FitToBounds();
}


void
ShowImageWindow::_ApplySettings()
{
	ShowImageSettings* settings = my_app->Settings();

	if (settings->Lock()) {
		fShowCaption = settings->GetBool("ShowCaption", fShowCaption);
		fPrintOptions.SetBounds(BRect(0, 0, 1023, 767));

		fSlideShowDelay = settings->GetTime("SlideShowDelay", fSlideShowDelay);

		fPrintOptions.SetOption((enum PrintOptions::Option)settings->GetInt32(
			"PO:Option", fPrintOptions.Option()));
		fPrintOptions.SetZoomFactor(
			settings->GetFloat("PO:ZoomFactor", fPrintOptions.ZoomFactor()));
		fPrintOptions.SetDPI(settings->GetFloat("PO:DPI", fPrintOptions.DPI()));
		fPrintOptions.SetWidth(
			settings->GetFloat("PO:Width", fPrintOptions.Width()));
		fPrintOptions.SetHeight(
			settings->GetFloat("PO:Height", fPrintOptions.Height()));

		fShowToolBar = settings->GetBool("ShowToolBar", fShowToolBar);

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
				float w1 = printableRect.Width() + 1;
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


void
ShowImageWindow::_SetSlideShowDelay(bigtime_t delay)
{
	if (fSlideShowDelay == delay)
		return;

	fSlideShowDelay = delay;

	ShowImageSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->SetTime("SlideShowDelay", fSlideShowDelay);
		settings->Unlock();
	}

	if (fSlideShowRunner != NULL)
		_StartSlideShow();
}


void
ShowImageWindow::_StartSlideShow()
{
	_StopSlideShow();

	BMessage nextSlide(kMsgNextSlide);
	fSlideShowRunner = new BMessageRunner(this, &nextSlide, fSlideShowDelay);
}


void
ShowImageWindow::_StopSlideShow()
{
	if (fSlideShowRunner != NULL) {
		delete fSlideShowRunner;
		fSlideShowRunner = NULL;
	}
}


void
ShowImageWindow::_ResetSlideShowDelay()
{
	if (fSlideShowRunner != NULL)
		fSlideShowRunner->SetInterval(fSlideShowDelay);
}


void
ShowImageWindow::_UpdateRatingMenu()
{
	BFile file(&fNavigator.CurrentRef(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;
	int32 rating;
	ssize_t size = sizeof(rating);
	if (file.ReadAttr("Media:Rating", B_INT32_TYPE, 0, &rating, size) != size)
		rating = 0;
	// TODO: Finding the correct item could be more robust, like by looking
	// at the message of each item.
	for (int32 i = 1; i <= 10; i++) {
		BMenuItem* item = fRatingMenu->ItemAt(i - 1);
		if (item == NULL)
			break;
		item->SetMarked(i == rating);
	}
}


void
ShowImageWindow::_SaveWidthAndHeight()
{
	if (fNavigator.CurrentPage() != 1)
		return;

	if (fImageView->Bitmap() == NULL)
		return;

	BRect bounds = fImageView->Bitmap()->Bounds();
	int32 width = bounds.IntegerWidth() + 1;
	int32 height = bounds.IntegerHeight() + 1;

	BNode node(&fNavigator.CurrentRef());
	if (node.InitCheck() != B_OK)
		return;

	const char* kWidthAttrName = "Media:Width";
	const char* kHeightAttrName = "Media:Height";

	int32 widthAttr;
	ssize_t attrSize = node.ReadAttr(kWidthAttrName, B_INT32_TYPE, 0,
		&widthAttr, sizeof(widthAttr));
	if (attrSize <= 0 || widthAttr != width)
		node.WriteAttr(kWidthAttrName, B_INT32_TYPE, 0, &width, sizeof(width));

	int32 heightAttr;
	attrSize = node.ReadAttr(kHeightAttrName, B_INT32_TYPE, 0,
		&heightAttr, sizeof(heightAttr));
	if (attrSize <= 0 || heightAttr != height)
		node.WriteAttr(kHeightAttrName, B_INT32_TYPE, 0, &height, sizeof(height));
}


void
ShowImageWindow::_SetToolBarVisible(bool visible, bool animate)
{
	if (visible == fToolBarVisible)
		return;

	fToolBarVisible = visible;
	float diff = fToolBar->Bounds().Height() + 2;
	if (!visible)
		diff = -diff;
	else
		fToolBar->Show();

	if (animate) {
		// Slide the controls into view. We do this with messages in order
		// not to block the window thread.
		const float kAnimationOffsets[] = { 0.05, 0.2, 0.5, 0.2, 0.05 };
		const int32 steps = sizeof(kAnimationOffsets) / sizeof(float);
		for (int32 i = 0; i < steps; i++) {
			BMessage message(kMsgSlideToolBar);
			message.AddFloat("offset", floorf(diff * kAnimationOffsets[i]));
			PostMessage(&message, this);
		}
		BMessage finalMessage(kMsgFinishSlidingToolBar);
		finalMessage.AddFloat("offset", visible ? 0 : diff);
		finalMessage.AddBool("show", visible);
		PostMessage(&finalMessage, this);
	} else {
		fScrollView->ResizeBy(0, -diff);
		fScrollView->MoveBy(0, diff);
		fToolBar->MoveBy(0, diff);
		if (!visible)
			fToolBar->Hide();
	}
}


void
ShowImageWindow::_SetToolBarBorder(bool visible)
{
	float inset = visible
		? ceilf(be_control_look->DefaultItemSpacing() / 2) : 0;

	fToolBar->GroupLayout()->SetInsets(inset, 0, inset, 0);
}


bool
ShowImageWindow::QuitRequested()
{
	if (fSavePanel) {
		// Don't allow this window to be closed if a save panel is open
		return false;
	}

	if (!_ClosePrompt())
		return false;

	ShowImageSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		if (fFullScreen)
			settings->SetRect("WindowFrame", fWindowFrame);
		else
			settings->SetRect("WindowFrame", Frame());
		settings->Unlock();
	}

	be_app->PostMessage(MSG_WINDOW_HAS_QUIT);

	return true;
}
