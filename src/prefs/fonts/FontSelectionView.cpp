/*! \file FontSelectionView.cpp
    \brief Header for the FontSelectionView class.
    
*/

#ifndef FONT_SELECTION_VIEW_H
	
	#include "FontSelectionView.h"

#endif

/**
 * Constructor
 * @param rect The size of the view.
 * @param name The name of the view.
 * @param type The type of the view: plain, italic, etc.
 */
FontSelectionView::FontSelectionView(BRect rect, const char *name, int type)
	   	   : BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	
	BBox *testTextBox;
	float x;
	float y;
	BRect viewSize = Bounds();
	BMenuField *fontListField;
	BMenuField *sizeListField;
	
	x = viewSize.Width() / 37;
	y = viewSize.Height() / 8;
	
	minSizeIndex = 9;
	maxSizeIndex = 12;
	
	switch(type){
	
		case PLAIN_FONT_SELECTION_VIEW:
		
			sprintf(typeLabel, "Plain font");
			origFont = be_plain_font;
			workingFont = be_plain_font;
			setSizeChangedMessage = PLAIN_SIZE_CHANGED_MSG;
			setFontChangedMessage = PLAIN_FONT_CHANGED_MSG;
			setStyleChangedMessage = PLAIN_STYLE_CHANGED_MSG;
			
			defaultFont = new BFont();
			defaultFont->SetFamilyAndStyle("Swis721 BT", "Roman");
			defaultFont->SetSize(10.0);
			
			break;
			
		case BOLD_FONT_SELECTION_VIEW:
		
			sprintf(typeLabel, "Bold font");
			origFont = be_bold_font;
			workingFont = be_bold_font;
			setSizeChangedMessage = BOLD_SIZE_CHANGED_MSG;
			setFontChangedMessage = BOLD_FONT_CHANGED_MSG;
			setStyleChangedMessage = BOLD_STYLE_CHANGED_MSG;
			
			defaultFont = new BFont();
			defaultFont->SetFamilyAndStyle("Swis721 BT", "Bold");
			defaultFont->SetSize(12.0);
			
			break;
			
		case FIXED_FONT_SELECTION_VIEW:
		
			sprintf(typeLabel, "Fixed font");
			origFont = be_fixed_font;
			workingFont = be_fixed_font;
			setSizeChangedMessage = FIXED_SIZE_CHANGED_MSG;
			setFontChangedMessage = FIXED_FONT_CHANGED_MSG;
			setStyleChangedMessage = FIXED_STYLE_CHANGED_MSG;
			
			defaultFont = new BFont();
			defaultFont->SetFamilyAndStyle("Courier10 BT", "Roman");
			defaultFont->SetSize(12.0);
			
			break;
			
	}//switch
	
	sizeList = new BPopUpMenu("sizeList", true, true, B_ITEMS_IN_COLUMN);
	fontList = new BPopUpMenu("fontList", true, true, B_ITEMS_IN_COLUMN);
	fontListField = new BMenuField(*(new BRect(x, y, (25 * x), (3 * y))), "fontField", typeLabel, fontList);
	fontListField->SetDivider(7 * x);
	sizeListField = new BMenuField(*(new BRect((27 * x), y, (36 * x), (3 * y))), "fontField", "Size", sizeList);
	sizeListField->SetDivider(31 * x);
	testText = new BStringView(*(new BRect((8 * x), (5 * y), (35 * x), (8 * y))), "testText", "The quick brown fox jumped over the lazy dog.", B_FOLLOW_ALL, B_WILL_DRAW);
	testText->SetFont(&workingFont);
	testTextBox = new BBox(*(new BRect((8 * x), (5 * y), (36 * x), (8 * y))), "TestTextBox", B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER);
	
	fontList->SetLabelFromMarked(true);
	
	buildMenus();

	SetViewColor(216, 216, 216, 0);
	
	AddChild(testTextBox);
	AddChild(testText);
	AddChild(sizeListField);
	AddChild(fontListField);
		
}

/**
 * Emptys the menu so it can be rebuilt.
 * @param m The menu to empty.
 */
void FontSelectionView::emptyMenu(BPopUpMenu *m){

	int32 cnt;
	int32 numItemsInMenu = m->CountItems();
	
	for(cnt = 0;cnt < numItemsInMenu;cnt++){
	
		BMenu *tmp;
		BMenuItem *tmpI;
		int32 cnt2;
		int32 numItemsInSubmenu;
		
		tmpI = m->RemoveItem((int32) 0);
		tmp = tmpI->Submenu();
		
		if(tmp != NULL){
		
			numItemsInSubmenu = tmp->CountItems();
			for(cnt2 = 0;cnt2 < numItemsInSubmenu;cnt2++){
			
				BMenuItem *tmpItem;
				
				tmpItem = tmp->RemoveItem((int32) 0);
				delete tmpItem;
				
			}//for
		
		}//if
		
		delete tmpI;
	
	}//for

}//emptyMenu

