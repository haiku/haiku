/*****************************************************************************/
// ShowImageWindow
// Written by Fernando Francisco de Oliveira, Michael Wilber, Michael Pfeiffer
//
// ShowImageWindow.cpp
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <stdio.h>
#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Entry.h>
#include <File.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <ScrollView.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <Alert.h>
#include <SupportDefs.h>
#include <Screen.h>
#include <Roster.h>
#include <PrintJob.h>
#include <Clipboard.h>

#include "ShowImageApp.h"
#include "ShowImageConstants.h"
#include "ShowImageWindow.h"
#include "ShowImageView.h"
#include "ShowImageStatusView.h"
#include "EntryMenuItem.h"


// Implementation of RecentDocumentsMenu

RecentDocumentsMenu::RecentDocumentsMenu(const char *title, menu_layout layout = B_ITEMS_IN_COLUMN)
	: BMenu(title, layout)
{
}

bool 
RecentDocumentsMenu::AddDynamicItem(add_state s)
{
	if (s != B_INITIAL_ADD) return false;
	
	BMenuItem *item;
	BMessage list, *msg;
	entry_ref ref;
	char name[B_FILE_NAME_LENGTH];

	while ((item = RemoveItem((int32)0)) != NULL) {
		delete item;
	}

	be_roster->GetRecentDocuments(&list, 20, NULL, APP_SIG);
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


// Implementation of ShowImageWindow

ShowImageWindow::ShowImageWindow(const entry_ref *pref, const BMessenger& trackerMessenger)
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
	LoadMenus(fBar);
	AddChild(fBar);

	BRect viewFrame = Bounds();
	viewFrame.top = fBar->Bounds().Height() + 1;
	viewFrame.right -= B_V_SCROLL_BAR_WIDTH;
	viewFrame.bottom -= B_H_SCROLL_BAR_HEIGHT;

	// create the image view	
	fImageView = new ShowImageView(viewFrame, "image_view", B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED);	
	// wrap a scroll view around the view
	BScrollView *pscrollView = new BScrollView("image_scroller", fImageView,
		B_FOLLOW_ALL, 0, false, false, B_PLAIN_BORDER);
	AddChild(pscrollView);
	
	const int32 kstatusWidth = 190;
	BRect rect;
	rect = Bounds();
	rect.top	= viewFrame.bottom + 1;
	rect.left 	= viewFrame.left + kstatusWidth;
	rect.right	= viewFrame.right + 1;	
	rect.bottom += 1;
	BScrollBar *phscroll;
	phscroll = new BScrollBar(rect, "hscroll", fImageView, 0, 150,
		B_HORIZONTAL);
	AddChild(phscroll);

	rect.left = 0;
	rect.right = kstatusWidth - 1;	
	fStatusView = new ShowImageStatusView(rect, "status_view", B_FOLLOW_BOTTOM,
		B_WILL_DRAW);
	fStatusView->SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	AddChild(fStatusView);
	
	rect = Bounds();
	rect.top    = viewFrame.top - 1;
	rect.left 	= viewFrame.right + 1;
	rect.bottom	= viewFrame.bottom + 1;
	rect.right	+= 1;
	BScrollBar *pvscroll;
	pvscroll = new BScrollBar(rect, "vscroll", fImageView, 0, 150, B_VERTICAL);
	AddChild(pvscroll);
	
	SetSizeLimits(250, 100000, 100, 100000);
	
	// finish creating the window
	fImageView->SetImage(pref);
	fImageView->SetTrackerMessenger(trackerMessenger);

	if (InitCheck() == B_OK) {
		// add View menu here so it can access ShowImageView methods 
		BMenu* pmenu = new BMenu("View");
		BuildViewMenu(pmenu);
		fBar->AddItem(pmenu);		
		MarkMenuItem(fBar, MSG_DITHER_IMAGE, fImageView->GetDither());
		UpdateTitle();

		SetPulseRate(100000); // every 1/10 second; ShowImageView needs it for marching ants

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
		Quit();
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
	else
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
ShowImageWindow::BuildViewMenu(BMenu *pmenu)
{
	AddItemMenu(pmenu, "Slide Show", MSG_SLIDE_SHOW, 0, 0, 'W', true);
	MarkMenuItem(pmenu, MSG_SLIDE_SHOW, fImageView->SlideShowStarted());
	BMenu* pDelay = new BMenu("Slide Delay");
	if (fSlideShowDelay == NULL) {
		fSlideShowDelay = pDelay;
	}
	pDelay->SetRadioMode(true);
	// Note: ShowImage loades images in window thread so it becomes unresponsive if
	// slide show delay is too short! (Especially if loading the image takes as long as
	// or longer than the slide show delay). Should load in background thread!
	AddDelayItem(pDelay, "Three Seconds", 3);
	AddDelayItem(pDelay, "Four Second", 4);
	AddDelayItem(pDelay, "Five Seconds", 5);
	AddDelayItem(pDelay, "Six Seconds", 6);
	AddDelayItem(pDelay, "Seven Seconds", 7);
	AddDelayItem(pDelay, "Eight Seconds", 8);
	AddDelayItem(pDelay, "Nine Seconds", 9);
	AddDelayItem(pDelay, "Ten Seconds", 10);
	AddDelayItem(pDelay, "Twenty Seconds", 20);
	pmenu->AddItem(pDelay);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Original Size", MSG_ORIGINAL_SIZE, 0, 0, 'W', true);
	AddItemMenu(pmenu, "Zoom In", MSG_ZOOM_IN, '+', 0, 'W', true);
	AddItemMenu(pmenu, "Zoom Out", MSG_ZOOM_OUT, '-', 0, 'W', true);	
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Scale Bilinear", MSG_SCALE_BILINEAR, 0, 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Shrink to Window", MSG_SHRINK_TO_WINDOW, 0, 0, 'W', true);
	AddItemMenu(pmenu, "Zoom to Window", MSG_ZOOM_TO_WINDOW, 0, 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Full Screen", MSG_FULL_SCREEN, 'F', 0, 'W', true);
	MarkMenuItem(pmenu, MSG_FULL_SCREEN, fFullScreen);
	BMessage *pFullScreen = new BMessage(MSG_FULL_SCREEN);
	AddShortcut(B_ENTER, 0, pFullScreen);
	
	AddItemMenu(pmenu, "Show Caption in Full Screen Mode", MSG_SHOW_CAPTION, 0, 0, 'W', true);
	MarkMenuItem(pmenu, MSG_SHOW_CAPTION, fShowCaption);

	bool shrink, zoom, enabled;
	MarkMenuItem(pmenu, MSG_SCALE_BILINEAR, fImageView->GetScaleBilinear());
	shrink = fImageView->GetShrinkToBounds();
	zoom = fImageView->GetZoomToBounds();
	MarkMenuItem(pmenu, MSG_SHRINK_TO_WINDOW, shrink);
	MarkMenuItem(pmenu, MSG_ZOOM_TO_WINDOW, zoom);
 	enabled = !(shrink || zoom);
	EnableMenuItem(pmenu, MSG_ORIGINAL_SIZE, enabled);
	EnableMenuItem(pmenu, MSG_ZOOM_IN, enabled);
	EnableMenuItem(pmenu, MSG_ZOOM_OUT, enabled);
}

void
ShowImageWindow::LoadMenus(BMenuBar *pbar)
{
	BMenu *pmenu = new BMenu("File");
	fOpenMenu = new RecentDocumentsMenu("Open");
	pmenu->AddItem(fOpenMenu);
	fOpenMenu->Superitem()->SetTrigger('O');
	fOpenMenu->Superitem()->SetMessage(new BMessage(MSG_FILE_OPEN));
	fOpenMenu->Superitem()->SetTarget(be_app);
	fOpenMenu->Superitem()->SetShortcut('O', 0);
	pmenu->AddSeparatorItem();
	BMenu *pmenuSaveAs = new BMenu("Save As" B_UTF8_ELLIPSIS, B_ITEMS_IN_COLUMN);
	BTranslationUtils::AddTranslationItems(pmenuSaveAs, B_TRANSLATOR_BITMAP);
		// Fill Save As submenu with all types that can be converted
		// to from the Be bitmap image format
	pmenu->AddItem(pmenuSaveAs);
	AddItemMenu(pmenu, "Close", MSG_CLOSE, 'W', 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Page Setup" B_UTF8_ELLIPSIS, MSG_PAGE_SETUP, 0, 0, 'W', true);
	AddItemMenu(pmenu, "Print" B_UTF8_ELLIPSIS, MSG_PREPARE_PRINT, 0, 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "About ShowImage" B_UTF8_ELLIPSIS, B_ABOUT_REQUESTED, 0, 0, 'A', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Quit", B_QUIT_REQUESTED, 'Q', 0, 'A', true);
	pbar->AddItem(pmenu);
	
	pmenu = new BMenu("Edit");
	AddItemMenu(pmenu, "Undo", B_UNDO, 'Z', 0, 'W', false);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Cut", B_CUT, 'X', 0, 'W', false);
	AddItemMenu(pmenu, "Copy", B_COPY, 'C', 0, 'W', false);
	AddItemMenu(pmenu, "Paste", B_PASTE, 'V', 0, 'W', false);
	AddItemMenu(pmenu, "Clear", MSG_CLEAR_SELECT, 0, 0, 'W', false);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Select All", MSG_SELECT_ALL, 'A', 0, 'W', true);
	pbar->AddItem(pmenu);

	pmenu = fBrowseMenu = new BMenu("Browse");
	AddItemMenu(pmenu, "First Page", MSG_PAGE_FIRST, B_LEFT_ARROW, B_SHIFT_KEY, 'W', true);
	AddItemMenu(pmenu, "Last Page", MSG_PAGE_LAST, B_RIGHT_ARROW, B_SHIFT_KEY, 'W', true);
	AddItemMenu(pmenu, "Next Page", MSG_PAGE_NEXT, B_RIGHT_ARROW, 0, 'W', true);
	AddItemMenu(pmenu, "Previous Page", MSG_PAGE_PREV, B_LEFT_ARROW, 0, 'W', true);
	fGoToPageMenu = new BMenu("Go To Page");
	fGoToPageMenu->SetRadioMode(true);
	pmenu->AddItem(fGoToPageMenu);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Next File", MSG_FILE_NEXT, B_DOWN_ARROW, 0, 'W', true);
	AddItemMenu(pmenu, "Previous File", MSG_FILE_PREV, B_UP_ARROW, 0, 'W', true);	
	pbar->AddItem(pmenu);

	pmenu = new BMenu("Image");
	AddItemMenu(pmenu, "Dither Image", MSG_DITHER_IMAGE, 0, 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Rotate +90", MSG_ROTATE_90, ']', 0, 'W', true);	
	AddItemMenu(pmenu, "Rotate -90", MSG_ROTATE_270, '[', 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Mirror Vertical", MSG_MIRROR_VERTICAL, 0, 0, 'W', true);
	AddItemMenu(pmenu, "Mirror Horizontal", MSG_MIRROR_HORIZONTAL, 0, 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Invert", MSG_INVERT, 0, 0, 'W', true);	
	pbar->AddItem(pmenu);
}

BMenuItem *
ShowImageWindow::AddItemMenu(BMenu *pmenu, char *caption, long unsigned int msg, 
	char shortcut, uint32 modifier, char target, bool benabled)
{
	BMenuItem* pitem;
	pitem = new BMenuItem(caption, new BMessage(msg), shortcut, modifier);
	
	if (target == 'A')
		pitem->SetTarget(be_app);
	else
		pitem->SetTarget(this);   
	pitem->SetEnabled(benabled);	   
	pmenu->AddItem(pitem);
	
	return pitem;
}

BMenuItem*
ShowImageWindow::AddDelayItem(BMenu *pmenu, char *caption, float value)
{
	BMenuItem* pitem;
	BMessage* pmsg;
	pmsg = new BMessage(MSG_SLIDE_SHOW_DELAY);
	pmsg->AddFloat("value", value);
	
	pitem = new BMenuItem(caption, pmsg, 0);
	pitem->SetTarget(this);

	bool marked = fImageView->GetSlideShowDelay() == value;	
	if (marked) pitem->SetMarked(true);	   
	pmenu->AddItem(pitem);
	
	return pitem;
}

void
ShowImageWindow::WindowRedimension(BBitmap *pbitmap)
{
	BScreen screen;
	BRect r(pbitmap->Bounds());
	float width, height;
	float maxWidth, maxHeight;
	float minW, maxW, minH, maxH;
	const float windowBorderWidth = 5;
	const float windowBorderHeight = 5;
	
	if (screen.Frame().right == 0.0) {	
		return; // invalid screen object
	}
	
	width = r.Width() + B_V_SCROLL_BAR_WIDTH;
	height = r.Height() + 1 + fBar->Frame().Height() + B_H_SCROLL_BAR_HEIGHT;

	// dimensions so that window does not reach outside of screen	
	maxWidth = screen.Frame().Width() + 1 - windowBorderWidth - Frame().left;
	maxHeight = screen.Frame().Height() + 1 - windowBorderHeight - Frame().top;
	
	// We have to check size limits manually, otherwise
	// menu bar will be too short for small images.
	GetSizeLimits(&minW, &maxW, &minH, &maxH); 
	if (maxWidth > maxW) maxWidth = maxW;
	if (maxHeight > maxH) maxHeight = maxH;
	if (width < minW) width = minW;
	if (height < minH) height = minH;
	
	if (width > maxWidth) width = maxWidth;
	if (height > maxHeight) height = maxHeight;	
	
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
ShowImageWindow::MessageReceived(BMessage *pmsg)
{
	ShowImageSettings* settings;
	switch (pmsg->what) {
		case MSG_MODIFIED:
			// If image has been modified due to a Cut or Paste
			fModified = true;
			break;
			
		case MSG_OUTPUT_TYPE:
			// User clicked Save As then choose an output format
			if (!fSavePanel)
				// If user doesn't already have a save panel open
				SaveAs(pmsg);
			break;
			
		case MSG_SAVE_PANEL:
			// User specified where to save the output image
			SaveToFile(pmsg);
			break;
			
		case MSG_CLOSE:
			if (CanQuit())
				Quit();
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
					BMenuItem *pitem;
					if (i < 10) {
						shortcut = '0' + i;
					} else if (i == 10) {
						shortcut = '0';
					}
					pitem = new BMenuItem(strCaption.String(), pgomsg, shortcut);
					if (curPage == i)
						pitem->SetMarked(true);
					fGoToPageMenu->AddItem(pitem);
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
			pmsg->FindInt32("colors", reinterpret_cast<int32 *>(&colors));
			EnableMenuItem(fBar, MSG_INVERT, (colors != B_CMAP8));
				
			BString str;
			if (pmsg->FindString("status", &str) == B_OK)
				fStatusView->SetText(str);
			
			UpdateTitle();
			break;
		}
		
		case MSG_SELECTION:
		{
			// The view sends this message when a selection is
			// made or the selection is cleared so that the window
			// can update the state of the appropriate menu items
			bool benable;
			if (pmsg->FindBool("has_selection", &benable) == B_OK) {
				EnableMenuItem(fBar, B_CUT, benable);
				EnableMenuItem(fBar, B_COPY, benable);
				EnableMenuItem(fBar, MSG_CLEAR_SELECT, benable);
			}
			break;
		}
		
		case MSG_UNDO_STATE:
		{
			bool benable;
			if (pmsg->FindBool("can_undo", &benable) == B_OK)
				EnableMenuItem(fBar, B_UNDO, benable);
			break;
		}
		
		case MSG_CLIPBOARD_CHANGED:
		{
			// The app sends this message after it examines
			// the clipboard in response to a B_CLIPBOARD_CHANGED
			// message
			bool bdata;
			if (pmsg->FindBool("data_available", &bdata) == B_OK)
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
				if (pmsg->FindInt32("page", &newPage) == B_OK) {
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
			}
			break;

		case MSG_DITHER_IMAGE:
			fImageView->SetDither(ToggleMenuItem(pmsg->what));
			break;
		
		case MSG_SHRINK_TO_WINDOW:
			ResizeToWindow(true, pmsg->what);
			break;
		case MSG_ZOOM_TO_WINDOW:
			ResizeToWindow(false, pmsg->what);
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
		case MSG_MIRROR_VERTICAL:
			fImageView->Mirror(true);
			break;
		case MSG_MIRROR_HORIZONTAL:
			fImageView->Mirror(false);
			break;
		case MSG_INVERT:
			fImageView->Invert();
			break;
		case MSG_SLIDE_SHOW:
			{
				BMenuItem *item;
				item = fBar->FindItem(pmsg->what);
				if (!item)
					break;
				if (item->IsMarked()) {
					item->SetMarked(false);
					fImageView->StopSlideShow();
				} else if (ClosePrompt()) {
					item->SetMarked(true);
					fImageView->StartSlideShow();
				}
			}
			break;
			
		case MSG_SLIDE_SHOW_DELAY:
			{
				float value;
				if (pmsg->FindFloat("value", &value) == B_OK) {
					fImageView->SetSlideShowDelay(value);
					// in case message is sent from popup menu
					MarkSlideShowDelay(value);
				}
			}
			break;
			
		case MSG_FULL_SCREEN:
			ToggleFullScreen();
			break;
		case MSG_SHOW_CAPTION:
			fShowCaption = ToggleMenuItem(pmsg->what);
			settings = my_app->Settings();
			if (settings->Lock()) {
				settings->SetBool("ShowCaption", fShowCaption);
				settings->Unlock();
			}
			if (fFullScreen) {
				fImageView->SetShowCaption(fShowCaption);
			}
			break;
		
		case MSG_PAGE_SETUP:
			PageSetup();
			break;
		case MSG_PREPARE_PRINT:
			PrepareForPrint();
			break;
		case MSG_PRINT:
			Print(pmsg);
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
			fImageView->SetScaleBilinear(ToggleMenuItem(pmsg->what));
			break;

		default:
			BWindow::MessageReceived(pmsg);
			break;
	}
}

void
ShowImageWindow::SaveAs(BMessage *pmsg)
{
	// Read the translator and output type the user chose
	translator_id outTranslator;
	uint32 outType;
	if (pmsg->FindInt32(TRANSLATOR_FLD,
		reinterpret_cast<int32 *>(&outTranslator)) != B_OK)
		return;	
	if (pmsg->FindInt32(TYPE_FLD,
		reinterpret_cast<int32 *>(&outType)) != B_OK)
		return;
		
	// Add the chosen translator and output type to the
	// message that the save panel will send back
	BMessage *ppanelMsg = new BMessage(MSG_SAVE_PANEL);
	ppanelMsg->AddInt32(TRANSLATOR_FLD, outTranslator);
	ppanelMsg->AddInt32(TYPE_FLD, outType);

	// Create save panel and show it
	fSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL, 0,
		false, ppanelMsg);
	if (!fSavePanel)
		return;
	fSavePanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
	fSavePanel->Show();
}

void
ShowImageWindow::SaveToFile(BMessage *pmsg)
{
	// Read in where the file should be saved	
	entry_ref dirref;
	if (pmsg->FindRef("directory", &dirref) != B_OK)
		return;
	const char *filename;
	if (pmsg->FindString("name", &filename) != B_OK)
		return;
		
	// Read in the translator and type to be used
	// to save the output image
	translator_id outTranslator;
	uint32 outType;
	if (pmsg->FindInt32(TRANSLATOR_FLD,
		reinterpret_cast<int32 *>(&outTranslator)) != B_OK)
		return;	
	if (pmsg->FindInt32(TYPE_FLD,
		reinterpret_cast<int32 *>(&outType)) != B_OK)
		return;
	
	// Find the translator_format information needed to
	// write a MIME attribute for the image file
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	const translation_format *pouts = NULL;
	int32 outsCount = 0;
	if (roster->GetOutputFormats(outTranslator, &pouts, &outsCount) != B_OK)
		return;
	if (outsCount < 1)
		return;
	int32 i;
	for (i = 0; i < outsCount; i++) {
		if (pouts[i].group == B_TRANSLATOR_BITMAP && pouts[i].type == outType)
			break;
	}
	if (i == outsCount)
		return;
	
	// Write out the image file
	BDirectory dir(&dirref);
	fImageView->SaveToFile(&dir, filename, NULL, &pouts[i]);
}

bool
ShowImageWindow::ClosePrompt()
{
	if (!fModified)
		return true;
	else {
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
		if (pAlert->Go() == 0)
			// Cancel
			return false;
		else {
			// Close
			fModified = false;
			return true;
		}
	}
}

bool
ShowImageWindow::CanQuit()
{
	if (fSavePanel)
		// Don't allow this window to be closed if a save panel is open
		return false;
	else
		return ClosePrompt();	
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
		fImageView->SetAlignment(B_ALIGN_LEFT, B_ALIGN_TOP);
	}
	fImageView->SetBorder(!fFullScreen);
	fImageView->SetShowCaption(fFullScreen && fShowCaption);
	MoveTo(frame.left, frame.top);
	ResizeTo(frame.Width(), frame.Height());
}

void
ShowImageWindow::LoadSettings()
{
	ShowImageSettings* settings;	
	settings = my_app->Settings();
	int32 op;
	float f;
	if (settings->Lock()) {
		fShowCaption = settings->GetBool("ShowCaption", fShowCaption);
		fPrintOptions.SetBounds(BRect(0, 0, 1023, 767));
		
		op = settings->GetInt32("PO:Option", fPrintOptions.Option());	
		fPrintOptions.SetOption((enum PrintOptions::Option)op);
		
		f = settings->GetFloat("PO:ZoomFactor", fPrintOptions.ZoomFactor());
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
	ShowImageSettings* settings;	
	settings = my_app->Settings();
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
	if (fPrintSettings != NULL) {
		printJob.SetSettings(new BMessage(*fPrintSettings));
	}
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
	if (fPrintSettings == NULL && !PageSetup()) {
		return;
	}

	fPrintOptions.SetBounds(fImageView->GetBitmap()->Bounds());
	fPrintOptions.SetWidth(fImageView->GetBitmap()->Bounds().Width()+1);

	new PrintOptionsWindow(BPoint(Frame().left+30, Frame().top+50), &fPrintOptions, this);
}

void
ShowImageWindow::Print(BMessage *msg)
{
	status_t st;
	if (msg->FindInt32("status", &st) != B_OK || st != B_OK) {
		return;
	}
	
	SavePrintOptions();
	
	BString name;
	fPrintOptions.SetBounds(fImageView->GetBitmap()->Bounds());
	fImageView->GetName(&name);
	BPrintJob printJob(name.String());
	printJob.SetSettings(new BMessage(*fPrintSettings));
	if (printJob.ConfigJob() == B_OK) {
		int32  firstPage;
		int32  lastPage;
		BRect  printableRect = printJob.PrintableRect();
		float width, imageWidth, imageHeight, w1, w2;
		BBitmap* bitmap;

		// first/lastPage is unused for now
		firstPage = printJob.FirstPage();
		lastPage = printJob.LastPage();
		if (firstPage < 1) {
			firstPage = 1;
		}
		if (lastPage < firstPage) {
			lastPage = firstPage;
		}
		
		bitmap = fImageView->GetBitmap();
		imageWidth = bitmap->Bounds().Width() + 1.0;
		imageHeight = bitmap->Bounds().Height() + 1.0;
		
		switch (fPrintOptions.Option()) {
			case PrintOptions::kFitToPage:
				w1 = printableRect.Width()+1;
				w2 = imageWidth * (printableRect.Height() + 1) / imageHeight;
				if (w2 < w1) {
					width = w2;
				} else {
					width = w1;
				}
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

		// XXX: eventually print large images on several pages
		printJob.BeginJob();
		fImageView->SetScale(width / imageWidth);
		printJob.DrawView(fImageView, bitmap->Bounds(), BPoint(printableRect.left, printableRect.top));
		fImageView->SetScale(1.0);
		printJob.SpoolPage();
		printJob.CommitJob();
	}
}

bool
ShowImageWindow::QuitRequested()
{
	return CanQuit();
}

void
ShowImageWindow::Quit()
{
	// tell the app to forget about this window
	be_app->PostMessage(MSG_WINDOW_QUIT);
	BWindow::Quit();
}

