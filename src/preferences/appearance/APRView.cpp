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
#include <DefaultColors.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <HSL.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Messenger.h>
#include <Path.h>
#include <SpaceLayoutItem.h>

#include "APRWindow.h"
#include "defs.h"
#include "ColorPreview.h"
#include "Colors.h"
#include "ColorWhichListView.h"
#include "ColorWhichItem.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Colors tab"

#define COLOR_DROPPED 'cldp'
#define AUTO_ADJUST_CHANGED 'madj'


APRView::APRView(const char* name)
	:
	BView(name, B_WILL_DRAW)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	LoadSettings();

	fAutoSelectCheckBox = new BCheckBox(B_TRANSLATE("Automatically pick secondary colors"),
		new BMessage(AUTO_ADJUST_CHANGED));
	fAutoSelectCheckBox->SetValue(true);

	// Set up list of color attributes
	fAttrList = new ColorWhichListView("AttributeList");

	fScrollView = new BScrollView("ScrollView", fAttrList, 0, false, true);
	fScrollView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	_CreateItems();

	fColorPreview = new ColorPreview(new BMessage(COLOR_DROPPED), 0);
	fColorPreview->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_VERTICAL_CENTER));

	fPicker = new BColorControl(B_ORIGIN, B_CELLS_32x8, 8.0,
		"picker", new BMessage(UPDATE_COLOR));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fAutoSelectCheckBox)
		.Add(fScrollView, 10.0)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fColorPreview)
			.AddGlue()
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
	fAutoSelectCheckBox->SetTarget(this);
	fPicker->SetTarget(this);
	fAttrList->SetTarget(this);
	fColorPreview->SetTarget(this);

	fAttrList->Select(0);
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}


