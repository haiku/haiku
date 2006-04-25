/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 */


#include "BackgroundImage.h"
#include "EntryMenuItem.h"
#include "ShowImageApp.h"
#include "ShowImageConstants.h"
#include "ShowImageStatusView.h"
#include "ShowImageView.h"
#include "ShowImageWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Clipboard.h>
#include <Entry.h>
#include <File.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <PrintJob.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <SupportDefs.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>

#include <new>
#include <stdio.h>


RecentDocumentsMenu::RecentDocumentsMenu(const char *title, menu_layout layout)
	: BMenu(title, layout)
{
}


bool 
RecentDocumentsMenu::AddDynamicItem(add_state addState)
{
	if (addState != B_INITIAL_ADD)
		return false;

	BMenuItem *item;
	BMessage list, *msg;
	entry_ref ref;
	char name[B_FILE_NAME_LENGTH];

	while ((item = RemoveItem((int32)0)) != NULL) {
		delete item;
	}

	be_roster->GetRecentDocuments(&list, 20, NULL, kApplicationSignature);
	for (int i = 0; list.FindRef("refs", i, &ref) == B_OK; i++) {
		BEntry entry(&ref);
		if (entry.Exists() && entry.GetName(name) == B_OK) {
			msg = new BMessage(B_REFS_RECEIVED);
			msg->AddRef("refs", &ref);
			item =  new EntryMenuItem(&ref, name, msg, 0, 0);
			AddItem(item);
			item->SetTarget(be_app, NULL);
		}
	}

	return false;
}


//	#pragma mark -


ShowImageWindow::ShowImageWindow(const entry_ref *ref,
		const BMessenger& trackerMessenger)
	: BWindow(BRect(5, 24, 250, 100), "", B_DOCUMENT_WINDOW, 0)
{
	fSavePanel = NULL;
	fModified = false;
	fFullScreen = false;
	fShowCaption = true;
	fPrintSettings = NULL;
	fImageView = NULL;
	fSlideShowDelay = NULL;

	LoadSettings();	

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
	BScrollView *scrollView = new BScrollView("image_scroller", fImageView,
		B_FOLLOW_ALL, 0, false, false, B_PLAIN_BORDER);
	AddChild(scrollView);

	const int32 kstatusWidth = 190;
	BRect rect;
	rect = Bounds();
	rect.top	= viewFrame.bottom + 1;
	rect.left 	= viewFrame.left + kstatusWidth;
	rect.right	= viewFrame.right + 1;	
	rect.bottom += 1;
	BScrollBar *horizontalScrollBar = new BScrollBar(rect, "hscroll",
		fImageView, 0, 150, B_HORIZONTAL);
	AddChild(horizontalScrollBar);

	rect.left = 0;
	rect.right = kstatusWidth - 1;	
	rect.bottom -= 1;
	fStatusView = new ShowImageStatusView(rect, "status_view", B_FOLLOW_BOTTOM,
		B_WILL_DRAW);
	AddChild(fStatusView);
	
	rect = Bounds();
	rect.top    = viewFrame.top - 1;
	rect.left 	= viewFrame.right + 1;
	rect.bottom	= viewFrame.bottom + 1;
	rect.right	+= 1;
	BScrollBar *verticalScrollBar = new BScrollBar(rect, "vscroll", fImageView,
		0, 150, B_VERTICAL);
	AddChild(verticalScrollBar);

	SetSizeLimits(250, 100000, 100, 100000);
	
	// finish creating the window
	fImageView->SetImage(ref);
	fImageView->SetTrackerMessenger(trackerMessenger);

	if (InitCheck() == B_OK) {
		// add View menu here so it can access ShowImageView methods 
		BMenu* menu = new BMenu("View");
		BuildViewMenu(menu);
		fBar->AddItem(menu);		
		MarkMenuItem(fBar, MSG_DITHER_IMAGE, fImageView->GetDither());
		UpdateTitle();

		SetPulseRate(100000);
			// every 1/10 second; ShowImageView needs it for marching ants

		fImageView->FlushToLeftTop();
		WindowRedimension(fImageView->GetBitmap());
		fImageView->MakeFocus(true); // to receive KeyDown messages
		Show();
	} else {
		BAlert* alert;
		alert = new BAlert("ShowImage", 
			"Could not load image! Either the file or an image translator for it does not exist.", 
			"OK", NULL, NULL, 
			B_WIDTH_AS_USUAL, B_INFO_ALERT);
		alert->Go();

		// exit if file could not be opened
		PostMessage(B_QUIT_REQUESTED);
		return;
	}

	// Tell application object to query the clipboard
	// and tell this window if it contains interesting data or not
	be_app_messenger.SendMessage(B_CLIPBOARD_CHANGED);
}


