/*! \file MainWindow.cpp
 *  \brief Code for the main window.
 *
 *  Displays the main window.
 *
*/

#ifndef MAIN_WINDOW_H
	
	#include "MainWindow.h"

#endif

/**
 * Constructor.
 * @param frame The size to make the window.
 */
MainWindow::MainWindow(BRect frame)
				: BWindow(frame, "Fonts", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE ){
				
	BRect r; 
	BTabView *tabView; 
	BTab *tab; 
	BBox *topLevelView;
	double buttonViewHeight = 38.0;
	
	r = Bounds(); 
	r.InsetBy(0, 10); 
	r.bottom -= buttonViewHeight;
	
	tabView = new BTabView(r, "tab_view", B_WIDTH_FROM_WIDEST); 
	tabView->SetViewColor(216,216,216,0); 
	
	r = tabView->Bounds(); 
	r.InsetBy(5,5); 
	r.bottom -= tabView->TabHeight(); 
	tab = new BTab(); 
	tabView->AddTab(fontPanel = new FontView(r), tab); 
	tab->SetLabel("Fonts"); 
	tab = new BTab(); 
	tabView->AddTab(cachePanel = new CacheView(r, 64, 4096, (int32) 256, (int32) 256), tab); 
	tab->SetLabel("Cache");
		
	r = Bounds();
	r.top = r.bottom - buttonViewHeight;
	
	buttonView = new ButtonView(r);
	
	topLevelView = new BBox(Bounds(), "TopLevelView", B_FOLLOW_ALL, B_WILL_DRAW, B_NO_BORDER);
	
	AddChild(buttonView);
	AddChild(tabView);
	AddChild(topLevelView);
	
}

/**
 * Runs when the user quits the application.
 */
bool MainWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}

/**
 * Handles messages sent to the class.
 * @param message The message to be handled.
 */
void MainWindow::MessageReceived(BMessage *message){

	char msg[100];
	
	switch(message->what){
	
		case PLAIN_SIZE_CHANGED_MSG:
		
			updateSize(fontPanel->plainSelectionView);
			buttonView->SetRevertState(true);
			break;
		
		case BOLD_SIZE_CHANGED_MSG:
		
			updateSize(fontPanel->boldSelectionView);
			buttonView->SetRevertState(true);
			break;
		
		case FIXED_SIZE_CHANGED_MSG:
		
			updateSize(fontPanel->fixedSelectionView);
			buttonView->SetRevertState(true);
			break;
		
		case PLAIN_FONT_CHANGED_MSG:
		
			updateFont(fontPanel->plainSelectionView);
			buttonView->SetRevertState(true);
			break;
		
		case BOLD_FONT_CHANGED_MSG:
		
			updateFont(fontPanel->boldSelectionView);
			buttonView->SetRevertState(true);
			break;
		
		case FIXED_FONT_CHANGED_MSG:
		
			updateFont(fontPanel->fixedSelectionView);
			buttonView->SetRevertState(true);
			break;
			
		case PLAIN_STYLE_CHANGED_MSG:
		
			updateStyle(fontPanel->plainSelectionView);
			buttonView->SetRevertState(true);
			break;
		
		case BOLD_STYLE_CHANGED_MSG:
		
			updateStyle(fontPanel->boldSelectionView);
			buttonView->SetRevertState(true);
			break;
		
		case FIXED_STYLE_CHANGED_MSG:
		
			updateStyle(fontPanel->fixedSelectionView);
			buttonView->SetRevertState(true);
			break;
		
		case RESCAN_FONTS_MSG:
		
			update_font_families(false);
			fontPanel->emptyMenus();
			fontPanel->buildMenus();
			updateFont(fontPanel->plainSelectionView);
			updateFont(fontPanel->boldSelectionView);
			updateFont(fontPanel->fixedSelectionView);
			break;
			
		case RESET_FONTS_MSG:
		
			fontPanel->resetToDefaults();
			cachePanel->resetToDefaults();
			buttonView->SetRevertState(true);
			break;
			
		case REVERT_MSG:
		
			fontPanel->revertToOriginal();
			cachePanel->revertToOriginal();
			buttonView->SetRevertState(false);
			break;
			
		case PRINT_FCS_UPDATE_MSG:
		
			printf("print update message\n");
			//buttonView->SetRevertState(true);
			break;
			
		case PRINT_FCS_MODIFICATION_MSG:
		
			sprintf(msg, "Printing font cache size : %d kB", cachePanel->getPrintFCSValue());
			cachePanel->updatePrintFCS(msg);
			buttonView->SetRevertState(true);
			break;
		
		case SCREEN_FCS_UPDATE_MSG:
		
			printf("screen update message\n");
			//buttonView->SetRevertState(true);
			buttonView->SetRevertState(true);
			break;
				
		case SCREEN_FCS_MODIFICATION_MSG:
		
			sprintf(msg, "Screen font cache size : %d kB", cachePanel->getScreenFCSValue());
			cachePanel->updateScreenFCS(msg);
			buttonView->SetRevertState(true);
			break;
		
		default:
		
			BWindow::MessageReceived(message);
	
	}
	
}

/**
 * Sets the size of the test text when a new size is picked.
 * @param theView The FontSelectionView that is affected.
 */
void MainWindow::updateSize(FontSelectionView *theView){

	BFont workingFont;
	
	workingFont = theView->GetTestTextFont();
	workingFont.SetSize(theView->GetSelectedSize());
	theView->SetTestTextFont(&workingFont);
		
}//updateSize

/**
 * Updates the test text to the selected font.
 * @param theView The FontSelectionView that is affected.
 */
void MainWindow::updateFont(FontSelectionView *theView){

	BFont workingFont;
	font_family updateTo;
	
	theView->UpdateFontSelection();
	workingFont = theView->GetTestTextFont();
	theView->GetSelectedFont(&updateTo);
	workingFont.SetFamilyAndStyle(updateTo, NULL);
	theView->SetTestTextFont(&workingFont);
	
}//updateFont

/**
 * Updates the test text to the selected style.
 * @param theView The FontSelectionView that is affected.
 */
void MainWindow::updateStyle(FontSelectionView *theView){

	BFont workingFont;
	font_style updateTo;
	font_family update;
	
	theView->UpdateFontSelectionFromStyle();
	workingFont = theView->GetTestTextFont();
	theView->GetSelectedStyle(&updateTo);
	theView->GetSelectedFont(&update);
	workingFont.SetFamilyAndStyle(update, updateTo);
	theView->SetTestTextFont(&workingFont);

}//updateStyle
