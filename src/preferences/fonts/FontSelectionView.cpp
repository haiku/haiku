/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#include "FontSelectionView.h"
#include "MainWindow.h"
#include "Pref_Utils.h"
#include <String.h>

#define SIZE_CHANGED_MSG 'plsz'
#define FONT_CHANGED_MSG 'plfn'
#define STYLE_CHANGED_MSG 'plst'

// should be changed to allow larger and smaller
#define MIN_SIZE_INDEX 9
#define MAX_SIZE_INDEX 14

extern void _set_system_font_(const char *which, font_family family, font_style style, float size);

FontSelectionView::FontSelectionView(BRect rect, const char *name, int type)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW),
	fType(type)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BString typelabel;
	
	switch(fType)
	{
		case BOLD_FONT_SELECTION_VIEW:
		{
			typelabel="Bold font:";
			fSavedFont = be_bold_font;
			fCurrentFont = be_bold_font;
			
			fDefaultFont.SetFamilyAndStyle("Swis721 BT", "Bold");
			fDefaultFont.SetSize(12.0);
			
			break;
		}	
		case FIXED_FONT_SELECTION_VIEW:
		{
			typelabel="Fixed font:";
			fSavedFont = be_fixed_font;
			fCurrentFont = be_fixed_font;
			
			fDefaultFont = new BFont();
			fDefaultFont.SetFamilyAndStyle("Courier10 BT", "Roman");
			fDefaultFont.SetSize(12.0);
			
			break;
		}
		default:
		{
			typelabel="Plain font:";
			fSavedFont = be_plain_font;
			fCurrentFont = be_plain_font;
			
			fDefaultFont = new BFont();
			fDefaultFont.SetFamilyAndStyle("Swis721 BT", "Roman");
			fDefaultFont.SetSize(10.0);
			
			break;
		}	
	}

	float fontheight = FontHeight(false);
	float divider = StringWidth("Fixed font:");

	fSizeMenu = new BPopUpMenu("fSizeMenu", true, true, B_ITEMS_IN_COLUMN);
	fFontMenu = new BPopUpMenu("fFontMenu", true, true, B_ITEMS_IN_COLUMN);
	
	// size box
	BRect r( Bounds() );
	
	float x = StringWidth("999") + 16;
	r.left = r.right -( x + StringWidth("Size: ") + 8.0 );
	r.bottom = fontheight +5;
	
	BMenuField *fSizeMenuField = new BMenuField(r, "fontField", "Size: ", fSizeMenu, true);
	fSizeMenuField->SetDivider( StringWidth("Size: ") + 5.0 );
	fSizeMenuField->SetAlignment(B_ALIGN_RIGHT);
	AddChild(fSizeMenuField);
	
	// font menu
	r.right = r.left;
	r.left = 1;
	r.bottom = fontheight *1.5;
	BMenuField *fFontMenuField = new BMenuField(r, "fontField", typelabel.String(), fFontMenu, false);
	fFontMenuField->SetDivider(divider + 6.0);
	fFontMenuField->SetAlignment(B_ALIGN_RIGHT);
	AddChild(fFontMenuField);
	
	r = Bounds();
	r.left = divider +8.0;
	r.top = fontheight *1.5 +4;
	r.InsetBy(1, 1);
	BBox *fPreviewTextBox = new BBox(r, "TestTextBox", B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER);
	AddChild(fPreviewTextBox);

	// Place the text slightly inside the entire box area, so it doesn't overlap the box outline.
	r = fPreviewTextBox->Bounds().InsetByCopy(2, 2);
	r.right -= 2;
	BRect fPreviewTextRect(r);
	fPreviewText = new BStringView(fPreviewTextRect, "fPreviewText", "The quick brown fox jumps over the lazy dog.", 
		B_FOLLOW_ALL, B_WILL_DRAW);
	fPreviewText->SetFont(&fCurrentFont);
	fPreviewTextBox->AddChild(fPreviewText);
}

