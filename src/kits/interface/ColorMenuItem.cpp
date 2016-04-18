/*
 * Copyright 2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "ColorMenuItem.h"

#include <math.h>
#include <string.h>
#include <syslog.h>

#include <algorithm>

#include <MenuField.h>
#include <MenuPrivate.h>


// golden ratio
#ifdef M_PHI
#	undef M_PHI
#endif
#define M_PHI 1.61803398874989484820


//	#pragma - ColorMenuItem


BColorMenuItem::BColorMenuItem(const char* label, BMessage* message,
	rgb_color color, char shortcut, uint32 modifiers)
	:
	BMenuItem(label, message, shortcut, modifiers),
	fColor(color)
{
}


BColorMenuItem::BColorMenuItem(BMenu* menu, rgb_color color,
	BMessage* message)
	:
	BMenuItem(menu, message),
	fColor(color)
{
}


BColorMenuItem::BColorMenuItem(BMessage* data)
	:
	BMenuItem(data)
{
	if (data->HasColor("_color")) {
		rgb_color color;

		color = data->GetColor("_color", (rgb_color){ 0, 0, 0 });
		fColor = color;
	} else
		fColor = (rgb_color){ 0, 0, 0 };
}


BArchivable*
BColorMenuItem::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BColorMenuItem"))
		return new BColorMenuItem(data);

	return NULL;
}


status_t
BColorMenuItem::Archive(BMessage* data, bool deep) const
{
	status_t result = BMenuItem::Archive(data, deep);

	if (result == B_OK)
		result = data->AddColor("_color", fColor);

	return result;
}


void
BColorMenuItem::DrawContent()
{
	float leftMargin = _LeftMargin();
	float padding = _Padding();
	float colorRectWidth = _ColorRectWidth();

	BRect colorRect(Frame().InsetByCopy(2.0f, 2.0f));
	colorRect.left = colorRect.right = leftMargin;
	colorRect.right += colorRectWidth;

	rgb_color highColor = Menu()->HighColor();

	Menu()->SetDrawingMode(B_OP_OVER);

	Menu()->SetHighColor(fColor);
	Menu()->FillRect(colorRect);

	Menu()->SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
	Menu()->StrokeRect(colorRect);

	float width = colorRectWidth + padding;

	Menu()->MovePenBy(width, 0.0f);
	Menu()->SetHighColor(highColor);

	BMenuItem::DrawContent();
}


void
BColorMenuItem::GetContentSize(float* _width, float* _height)
{
	float labelWidth;
	float height;
	BMenuItem::GetContentSize(&labelWidth, &height);

	if (_width != NULL)
		*_width = _LeftMargin() + _ColorRectWidth() + _Padding() + labelWidth;

	if (_height != NULL)
		*_height = height;
}


void
BColorMenuItem::SetMarked(bool mark)
{
	BMenuItem::SetMarked(mark);

	if (!mark)
		return;

	// we are marking the item

	BMenu* menu = Menu();
	if (menu == NULL)
		return;

	// we have a parent menu

	BMenu* _menu = menu;
	while ((_menu = _menu->Supermenu()) != NULL)
		menu = _menu;

	// went up the hierarchy to found the topmost menu

	if (menu == NULL || menu->Parent() == NULL)
		return;

	// our topmost menu has a parent

	if (dynamic_cast<BMenuField*>(menu->Parent()) == NULL)
		return;

	// our topmost menu's parent is a BMenuField

	BMenuItem* topLevelItem = menu->ItemAt((int32)0);

	if (topLevelItem == NULL)
		return;

	// our topmost menu has a menu item

	BColorMenuItem* topLevelColorMenuItem
		= dynamic_cast<BColorMenuItem*>(topLevelItem);
	if (topLevelColorMenuItem == NULL)
		return;

	// our topmost menu's item is a BColorMenuItem

	// update the color
	topLevelColorMenuItem->SetColor(fColor);
	menu->Invalidate();
}


//	#pragma mark - BColorMenuItem private methods


float
BColorMenuItem::_LeftMargin()
{
	BPrivate::MenuPrivate menuPrivate(Menu());
	float leftMargin;
	menuPrivate.GetItemMargins(&leftMargin, NULL, NULL, NULL);
	return leftMargin;
}


float
BColorMenuItem::_Padding()
{
	return floorf(std::max(14.0f, be_plain_font->Size() + 2) / 2);
}


float
BColorMenuItem::_ColorRectWidth()
{
	return floorf(std::max(14.0f, be_plain_font->Size() + 2) * M_PHI);
}



// #pragma mark - BColorMenuItem reserved virtual methods


void BColorMenuItem::_ReservedColorMenuItem1() {}
void BColorMenuItem::_ReservedColorMenuItem2() {}
void BColorMenuItem::_ReservedColorMenuItem3() {}
void BColorMenuItem::_ReservedColorMenuItem4() {}
void BColorMenuItem::_ReservedColorMenuItem5() {}
void BColorMenuItem::_ReservedColorMenuItem6() {}
void BColorMenuItem::_ReservedColorMenuItem7() {}
void BColorMenuItem::_ReservedColorMenuItem8() {}
void BColorMenuItem::_ReservedColorMenuItem9() {}
void BColorMenuItem::_ReservedColorMenuItem10() {}
void BColorMenuItem::_ReservedColorMenuItem11() {}
void BColorMenuItem::_ReservedColorMenuItem12() {}
void BColorMenuItem::_ReservedColorMenuItem13() {}
void BColorMenuItem::_ReservedColorMenuItem14() {}
void BColorMenuItem::_ReservedColorMenuItem15() {}
void BColorMenuItem::_ReservedColorMenuItem16() {}
void BColorMenuItem::_ReservedColorMenuItem17() {}
void BColorMenuItem::_ReservedColorMenuItem18() {}
void BColorMenuItem::_ReservedColorMenuItem19() {}
void BColorMenuItem::_ReservedColorMenuItem20() {}
