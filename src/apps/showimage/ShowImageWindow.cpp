/*
    OBOS ShowImage 0.1 - 17/02/2002 - 22:22 - Fernando Francisco de Oliveira
*/

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

status_t ShowImageWindow::NewWindow(const entry_ref* ref)
{
	// Get identify string (image type)
	BString strId = "Unknown";
	BTranslatorRoster *proster = BTranslatorRoster::Default();
	if (!proster)
		return B_ERROR;
	BFile file(ref, B_READ_ONLY);
	translator_info info;
	if (proster->Identify(&file, NULL, &info) == B_OK)
		strId = info.name;
	
	// Translate image data and create a new ShowImage window
	file.Seek(0, SEEK_SET);
	BBitmap* pBitmap = BTranslationUtils::GetBitmap(&file);
	if (pBitmap) {
		ShowImageWindow* pWin = new ShowImageWindow(ref, pBitmap, strId);
		return pWin->InitCheck();			
	}

	return B_ERROR;		
}

ShowImageWindow::ShowImageWindow(const entry_ref* ref, BBitmap* pBitmap, BString &strId)
	: BWindow(BRect(50, 50, 350, 250), "", B_DOCUMENT_WINDOW, 0),
	m_pReferences(0)
{
	fpsavePanel = NULL;
	
	// create menu bar	
	pBar = new BMenuBar( BRect(0,0, Bounds().right, 20), "menu_bar");
	LoadMenus(pBar);
	AddChild(pBar);

	BRect viewFrame = Bounds();
	viewFrame.top		= pBar->Bounds().bottom+1;
	viewFrame.right		-= B_V_SCROLL_BAR_WIDTH;
	viewFrame.bottom	-= B_H_SCROLL_BAR_HEIGHT;
	
	// create the image view	
	m_PrivateView = new ShowImageView(viewFrame, "image_view", B_FOLLOW_ALL, 
										B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED);
	m_PrivateView->SetBitmap(pBitmap);
	
	// wrap a scroll view around the view
	BScrollView* pScrollView = new BScrollView("image_scroller", m_PrivateView, B_FOLLOW_ALL, 
		0, false, false, B_PLAIN_BORDER);
			
	AddChild(pScrollView);
	
	BScrollBar *hor_scroll;

	BRect rect;
	
	const int32 kstatusWidth = 190;
	rect = Bounds();
	rect.top	= viewFrame.bottom + 1;
	rect.left 	= viewFrame.left   + kstatusWidth;
	rect.right	= viewFrame.right;
	
	hor_scroll = new BScrollBar( rect, "hor_scroll", m_PrivateView, 0,150, B_HORIZONTAL );
	AddChild( hor_scroll );

	ShowImageStatusView * status_bar;

	rect.left = 0;
	rect.right = kstatusWidth - 1;

	status_bar = new ShowImageStatusView( rect, "status_bar", B_FOLLOW_BOTTOM, B_WILL_DRAW );
	status_bar->SetViewColor( ui_color( B_MENU_BACKGROUND_COLOR ) );
	status_bar->SetText(strId);
		
	AddChild( status_bar );
	
	BScrollBar *vert_scroll;
	
	rect = Bounds();
	rect.top    = viewFrame.top;
	rect.left 	= viewFrame.right + 1;
	rect.bottom	= viewFrame.bottom;
	
	vert_scroll = new BScrollBar( rect, "vert_scroll", m_PrivateView, 0,150, B_VERTICAL );
	AddChild( vert_scroll );
	
	WindowRedimension( pBitmap );
	
	// finish creating window
	SetRef(ref);
	UpdateTitle();
	
	m_PrivateView->pBar = pBar;	

	Show();
}

ShowImageWindow::~ShowImageWindow()
{
	delete m_pReferences;
}

void ShowImageWindow::WindowActivated(bool active)
{
//	WindowRedimension( pBitmap );
}

status_t ShowImageWindow::InitCheck()
{
	if (! m_pReferences) {
		return B_ERROR;
	} else {
		return B_OK;
	}
}