FontSelectionView::~FontSelectionView(void)
{
	font_family family;
	font_style style;
	fCurrentFont.GetFamilyAndStyle(&family,&style);
	
	switch(fType)
	{
		case PLAIN_FONT_SELECTION_VIEW:
		{
			_set_system_font_("plain", family, style, fCurrentFont.Size());
			break;
		}
		case BOLD_FONT_SELECTION_VIEW:
		{
			_set_system_font_("bold", family, style, fCurrentFont.Size());
			break;
		}
		case FIXED_FONT_SELECTION_VIEW:
		{
			_set_system_font_("fixed", family, style, fCurrentFont.Size());
			break;
		}
		default:
		{
			break;
		}
	}
}

void
FontSelectionView::AttachedToWindow(void)
{
	BuildMenus();		
}

void
FontSelectionView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case SIZE_CHANGED_MSG: 
		{
			int32 size;
			if(msg->FindInt32("size",&size)!=B_OK)
				break;
			
			BString str(B_EMPTY_STRING);
			str << size;
			
			BMenuItem *item=fSizeMenu->FindItem(str.String());
			if(item)
			{
				item->SetMarked(true);
				
				fCurrentFont.SetSize(size);
				fPreviewText->SetFont(&fCurrentFont, B_FONT_ALL);
				fPreviewText->Invalidate();
			}
			
			NotifyFontChange();
			Window()->PostMessage(M_ENABLE_REVERT);
			break;
		}
		case FONT_CHANGED_MSG: 
		{
			BString str;
			if(msg->FindString("family",&str)!=B_OK)
				break;
				
			font_family family;
			sprintf(family,"%s",str.String());
			BMenuItem *menu=fFontMenu->FindItem(family);
			if(menu)
			{
				menu->SetMarked(true);
				fCurrentFont.SetFamilyAndFace(family,B_REGULAR_FACE);
				
				font_style style;
				fCurrentFont.GetFamilyAndStyle(&family,&style);
				
				BMenuItem *item=menu->Submenu()->FindItem(style);
				if(item)
				{
					fCurrentStyle->SetMarked(false);
					item->SetMarked(true);
					fCurrentStyle=item;
					
					fPreviewText->SetFont(&fCurrentFont, B_FONT_ALL);
					fPreviewText->Invalidate();
				}
			}
			
			NotifyFontChange();
			Window()->PostMessage(M_ENABLE_REVERT);
			break;
		}
		case STYLE_CHANGED_MSG: 
		{
			BString str;
			if(msg->FindString("family",&str)!=B_OK)
				break;
				
			font_family family;
			font_style style;
			sprintf(family,"%s",str.String());
			
			if(msg->FindString("style",&str)!=B_OK)
				break;
			sprintf(style,"%s",str.String());
			
			BMenuItem *menu=fFontMenu->FindItem(family);
			if(!menu)
				break;
			
			BMenuItem *item=menu->Submenu()->FindItem(style);
			if(item)
			{
				fCurrentStyle->SetMarked(false);
				menu->SetMarked(true);
				item->SetMarked(true);
				fCurrentStyle=item;
				
				fCurrentFont.SetFamilyAndStyle(family,style);
				fPreviewText->SetFont(&fCurrentFont, B_FONT_ALL);
				fPreviewText->Invalidate();
			}
				
			NotifyFontChange();
			Window()->PostMessage(M_ENABLE_REVERT);
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}

void
FontSelectionView::EmptyMenus(void)
{
	// Empty the font list
	for(int32 i = 0; i < fFontMenu->CountItems(); i++)
	{
		BMenu *menu = fFontMenu->SubmenuAt(0L);
		
		// We should never have a regular menu item in the font list
		if(!menu)
			continue;
		
		for(int32 j=0; j < menu->CountItems(); j++)
		{
			BMenuItem *item = menu->RemoveItem(0L);
			delete item;
		}
		
		fFontMenu->RemoveItem(menu);
		delete menu;
	}
	
	// empty the size list
	for(int32 i = 0; i < fSizeMenu->CountItems(); i++)
	{
		BMenuItem *item = fSizeMenu->RemoveItem(0L);
		delete item;
	}
}

