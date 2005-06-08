/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#include "FontSelectionView.h"
#include "Pref_Utils.h"

#define SIZE_CHANGED_MSG 'plsz'
#define FONT_CHANGED_MSG 'plfn'
#define STYLE_CHANGED_MSG 'plst'

// should be changed to allow larger and smaller
#define minSizeIndex 9
#define maxSizeIndex 14

// constants for labels
const char *kPlainFont = "Plain font:";
const char *kBoldFont =	 "Bold font:";
const char *kFixedFont = "Fixed font:";
const char *kSize = "Size: ";

FontSelectionView::FontSelectionView(BRect rect, const char *name, int type)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	switch(type)
	{
		case BOLD_FONT_SELECTION_VIEW:
		{
			sprintf(typeLabel, kBoldFont);
			origFont = be_bold_font;
			workingFont = be_bold_font;
			
			defaultFont = new BFont();
			defaultFont->SetFamilyAndStyle("Swis721 BT", "Bold");
			defaultFont->SetSize(12.0);
			
			break;
		}	
		case FIXED_FONT_SELECTION_VIEW:
		{
			sprintf(typeLabel, kFixedFont);
			origFont = be_fixed_font;
			workingFont = be_fixed_font;
			
			defaultFont = new BFont();
			defaultFont->SetFamilyAndStyle("Courier10 BT", "Roman");
			defaultFont->SetSize(12.0);
			
			break;
		}
		default:
		{
			sprintf(typeLabel, kPlainFont);
			origFont = be_plain_font;
			workingFont = be_plain_font;
			
			defaultFont = new BFont();
			defaultFont->SetFamilyAndStyle("Swis721 BT", "Roman");
			defaultFont->SetSize(10.0);
			
			break;
		}	
	}

	float fontheight = FontHeight(false);
	float divider = StringWidth(kFixedFont);

	sizeList = new BPopUpMenu("sizeList", true, true, B_ITEMS_IN_COLUMN);
	fontList = new BPopUpMenu("fontList", true, true, B_ITEMS_IN_COLUMN);
	
	// create menus
	
	// size box
	rect = Bounds();
	float x = StringWidth("999") +16;
	rect.left = rect.right -(x +StringWidth(kSize)+8.0);
	rect.bottom = fontheight +5;
	BMenuField *sizeListField = new BMenuField(rect, "fontField", kSize, sizeList, true);
	sizeListField->SetDivider(StringWidth(kSize)+5.0);
	sizeListField->SetAlignment(B_ALIGN_RIGHT);
	
	// font menu
	rect.right = rect.left;
	rect.left = 1;
	rect.bottom = fontheight *1.5;
	BMenuField *fontListField = new BMenuField(rect, "fontField", typeLabel, fontList, false);
	fontListField->SetDivider(divider +6.0);
	fontListField->SetAlignment(B_ALIGN_RIGHT);
	
	rect = Bounds();
	rect.left = divider +8.0;
	rect.top = fontheight *1.5 +4;
	rect.InsetBy(1, 1);
	BBox *testTextBox = new BBox(rect, "TestTextBox", B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER);

	// Place the text slightly inside the entire box area, so it doesn't overlap the box outline.
	rect = testTextBox->Bounds().InsetByCopy(2, 2);
	rect.right -= 2;
	BRect testTextRect(rect);
	testText = new BStringView(testTextRect, "testText", "The quick brown fox jumps over the lazy dog.", 
		B_FOLLOW_ALL, B_WILL_DRAW);
	testText->SetFont(&workingFont);
	
	fontList->SetLabelFromMarked(true);
	
	buildMenus();

	SetViewColor(216, 216, 216, 0);
	
	AddChild(testTextBox);
	testTextBox->AddChild(testText);
	AddChild(sizeListField);
	AddChild(fontListField);
		
}

FontSelectionView::~FontSelectionView(void)
{
	delete defaultFont;
}

void
FontSelectionView::AttachedToWindow(void)
{
}

void
FontSelectionView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
/*		case SIZE_CHANGED_MSG: {
		
			updateSize(fSelectorView->plainSelectionView);
			fButtonView->SetRevertState(true);
			break;
		}
		case FONT_CHANGED_MSG: {
		
			updateFont(fSelectorView->plainSelectionView);
			fButtonView->SetRevertState(true);
			break;
		}
		case STYLE_CHANGED_MSG: {
		
			updateStyle(fSelectorView->plainSelectionView);
			fButtonView->SetRevertState(true);
			break;
		}
		case RESCAN_FONTS_MSG: {
		
			update_font_families(false);
			fSelectorView->emptyMenus();
			fSelectorView->buildMenus();
			updateFont(fSelectorView->plainSelectionView);
			updateFont(fSelectorView->boldSelectionView);
			updateFont(fSelectorView->fixedSelectionView);
			break;
		}
		case RESET_FONTS_MSG: {
		
			fSelectorView->resetToDefaults();
			fCacheView->resetToDefaults();
			fButtonView->SetRevertState(true);
			break;
		}
*/		default:
			BView::MessageReceived(msg);
	}
}

