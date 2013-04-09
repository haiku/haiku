/*
 * Copyright 2002-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		Rene Gollent, rene@gollent.com
 *		John Scipione, jscipione@gmail.com
 */


#include "APRView.h"

#include <stdio.h>

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

#include "APRWindow.h"
#include "defs.h"
#include "ColorPreview.h"
#include "ColorSet.h"
#include "ColorWhichItem.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Colors tab"

#define COLOR_DROPPED 'cldp'
#define DECORATOR_CHANGED 'dcch'


APRView::APRView(const char* name)
	:
	BView(name, B_WILL_DRAW),
	fDefaultSet(ColorSet::DefaultColorSet())
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

#if 0
	fDecorMenu = new BMenu("Window Style");
	int32 decorCount = fDecorUtil->CountDecorators();
	DecorInfo* decor = NULL;
	if (decorCount > 1) {
		for (int32 i = 0; i < decorCount; i++) {
			decor = fDecorUtil->GetDecorator(i);
			if (!decor)
				continue;
			fDecorMenu->AddItem(new BMenuItem(decor->Name().String(),
				new BMessage(DECORATOR_CHANGED)));
		}

		BMenuField* field = new BMenuField("Window Style", fDecorMenu);
		// TODO: use this menu field.
	}
	BMenuItem* marked = fDecorMenu->ItemAt(fDecorUtil->IndexOfCurrentDecorator());
	if (marked)
		marked->SetMarked(true);
	else {
		marked = fDecorMenu->FindItem("Default");
		if (marked)
			marked->SetMarked(true);
	}
#endif

	LoadSettings();

	// Set up list of color attributes
	fAttrList = new BListView("AttributeList", B_SINGLE_SELECTION_LIST);

	fScrollView = new BScrollView("ScrollView", fAttrList, 0, false, true);
	fScrollView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	int32 count = color_description_count();
	for (int32 i = 0; i < count; i++) {
		const ColorDescription& description = *get_color_description(i);
		const char* text = B_TRANSLATE_NOCOLLECT(description.text);
		color_which which = description.which;
		fAttrList->AddItem(new ColorWhichItem(text, which,
			fCurrentSet.GetColor(which)));
	}

	BRect wellrect(0, 0, 50, 50);
	fColorPreview = new ColorPreview(wellrect, new BMessage(COLOR_DROPPED), 0);
	fColorPreview->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_BOTTOM));

	fPicker = new BColorControl(B_ORIGIN, B_CELLS_32x8, 8.0,
		"picker", new BMessage(UPDATE_COLOR));

	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(fScrollView)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(5))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(fColorPreview)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5))
			.Add(fPicker)
		)
		.SetInsets(10, 10, 10, 10)
	);

	fColorPreview->Parent()->SetExplicitMaxSize(
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
	fColorPreview->SetTarget(this);

	fAttrList->Select(0);
}


void
APRView::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		rgb_color* color = NULL;
		ssize_t size = 0;

		if (msg->FindData("RGBColor", (type_code)'RGBC', (const void**)&color,
				&size) == B_OK) {
			_SetCurrentColor(*color);
		}
	}

	switch (msg->what) {
		case UPDATE_COLOR:
		{
			// Received from the color fPicker when its color changes
			rgb_color color = fPicker->ValueAsColor();
			_SetCurrentColor(color);

			Window()->PostMessage(kMsgUpdate);
			break;
		}

		case ATTRIBUTE_CHOSEN:
		{
			// Received when the user chooses a GUI fAttribute from the list

			ColorWhichItem* item = (ColorWhichItem*)
				fAttrList->ItemAt(fAttrList->CurrentSelection());
			if (item == NULL)
				break;

			fWhich = item->ColorWhich();
			rgb_color color = fCurrentSet.GetColor(fWhich);
			_SetCurrentColor(color);

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
	int32 count = color_description_count();
	for (int32 i = 0; i < count; i++) {
		color_which which = get_color_description(i)->which;
		fCurrentSet.SetColor(which, ui_color(which));
	}

	fPrevSet = fCurrentSet;
}


void
APRView::SetDefaults()
{
	fCurrentSet = ColorSet::DefaultColorSet();

	_UpdateAllColors();

	rgb_color color = fCurrentSet.GetColor(fWhich);
	fPicker->SetValue(color);
	fColorPreview->SetColor(color);
	fColorPreview->Invalidate();

	Window()->PostMessage(kMsgUpdate);
}


void
APRView::Revert()
{
	fCurrentSet = fPrevSet;

	_UpdateAllColors();

	rgb_color color = fCurrentSet.GetColor(fWhich);
	fPicker->SetValue(color);
	fColorPreview->SetColor(color);
	fColorPreview->Invalidate();

	Window()->PostMessage(kMsgUpdate);
}


bool
APRView::IsDefaultable()
{
	for (int32 i = color_description_count() - 1; i >= 0; i--) {
		color_which which = get_color_description(i)->which;
		if (fCurrentSet.GetColor(which) != fDefaultSet.GetColor(which))
			return true;
	}

	return false;
}


bool
APRView::IsRevertable()
{
	for (int32 i = color_description_count() - 1; i >= 0; i--) {
		color_which which = get_color_description(i)->which;
		if (fCurrentSet.GetColor(which) != fPrevSet.GetColor(which))
			return true;
	}

	return false;
}


void
APRView::_SetCurrentColor(rgb_color color)
{
	fCurrentSet.SetColor(fWhich, color);
	set_ui_color(fWhich, color);

	int32 currentIndex = fAttrList->CurrentSelection();
	ColorWhichItem* item = (ColorWhichItem*)fAttrList->ItemAt(currentIndex);
	if (item != NULL) {
		item->SetColor(color);
		fAttrList->InvalidateItem(currentIndex);
	}

	fPicker->SetValue(color);
	fColorPreview->SetColor(color);
	fColorPreview->Invalidate();
}


void
APRView::_UpdateAllColors()
{
	for (int32 i = color_description_count() - 1; i >= 0; i--) {
		color_which which = get_color_description(i)->which;
		rgb_color color = fCurrentSet.GetColor(which);
		set_ui_color(which, color);
		static_cast<ColorWhichItem*>(fAttrList->ItemAt(i))->SetColor(color);
		fAttrList->InvalidateItem(i);
	}
}
