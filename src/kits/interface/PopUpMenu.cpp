//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku
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
//	File Name:		PopUpMenu.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BPopUpMenu represents a menu that pops up when you
//                  activate it.
//------------------------------------------------------------------------------

#include "PopUpMenu.h"
#include "MenuItem.h"


BPopUpMenu::BPopUpMenu(const char *title, bool radioMode, bool autoRename,
	menu_layout layout)
	: BMenu(title, layout),
	fUseWhere(false),
	fAutoDestruct(false),
	fTrackThread(-1)
{
	if (radioMode)
		SetRadioMode(true);

	if (autoRename)
		SetLabelFromMarked(true);
}


BPopUpMenu::BPopUpMenu(BMessage *archive)
	: BMenu(archive),
	fUseWhere(false),
	fAutoDestruct(false),
	fTrackThread(-1)
{
}


BPopUpMenu::~BPopUpMenu()
{
	/*if (fTrackThread != 0)
	{
		while (wait_for_thread() == );
	}*/
}


status_t
BPopUpMenu::Archive(BMessage *data, bool deep) const
{
	return BMenu::Archive(data, deep);
}


BArchivable *
BPopUpMenu::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BPopUpMenu"))
		return new BPopUpMenu(data);

	return NULL;
}


BMenuItem *
BPopUpMenu::Go(BPoint where, bool delivers_message, bool open_anyway, bool async)
{
	//return _go(where, delivers_message, open_anyway, NULL, async);

	if (async) {
		fWhere = where;
		fUseWhere = true;
		Show();
	}

	return NULL;
}


BMenuItem *
BPopUpMenu::Go(BPoint where, bool deliversMessage, bool openAnyway,
	BRect clickToOpen, bool async)
{
	//return _go(where, deliversMessage, openAnyway, clickToOpen, async);

	return NULL;
}


void
BPopUpMenu::MessageReceived(BMessage *msg)
{
	BMenu::MessageReceived(msg);
}


void
BPopUpMenu::MouseDown(BPoint point)
{
	BMenu::MouseDown(point);
}


void
BPopUpMenu::MouseUp(BPoint point)
{
	BMenu::MouseUp(point);
}


void
BPopUpMenu::MouseMoved(BPoint point, uint32 code, const BMessage *msg)
{
	BMenu::MouseMoved(point, code, msg);
}


void
BPopUpMenu::AttachedToWindow()
{
	BMenu::AttachedToWindow();
}


void
BPopUpMenu::DetachedFromWindow()
{
	BMenu::DetachedFromWindow();
}


void
BPopUpMenu::FrameMoved(BPoint newPosition)
{
	BMenu::FrameMoved(newPosition);
}


void
BPopUpMenu::FrameResized(float newWidth, float newHeight)
{
	BMenu::FrameResized(newWidth, newHeight);
}


BHandler *
BPopUpMenu::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
	int32 form, const char *property)
{
	return BMenu::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
BPopUpMenu::GetSupportedSuites(BMessage *data)
{
	return BMenu::GetSupportedSuites(data);
}


status_t
BPopUpMenu::Perform(perform_code d, void *arg)
{
	return BMenu::Perform(d, arg);
}


void
BPopUpMenu::ResizeToPreferred()
{
	BMenu::ResizeToPreferred();
}


void
BPopUpMenu::GetPreferredSize(float *_width, float *_height)
{
	BMenu::GetPreferredSize(_width, _height);
}


void
BPopUpMenu::MakeFocus(bool state)
{
	BMenu::MakeFocus(state);
}


void
BPopUpMenu::AllAttached()
{
	BMenu::AllAttached();
}


void
BPopUpMenu::AllDetached()
{
	BMenu::AllDetached();
}


void
BPopUpMenu::SetAsyncAutoDestruct(bool state)
{
	fAutoDestruct = state;
}


bool
BPopUpMenu::AsyncAutoDestruct() const
{
	return fAutoDestruct;
}


BPoint
BPopUpMenu::ScreenLocation()
{
	if (fUseWhere)
		return fWhere;

	BMenuItem *item = Superitem();
	BMenu *menu = Supermenu();
	BMenuItem *selectedItem = FindItem(item->Label());
	BRect rect = item->Frame();
	BPoint point = rect.LeftTop();

	menu->ConvertToScreen(&point);

	if (selectedItem)
		point.y -= selectedItem->Frame().top;

	return point;
}


//------------------------------------------------------------------------------
//	#pragma mark -
//	private methods


void BPopUpMenu::_ReservedPopUpMenu1() {}
void BPopUpMenu::_ReservedPopUpMenu2() {}
void BPopUpMenu::_ReservedPopUpMenu3() {}


BPopUpMenu &
BPopUpMenu::operator=(const BPopUpMenu &)
{
	return *this;
}


BMenuItem *
BPopUpMenu::_go(BPoint where, bool autoInvoke, bool startOpened,
	BRect *_specialRect, bool async)
{
	return NULL;
}


int32
BPopUpMenu::entry(void *)
{
	return -1;
}


BMenuItem *
BPopUpMenu::start_track(BPoint where, bool autoInvoke,
	bool startOpened, BRect *_specialRect)
{
	return NULL;
}

