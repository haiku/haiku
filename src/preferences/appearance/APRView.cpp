/*
 * Copyright 2002-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 *		Rene Gollent (rene@gollent.com)
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
#include "APRWindow.h"
#include "defs.h"
#include "ColorWell.h"
#include "ColorWhichItem.h"
#include "ColorSet.h"


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
 	fDefaultSet(ColorSet::DefaultColorSet()),
 	fDecorMenu(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect rect(Bounds().InsetByCopy(kBorderSpace,kBorderSpace));
	
	#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	
	fDecorMenu = new BMenu("Window Style");
	int32 decorCount = BPrivate::count_decorators();
	if (decorCount > 1) {
		for (int32 i = 0; i < decorCount; i++) {
			BString name;
			BPrivate::get_decorator_name(i, name);
			if (name.CountChars() < 1)
				continue;
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
	BMenuItem *marked = fDecorMenu->ItemAt(BPrivate::get_decorator());
	if (marked)
		marked->SetMarked(true);
	else
	{
		marked = fDecorMenu->FindItem("Default");
		if (marked)
			marked->SetMarked(true);
	}
	
	#endif
	
	// Set up list of color fAttributes
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom = rect.top + 75;
	fAttrList = new BListView(rect,"AttributeList", B_SINGLE_SELECTION_LIST,
							B_FOLLOW_ALL_SIDES);
	
	fScrollView = new BScrollView("ScrollView",fAttrList, B_FOLLOW_ALL_SIDES, 
		0, false, true);
	AddChild(fScrollView);
	fScrollView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fAttrList->SetSelectionMessage(new BMessage(ATTRIBUTE_CHOSEN));

	for (int32 i = 0; i < color_description_count(); i++) {
		const ColorDescription& description = *get_color_description(i); 
		const char* text = description.text;
		color_which which = description.which;
		fAttrList->AddItem(new ColorWhichItem(text, which));
	}
	
	rect = fScrollView->Frame();
	BRect wellrect(0, 0, 50, 50);
	wellrect.OffsetBy(rect.left, rect.bottom + kBorderSpace);
	fColorWell = new ColorWell(wellrect, new BMessage(COLOR_DROPPED), 
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fColorWell);
	
	fPicker = new BColorControl(BPoint(wellrect.right + kBorderSpace, wellrect.top),
			B_CELLS_32x8, 5.0, "fPicker", new BMessage(UPDATE_COLOR));
	fPicker->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fPicker);	
	
	// bottom align ColorWell and ColorPicker
	float bottom = Bounds().bottom - kBorderSpace;
	float colorWellBottom = fColorWell->Frame().bottom;
	float pickerBottom = fPicker->Frame().bottom;
	float delta = bottom - max_c(colorWellBottom, pickerBottom);
	fColorWell->MoveBy(0, delta);
	fPicker->MoveBy(0, delta);
	fScrollView->ResizeBy(0, delta);
	// since this view is not attached to a window yet,
	// we have to resize the fScrollView children too
	fScrollView->ScrollBar(B_VERTICAL)->ResizeBy(0, delta);
	fAttrList->ResizeBy(0, delta);
}

APRView::~APRView(void)
{
}

void
APRView::AttachedToWindow(void)
{
	fPicker->SetTarget(this);
	fAttrList->SetTarget(this);
	fColorWell->SetTarget(this);

	if (fDecorMenu)
		fDecorMenu->SetTargetForItems(BMessenger(this));

	LoadSettings();
	fAttrList->Select(0);
}

void
APRView::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		rgb_color *color;
		ssize_t size;
		
		if (msg->FindData("RGBColor", (type_code)'RGBC', (const void**)&color, &size) == B_OK) {
			SetCurrentColor(*color);
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
			rgb_color color = fPicker->ValueAsColor();
			SetCurrentColor(color);	
						
			Window()->PostMessage(kMsgUpdate);
			break;
		}
		case ATTRIBUTE_CHOSEN: {
			// Received when the user chooses a GUI fAttribute from the list
			
			ColorWhichItem *item = (ColorWhichItem*)
										fAttrList->ItemAt(fAttrList->CurrentSelection());
			
			if (item == NULL)
				break;
			
			fWhich = item->ColorWhich();
			rgb_color color = fCurrentSet.GetColor(fWhich);
			SetCurrentColor(color);
			
			Window()->PostMessage(kMsgUpdate);
			break;
		}
		case REVERT_SETTINGS: {
			fCurrentSet = fPrevSet;
			
			UpdateControls();
			UpdateAllColors();
			
			Window()->PostMessage(kMsgUpdate);
			break;
		}
		case DEFAULT_SETTINGS: {
			fCurrentSet = ColorSet::DefaultColorSet();
			
			UpdateControls();
			UpdateAllColors();

			BMenuItem *item = fDecorMenu->FindItem("Default");
			if (item)
			{
				item->SetMarked(true);
				#ifdef HAIKU_TARGET_PLATFORM_HAIKU
				BPrivate::set_decorator(fDecorMenu->IndexOf(item));
				#endif
			}
			Window()->PostMessage(kMsgUpdate);
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void APRView::LoadSettings(void)
{
	for (int32 i = 0; i < color_description_count(); i++) {
		color_which which = get_color_description(i)->which; 
		fCurrentSet.SetColor(which, ui_color(which));
	}

	fPrevSet = fCurrentSet;
}

bool APRView::IsDefaultable(void)
{
	return fCurrentSet != fDefaultSet;
}

void APRView::UpdateAllColors(void)
{
	for (int32 i = 0; i < color_description_count(); i++) {
		color_which which = get_color_description(i)->which; 
		rgb_color color = fCurrentSet.GetColor(which);
		set_ui_color(which, color);
	}
}


void 
APRView::SetCurrentColor(rgb_color color)
{
	fCurrentSet.SetColor(fWhich, color);
	set_ui_color(fWhich, color);
	UpdateControls();
}


void 
APRView::UpdateControls()
{
	rgb_color color = fCurrentSet.GetColor(fWhich);
	fPicker->SetValue(color);
	fColorWell->SetColor(color);
	fColorWell->Invalidate();
}
