//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005 Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		SeparatorItem.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//					Bill Hayden (haydentech@users.sourceforge.net)
//					Stefano Ceccherini (burton666@libero.it)
//	Description:	Display separator item for BMenu class
//
//------------------------------------------------------------------------------
#include <MenuItem.h>


BSeparatorItem::BSeparatorItem()
	:	BMenuItem(NULL, NULL, 0, 0)
{
}


BSeparatorItem::BSeparatorItem(BMessage *data)
	:	BMenuItem(data)
{
}


BSeparatorItem::~BSeparatorItem()
{
}


status_t
BSeparatorItem::Archive(BMessage *data, bool deep) const
{
	return BMenuItem::Archive(data, deep);
}


BArchivable *
BSeparatorItem::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BSeparatorItem"))
		return new BSeparatorItem(data);
	else
		return NULL;
}


void
BSeparatorItem::SetEnabled(bool state)
{
	// Don't do anything
}


void
BSeparatorItem::GetContentSize(float *width, float *height)
{
	if (width != NULL)
		*width = 2.0f;
	if (height != NULL)
		*height = 8.0f;
}


void
BSeparatorItem::Draw()
{
	BMenu *menu = Menu();
	if (menu == NULL)
		return;
		
	BRect bounds = Frame();
	
	menu_info menuInfo;
	get_menu_info(&menuInfo);
	switch (menuInfo.separator) {
		case 0:
			// TODO: Check if drawing is pixel perfect
			menu->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
				B_DARKEN_2_TINT));
			menu->StrokeLine(BPoint(bounds.left + 1.0f, bounds.top + 4.0f),
				BPoint(bounds.right - 1.0f, bounds.top + 4.0f));
			menu->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
				B_LIGHTEN_2_TINT));
			menu->StrokeLine(BPoint(bounds.left + 1.0f, bounds.top + 5.0f),
				BPoint(bounds.right - 1.0f, bounds.top + 5.0f));
			menu->SetHighColor(0, 0, 0);
			break;
		
		case 1:
			// TODO: Check if drawing is pixel perfect
			menu->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
				B_DARKEN_2_TINT));
			menu->StrokeLine(BPoint(bounds.left + 9.0f, bounds.top + 4.0f),
				BPoint(bounds.right - 9.0f, bounds.top + 4.0f));
			menu->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
				B_LIGHTEN_2_TINT));
			menu->StrokeLine(BPoint(bounds.left + 9.0f, bounds.top + 5.0f),
				BPoint(bounds.right - 9.0f, bounds.top + 5.0f));
			menu->SetHighColor(0, 0, 0);
			break;
			
		case 2:
			menu->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
				B_DARKEN_2_TINT));
			menu->StrokeLine(BPoint(bounds.left + 9.0f, bounds.top + 4.0f),
				BPoint(bounds.right - 9.0f, bounds.top + 4.0f));
			menu->StrokeLine(BPoint(bounds.left + 10.0f, bounds.top + 5.0f),
				BPoint(bounds.right - 10.0f, bounds.top + 5.0f));
			menu->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
				B_LIGHTEN_2_TINT));
			menu->StrokeLine(BPoint(bounds.left + 11.0f, bounds.top + 6.0f),
				BPoint(bounds.right - 11.0f, bounds.top + 6.0f));
			menu->SetHighColor(0, 0, 0);
			break;
		
		default:
			break;
	}
}


void BSeparatorItem::_ReservedSeparatorItem1() {}
void BSeparatorItem::_ReservedSeparatorItem2() {}


BSeparatorItem &
BSeparatorItem::operator=(const BSeparatorItem &)
{
	return *this;
}