void ShowImageWindow::SetRef(const entry_ref* ref)
{
	if (! m_pReferences) {
		m_pReferences = new entry_ref(*ref);
	} else {
		*m_pReferences = *ref;
	}
}

void ShowImageWindow::UpdateTitle()
{
	BEntry entry(m_pReferences);
	if (entry.InitCheck() == B_OK) {
		BPath path;
		entry.GetPath(&path);
		if (path.InitCheck() == B_OK) {
			SetTitle(path.Path());
		}
	}		
}

void ShowImageWindow::LoadMenus(BMenuBar* pBar)
{
	BMenu* pMenu = new BMenu("File");
	
	AddItemMenu( pMenu, "Open", MSG_FILE_OPEN, 'O', 0, 'A', true );
	pMenu->AddSeparatorItem();
	
	BMenu* pMenuSaveAs = new BMenu( "Save As...", B_ITEMS_IN_COLUMN );
	BTranslationUtils::AddTranslationItems(pMenuSaveAs, B_TRANSLATOR_BITMAP);
		// Fill Save As submenu with all types that can be converted
		// to from the Be bitmap image format
	pMenu->AddItem( pMenuSaveAs );
	
	AddItemMenu( pMenu, "Close", MSG_CLOSE, 'W', 0, 'W', true);
	pMenu->AddSeparatorItem();
	AddItemMenu( pMenu, "About ShowImage...", B_ABOUT_REQUESTED, 0, 0, 'A', true);
	pMenu->AddSeparatorItem();
	AddItemMenu( pMenu, "Quit", B_QUIT_REQUESTED, 'Q', 0, 'A', true);

	pBar->AddItem(pMenu);
	
	pMenu = new BMenu("Edit");
	
	AddItemMenu( pMenu, "Undo", B_UNDO, 'Z', 0, 'W', false);
	pMenu->AddSeparatorItem();
	AddItemMenu( pMenu, "Cut", B_CUT, 'X', 0, 'W', false);
	AddItemMenu( pMenu, "Copy", B_COPY, 'C', 0, 'W', false);
	AddItemMenu( pMenu, "Paste", B_PASTE, 'V', 0, 'W', false);
	AddItemMenu( pMenu, "Clear", MSG_CLEAR_SELECT, 0, 0, 'W', false);
	pMenu->AddSeparatorItem();
	AddItemMenu( pMenu, "Select All", MSG_SELECT_ALL, 'A', 0, 'W', true);

	pBar->AddItem(pMenu);

	pMenu = new BMenu("Image");
	AddItemMenu( pMenu, "Dither Image", MSG_DITHER_IMAGE, 0, 0, 'W', true);
	pBar->AddItem(pMenu);
}

BMenuItem * ShowImageWindow::AddItemMenu( BMenu *pMenu, char *Caption, long unsigned int msg, 
		char shortcut, uint32 modifier, char target, bool enabled ) {

	BMenuItem* pItem;
	
	pItem = new BMenuItem( Caption, new BMessage(msg), shortcut );
	
	if ( target == 'A' )
	   pItem->SetTarget(be_app);
	   
	pItem->SetEnabled( enabled );	   
	pMenu->AddItem(pItem);
	
	return( pItem );
}

void ShowImageWindow::WindowRedimension( BBitmap *pBitmap )
{
	// set the window's min & max size limits
	// based on document's data bounds
	float maxWidth = pBitmap->Bounds().Width() + B_V_SCROLL_BAR_WIDTH;
	float maxHeight = pBitmap->Bounds().Height()
		+ pBar->Frame().Height()
		+ B_H_SCROLL_BAR_HEIGHT + 1;
	float minWidth = min(maxWidth, 100.0f);
	float minHeight = min(maxHeight, 100.0f);

	// adjust the window's current size based on new min/max values	
	float curWidth = Bounds().Width();
	float curHeight = Bounds().Height();	
	if (curWidth < minWidth) {
		curWidth = minWidth;
	} else if (curWidth > maxWidth) {
		curWidth = maxWidth;
	}
	if (curHeight < minHeight) {
		curHeight = minHeight;
	} else if (curHeight > maxHeight) {
		curHeight = maxHeight;
	}
	if ( minWidth < 250 ) {
		minWidth = 250;
	}
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
	ResizeTo(curWidth, curHeight);
}