void FontSelectionView::emptyMenus(void)
{
	// Empty the font list
	for(int32 i = 0; i < fontList->CountItems(); i++)
	{
		BMenu *menu = fontList->SubmenuAt(0L);
		
		// We should never have a regular menu item in the font list
		if(!menu)
			continue;
		
		for(int32 j=0; j < menu->CountItems(); j++)
		{
			BMenuItem *item = menu->RemoveItem(0L);
			delete item;
		}
		
		fontList->RemoveItem(menu);
		delete menu;
	}
	
	// empty the size list
	for(int32 i = 0; i < sizeList->CountItems(); i++)
	{
		BMenuItem *item = sizeList->RemoveItem(0L);
		delete item;
	}
}

void FontSelectionView::buildMenus(void)
{
	int32 numFamilies;
	int counter;
	
	numFamilies = count_font_families(); 
	for ( int32 i = 0; i < numFamilies; i++ )
	{ 
		font_family family; 
		
		uint32 flags;
		int32 numStyles;
		
		BMenu *tmpStyleMenu;
		bool markFamily;
		
		if ( get_font_family(i, &family, &flags) != B_OK ) 
			continue;
		
		
		markFamily = false;
		
		tmpStyleMenu = new BMenu(family);
		tmpStyleMenu->SetRadioMode(true);
		numStyles = count_font_styles(family); 
		
		for ( int32 j = 0; j < numStyles; j++ )
		{ 
			font_style style; 
			BMenuItem *tmpItem;
			
			char workingStyle[64];
			char workingFamily[64];
			
			if (get_font_style(family, j, &style, &flags) != B_OK)
				continue;
			
			
			workingFont.GetFamilyAndStyle(&workingFamily, &workingStyle);
			tmpItem = new BMenuItem(style, new BMessage(STYLE_CHANGED_MSG));
			
			if((strcmp(style, workingStyle) == 0) && (strcmp(family, workingFamily) == 0))
			{
				markFamily = true;
				tmpItem->SetMarked(true);
			}
			tmpStyleMenu->AddItem(tmpItem);
		}
		
		fontList->AddItem(new BMenuItem((tmpStyleMenu), new BMessage(FONT_CHANGED_MSG)));
		
		if(markFamily)
			tmpStyleMenu->Superitem()->SetMarked(true);
	}
	
	// build size menu
	for(counter = minSizeIndex; counter < (maxSizeIndex + 1); counter++)
	{
		char buf[1];
		BMenuItem *tmp;
		
		sprintf(buf, "%d", counter);
		sizeList->AddItem(tmp = new BMenuItem(buf, new BMessage(SIZE_CHANGED_MSG)));
		if(counter == (int) workingFont.Size())
			tmp->SetMarked(true);
	}
}

/**
 * Writes the test text in the given font.
 * @param fnt The font to write the test text in.
 */
void FontSelectionView::SetTestTextFont(BFont *fnt)
{
	testText->SetFont(fnt, B_FONT_ALL);
	testText->Invalidate();
}

BFont FontSelectionView::GetTestTextFont()
{

	BFont rtrnFont;
	
	testText->GetFont(&rtrnFont);
	
	return rtrnFont;

}

float
FontSelectionView::GetSelectedSize()
{
	return minSizeIndex + sizeList->IndexOf(sizeList->FindMarked());

}

void FontSelectionView::GetSelectedFont(font_family *family)
{
	int numFamilies = count_font_families();
	for ( int32 i = 0; i < numFamilies; i++ )
	{ 
		font_family fam; 
		uint32 flags;
		
		if ( get_font_family(i, &fam, &flags) == B_OK ) 
		{ 
			if(strcmp(fam, fontList->FindMarked()->Label()) == 0)
				get_font_family(i, family, &flags);
		}
	}
}

void FontSelectionView::GetSelectedStyle(font_style *style)
{
	int numFamilies = count_font_families();
	font_family curr;
	
	GetSelectedFont(&curr);
	
	for ( int32 i = 0; i < numFamilies; i++ )
	{ 
		font_family fam; 
		uint32 flags;
		
		if (get_font_family(i, &fam, &flags) == B_OK  && strcmp(fam, curr) == 0 )
		{
			int32 numStyles = count_font_styles(fam); 
			for ( int32 j = 0; j < numStyles; j++ ) 
			{ 
				font_style sty; 
				if ( get_font_style(fam, j, &sty, &flags) == B_OK )
				{ 
					if(strcmp(sty, fontList->FindMarked()->Submenu()->FindMarked()->Label()) == 0)
						get_font_style(fam, j, style, &flags);
				}
			}
		}
	}
}

/**
 * If a style is selected, this function is called to update the font menu,
 * in case the selected style is from a non selected font.  It also marks the
 * selected style, and unmarks all other styles.
 */
void FontSelectionView::UpdateFontSelectionFromStyle()
{
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
}

/**
 * Updates the font menu based on the user selection.
 */
void FontSelectionView::UpdateFontSelection()
{
	for(int32 i = 0; i < fontList->CountItems();i++)
	{
		for(int32 j = 0;j < fontList->ItemAt(i)->Submenu()->CountItems();j++)
		{
			if(fontList->ItemAt(i)->Submenu()->ItemAt(j)->IsMarked())
			{
				if(strcmp(fontList->ItemAt(i)->Label(), fontList->FindMarked()->Label()) > 0)
				{
					fontList->ItemAt(i)->Submenu()->FindMarked()->SetMarked(false);
					fontList->FindMarked()->Submenu()->ItemAt(0)->SetMarked(true);
				}
			}
		}
	}
}

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
			break;		
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