ShowImageWindow::~ShowImageWindow()
{
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
ShowImageWindow::BuildViewMenu(BMenu *menu)
{
	AddItemMenu(menu, "Slide Show", MSG_SLIDE_SHOW, 0, 0, 'W', true);
	MarkMenuItem(menu, MSG_SLIDE_SHOW, fImageView->SlideShowStarted());
	BMenu* delayMenu = new BMenu("Slide Delay");
	if (fSlideShowDelay == NULL)
		fSlideShowDelay = delayMenu;

	delayMenu->SetRadioMode(true);
	// Note: ShowImage loads images in window thread so it becomes unresponsive if
	// slide show delay is too short! (Especially if loading the image takes as long as
	// or longer than the slide show delay). Should load in background thread!
	AddDelayItem(delayMenu, "Three Seconds", 3);
	AddDelayItem(delayMenu, "Four Second", 4);
	AddDelayItem(delayMenu, "Five Seconds", 5);
	AddDelayItem(delayMenu, "Six Seconds", 6);
	AddDelayItem(delayMenu, "Seven Seconds", 7);
	AddDelayItem(delayMenu, "Eight Seconds", 8);
	AddDelayItem(delayMenu, "Nine Seconds", 9);
	AddDelayItem(delayMenu, "Ten Seconds", 10);
	AddDelayItem(delayMenu, "Twenty Seconds", 20);
	menu->AddItem(delayMenu);

	menu->AddSeparatorItem();

	AddItemMenu(menu, "Original Size", MSG_ORIGINAL_SIZE, 0, 0, 'W', true);
	AddItemMenu(menu, "Zoom In", MSG_ZOOM_IN, '+', 0, 'W', true);
	AddItemMenu(menu, "Zoom Out", MSG_ZOOM_OUT, '-', 0, 'W', true);	

	menu->AddSeparatorItem();

	AddItemMenu(menu, "Scale Bilinear", MSG_SCALE_BILINEAR, 0, 0, 'W', true);

	menu->AddSeparatorItem();

	AddItemMenu(menu, "Shrink to Window", MSG_SHRINK_TO_WINDOW, 0, 0, 'W', true);
	AddItemMenu(menu, "Zoom to Window", MSG_ZOOM_TO_WINDOW, 0, 0, 'W', true);

	menu->AddSeparatorItem();

	AddItemMenu(menu, "Full Screen", MSG_FULL_SCREEN, 'F', 0, 'W', true);
	MarkMenuItem(menu, MSG_FULL_SCREEN, fFullScreen);

	AddShortcut(B_ENTER, 0, new BMessage(MSG_FULL_SCREEN));

	AddItemMenu(menu, "Show Caption in Full Screen Mode", MSG_SHOW_CAPTION, 0, 0, 'W', true);
	MarkMenuItem(menu, MSG_SHOW_CAPTION, fShowCaption);

	MarkMenuItem(menu, MSG_SCALE_BILINEAR, fImageView->GetScaleBilinear());

	bool shrink, zoom, enabled;

	shrink = fImageView->GetShrinkToBounds();
	zoom = fImageView->GetZoomToBounds();
	MarkMenuItem(menu, MSG_SHRINK_TO_WINDOW, shrink);
	MarkMenuItem(menu, MSG_ZOOM_TO_WINDOW, zoom);

 	enabled = !(shrink || zoom);
	EnableMenuItem(menu, MSG_ORIGINAL_SIZE, enabled);
	EnableMenuItem(menu, MSG_ZOOM_IN, enabled);
	EnableMenuItem(menu, MSG_ZOOM_OUT, enabled);
	
	menu->AddSeparatorItem();

	AddItemMenu(menu, "As Desktop Background", MSG_DESKTOP_BACKGROUND, 0, 0, 'W',
		true);
}


void
ShowImageWindow::AddMenus(BMenuBar *bar)
{
	BMenu *menu = new BMenu("File");
	fOpenMenu = new RecentDocumentsMenu("Open");
	menu->AddItem(fOpenMenu);
	fOpenMenu->Superitem()->SetTrigger('O');
	fOpenMenu->Superitem()->SetMessage(new BMessage(MSG_FILE_OPEN));
	fOpenMenu->Superitem()->SetTarget(be_app);
	fOpenMenu->Superitem()->SetShortcut('O', 0);
	menu->AddSeparatorItem();
	BMenu *pmenuSaveAs = new BMenu("Save As" B_UTF8_ELLIPSIS, B_ITEMS_IN_COLUMN);
	BTranslationUtils::AddTranslationItems(pmenuSaveAs, B_TRANSLATOR_BITMAP);
		// Fill Save As submenu with all types that can be converted
		// to from the Be bitmap image format
	menu->AddItem(pmenuSaveAs);
	AddItemMenu(menu, "Close", B_QUIT_REQUESTED, 'W', 0, 'W', true);
	menu->AddSeparatorItem();
	AddItemMenu(menu, "Page Setup" B_UTF8_ELLIPSIS, MSG_PAGE_SETUP, 0, 0, 'W', true);
	AddItemMenu(menu, "Print" B_UTF8_ELLIPSIS, MSG_PREPARE_PRINT, 0, 0, 'W', true);
	menu->AddSeparatorItem();
	AddItemMenu(menu, "About ShowImage" B_UTF8_ELLIPSIS, B_ABOUT_REQUESTED, 0, 0, 'A', true);
	menu->AddSeparatorItem();
	AddItemMenu(menu, "Quit", B_QUIT_REQUESTED, 'Q', 0, 'A', true);
	bar->AddItem(menu);

	menu = new BMenu("Edit");
	AddItemMenu(menu, "Undo", B_UNDO, 'Z', 0, 'W', false);
	menu->AddSeparatorItem();
	AddItemMenu(menu, "Cut", B_CUT, 'X', 0, 'W', false);
	AddItemMenu(menu, "Copy", B_COPY, 'C', 0, 'W', false);
	AddItemMenu(menu, "Paste", B_PASTE, 'V', 0, 'W', false);
	AddItemMenu(menu, "Clear", MSG_CLEAR_SELECT, 0, 0, 'W', false);
	menu->AddSeparatorItem();
	AddItemMenu(menu, "Select All", MSG_SELECT_ALL, 'A', 0, 'W', true);
	bar->AddItem(menu);

	menu = fBrowseMenu = new BMenu("Browse");
	AddItemMenu(menu, "First Page", MSG_PAGE_FIRST, B_LEFT_ARROW, B_SHIFT_KEY, 'W', true);
	AddItemMenu(menu, "Last Page", MSG_PAGE_LAST, B_RIGHT_ARROW, B_SHIFT_KEY, 'W', true);
	AddItemMenu(menu, "Previous Page", MSG_PAGE_PREV, B_LEFT_ARROW, 0, 'W', true);
	AddItemMenu(menu, "Next Page", MSG_PAGE_NEXT, B_RIGHT_ARROW, 0, 'W', true);
	fGoToPageMenu = new BMenu("Go To Page");
	fGoToPageMenu->SetRadioMode(true);
	menu->AddItem(fGoToPageMenu);
	menu->AddSeparatorItem();
	AddItemMenu(menu, "Previous File", MSG_FILE_PREV, B_UP_ARROW, 0, 'W', true);
	AddItemMenu(menu, "Next File", MSG_FILE_NEXT, B_DOWN_ARROW, 0, 'W', true);
	bar->AddItem(menu);

	menu = new BMenu("Image");
	AddItemMenu(menu, "Dither Image", MSG_DITHER_IMAGE, 0, 0, 'W', true);
	menu->AddSeparatorItem();
	AddItemMenu(menu, "Rotate -90°", MSG_ROTATE_270, '[', 0, 'W', true);
	AddItemMenu(menu, "Rotate +90°", MSG_ROTATE_90, ']', 0, 'W', true);
	menu->AddSeparatorItem();
	AddItemMenu(menu, "Flip Left To Right", MSG_FLIP_LEFT_TO_RIGHT, 0, 0, 'W', true);
	AddItemMenu(menu, "Flip Top To Bottom", MSG_FLIP_TOP_TO_BOTTOM, 0, 0, 'W', true);
	menu->AddSeparatorItem();
	AddItemMenu(menu, "Invert", MSG_INVERT, 0, 0, 'W', true);
	bar->AddItem(menu);
}


BMenuItem *
ShowImageWindow::AddItemMenu(BMenu *menu, char *caption, uint32 command, 
	char shortcut, uint32 modifier, char target, bool enabled)
{
	BMenuItem* item = new BMenuItem(caption, new BMessage(command), shortcut, modifier);

	if (target == 'A')
		item->SetTarget(be_app);
	else
		item->SetTarget(this);

	item->SetEnabled(enabled);
	menu->AddItem(item);

	return item;
}


BMenuItem*
ShowImageWindow::AddDelayItem(BMenu *menu, char *caption, float value)
{
	BMessage* message = new BMessage(MSG_SLIDE_SHOW_DELAY);
	message->AddFloat("value", value);

	BMenuItem* item = new BMenuItem(caption, message, 0);
	item->SetTarget(this);

	bool marked = fImageView->GetSlideShowDelay() == value;
	if (marked)
		item->SetMarked(true);

	menu->AddItem(item);
	return item;
}


void
ShowImageWindow::WindowRedimension(BBitmap *pbitmap)
{
	BScreen screen;
	if (!screen.IsValid())
		return;

	BRect r(pbitmap->Bounds());
	const float windowBorderWidth = 5;
	const float windowBorderHeight = 5;

	float width = r.Width() + 2 * PEN_SIZE + B_V_SCROLL_BAR_WIDTH;
	float height = r.Height() + 2 * PEN_SIZE + 1 + fBar->Frame().Height() + B_H_SCROLL_BAR_HEIGHT;

	// dimensions so that window does not reach outside of screen	
	float maxWidth = screen.Frame().Width() + 1 - windowBorderWidth - Frame().left;
	float maxHeight = screen.Frame().Height() + 1 - windowBorderHeight - Frame().top;

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
}


bool
ShowImageWindow::ToggleMenuItem(uint32 what)
{
	BMenuItem *item;
	bool marked = false;
	item = fBar->FindItem(what);
	if (item != NULL) {
		marked = !item->IsMarked();
		item->SetMarked(marked);
	}
	return marked;
}


void
ShowImageWindow::EnableMenuItem(BMenu *menu, uint32 what, bool enable)
{
	BMenuItem* item;
	item = menu->FindItem(what);
	if (item && item->IsEnabled() != enable) {
		item->SetEnabled(enable);
	}
}


void 
ShowImageWindow::MarkMenuItem(BMenu *menu, uint32 what, bool marked)
{
	BMenuItem* item;
	item = menu->FindItem(what);
	if (item && item->IsMarked() != marked) {
		item->SetMarked(marked);
	}
}


void
ShowImageWindow::MarkSlideShowDelay(float value)
{
	const int32 n = fSlideShowDelay->CountItems();
	float v;
	for (int32 i = 0; i < n; i ++) {
		BMenuItem* item = fSlideShowDelay->ItemAt(i);
		if (item) {
			if (item->Message()->FindFloat("value", &v) == B_OK && v == value) {
				if (!item->IsMarked()) {
					item->SetMarked(true);
				}
				return;
			}
		}
	}
}


void
ShowImageWindow::ResizeToWindow(bool shrink, uint32 what)
{
	bool enabled;
	enabled = ToggleMenuItem(what);
	if (shrink) {
		fImageView->SetShrinkToBounds(enabled);
	} else {
		fImageView->SetZoomToBounds(enabled);
	}
	enabled = !(fImageView->GetShrinkToBounds() || fImageView->GetZoomToBounds());
	EnableMenuItem(fBar, MSG_ORIGINAL_SIZE, enabled);
	EnableMenuItem(fBar, MSG_ZOOM_IN, enabled);
	EnableMenuItem(fBar, MSG_ZOOM_OUT, enabled);
}


void
ShowImageWindow::MessageReceived(BMessage *message)
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
				SaveAs(message);
			break;

		case MSG_SAVE_PANEL:
			// User specified where to save the output image
			SaveToFile(message);
			break;

		case B_CANCEL:
			delete fSavePanel;
			fSavePanel = NULL;
			break;

		case MSG_UPDATE_STATUS:
		{
			int32 pages, curPage;
			pages = fImageView->PageCount();
			curPage = fImageView->CurrentPage();
			
			bool benable = (pages > 1) ? true : false;
			EnableMenuItem(fBar, MSG_PAGE_FIRST, benable);
			EnableMenuItem(fBar, MSG_PAGE_LAST, benable);
			EnableMenuItem(fBar, MSG_PAGE_NEXT, benable);
			EnableMenuItem(fBar, MSG_PAGE_PREV, benable);
			
			EnableMenuItem(fBar, MSG_FILE_NEXT, fImageView->HasNextFile());
			EnableMenuItem(fBar, MSG_FILE_PREV, fImageView->HasPrevFile());
			
			if (fGoToPageMenu->CountItems() != pages) {
				// Only rebuild the submenu if the number of
				// pages is different
				
				while (fGoToPageMenu->CountItems() > 0)
					// Remove all page numbers
					delete fGoToPageMenu->RemoveItem(0L);
			
				for (int32 i = 1; i <= pages; i++) {
					// Fill Go To page submenu with an entry for each page
					BMessage *pgomsg;
					char shortcut = 0;
					pgomsg = new BMessage(MSG_GOTO_PAGE);
					pgomsg->AddInt32("page", i);
					BString strCaption;
					strCaption << i;
					BMenuItem *item;
					if (i < 10) {
						shortcut = '0' + i;
					} else if (i == 10) {
						shortcut = '0';
					}
					item = new BMenuItem(strCaption.String(), pgomsg, shortcut);
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
			EnableMenuItem(fBar, MSG_INVERT, (colors != B_CMAP8));
				
			BString status;
			int32 width, height;
			if (message->FindInt32("width", &width) >= B_OK
				&& message->FindInt32("height", &height) >= B_OK) {
				status << width << "x" << height << ", ";
			}
			
			BString str;
			if (message->FindString("status", &str) == B_OK) {
				status << str;
			}
			
			fStatusView->SetText(status);
			
			UpdateTitle();
			break;
		}

		case MSG_SELECTION:
		{
			// The view sends this message when a selection is
			// made or the selection is cleared so that the window
			// can update the state of the appropriate menu items
			bool benable;
			if (message->FindBool("has_selection", &benable) == B_OK) {
				EnableMenuItem(fBar, B_CUT, benable);
				EnableMenuItem(fBar, B_COPY, benable);
				EnableMenuItem(fBar, MSG_CLEAR_SELECT, benable);
			}
			break;
		}

		case MSG_UNDO_STATE:
		{
			bool benable;
			if (message->FindBool("can_undo", &benable) == B_OK)
				EnableMenuItem(fBar, B_UNDO, benable);
			break;
		}

		case MSG_CLIPBOARD_CHANGED:
		{
			// The app sends this message after it examines
			// the clipboard in response to a B_CLIPBOARD_CHANGED
			// message
			bool bdata;
			if (message->FindBool("data_available", &bdata) == B_OK)
				EnableMenuItem(fBar, B_PASTE, bdata);
			break;
		}

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
			if (ClosePrompt())
				fImageView->FirstPage();
			break;

		case MSG_PAGE_LAST:
			if (ClosePrompt())
				fImageView->LastPage();
			break;

		case MSG_PAGE_NEXT:
			if (ClosePrompt())
				fImageView->NextPage();
			break;

		case MSG_PAGE_PREV:
			if (ClosePrompt())
				fImageView->PrevPage();
			break;

		case MSG_GOTO_PAGE:
		{
			if (!ClosePrompt())
				break;

			int32 curPage, newPage, pages;
			if (message->FindInt32("page", &newPage) == B_OK) {
				curPage = fImageView->CurrentPage();
				pages = fImageView->PageCount();
				
				if (newPage > 0 && newPage <= pages) {
					BMenuItem *pcurItem, *pnewItem;
					pcurItem = fGoToPageMenu->ItemAt(curPage - 1);
					pnewItem = fGoToPageMenu->ItemAt(newPage - 1);
					if (!pcurItem || !pnewItem)
						break;
					pcurItem->SetMarked(false);
					pnewItem->SetMarked(true);
					fImageView->GoToPage(newPage);
				}
			}
			break;
		}

		case MSG_DITHER_IMAGE:
			fImageView->SetDither(ToggleMenuItem(message->what));
			break;

		case MSG_SHRINK_TO_WINDOW:
			ResizeToWindow(true, message->what);
			break;
		case MSG_ZOOM_TO_WINDOW:
			ResizeToWindow(false, message->what);
			break;

		case MSG_FILE_PREV:
			if (ClosePrompt())
				fImageView->PrevFile();
			break;

		case MSG_FILE_NEXT:
			if (ClosePrompt())
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
		case MSG_SLIDE_SHOW:
		{
			BMenuItem *item;
			item = fBar->FindItem(message->what);
			if (!item)
				break;
			if (item->IsMarked()) {
				item->SetMarked(false);
				fImageView->StopSlideShow();
			} else if (ClosePrompt()) {
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
				MarkSlideShowDelay(value);
			}
			break;
		}

		case MSG_FULL_SCREEN:
			ToggleFullScreen();
			break;
		case MSG_EXIT_FULL_SCREEN:
			if (fFullScreen)
				ToggleFullScreen();
			break;
		case MSG_SHOW_CAPTION:
		{
			fShowCaption = ToggleMenuItem(message->what);
			ShowImageSettings* settings = my_app->Settings();

			if (settings->Lock()) {
				settings->SetBool("ShowCaption", fShowCaption);
				settings->Unlock();
			}
			if (fFullScreen)
				fImageView->SetShowCaption(fShowCaption);
			break;
		}

		case MSG_PAGE_SETUP:
			PageSetup();
			break;
		case MSG_PREPARE_PRINT:
			PrepareForPrint();
			break;
		case MSG_PRINT:
			Print(message);
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
			fImageView->SetScaleBilinear(ToggleMenuItem(message->what));
			break;

		case MSG_DESKTOP_BACKGROUND:
		{
			BPath path;
			if (path.SetTo(fImageView->Image()) == B_OK) {
				BackgroundImage::SetDesktopImage(B_CURRENT_WORKSPACE,
					path.Path(), BackgroundImage::kScaledToFit,
					BPoint(0, 0), false);
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ShowImageWindow::SaveAs(BMessage *message)
{
	// Read the translator and output type the user chose
	translator_id outTranslator;
	uint32 outType;
	if (message->FindInt32(TRANSLATOR_FLD,
			reinterpret_cast<int32 *>(&outTranslator)) != B_OK
		|| message->FindInt32(TYPE_FLD,
			reinterpret_cast<int32 *>(&outType)) != B_OK)
		return;

	// Add the chosen translator and output type to the
	// message that the save panel will send back
	BMessage *panelMsg = new BMessage(MSG_SAVE_PANEL);
	panelMsg->AddInt32(TRANSLATOR_FLD, outTranslator);
	panelMsg->AddInt32(TYPE_FLD, outType);

	// Create save panel and show it
	fSavePanel = new (std::nothrow) BFilePanel(B_SAVE_PANEL,
		new BMessenger(this), NULL, 0, false, panelMsg);
	if (!fSavePanel)
		return;

	fSavePanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
	fSavePanel->Show();
}


void
ShowImageWindow::SaveToFile(BMessage *message)
{
	// Read in where the file should be saved	
	entry_ref dirRef;
	if (message->FindRef("directory", &dirRef) != B_OK)
		return;

	const char *filename;
	if (message->FindString("name", &filename) != B_OK)
		return;

	// Read in the translator and type to be used
	// to save the output image
	translator_id outTranslator;
	uint32 outType;
	if (message->FindInt32(TRANSLATOR_FLD,
			reinterpret_cast<int32 *>(&outTranslator)) != B_OK
		|| message->FindInt32(TYPE_FLD,
			reinterpret_cast<int32 *>(&outType)) != B_OK)
		return;

	// Find the translator_format information needed to
	// write a MIME attribute for the image file
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	const translation_format *outFormat = NULL;
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


bool
ShowImageWindow::ClosePrompt()
{
	if (!fModified)
		return true;

	int32 page, count;
	count = fImageView->PageCount();
	page = fImageView->CurrentPage();
	BString prompt, name;
	fImageView->GetName(&name);
	prompt << "The document '" << name << "'";
	if (count > 1)
		prompt << " (page " << page << ")";

	prompt << " has been changed. "
	       << "Do you want to close the document?";
	BAlert *pAlert = new BAlert("Close document", prompt.String(),
		"Cancel", "Close");
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
ShowImageWindow::ToggleFullScreen()
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
		fImageView->SetAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE);
	} else {
		frame = fWindowFrame;

		SetFlags(Flags() & ~(B_NOT_RESIZABLE | B_NOT_MOVABLE));
// NOTE: I changed this to not use left/top alignment at all, because
// I have no idea why it would be useful. The layouting is much more
// predictable now. -Stephan
//		fImageView->SetAlignment(B_ALIGN_LEFT, B_ALIGN_TOP);
		fImageView->SetAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE);
	}

	fImageView->SetBorder(!fFullScreen);
	fImageView->SetShowCaption(fFullScreen && fShowCaption);
	MoveTo(frame.left, frame.top);
	ResizeTo(frame.Width(), frame.Height());
}


void
ShowImageWindow::LoadSettings()
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
ShowImageWindow::SavePrintOptions()
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
ShowImageWindow::PageSetup()
{
	status_t st;
	BString name;
	fImageView->GetName(&name);
	BPrintJob printJob(name.String());
	if (fPrintSettings != NULL)
		printJob.SetSettings(new BMessage(*fPrintSettings));

	st = printJob.ConfigPage();
	if (st == B_OK) {
		delete fPrintSettings;
		fPrintSettings = printJob.Settings();
	}

	return st == B_OK;
}


void
ShowImageWindow::PrepareForPrint()
{
	if (fPrintSettings == NULL && !PageSetup())
		return;

	fPrintOptions.SetBounds(fImageView->GetBitmap()->Bounds());
	fPrintOptions.SetWidth(fImageView->GetBitmap()->Bounds().Width()+1);

	new PrintOptionsWindow(BPoint(Frame().left+30, Frame().top+50),
		&fPrintOptions, this);
}


void
ShowImageWindow::Print(BMessage *msg)
{
	status_t st;
	if (msg->FindInt32("status", &st) != B_OK || st != B_OK)
		return;

	SavePrintOptions();

	BString name;
	fPrintOptions.SetBounds(fImageView->GetBitmap()->Bounds());
	fImageView->GetName(&name);
	BPrintJob printJob(name.String());
	printJob.SetSettings(new BMessage(*fPrintSettings));
	if (printJob.ConfigJob() == B_OK) {
		int32 firstPage;
		int32 lastPage;
		BRect printableRect = printJob.PrintableRect();
		float width, imageWidth, imageHeight, w1, w2;
		BBitmap* bitmap;

		// first/lastPage is unused for now
		firstPage = printJob.FirstPage();
		lastPage = printJob.LastPage();
		if (firstPage < 1)
			firstPage = 1;
		if (lastPage < firstPage)
			lastPage = firstPage;

		bitmap = fImageView->GetBitmap();
		imageWidth = bitmap->Bounds().Width() + 1.0;
		imageHeight = bitmap->Bounds().Height() + 1.0;

		switch (fPrintOptions.Option()) {
			case PrintOptions::kFitToPage:
				w1 = printableRect.Width()+1;
				w2 = imageWidth * (printableRect.Height() + 1) / imageHeight;
				if (w2 < w1)
					width = w2;
				else
					width = w1;
				break;
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

	bool quit = ClosePrompt();

	if (quit) {
		// tell the app to forget about this window
		be_app->PostMessage(MSG_WINDOW_QUIT);
	}

	return quit;
}


void
ShowImageWindow::Zoom(BPoint origin, float width, float height)
{
	// just go into fullscreen
	ToggleFullScreen();
}

