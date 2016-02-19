/*
 * Copyright 2002-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		Rene Gollent, rene@gollent.com
 *		John Scipione, jscipione@gmail.com
 *		Joseph Groover <looncraz@looncraz.net>
 */


#include "APRView.h"

#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Messenger.h>
#include <Path.h>
#include <SpaceLayoutItem.h>

#include "APRWindow.h"
#include "defs.h"
#include "ColorPreview.h"
#include "Colors.h"
#include "ColorWhichItem.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Colors tab"

#define COLOR_DROPPED 'cldp'
#define DECORATOR_CHANGED 'dcch'


APRView::APRView(const char* name)
	:
	BView(name, B_WILL_DRAW)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

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
	fScrollView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	int32 count = color_description_count();
	for (int32 i = 0; i < count; i++) {
		const ColorDescription& description = *get_color_description(i);
		const char* text = B_TRANSLATE_NOCOLLECT(description.text);
		color_which which = description.which;
		fAttrList->AddItem(new ColorWhichItem(text, which, ui_color(which)));
	}

	BRect wellrect(0, 0, 50, 50);
	fColorPreview = new ColorPreview(wellrect, new BMessage(COLOR_DROPPED), 0);
	fColorPreview->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_BOTTOM));

	fPicker = new BColorControl(B_ORIGIN, B_CELLS_32x8, 8.0,
		"picker", new BMessage(UPDATE_COLOR));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fScrollView, 10.0)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fColorPreview)
			.Add(fPicker)
			.End()
		.SetInsets(B_USE_WINDOW_SPACING);

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
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
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

			Window()->PostMessage(kMsgUpdate);
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
			rgb_color color = ui_color(fWhich);
			_SetCurrentColor(color);
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
	get_default_colors(&fDefaultColors);
	get_current_colors(&fCurrentColors);
	fPrevColors = fCurrentColors;
}


void
APRView::SetDefaults()
{
	_SetUIColors(fDefaultColors);
	_UpdatePreviews(fDefaultColors);

	// Use a default color that stands out to show errors clearly
	rgb_color color = fDefaultColors.GetColor(ui_color_name(fWhich),
		make_color(255, 0, 255));

	fPicker->SetValue(color);
	fColorPreview->SetColor(color);
	fColorPreview->Invalidate();

	Window()->PostMessage(kMsgUpdate);
}


void
APRView::Revert()
{
	_SetUIColors(fPrevColors);
	_UpdatePreviews(fPrevColors);

	rgb_color color = fPrevColors.GetColor(ui_color_name(fWhich),
		make_color(255, 0, 255));
	fPicker->SetValue(color);
	fColorPreview->SetColor(color);
	fColorPreview->Invalidate();

	Window()->PostMessage(kMsgUpdate);
}


bool
APRView::IsDefaultable()
{
	return !fDefaultColors.HasSameData(fCurrentColors);
}


bool
APRView::IsRevertable()
{
	return !fPrevColors.HasSameData(fCurrentColors);
}


void
APRView::_SetCurrentColor(rgb_color color)
{
	set_ui_color(fWhich, color);
	fCurrentColors.SetColor(ui_color_name(fWhich), color);

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
APRView::_SetUIColors(const BMessage& colors)
{
	set_ui_colors(&colors);
	fCurrentColors = colors;
}


void
APRView::_UpdatePreviews(const BMessage& colors)
{
	rgb_color color;
	for (int32 i = color_description_count() - 1; i >= 0; i--) {
		ColorWhichItem* item = static_cast<ColorWhichItem*>(fAttrList->ItemAt(i));
		if (item == NULL)
			continue;

		color = colors.GetColor(ui_color_name(get_color_description(i)->which),
			make_color(255, 0, 255));

		item->SetColor(color);
		fAttrList->InvalidateItem(i);
	}
}
