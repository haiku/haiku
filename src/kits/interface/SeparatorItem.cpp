//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	Description:	Display separator item for BMenu class
//
//------------------------------------------------------------------------------
#include <MenuItem.h>
#include <Message.h>


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
}


void
BSeparatorItem::GetContentSize(float *width, float *height)
{
	if (width)
		*width = 2.0f;
	if (height)
		*height = 8.0f;
}


void
BSeparatorItem::Draw()
{
	BRect bounds = Frame();

	Menu()->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
		B_DARKEN_2_TINT));
	Menu()->StrokeLine(BPoint(bounds.left + 1.0f, bounds.top + 4.0f),
		BPoint(bounds.right - 1.0f, bounds.top + 4.0f));
	Menu()->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
		B_LIGHTEN_2_TINT));
	Menu()->StrokeLine(BPoint(bounds.left + 1.0f, bounds.top + 5.0f),
		BPoint(bounds.right - 1.0f, bounds.top + 5.0f));

	Menu()->SetHighColor(0, 0, 0);
}


void BSeparatorItem::_ReservedSeparatorItem1() {}
void BSeparatorItem::_ReservedSeparatorItem2() {}


BSeparatorItem &
BSeparatorItem::operator=(const BSeparatorItem &)
{
	return *this;
}
