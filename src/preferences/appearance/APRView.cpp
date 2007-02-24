/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 */
#include <OS.h>
#include <Directory.h>
#include <Alert.h>
#include <Messenger.h>
#include <storage/Path.h>
#include <Entry.h>
#include <File.h>
#include <stdio.h>

#include <InterfaceDefs.h>

#include "APRView.h"
#include "defs.h"
#include "ColorWell.h"
#include "ColorWhichItem.h"
#include "ColorSet.h"

//#define DEBUG_APRVIEW

#ifdef DEBUG_APRVIEW
	#define STRACE(a) printf a
#else
	#define STRACE(A) /* nothing */
#endif

#define COLOR_DROPPED 'cldp'
#define DECORATOR_CHANGED 'dcch'

namespace BPrivate
{
	int32 count_decorators(void);
	status_t set_decorator(const int32 &index);
	int32 get_decorator(void);
	status_t get_decorator_name(const int32 &index, BString &name);
	status_t get_decorator_preview(const int32 &index, BBitmap *bitmap);
}

APRView::APRView(const BRect &frame, const char *name, int32 resize, int32 flags)
 :	BView(frame,name,resize,flags),
 	fDecorMenu(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect rect(Bounds().InsetByCopy(10,10));
	
	#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	fDecorMenu = new BMenu("Window Style");
	int32 decorCount = BPrivate::count_decorators();
	if (decorCount > 1) {
		for (int32 i = 0; i < decorCount; i++) {
			BString name;
			BPrivate::get_decorator_name(i, name);
			fDecorMenu->AddItem(new BMenuItem(name.String(),
								new BMessage(DECORATOR_CHANGED)));
		}
		
		BMenuField *field = new BMenuField(rect, "menufield", "Window Style",
										fDecorMenu, B_FOLLOW_RIGHT | 
										B_FOLLOW_TOP);
		AddChild(field);
		field->SetDivider(be_plain_font->StringWidth("Window style: ") + 5);
		field->ResizeToPreferred();
		field->MoveTo(Bounds().right - field->Bounds().Width(), 10);
		rect = Bounds().InsetByCopy(10,10);
		rect.OffsetTo(10, field->Frame().bottom + 10);
	}
	fDecorMenu->ItemAt(BPrivate::get_decorator())->SetMarked(true);
	#endif
	
	// Set up list of color fAttributes
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom = rect.top + 75;
	fAttrList = new BListView(rect,"AttributeList", B_SINGLE_SELECTION_LIST,
							B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	
	fScrollView = new BScrollView("ScrollView",fAttrList, B_FOLLOW_LEFT_RIGHT | 
									B_FOLLOW_TOP, 0, false, true);
	AddChild(fScrollView);
	fScrollView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fAttrList->SetSelectionMessage(new BMessage(ATTRIBUTE_CHOSEN));

	fAttrList->AddItem(new ColorWhichItem(B_PANEL_BACKGROUND_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_PANEL_TEXT_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_DOCUMENT_BACKGROUND_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_DOCUMENT_TEXT_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_CONTROL_BACKGROUND_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_CONTROL_TEXT_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_CONTROL_BORDER_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_CONTROL_HIGHLIGHT_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_NAVIGATION_BASE_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_NAVIGATION_PULSE_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_SHINE_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_SHADOW_COLOR));
	fAttrList->AddItem(new ColorWhichItem(B_MENU_BACKGROUND_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_MENU_SELECTED_BACKGROUND_COLOR));
	fAttrList->AddItem(new ColorWhichItem(B_MENU_ITEM_TEXT_COLOR));
	fAttrList->AddItem(new ColorWhichItem(B_MENU_SELECTED_ITEM_TEXT_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_MENU_SELECTED_BORDER_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_TOOLTIP_BACKGROUND_COLOR));
	
	fAttrList->AddItem(new ColorWhichItem((color_which)B_SUCCESS_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_FAILURE_COLOR));
	fAttrList->AddItem(new ColorWhichItem(B_WINDOW_TAB_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_WINDOW_TAB_TEXT_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_INACTIVE_WINDOW_TAB_COLOR));
	fAttrList->AddItem(new ColorWhichItem((color_which)B_INACTIVE_WINDOW_TAB_TEXT_COLOR));


	BRect wellrect(0,0,50,50);
	wellrect.OffsetTo(rect.right + 30, rect.top +
					(fScrollView->Bounds().Height() - wellrect.Height())/2 );

	fColorWell = new ColorWell(wellrect,new BMessage(COLOR_DROPPED),true);
	AddChild(fColorWell);
	
	// Center the list and color well
	
	rect = fScrollView->Frame();
	rect.right = wellrect.right;
	rect.OffsetTo((Bounds().Width()-rect.Width())/2,rect.top);
	
	fPicker = new BColorControl(BPoint(10,fScrollView->Frame().bottom+20),B_CELLS_32x8,5.0,"fPicker",
								new BMessage(UPDATE_COLOR));
	AddChild(fPicker);
	
	fDefaults = new BButton(BRect(0,0,1,1),"DefaultsButton","Defaults",
							new BMessage(DEFAULT_SETTINGS),
							B_FOLLOW_LEFT |B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	fDefaults->ResizeToPreferred(); 
	fDefaults->MoveTo((fPicker->Frame().right-(fDefaults->Frame().Width()*2)-20)/2,fPicker->Frame().bottom+20);
	AddChild(fDefaults);
	
	
	BRect cvrect(fDefaults->Frame());
	cvrect.OffsetBy(cvrect.Width() + 20,0);

	fRevert = new BButton(cvrect,"RevertButton","Revert", 
						new BMessage(REVERT_SETTINGS),
						B_FOLLOW_LEFT |B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	AddChild(fRevert);
	fRevert->SetEnabled(false);
	
}

APRView::~APRView(void)
{
	ColorSet::SaveColorSet("/boot/home/config/settings/app_server/system_colors",fCurrentSet);
}

void
APRView::AttachedToWindow(void)
{
	fPicker->SetTarget(this);
	fAttrList->SetTarget(this);
	fDefaults->SetTarget(this);
	fRevert->SetTarget(this);
	fColorWell->SetTarget(this);

	fPicker->SetValue(fCurrentSet.StringToColor(fAttrString.String()));
	
	fDecorMenu->SetTargetForItems(BMessenger(this));
	
	Window()->ResizeTo(MAX(fPicker->Frame().right,fPicker->Frame().right) + 10,
					   fDefaults->Frame().bottom + 10);
	LoadSettings();
	fAttrList->Select(0);
}

void
APRView::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		rgb_color *col;
		ssize_t size;
		
		if (msg->FindData("RGBColor",(type_code)'RGBC',(const void**)&col,&size)==B_OK) {
			fPicker->SetValue(*col);
			fColorWell->SetColor(*col);
			fColorWell->Invalidate();
		}
	}

	switch(msg->what) {
		case DECORATOR_CHANGED: {
			int32 index = fDecorMenu->IndexOf(fDecorMenu->FindMarked());
			#ifdef HAIKU_TARGET_PLATFORM_HAIKU
			if (index >= 0)
				BPrivate::set_decorator(index);
			#endif
			break;
		}
		case UPDATE_COLOR: {
			// Received from the color fPicker when its color changes
			
			rgb_color col=fPicker->ValueAsColor();

			fColorWell->SetColor(col);
			fColorWell->Invalidate();
			
			// Update current fAttribute in the settings
			fCurrentSet.SetColor(fAttrString.String(),col);
			
			fRevert->SetEnabled(true);
			
			break;
		}
		case ATTRIBUTE_CHOSEN: {
			// Received when the user chooses a GUI fAttribute from the list
			
			ColorWhichItem *whichitem = (ColorWhichItem*)
										fAttrList->ItemAt(fAttrList->CurrentSelection());
			
			if (!whichitem)
				break;
			
			fAttrString=whichitem->Text();
			UpdateControlsFromAttr(whichitem->Text());
			break;
		}
		case REVERT_SETTINGS: {
			fCurrentSet=fPrevSet;
			UpdateControlsFromAttr(fAttrString.String());
			
			fRevert->SetEnabled(false);
			
			break;
		}
		case DEFAULT_SETTINGS: {
			fCurrentSet.SetToDefaults();
			
			UpdateControlsFromAttr(fAttrString.String());
			BMenuItem *item = fDecorMenu->FindItem("Default");
			if (item)
			{
				item->SetMarked(true);
				#ifdef HAIKU_TARGET_PLATFORM_HAIKU
				BPrivate::set_decorator(fDecorMenu->IndexOf(item));
				#endif
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void APRView::LoadSettings(void)
{
	// Load the current GUI color settings from disk. This is done instead of
	// getting them from the server at this point for testing purposes.
	
	// Query the server for the current settings
	//BPrivate::get_system_colors(&fCurrentSet);
	
	// TODO: remove this and enable the get_system_colors() call
	if (ColorSet::LoadColorSet("/boot/home/config/settings/app_server/system_colors",&fCurrentSet) != B_OK) {
		fCurrentSet.SetToDefaults();
		ColorSet::SaveColorSet("/boot/home/config/settings/app_server/system_colors",fCurrentSet);
	}
	
	fPrevSet = fCurrentSet;
}

void APRView::UpdateControlsFromAttr(const char *string)
{
	if (!string)
		return;
	STRACE(("Update color for %s\n",string));

	fPicker->SetValue(fCurrentSet.StringToColor(string));
	fColorWell->SetColor(fPicker->ValueAsColor());
	fColorWell->Invalidate();
}
