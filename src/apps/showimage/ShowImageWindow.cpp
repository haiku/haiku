/*****************************************************************************/
// ShowImageWindow
// Written by Fernando Francisco de Oliveira, Michael Wilber
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

#include "ShowImageConstants.h"
#include "ShowImageWindow.h"
#include "ShowImageView.h"
#include "ShowImageStatusView.h"

ShowImageWindow::ShowImageWindow(const entry_ref *pref)
	: BWindow(BRect(50, 50, 350, 250), "", B_DOCUMENT_WINDOW, 0)
{
	fpsavePanel = NULL;
	fpref = NULL;
	
	// create menu bar	
	fpbar = new BMenuBar(BRect(0, 0, Bounds().right, 20), "menu_bar");
	LoadMenus(fpbar);
	AddChild(fpbar);

	BRect viewFrame = Bounds();
	viewFrame.top		= fpbar->Bounds().bottom + 1;
	viewFrame.right		-= B_V_SCROLL_BAR_WIDTH;
	viewFrame.bottom	-= B_H_SCROLL_BAR_HEIGHT;
	
	// create the image view	
	fpimageView = new ShowImageView(viewFrame, "image_view", B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS);	
	// wrap a scroll view around the view
	BScrollView *pscrollView = new BScrollView("image_scroller", fpimageView,
		B_FOLLOW_ALL, 0, false, false, B_PLAIN_BORDER);
	AddChild(pscrollView);
	
	const int32 kstatusWidth = 190;
	BRect rect;
	rect = Bounds();
	rect.top	= viewFrame.bottom + 1;
	rect.left 	= viewFrame.left + kstatusWidth;
	rect.right	= viewFrame.right;	
	BScrollBar *phscroll;
	phscroll = new BScrollBar(rect, "hscroll", fpimageView, 0, 150,
		B_HORIZONTAL);
	AddChild(phscroll);

	rect.left = 0;
	rect.right = kstatusWidth - 1;	
	fpstatusView = new ShowImageStatusView(rect, "status_view", B_FOLLOW_BOTTOM,
		B_WILL_DRAW);
	fpstatusView->SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	AddChild(fpstatusView);
	
	rect = Bounds();
	rect.top    = viewFrame.top;
	rect.left 	= viewFrame.right + 1;
	rect.bottom	= viewFrame.bottom;
	BScrollBar *pvscroll;
	pvscroll = new BScrollBar(rect, "vscroll", fpimageView, 0, 150, B_VERTICAL);
	AddChild(pvscroll);
	
	SetSizeLimits(250, 100000, 100, 100000);
	
	// finish creating window
	SetRef(pref);
	UpdateTitle();

	fpimageView->SetImage(pref);

	Show();
}

ShowImageWindow::~ShowImageWindow()
{
	delete fpref;
}

status_t
ShowImageWindow::InitCheck()
{
	if (!fpref || !fpimageView)
		return B_ERROR;
	else
		return B_OK;
}

void
ShowImageWindow::SetRef(const entry_ref *pref)
{
	if (!fpref)
		fpref = new entry_ref(*pref);
	else
		*fpref = *pref;
}

void
ShowImageWindow::UpdateTitle()
{
	BEntry entry(fpref);
	if (entry.InitCheck() == B_OK) {
		BPath path;
		entry.GetPath(&path);
		if (path.InitCheck() == B_OK)
			SetTitle(path.Path());
	}		
}