/**
 * Emptys both the font and the size menus.
 */
void FontSelectionView::emptyMenus(){

	//empty font menu
	emptyMenu(fontList);
	
	//empty size menu
	emptyMenu(sizeList);
	
}//emptyMenus

/**
 * Builds the font and size menus.
 */
void FontSelectionView::buildMenus(){

	int32 numFamilies;
	int counter;
	
	numFamilies = count_font_families(); 
	for ( int32 i = 0; i < numFamilies; i++ ) { 
		
		font_family family; 
		uint32 flags;
		
		if ( get_font_family(i, &family, &flags) == B_OK ) { 
			
			BMenu *tmpStyleMenu;
			int32 numStyles;
			bool markFamily;
			
			markFamily = false;
			
			tmpStyleMenu = new BMenu(family);
			tmpStyleMenu->SetRadioMode(true);
			numStyles = count_font_styles(family); 
			for ( int32 j = 0; j < numStyles; j++ ) { 
				
				font_style style; 
				if ( get_font_style(family, j, &style, &flags) == B_OK ) { 
				
					BMenuItem *tmpItem;
					char workingStyle[64];
					char workingFamily[64];
					
					workingFont.GetFamilyAndStyle(&workingFamily, &workingStyle);
					tmpItem = new BMenuItem(style, new BMessage(setStyleChangedMessage));
					if((strcmp(style, workingStyle) == 0) && (strcmp(family, workingFamily) == 0)){
					
						markFamily = true;
						tmpItem->SetMarked(true);
						
					}//if
					
					tmpStyleMenu->AddItem(tmpItem);
				
				}//if
			
			}//for
			
			fontList->AddItem(new BMenuItem((tmpStyleMenu), new BMessage(setFontChangedMessage)));
			
			if(markFamily){
				
				tmpStyleMenu->Superitem()->SetMarked(true);
						
			}//if
			
		}//if
	
	}//for
	
	//build size menu
	for(counter = minSizeIndex;counter < (maxSizeIndex + 1);counter++){
	
		char buf[1];
		BMenuItem *tmp;
		
		sprintf(buf, "%d", counter);
		sizeList->AddItem(tmp = new BMenuItem(buf, new BMessage(setSizeChangedMessage)));
		if(counter == (int) workingFont.Size()){
		
			tmp->SetMarked(true);
			
		}//if
		
	}//for
	
}//buildMenus

/**
 * Writes the test text in the given font.
 * @param fnt The font to write the test text in.
 */
void FontSelectionView::SetTestTextFont(BFont *fnt){

	testText->SetFont(fnt, B_FONT_ALL);
	testText->Invalidate();
	
}//SetTextTextFont

/**
 * Gets the current font.
 */
BFont FontSelectionView::GetTestTextFont(){

	BFont rtrnFont;
	
	testText->GetFont(&rtrnFont);
	
	return rtrnFont;

}//SetTextTextFont

/**
 * Gets the selected size.
 */
float FontSelectionView::GetSelectedSize(){

	return minSizeIndex + sizeList->IndexOf(sizeList->FindMarked());

}//GetSelectedSize

/**
 * Gets the selected font_family.
 * @param family The selected font_family.
 */
void FontSelectionView::GetSelectedFont(font_family *family){

	int numFamilies = count_font_families();
	for ( int32 i = 0; i < numFamilies; i++ ) { 
		
		font_family fam; 
		uint32 flags;
		
		if ( get_font_family(i, &fam, &flags) == B_OK ) { 
			
			if(strcmp(fam, fontList->FindMarked()->Label()) == 0){
			
				get_font_family(i, family, &flags);
				
			}//if
			
		}//if
	
	}//for
	 
}//GetSelectedFont

/**
 * Gets the selected style.
 * @param style The selected style.
 */
