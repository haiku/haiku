/*
    OBOS ShowImage 0.1 - 17/02/2002 - 22:22 - Fernando Francisco de Oliveira
*/

#include <algobase.h>
#include <Application.h>
#include <Bitmap.h>
#include <Entry.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <ScrollView.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <Locker.h>
#include <Alert.h>

#include "ShowImageConstants.h"
#include "ShowImageWindow.h"
#include "ShowImageView.h"
#include "ShowImageStatusView.h"

BLocker ShowImageWindow::s_winListLocker("ShowImageWindow list lock");
BList ShowImageWindow::s_winList;

status_t ShowImageWindow::NewWindow(const entry_ref* ref)
{
	BEntry entry(ref);
	if (entry.InitCheck() == B_OK) {
		BPath path;
		entry.GetPath(&path);
		if (path.InitCheck() == B_OK) {
			BBitmap* pBitmap = BTranslationUtils::GetBitmap(path.Path());
			if (pBitmap) {
				ShowImageWindow* pWin = new ShowImageWindow(ref, pBitmap);
				return pWin->InitCheck();			
			}
		}
	}
	return B_ERROR;		
}

int32 ShowImageWindow::CountWindows()
{
	int32 count = -1;
	if (s_winListLocker.Lock()) {
		count = s_winList.CountItems();
		s_winListLocker.Unlock();
	}
	return count;
}

ShowImageWindow::ShowImageWindow(const entry_ref* ref, BBitmap* pBitmap)
	: BWindow(BRect(50, 50, 350, 250), "", B_DOCUMENT_WINDOW, 0),
	m_pReferences(0)
{
	SetPulseRate( 200000.0 );
	
	// create menu bar	
	pBar = new BMenuBar( BRect(0,0, Bounds().right, 20), "menu_bar");
	LoadMenus(pBar);
	AddChild(pBar);

	BRect viewFrame = Bounds();
//	viewFrame.left      += 20;
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
	
	rect = Bounds();
	rect.top	= viewFrame.bottom + 1;
	rect.left 	= viewFrame.left   + 160;
	rect.right	= viewFrame.right;
	
	hor_scroll = new BScrollBar( rect, "hor_scroll", m_PrivateView, 0,150, B_HORIZONTAL );
	AddChild( hor_scroll );

	ShowImageStatusView * status_bar;

	rect.left = 0;
	rect.right = 159;

	status_bar = new ShowImageStatusView( rect, "status_bar", B_FOLLOW_BOTTOM, B_WILL_DRAW );
	status_bar->SetViewColor( ui_color( B_MENU_BACKGROUND_COLOR ) );
	status_bar->SetCaption( "ImageShow" );
		
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
	if (s_winListLocker.Lock()) {
		s_winList.AddItem(this);
		s_winListLocker.Unlock();
	}
	
	m_PrivateView->pBar = pBar;	

	Show();
}

ShowImageWindow::~ShowImageWindow()
{
	if (m_pReferences) {
		delete m_pReferences;
	}
	
	if (s_winListLocker.Lock()) {
		s_winList.RemoveItem(this);
		s_winListLocker.Unlock();
	}
	if (CountWindows() < 1) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
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
	long unsigned int MSG_QUIT  = B_QUIT_REQUESTED;
	long unsigned int MSG_ABOUT = B_ABOUT_REQUESTED;
	
	BMenu* pMenu = new BMenu("File");
	
	AddItemMenu( pMenu, "Open", MSG_FILE_OPEN, 'O', 0, 'A', true );
	pMenu->AddSeparatorItem();
	
//	BMenuItem * pMenuSaveAs = AddItemMenu( pMenu, "Save As...", MSG_FILE_SAVE, 'S', 0, 'W', true);
	
	BMenu* pMenuSaveAs = new BMenu( "Save As...", B_ITEMS_IN_COLUMN );
	
	pMenu->AddItem( pMenuSaveAs );
	

	BTranslatorRoster *roster = BTranslatorRoster::Default(); 
	
	int32 num_translators, i; 
	translator_id *translators; 
	const char *translator_name, *translator_info; 
	int32 translator_version; 

	roster->GetAllTranslators(&translators, &num_translators); 
	
	for (i=0;i<num_translators;i++) { 
	
		roster->GetTranslatorInfo(translators[i], &translator_name, 
			&translator_info, &translator_version); 
	
		BMenuItem * pSubMenu = new BMenuItem( translator_name , NULL );
		
		pMenuSaveAs->AddItem( pSubMenu );
		//printf("%s: %s (%.2f)n", translator_name, translator_info, translator_version/100.); 
	}

	delete [] translators; // clean up our droppings
	
	AddItemMenu( pMenu, "Close", MSG_QUIT, 'W', 0, 'A', true);
	pMenu->AddSeparatorItem();
	AddItemMenu( pMenu, "About ShowImage...", MSG_ABOUT, 0, 0, 'A', true);
	pMenu->AddSeparatorItem();
	AddItemMenu( pMenu, "Quit", MSG_QUIT, 'Q', 0, 'A', true);

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
	case MSG_FILE_SAVE :
	     pAlert = new BAlert( "File/Save", 
				  			  "File/Save not implemented yet", "OK");
  		 pAlert->Go();
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
// 		BMenu* pMenuDither = ;