void
APRView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case SET_COLOR:
		{
			rgb_color* color;
			ssize_t size;
			color_which which;

			if (msg->FindData(kRGBColor, B_RGB_COLOR_TYPE,
					(const void**)&color, &size) == B_OK
				&& msg->FindUInt32(kWhich, (uint32*)&which) == B_OK) {
				_SetColor(which, *color);
				Window()->PostMessage(kMsgUpdate);
			}
			break;
		}

		case SET_CURRENT_COLOR:
		{
			rgb_color* color;
			ssize_t size;

			if (msg->FindData(kRGBColor, B_RGB_COLOR_TYPE,
					(const void**)&color, &size) == B_OK) {
				_SetCurrentColor(*color);
				Window()->PostMessage(kMsgUpdate);
			}
			break;
		}

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

		case AUTO_ADJUST_CHANGED:
		{
			_CreateItems();
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
APRView::_CreateItems()
{
	while (fAttrList->CountItems() > 0)
		delete fAttrList->RemoveItem((int32)0);

	const bool autoSelect = fAutoSelectCheckBox->Value();
	const int32 count = color_description_count();
	for (int32 i = 0; i < count; i++) {
		const ColorDescription& description = *get_color_description(i);
		const color_which which = description.which;
		if (autoSelect) {
			if (which != B_PANEL_BACKGROUND_COLOR
					&& which != B_STATUS_BAR_COLOR
					&& which != B_WINDOW_TAB_COLOR) {
				continue;
			}
		}

		const char* text = B_TRANSLATE_NOCOLLECT(description.text);
		fAttrList->AddItem(new ColorWhichItem(text, which, ui_color(which)));
	}

	if (Window() != NULL)
		fAttrList->Select(0);
}


void
APRView::_UpdatePreviews(const BMessage& colors)
{
	rgb_color color;
	for (int32 i = color_description_count() - 1; i >= 0; i--) {
		ColorWhichItem* item = static_cast<ColorWhichItem*>(fAttrList->ItemAt(i));
		if (item == NULL)
			continue;

		color = colors.GetColor(ui_color_name(item->ColorWhich()),
			make_color(255, 0, 255));

		item->SetColor(color);
		fAttrList->InvalidateItem(i);
	}
}


void
APRView::_SetUIColors(const BMessage& colors)
{
	set_ui_colors(&colors);
	fCurrentColors = colors;
}


void
APRView::_SetCurrentColor(rgb_color color)
{
	_SetColor(fWhich, color);

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
APRView::_SetColor(color_which which, rgb_color color)
{
	_SetOneColor(which, color);

	if (!fAutoSelectCheckBox->Value())
		return;

	// Protect against accidentally overwriting colors.
	if (ui_color(which) == color)
		return;

	if (which == B_PANEL_BACKGROUND_COLOR) {
		const bool isDark = color.IsDark();

		_SetOneColor(B_MENU_BACKGROUND_COLOR, color);
		_SetOneColor(B_SCROLL_BAR_THUMB_COLOR, color);

		const rgb_color menuSelectedBackground
			= tint_color(color, isDark ? 0.8 /* lighten "< 1" */ : B_DARKEN_2_TINT);
		_SetOneColor(B_MENU_SELECTED_BACKGROUND_COLOR, menuSelectedBackground);

		const rgb_color controlBackground = tint_color(color, isDark
			? 0.8 /* lighten "< 1" */ : 0.25 /* lighten "> 2" */);
		_SetOneColor(B_CONTROL_BACKGROUND_COLOR, controlBackground);

		const rgb_color controlBorder
			= tint_color(color, isDark ? 0.4875 : 1.20 /* lighten/darken "1.5" */);
		_SetOneColor(B_CONTROL_BORDER_COLOR, controlBorder);

		const rgb_color windowBorder = tint_color(color, 0.75);
		_SetOneColor(B_WINDOW_BORDER_COLOR, windowBorder);

		const rgb_color inactiveWindowBorder = tint_color(color, B_LIGHTEN_1_TINT);
		_SetOneColor(B_WINDOW_INACTIVE_TAB_COLOR, inactiveWindowBorder);
		_SetOneColor(B_WINDOW_INACTIVE_BORDER_COLOR, inactiveWindowBorder);

		const rgb_color listSelectedBackground
			= tint_color(color, isDark ? 0.77 : 1.12 /* lighten/darken "< 1" */ );
		_SetOneColor(B_LIST_SELECTED_BACKGROUND_COLOR, listSelectedBackground);

		const color_which fromDefaults[] = {
			B_MENU_ITEM_TEXT_COLOR,
			B_MENU_SELECTED_ITEM_TEXT_COLOR,
			B_MENU_SELECTED_BORDER_COLOR,
			B_PANEL_TEXT_COLOR,
			B_DOCUMENT_BACKGROUND_COLOR,
			B_DOCUMENT_TEXT_COLOR,
			B_CONTROL_TEXT_COLOR,
			B_NAVIGATION_PULSE_COLOR,
			B_WINDOW_INACTIVE_TEXT_COLOR,
			B_LIST_BACKGROUND_COLOR,
			B_LIST_ITEM_TEXT_COLOR,
			B_LIST_SELECTED_ITEM_TEXT_COLOR,

			B_SHINE_COLOR,
			B_SHADOW_COLOR,

			B_LINK_TEXT_COLOR,
			B_LINK_HOVER_COLOR,
			B_LINK_ACTIVE_COLOR,
			B_LINK_VISITED_COLOR,
		};
		for (size_t i = 0; i < B_COUNT_OF(fromDefaults); i++)
			_SetOneColor(fromDefaults[i], BPrivate::GetSystemColor(fromDefaults[i], isDark));
	} else if (which == B_STATUS_BAR_COLOR) {
		const hsl_color statusColorHSL = hsl_color::from_rgb(color);

		hsl_color controlHighlight = statusColorHSL;
		controlHighlight.saturation = max_c(0.2f, controlHighlight.saturation / 2.f);
		_SetOneColor(B_CONTROL_HIGHLIGHT_COLOR, controlHighlight.to_rgb());

		hsl_color controlMark = statusColorHSL;
		controlMark.saturation = max_c(0.2f, controlMark.saturation * 0.67f);
		controlMark.lightness = max_c(0.25f, controlMark.lightness * 0.55f);
		_SetOneColor(B_CONTROL_MARK_COLOR, controlMark.to_rgb());

		rgb_color keyboardNav; {
			hsl_color keyboardNavHSL = statusColorHSL;
			keyboardNavHSL.lightness = max_c(0.2f, keyboardNavHSL.lightness * 0.75f);
			keyboardNav = keyboardNavHSL.to_rgb();

			// Use primary color channel only.
			if (keyboardNav.blue >= max_c(keyboardNav.red, keyboardNav.green))
				keyboardNav.red = keyboardNav.green = 0;
			else if (keyboardNav.red >= max_c(keyboardNav.green, keyboardNav.blue))
				keyboardNav.green = keyboardNav.blue = 0;
			else
				keyboardNav.red = keyboardNav.blue = 0;
		}
		_SetOneColor(B_KEYBOARD_NAVIGATION_COLOR, keyboardNav);
	} else if (which == B_WINDOW_TAB_COLOR) {
		const bool isDark = color.IsDark();
		const hsl_color tabColorHSL = hsl_color::from_rgb(color);
		const float tabColorSaturation
			= tabColorHSL.saturation != 0 ? tabColorHSL.saturation : tabColorHSL.lightness;

		_SetOneColor(B_WINDOW_TEXT_COLOR,
			BPrivate::GetSystemColor(B_WINDOW_TEXT_COLOR, isDark));
		_SetOneColor(B_TOOL_TIP_TEXT_COLOR,
			BPrivate::GetSystemColor(B_TOOL_TIP_TEXT_COLOR, isDark));

		const rgb_color toolTipBackground = tint_color(color, isDark ? 1.7 : 0.15);
		_SetOneColor(B_TOOL_TIP_BACKGROUND_COLOR, toolTipBackground);

		hsl_color success = hsl_color::from_rgb(BPrivate::GetSystemColor(B_SUCCESS_COLOR, isDark));
		success.saturation = max_c(0.25f, tabColorSaturation * 0.68f);
		_SetOneColor(B_SUCCESS_COLOR, success.to_rgb());

		hsl_color failure = hsl_color::from_rgb(BPrivate::GetSystemColor(B_FAILURE_COLOR, isDark));
		failure.saturation = max_c(0.25f, tabColorSaturation);
		_SetOneColor(B_FAILURE_COLOR, failure.to_rgb());
	}
}


void
APRView::_SetOneColor(color_which which, rgb_color color)
{
	if (ui_color(which) == color)
		return;

	set_ui_color(which, color);
	fCurrentColors.SetColor(ui_color_name(which), color);
}