void
ShowImageWindow::LoadMenus(BMenuBar *pbar)
{
	BMenu *pmenu = new BMenu("File");
	AddItemMenu(pmenu, "Open", MSG_FILE_OPEN, 'O', 0, 'A', true);
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
	AddItemMenu(pmenu, "Copy", B_COPY, 'C', 0, 'W', false);
	AddItemMenu(pmenu, "Paste", B_PASTE, 'V', 0, 'W', false);
	AddItemMenu(pmenu, "Clear", MSG_CLEAR_SELECT, 0, 0, 'W', false);
	pmenu->AddSeparatorItem();
	AddItemMenu(pmenu, "Select All", MSG_SELECT_ALL, 'A', 0, 'W', false);
	pbar->AddItem(pmenu);

	pmenu = fppageMenu = new BMenu("Page");
	AddItemMenu(pmenu, "First", MSG_PAGE_FIRST, 'F', 0, 'W', true);
	AddItemMenu(pmenu, "Last", MSG_PAGE_LAST, 'L', 0, 'W', true);
	AddItemMenu(pmenu, "Next", MSG_PAGE_NEXT, 'N', 0, 'W', true);
	AddItemMenu(pmenu, "Previous", MSG_PAGE_PREV, 'P', 0, 'W', true);
	pbar->AddItem(pmenu);

	pmenu = new BMenu("Image");
	AddItemMenu(pmenu, "Dither Image", MSG_DITHER_IMAGE, 0, 0, 'W', true);
	pbar->AddItem(pmenu);
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

void
ShowImageWindow::WindowRedimension(BBitmap *pbitmap)
{
	// set the window's min & max size limits
	// based on document's data bounds
	float maxWidth = pbitmap->Bounds().Width() + B_V_SCROLL_BAR_WIDTH;
	float maxHeight = pbitmap->Bounds().Height() + fpbar->Frame().Height() +
		B_H_SCROLL_BAR_HEIGHT + 1;
	float minWidth = min(maxWidth, 100.0f);
	float minHeight = min(maxHeight, 100.0f);

	// adjust the window's current size based on new min/max values	
	float curWidth = Bounds().Width();
	float curHeight = Bounds().Height();	
	if (curWidth < minWidth)
		curWidth = minWidth;
	else if (curWidth > maxWidth)
		curWidth = maxWidth;

	if (curHeight < minHeight)
		curHeight = minHeight;
	else if (curHeight > maxHeight)
		curHeight = maxHeight;

	if (minWidth < 250)
		minWidth = 250;

	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
	ResizeTo(curWidth, curHeight);
}

void
ShowImageWindow::FrameResized(float width, float height)
{
}

void
ShowImageWindow::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		case MSG_OUTPUT_TYPE:
			// User clicked Save As then choose an output format
			if (!fpsavePanel)
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
			delete fpsavePanel;
			fpsavePanel = NULL;
			break;
			
		case MSG_UPDATE_STATUS:
		{
			bool benable = (fpimageView->PageCount() > 1) ? true : false;
			if (fppageMenu->IsEnabled() != benable)
				// Only call this function if the state is changing
				// to avoid flickering
				fppageMenu->SetEnabled(benable);
				
			BString str;
			if (pmsg->FindString("status", &str) == B_OK)
				fpstatusView->SetText(str);
			break;
		}

		case B_UNDO:
			break;
		case B_CUT:
			break;
		case B_COPY:
			break;
		case B_PASTE:
			break;
		case MSG_CLEAR_SELECT:
			break;
		case MSG_SELECT_ALL:
			break;
			
		case MSG_PAGE_FIRST:
			fpimageView->FirstPage();
			break;
			
		case MSG_PAGE_LAST:
			fpimageView->LastPage();
			break;
			
		case MSG_PAGE_NEXT:
			fpimageView->NextPage();
			break;
			
		case MSG_PAGE_PREV:
			fpimageView->PrevPage();
			break;

		case MSG_DITHER_IMAGE:
		     BMenuItem *pmenuDither;
	    	 pmenuDither = fpbar->FindItem(pmsg->what);
		     pmenuDither->SetMarked(!pmenuDither->IsMarked());
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
	fpsavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL, 0,
		false, ppanelMsg);
	if (!fpsavePanel)
		return;
	fpsavePanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
	fpsavePanel->Show();
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
	BBitmapStream stream(fpimageView->GetBitmap());	
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
	if (fpsavePanel)
		// Don't allow this window to be closed if a save panel is open
		return false;
	else
		return true;	
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

