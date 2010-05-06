/*
 * Copyright 2002-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 *		Rene Gollent (rene@gollent.com)
 */

#include "APRView.h"

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <Messenger.h>
#include <Path.h>
#include <SpaceLayoutItem.h>

#include <stdio.h>

#include "APRWindow.h"
#include "defs.h"
#include "ColorWell.h"
#include "ColorWhichItem.h"
#include "ColorSet.h"


#undef TR_CONTEXT
#define TR_CONTEXT "Colors tab"

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

APRView::APRView(const char *name, uint32 flags)
 :	BView(name, flags),
 	fDefaultSet(ColorSet::DefaultColorSet()),
 	fDecorMenu(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

#if 0
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

		BMenuField *field = new BMenuField("Window Style", fDecorMenu);
		// TODO: use this menu field.
	}
	BMenuItem *marked = fDecorMenu->ItemAt(BPrivate::get_decorator());
	if (marked)
		marked->SetMarked(true);
	else {
		marked = fDecorMenu->FindItem("Default");
		if (marked)
			marked->SetMarked(true);
	}
#endif

	// Set up list of color attributes
	fAttrList = new BListView("AttributeList", B_SINGLE_SELECTION_LIST);

	fScrollView = new BScrollView("ScrollView", fAttrList, 0, false, true);
	fScrollView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	for (int32 i = 0; i < color_description_count(); i++) {
		const ColorDescription& description = *get_color_description(i);
		const char* text = B_TRANSLATE(description.text);
		color_which which = description.which;
		fAttrList->AddItem(new ColorWhichItem(text, which));
	}

	BRect wellrect(0, 0, 50, 50);
	fColorWell = new ColorWell(wellrect, new BMessage(COLOR_DROPPED), 0);
	fColorWell->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_BOTTOM));

	fPicker = new BColorControl(B_ORIGIN, B_CELLS_32x8, 8.0,
		"picker", new BMessage(UPDATE_COLOR));

	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(fScrollView)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(5))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(fColorWell)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5))
			.Add(fPicker)
		)
		.SetInsets(10, 10, 10, 10)
	);

	fColorWell->Parent()->SetExplicitMaxSize(
		BSize(B_SIZE_UNSET, fPicker->Bounds().Height()));
	fAttrList->SetSelectionMessage(new BMessage(ATTRIBUTE_CHOSEN));
}


APRView::~APRView()
{
}


void
APRView::AttachedToWindow()
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

		if (msg->FindData("RGBColor", (type_code)'RGBC', (const void**)&color,
				&size) == B_OK) {
			SetCurrentColor(*color);
		}
	}

	switch (msg->what) {
		case DECORATOR_CHANGED:
		{
			int32 index = fDecorMenu->IndexOf(fDecorMenu->FindMarked());
			#ifdef HAIKU_TARGET_PLATFORM_HAIKU
			if (index >= 0)
				BPrivate::set_decorator(index);
			#endif
			break;
		}
		case UPDATE_COLOR:
		{
			// Received from the color fPicker when its color changes
			rgb_color color = fPicker->ValueAsColor();
			SetCurrentColor(color);

			Window()->PostMessage(kMsgUpdate);
			break;
		}
		case ATTRIBUTE_CHOSEN:
		{
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
		case REVERT_SETTINGS:
		{
			fCurrentSet = fPrevSet;

			UpdateControls();
			UpdateAllColors();

			Window()->PostMessage(kMsgUpdate);
			break;
		}
		case DEFAULT_SETTINGS:
		{
			fCurrentSet = ColorSet::DefaultColorSet();

			UpdateControls();
			UpdateAllColors();

			if (fDecorMenu) {
				BMenuItem *item = fDecorMenu->FindItem("Default");
				if (item) {
					item->SetMarked(true);
					#ifdef HAIKU_TARGET_PLATFORM_HAIKU
					BPrivate::set_decorator(fDecorMenu->IndexOf(item));
					#endif
				}
			}
			Window()->PostMessage(kMsgUpdate);
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
APRView::LoadSettings()
{
	for (int32 i = 0; i < color_description_count(); i++) {
		color_which which = get_color_description(i)->which;
		fCurrentSet.SetColor(which, ui_color(which));
	}

	fPrevSet = fCurrentSet;
}


bool
APRView::IsDefaultable()
{
	return fCurrentSet != fDefaultSet;
}


void
APRView::UpdateAllColors()
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