void
FontSelectionView::BuildMenus(void)
{
	int32 numFamilies;
	int counter;
	BMessage *msg;
	
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
			
			fCurrentFont.GetFamilyAndStyle(&workingFamily, &workingStyle);
			
			msg = new BMessage(STYLE_CHANGED_MSG);
			msg->AddString("family",(char*)family);
			msg->AddString("style",(char*)style);
			tmpItem = new BMenuItem(style, msg);
			
			if((strcmp(style, workingStyle) == 0) && (strcmp(family, workingFamily) == 0))
			{
				markFamily = true;
				tmpItem->SetMarked(true);
				fCurrentStyle=tmpItem;
			}
			tmpStyleMenu->AddItem(tmpItem);
		}
		
		msg = new BMessage(FONT_CHANGED_MSG);
		msg->AddString("family",family);
		fFontMenu->AddItem(new BMenuItem((tmpStyleMenu), msg));
		tmpStyleMenu->SetTargetForItems(this);
		
		if(markFamily)
			tmpStyleMenu->Superitem()->SetMarked(true);
	}
	fFontMenu->SetTargetForItems(this);
	
	// build size menu
	for(counter = MIN_SIZE_INDEX; counter < (MAX_SIZE_INDEX + 1); counter++)
	{
		char buf[1];
		BMenuItem *tmp;
		
		sprintf(buf, "%d", counter);
		msg = new BMessage(SIZE_CHANGED_MSG);
		msg->AddInt32("size",counter);
		fSizeMenu->AddItem(tmp = new BMenuItem(buf, msg));
		if(counter == (int) fCurrentFont.Size())
			tmp->SetMarked(true);
	}
	fSizeMenu->SetTargetForItems(this);
}

// This method is called by outsiders only. As a result,
// the owning window is NOT notified
void
FontSelectionView::SetDefaults(void)
{
	font_family family;
	font_style style;
	
	fDefaultFont.GetFamilyAndStyle(&family, &style);
	
	BMenuItem *menu=fFontMenu->FindItem(family);
	if(!menu)
		return;
			
	BMenuItem *item=menu->Submenu()->FindItem(style);
	if(!item)
		return;
	
	fCurrentStyle->SetMarked(false);
	menu->SetMarked(true);
	item->SetMarked(true);
	fCurrentStyle=item;
	
	char string[5];
	sprintf(string,"%d",(int)fDefaultFont.Size());
	item = fSizeMenu->FindItem(string);
	if(item)
		item->SetMarked(true);
	
	fCurrentFont=fDefaultFont;
	fPreviewText->SetFont(&fCurrentFont, B_FONT_ALL);
	fPreviewText->Invalidate();
}

// This method is called by outsiders only. As a result,
// the owning window is NOT notified
void
FontSelectionView::Revert(void)
{
	font_family family;
	font_style style;
	
	fSavedFont.GetFamilyAndStyle(&family, &style);
	
	BMenuItem *menu=fFontMenu->FindItem(family);
	if(!menu)
		return;
			
	BMenuItem *item=menu->Submenu()->FindItem(style);
	if(!item)
		return;
	
	fCurrentStyle->SetMarked(false);
	menu->SetMarked(true);
	item->SetMarked(true);
	fCurrentStyle=item;
		
	char string[5];
	sprintf(string,"%d",(int)fSavedFont.Size());
	item = fSizeMenu->FindItem(string);
	if(item)
		item->SetMarked(true);
	
	fCurrentFont=fSavedFont;
	fPreviewText->SetFont(&fCurrentFont, B_FONT_ALL);
	fPreviewText->Invalidate();
}

void
FontSelectionView::NotifyFontChange(void)
{
	BMessage msg;
	
	switch(fType)
	{
		case BOLD_FONT_SELECTION_VIEW:
		{
			msg.what=M_SET_BOLD;
			break;
		}
		case FIXED_FONT_SELECTION_VIEW:
		{
			msg.what=M_SET_FIXED;
			break;
		}
		default:
		{
			msg.what=M_SET_PLAIN;
			break;
		}
	}
	
	font_family family;
	font_style style;
	fCurrentFont.GetFamilyAndStyle(&family,&style);
	
	msg.AddInt32("size",fCurrentFont.Size());
	msg.AddString("family",family);
	msg.AddString("style",style);
	Window()->PostMessage(&msg);
}

void
FontSelectionView::RescanFonts(void)
{
	EmptyMenus();
	BuildMenus();
}