void ShowImageWindow::FrameResized( float new_width, float new_height )
{
}

void ShowImageWindow::MessageReceived(BMessage* message)
{
	BAlert* pAlert;
	
	switch (message->what) {
		case MSG_OUTPUT_TYPE:
			// User clicked Save As then choose an output format
			SaveAs(message);			
			break;
			
		case MSG_SAVE_PANEL:
			// User specified where to save the output image
			SaveToFile(message);
			break;
			
		case MSG_CLOSE:
			Quit();
			break;

	case B_UNDO :
	     pAlert = new BAlert( "Edit/Undo", 
				  			  "Edit/Undo not implemented yet", "OK");
  		 pAlert->Go();
		break;
	case B_CUT :
	     pAlert = new BAlert( "Edit/Cut", 
				  			  "Edit/Cut not implemented yet", "OK");
  		 pAlert->Go();
		break;
	case B_COPY :
	     pAlert = new BAlert( "Edit/Copy", 
				  			  "Edit/Copy not implemented yet", "OK");
  		 pAlert->Go();
		break;
	case B_PASTE :
	     pAlert = new BAlert( "Edit/Paste", 
				  			  "Edit/Paste not implemented yet", "OK");
  		 pAlert->Go();
		break;
	case MSG_CLEAR_SELECT :
	     pAlert = new BAlert( "Edit/Clear Select", 
				  			  "Edit/Clear Select not implemented yet", "OK");
  		 pAlert->Go();
		break;
	case MSG_SELECT_ALL :
	     pAlert = new BAlert( "Edit/Select All", 
				  			  "Edit/Select All not implemented yet", "OK");
  		 pAlert->Go();
		break;
	case MSG_DITHER_IMAGE :
	     BMenuItem   * pMenuDither;
	     pMenuDither = pBar->FindItem( message->what );
	     pMenuDither->SetMarked( ! pMenuDither->IsMarked() );
	     
		 break;
		 
	default:
		BWindow::MessageReceived(message);
		break;
	}
}

void
ShowImageWindow::SaveAs(BMessage *pmsg)
{
	// Handle SaveAs menu choice by setting the
	// translator and desired output format
	// and giving the user a save panel

	if (pmsg->FindInt32("be:translator",
		reinterpret_cast<int32 *>(&foutTranslator)) != B_OK)
		return;	
	if (pmsg->FindInt32("be:type",
		reinterpret_cast<int32 *>(&foutType)) != B_OK)
		return;

	if (!fpsavePanel) {
		fpsavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL, 0,
			false, new BMessage(MSG_SAVE_PANEL));
		if (!fpsavePanel)
			return;
	}
	
	fpsavePanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
	fpsavePanel->Show();
}

void
ShowImageWindow::SaveToFile(BMessage *pmsg)
{
	// After the user has chosen which format
	// to output to and where to save the file,
	// this function is called to save the currently
	// open image to a file in the desired format.
	
	entry_ref dirref;
	if (pmsg->FindRef("directory", &dirref) != B_OK)
		return;
	const char *filename;
	if (pmsg->FindString("name", &filename) != B_OK)
		return;
		
	BDirectory dir(&dirref);
	BFile file(&dir, filename, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK)
		return;
	
	BBitmapStream stream(m_PrivateView->GetBitmap());	
	BTranslatorRoster *proster = BTranslatorRoster::Default();
	if (proster->Translate(foutTranslator, &stream, NULL,
		&file, foutType) != B_OK) {
		BAlert *palert = new BAlert(NULL, "Error writing image file.", "Ok");
		palert->Go();
	}
	
	BBitmap *pout = NULL;
	stream.DetachBitmap(&pout);
		// bitmap used by stream still belongs to the view,
		// detach so it doesn't get deleted
}

void
ShowImageWindow::Quit()
{
	// tell the app to forget about this window
	be_app->PostMessage(MSG_WINDOW_QUIT);
	BWindow::Quit();
}

// 		BMenu* pMenuDither = ;
