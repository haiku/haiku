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

#include <algobase.h>
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

#include "ShowImageConstants.h"
#include "ShowImageWindow.h"
#include "ShowImageView.h"
#include "ShowImageStatusView.h"

ShowImageWindow::ShowImageWindow(const entry_ref *pref)
	: BWindow(BRect(50, 50, 350, 250), "", B_DOCUMENT_WINDOW, 0)
{
	fpSavePanel = NULL;
	fFullScreen = false;
	fShowCaption = true; // XXX what is best for default?
		
	// create menu bar	
	fpBar = new BMenuBar(BRect(0, 0, Bounds().right, 20), "menu_bar");
	LoadMenus(fpBar);
	AddChild(fpBar);

	BRect viewFrame = Bounds();
	viewFrame.top		= fpBar->Bounds().bottom + 1;
	viewFrame.right		-= B_V_SCROLL_BAR_WIDTH;
	viewFrame.bottom	-= B_H_SCROLL_BAR_HEIGHT;
	
	// create the image view	
	fpImageView = new ShowImageView(viewFrame, "image_view", B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED);	
	// wrap a scroll view around the view
	BScrollView *pscrollView = new BScrollView("image_scroller", fpImageView,
		B_FOLLOW_ALL, 0, false, false, B_PLAIN_BORDER);
	AddChild(pscrollView);
	
	const int32 kstatusWidth = 190;
	BRect rect;
	rect = Bounds();
	rect.top	= viewFrame.bottom + 1;
	rect.left 	= viewFrame.left + kstatusWidth;
	rect.right	= viewFrame.right;	
	BScrollBar *phscroll;
	phscroll = new BScrollBar(rect, "hscroll", fpImageView, 0, 150,
		B_HORIZONTAL);
	AddChild(phscroll);

	rect.left = 0;
	rect.right = kstatusWidth - 1;	
	fpStatusView = new ShowImageStatusView(rect, "status_view", B_FOLLOW_BOTTOM,
		B_WILL_DRAW);
	fpStatusView->SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	AddChild(fpStatusView);
	
	rect = Bounds();
	rect.top    = viewFrame.top;
	rect.left 	= viewFrame.right + 1;
	rect.bottom	= viewFrame.bottom;
	BScrollBar *pvscroll;
	pvscroll = new BScrollBar(rect, "vscroll", fpImageView, 0, 150, B_VERTICAL);
	AddChild(pvscroll);
	
	SetSizeLimits(250, 100000, 100, 100000);
	
	// finish creating window
	UpdateTitle();

	fpImageView->SetImage(pref);

	SetPulseRate(100000); // every 1/10 second; ShowImageView needs it for marching ants

	if (InitCheck() == B_OK) {
		fpImageView->FlushToLeftTop();
		WindowRedimension(fpImageView->GetBitmap());
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
}

ShowImageWindow::~ShowImageWindow()
{
}

status_t
ShowImageWindow::InitCheck()
{
	if (!fpImageView || fpImageView->GetBitmap() == NULL)
		return B_ERROR;
	else
		return B_OK;
}

void
ShowImageWindow::UpdateTitle()
{
	BString path;
	fpImageView->GetPath(&path);
	SetTitle(path.String());
}

void
ShowImageWindow::UpdateRecentDocumentsMenu()
{
	BMenuItem *item;
	BMessage list, *msg;
	entry_ref ref;
	char name[B_FILE_NAME_LENGTH];

	while ((item = fpOpenMenu->RemoveItem((int32)0)) != NULL) {
		delete item;
	}

	be_roster->GetRecentDocuments(&list, 20, NULL, APP_SIG);
	for (int i = 0; list.FindRef("refs", i, &ref) == B_OK; i++) {
		BEntry entry(&ref);
		entry.GetName(name);
		msg = new BMessage(B_REFS_RECEIVED);
		msg->AddRef("refs", &ref);
		item =  new BMenuItem(name, msg, 0, 0);
		fpOpenMenu->AddItem(item);
		item->SetTarget(be_app, NULL);
	}
}

void
ShowImageWindow::LoadMenus(BMenuBar *pbar)
{
	BMenu *pmenu = new BMenu("File");
	fpOpenMenu = new BMenu("Open");
	pmenu->AddItem(fpOpenMenu);
	fpOpenMenu->Superitem()->SetTrigger('O');
	fpOpenMenu->Superitem()->SetMessage(new BMessage(MSG_FILE_OPEN));
	fpOpenMenu->Superitem()->SetTarget(be_app);
	fpOpenMenu->Superitem()->SetShortcut('O', 0);
	pmenu->AddSeparatorItem();
	BMenu *pmenuSaveAs = new BMenu("Save As...", B_ITEMS_IN_COLUMN);
	BTranslationUtils::AddTranslationItems(pmenuSaveAs, B_TRANSLATOR_BITMAP);
		// Fill Save As submenu with all types that can be converted
		// to from the Be bitmap image format
	pmenu->AddItem(pmenuSaveAs);
	AddItemMenu(pmenu, "Close", MSG_CLOSE, 'W', 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "About ShowImage...", B_ABOUT_REQUESTED, 0, 0, 'A', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Quit", B_QUIT_REQUESTED, 'Q', 0, 'A', true);
	pbar->AddItem(pmenu);
	
	pmenu = new BMenu("Edit");
	AddItemMenu(pmenu, "Undo", B_UNDO, 'Z', 0, 'W', false);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Cut", B_CUT, 'X', 0, 'W', false);
	AddItemMenu(pmenu, "Copy", B_COPY, 'C', 0, 'W', true);
	AddItemMenu(pmenu, "Paste", B_PASTE, 'V', 0, 'W', false);
	AddItemMenu(pmenu, "Clear", MSG_CLEAR_SELECT, 0, 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Select All", MSG_SELECT_ALL, 'A', 0, 'W', true);
	pbar->AddItem(pmenu);

	pmenu = fpBrowseMenu = new BMenu("Browse");
	AddItemMenu(pmenu, "First Page", MSG_PAGE_FIRST, 'F', 0, 'W', true);
	AddItemMenu(pmenu, "Last Page", MSG_PAGE_LAST, 'L', 0, 'W', true);
	AddItemMenu(pmenu, "Next Page", MSG_PAGE_NEXT, 'N', 0, 'W', true);
	AddItemMenu(pmenu, "Previous Page", MSG_PAGE_PREV, 'P', 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Next File", MSG_FILE_NEXT, B_DOWN_ARROW, 0, 'W', true);
	AddItemMenu(pmenu, "Previous File", MSG_FILE_PREV, B_UP_ARROW, 0, 'W', true);	
	pbar->AddItem(pmenu);

	pmenu = new BMenu("Image");
	AddItemMenu(pmenu, "Dither Image", MSG_DITHER_IMAGE, 0, 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Rotate Clockwise", MSG_ROTATE_CLOCKWISE, B_RIGHT_ARROW, 0, 'W', true);	
	AddItemMenu(pmenu, "Rotate Anticlockwise", MSG_ROTATE_ACLKWISE, B_LEFT_ARROW, 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Mirror Vertical", MSG_MIRROR_VERTICAL, 0, 0, 'W', true);
	AddItemMenu(pmenu, "Mirror Horizontal", MSG_MIRROR_HORIZONTAL, 0, 0, 'W', true);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Invert", MSG_INVERT, 0, 0, 'W', true);	
	pbar->AddItem(pmenu);

	pmenu = new BMenu("View");
	AddItemMenu(pmenu, "Slide Show", MSG_SLIDE_SHOW, 0, 0, 'W', true);
	BMenu* pDelay = new BMenu("Slide Delay");
	pDelay->SetRadioMode(true);
	// Note: ShowImage loades images in window thread so it becomes unresponsive if
	// slide show delay is too short! (Especially if loading the image takes as long as
	// or longer than the slide show delay). Should load in background thread!
	// AddDelayItem(pDelay, "Half a Second", 0.5, false);
	// AddDelayItem(pDelay, "One Second", 1, false);
	// AddDelayItem(pDelay, "Two Second", 2, false);
	AddDelayItem(pDelay, "Three Seconds", 3, true);
	AddDelayItem(pDelay, "Four Second", 4, false);
	AddDelayItem(pDelay, "Five Seconds", 5, false);
	AddDelayItem(pDelay, "Six Seconds", 6, false);
	AddDelayItem(pDelay, "Seven Seconds", 7, false);
	AddDelayItem(pDelay, "Eight Seconds", 8, false);
	AddDelayItem(pDelay, "Nine Seconds", 9, false);
	AddDelayItem(pDelay, "Ten Seconds", 10, false);
	AddDelayItem(pDelay, "Tweenty Seconds", 20, false);
	pmenu->AddItem(pDelay);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Fit To Window Size", MSG_FIT_TO_WINDOW_SIZE, 0, 0, 'W', true);
	AddItemMenu(pmenu, "Full Screen", MSG_FULL_SCREEN, B_ENTER, 0, 'W', true);
	AddItemMenu(pmenu, "Show Caption in Full Screen Mode", MSG_SHOW_CAPTION, 0, 0, 'W', true);
	pbar->AddItem(pmenu);
	MarkMenuItem(MSG_SHOW_CAPTION, fShowCaption);

	UpdateRecentDocumentsMenu();
}

BMenuItem *
ShowImageWindow::AddItemMenu(BMenu *pmenu, char *caption, long unsigned int msg, 
	char shortcut, uint32 modifier, char target, bool benabled)
{
	BMenuItem* pitem;
	pitem = new BMenuItem(caption, new BMessage(msg), shortcut);
	
	if (target == 'A')
	   pitem->SetTarget(be_app);
	   
	pitem->SetEnabled(benabled);	   
	pmenu->AddItem(pitem);
	
	return pitem;
}

BMenuItem*
ShowImageWindow::AddDelayItem(BMenu *pmenu, char *caption, float value, bool marked)
{
	BMenuItem* pitem;
	BMessage* pmsg;
	pmsg = new BMessage(MSG_SLIDE_SHOW_DELAY);
	pmsg->AddFloat("value", value);
	
	pitem = new BMenuItem(caption, pmsg, 0);
	
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
	const float windowBorderWidth = 5;
	const float windowBorderHeight = 5;
	
	if (screen.Frame().right == 0.0) {	
		return; // invalid screen object
	}
	
	width = r.Width() + B_V_SCROLL_BAR_WIDTH;
	height = r.Height() + 1 + fpBar->Frame().Height() + B_H_SCROLL_BAR_HEIGHT;

	// dimensions so that window does not reach outside of screen	
	maxWidth = screen.Frame().Width() + 1 - windowBorderWidth - Frame().left;
	maxHeight = screen.Frame().Height() + 1 - windowBorderHeight - Frame().top;
	
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
	item = fpBar->FindItem(what);
	if (item != NULL) {
		marked = !item->IsMarked();
		item->SetMarked(marked);
	}
	return marked;
}

void
ShowImageWindow::EnableMenuItem(uint32 what, bool enable)
{
	BMenuItem* item;
	item = fpBar->FindItem(what);
	if (item && item->IsEnabled() != enable) {
		// XXX: Does this apply to menu items too?
		// Only call this function if the state is changing
		// to avoid flickering
		item->SetEnabled(enable);
	}
}

void 
ShowImageWindow::MarkMenuItem(uint32 what, bool marked)
{
	BMenuItem* item;
	item = fpBar->FindItem(what);
	if (item && item->IsMarked() != marked) {
		item->SetMarked(marked);
	}
}


void
ShowImageWindow::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		case MSG_OUTPUT_TYPE:
			// User clicked Save As then choose an output format
			if (!fpSavePanel)
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
			delete fpSavePanel;
			fpSavePanel = NULL;
			break;
			
		case MSG_UPDATE_STATUS:
		{
			bool benable = (fpImageView->PageCount() > 1) ? true : false;
			EnableMenuItem(MSG_PAGE_FIRST, benable);
			EnableMenuItem(MSG_PAGE_LAST, benable);
			EnableMenuItem(MSG_PAGE_NEXT, benable);
			EnableMenuItem(MSG_PAGE_PREV, benable);
				
			BString str;
			if (pmsg->FindString("status", &str) == B_OK)
				fpStatusView->SetText(str);
			
			UpdateTitle();
			break;
		}

		case B_UNDO:
			break;
		case B_CUT:
			break;
		case B_COPY:
			fpImageView->CopySelectionToClipboard();
			break;
		case B_PASTE:
			break;
		case MSG_CLEAR_SELECT:
			fpImageView->Unselect();
			break;
		case MSG_SELECT_ALL:
			fpImageView->SelectAll();
			break;
			
		case MSG_PAGE_FIRST:
			fpImageView->FirstPage();
			break;
			
		case MSG_PAGE_LAST:
			fpImageView->LastPage();
			break;
			
		case MSG_PAGE_NEXT:
			fpImageView->NextPage();
			break;
			
		case MSG_PAGE_PREV:
			fpImageView->PrevPage();
			break;

		case MSG_DITHER_IMAGE:
			ToggleMenuItem(pmsg->what);
			break;
		
		case MSG_FIT_TO_WINDOW_SIZE:
			{
				bool resize;
				resize = ToggleMenuItem(pmsg->what);
				fpImageView->ResizeToViewBounds(resize);
			}
			break;

		case MSG_FILE_PREV:
			fpImageView->PrevFile();
			break;
			
		case MSG_FILE_NEXT:
			fpImageView->NextFile();
			break;
		
		case MSG_ROTATE_CLOCKWISE:
			fpImageView->Rotate(90);
			break;
		case MSG_ROTATE_ACLKWISE:
			fpImageView->Rotate(270);
			break;
		case MSG_MIRROR_VERTICAL:
			fpImageView->Mirror(true);
			break;
		case MSG_MIRROR_HORIZONTAL:
			fpImageView->Mirror(false);
			break;
		case MSG_INVERT:
			fpImageView->Invert();
			break;
		case MSG_SLIDE_SHOW:
			if (ToggleMenuItem(pmsg->what)) {
				fpImageView->StartSlideShow();
			} else {
				fpImageView->StopSlideShow();
			}
		case MSG_SLIDE_SHOW_DELAY:
			{
				float value;
				if (pmsg->FindFloat("value", &value) == B_OK) {
					fpImageView->SetSlideShowDelay(value);
				}
			}
			break;
			
		case MSG_FULL_SCREEN:
			ToggleFullScreen();
			break;
		case MSG_SHOW_CAPTION:
			fShowCaption = ToggleMenuItem(pmsg->what);
			if (fFullScreen) {
				fpImageView->SetShowCaption(fShowCaption);
			}
			break;
		
		case MSG_UPDATE_RECENT_DOCUMENTS:
			UpdateRecentDocumentsMenu();
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
	fpSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL, 0,
		false, ppanelMsg);
	if (!fpSavePanel)
		return;
	fpSavePanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
	fpSavePanel->Show();
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
	
	// Create the output file
	BDirectory dir(&dirref);
	BFile file(&dir, filename, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK)
		return;
	
	// Translate the image and write it out to the output file
	BBitmapStream stream(fpImageView->GetBitmap());	
	BTranslatorRoster *proster = BTranslatorRoster::Default();
	if (proster->Translate(outTranslator, &stream, NULL,
		&file, outType) != B_OK) {
		BAlert *palert = new BAlert(NULL, "Error writing image file.", "Ok");
		palert->Go();
	}
	
	BBitmap *pout = NULL;
	stream.DetachBitmap(&pout);
		// bitmap used by stream still belongs to the view,
		// detach so it doesn't get deleted
}

bool
ShowImageWindow::CanQuit()
{
	if (fpSavePanel)
		// Don't allow this window to be closed if a save panel is open
		return false;
	else
		return true;	
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
		frame.top -= fpBar->Bounds().Height()+1;
		frame.right += B_V_SCROLL_BAR_WIDTH;
		frame.bottom += B_H_SCROLL_BAR_HEIGHT;
		frame.InsetBy(-1, -1); // PEN_SIZE in ShowImageView

		SetFlags(Flags() | B_NOT_RESIZABLE | B_NOT_MOVABLE);
	} else {
		frame = fWindowFrame;

		SetFlags(Flags() & ~(B_NOT_RESIZABLE | B_NOT_MOVABLE));
	}
	fpImageView->SetShowCaption(fFullScreen && fShowCaption);
	MoveTo(frame.left, frame.top);
	ResizeTo(frame.Width(), frame.Height());
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