void FontSelectionView::GetSelectedStyle(font_style *style){

	int numFamilies = count_font_families();
	font_family curr;
	
	GetSelectedFont(&curr);
	
	for ( int32 i = 0; i < numFamilies; i++ ) { 
		
		font_family fam; 
		uint32 flags;
		
		if ( get_font_family(i, &fam, &flags) == B_OK ) { 
			
			if(strcmp(fam, curr) == 0){
			
				int32 numStyles = count_font_styles(fam); 
				for ( int32 j = 0; j < numStyles; j++ ) { 
					
					font_style sty; 
					if ( get_font_style(fam, j, &sty, &flags) == B_OK ) { 
					
						if(strcmp(sty, fontList->FindMarked()->Submenu()->FindMarked()->Label()) == 0){
				
							get_font_style(fam, j, style, &flags);
							
						}//if
					
					}//if
				
				}//for
				
			}//if
			
		}//if
	
	}//for
	 
}//GetSelectedStyle

/**
 * If a style is selected, this function is called to update the font menu,
 * in case the selected style is from a non selected font.  It also marks the
 * selected style, and unmarks all other styles.
 */
void FontSelectionView::UpdateFontSelectionFromStyle(){

	int i = 0;
	
	for(i = 0;i < fontList->CountItems();i++){
	
		int j = 0;
		
		for(j = 0;j < fontList->ItemAt(i)->Submenu()->CountItems();j++){
		
			if(fontList->ItemAt(i)->Submenu()->ItemAt(j)->IsMarked()){
			
				if(!strcmp(fontList->ItemAt(i)->Label(), fontList->FindMarked()->Label()) == 0){
				
					fontList->FindMarked()->Submenu()->FindMarked()->SetMarked(false);
					fontList->ItemAt(i)->SetMarked(true);
					
				}//if
			
			}//if
		
		}//for
		
	}//for
	
}//UpdateFontSelection

/**
 * Updates the font menu based on the user selection.
 */
void FontSelectionView::UpdateFontSelection(){

	int i = 0;
	
	for(i = 0;i < fontList->CountItems();i++){
	
		int j = 0;
		
		for(j = 0;j < fontList->ItemAt(i)->Submenu()->CountItems();j++){
		
			if(fontList->ItemAt(i)->Submenu()->ItemAt(j)->IsMarked()){
			
				if(strcmp(fontList->ItemAt(i)->Label(), fontList->FindMarked()->Label()) > 0){
			
					fontList->ItemAt(i)->Submenu()->FindMarked()->SetMarked(false);
					fontList->FindMarked()->Submenu()->ItemAt(0)->SetMarked(true);
			
				}//if
			
			}//if
		
		}//for
		
	}//for

}//UpdateFontSelection

/**
 * Updates the font and size menus based on the given font.
 * @param fnt The font to set the menus to.
 * \note This methd needs rewriting BADLY - it's horribly written
 */
void FontSelectionView::UpdateFontSelection(BFont *fnt){

	int i = 0;
	char style[64];
	char family[64];
	
	fnt->GetFamilyAndStyle(&family, &style);
	
	for(i = 0;i < fontList->CountItems();i++){
	
		int j = 0;
		
		if(strcmp(fontList->ItemAt(i)->Label(), family) == 0){
		
			fontList->ItemAt(i)->SetMarked(true);
		
		}//if
		
		for(j = 0;j < fontList->ItemAt(i)->Submenu()->CountItems();j++){
		
			if(fontList->ItemAt(i)->Submenu()->ItemAt(j)->IsMarked()){
				
				fontList->ItemAt(i)->Submenu()->ItemAt(j)->SetMarked(false);
				
			}//if
			if(strcmp(fontList->ItemAt(i)->Label(), family) == 0){
			
				if(strcmp(fontList->ItemAt(i)->Submenu()->ItemAt(j)->Label(), style) == 0){
				
					fontList->ItemAt(i)->Submenu()->ItemAt(j)->SetMarked(true);
				
				}//if
			
			}//if
		
		}//for
		
	}//for
	
	//Update size menu
	for(i = 0;i < sizeList->CountItems();i++){
	
		char size[1];
		
		sprintf(size, "%d", (int)fnt->Size());
		if(strcmp(sizeList->ItemAt(i)->Label(), size) == 0){
		
			sizeList->ItemAt(i)->SetMarked(true);
		
		}//if
	
	}//for
	
}//UpdateFontSelection

/**
 * Resets the test text to the default font.
 */
void FontSelectionView::resetToDefaults(){

	//Update menus
	UpdateFontSelection(defaultFont);

	//Update test text
	SetTestTextFont(defaultFont);
	
}//resetToDefaults

/**
 * Resets the test text to the original font.
 */
void FontSelectionView::revertToOriginal(){

	//Update menus
	UpdateFontSelection(&origFont);

	//Update test text
	SetTestTextFont(&origFont);
	
}//resetToDefaults



