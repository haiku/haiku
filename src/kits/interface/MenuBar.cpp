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
//	File Name:		Menubar.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BMenuBar is a menu that's at the root of a menu hierarchy.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <MenuBar.h>
#include <MenuItem.h>
#include <Window.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BMenuBar::BMenuBar(BRect frame, const char *title, uint32 resizeMask,
				   menu_layout layout, bool resizeToFit)
	:	BMenu(frame, title, resizeMask,
			B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, layout, resizeToFit),
		fBorder(B_BORDER_FRAME),
		fTrackingPID(-1),
		fPrevFocusToken(-1),
		fMenuSem(-1),
		fLastBounds(NULL),
		fTracking(false)
{
	// TODO: which flags to pass to BMenu?
	InitData(layout);
}
//------------------------------------------------------------------------------
BMenuBar::BMenuBar(BMessage *data)
	:	BMenu(data),
		fBorder(B_BORDER_FRAME),
		fTrackingPID(-1),
		fPrevFocusToken(-1),
		fMenuSem(-1),
		fLastBounds(NULL),
		fTracking(false)
{
	int32 border;
	
	if (data->FindInt32("_border", &border) == B_OK)
		SetBorder((menu_bar_border)border);
}
//------------------------------------------------------------------------------
BMenuBar::~BMenuBar()
{
}
//------------------------------------------------------------------------------
BArchivable *BMenuBar::Instantiate(BMessage *data)
{
	if ( validate_instantiation(data, "BMenuBar"))
		return new BMenuBar(data);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BMenuBar::Archive(BMessage *data, bool deep) const
{
	status_t err = BMenu::Archive(data, deep);

	if (err != B_OK)
		return err;
	
	if (Border() != B_BORDER_FRAME)
		err = data->AddInt32("_border", Border());

	return err;
}
//------------------------------------------------------------------------------
void BMenuBar::SetBorder(menu_bar_border border)
{
	fBorder = border;
}
//------------------------------------------------------------------------------
menu_bar_border	BMenuBar::Border() const
{
	return fBorder;
}
//------------------------------------------------------------------------------
void BMenuBar::Draw(BRect updateRect)
{
	// TODO: implement additional border styles
	BRect bounds(Bounds());
	
	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
	StrokeLine(BPoint(0.0f, bounds.bottom - 2.0f ), BPoint(0.0f, 0.0f));
	StrokeLine(BPoint(bounds.right, 0.0f ) );

	SetHighColor(tint_color ( ui_color ( B_MENU_BACKGROUND_COLOR ), B_DARKEN_1_TINT));
	StrokeLine(BPoint ( 1.0f, bounds.bottom - 1.0f ),
		BPoint(bounds.right, bounds.bottom - 1.0f ) );

	SetHighColor(tint_color(ui_color (B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT));
	StrokeLine(BPoint(0.0f, bounds.bottom), BPoint(bounds.right, bounds.bottom));
	StrokeLine(BPoint(bounds.right, 0.0f), BPoint(bounds.right, bounds.bottom));

	DrawItems(updateRect);
}
//------------------------------------------------------------------------------
void BMenuBar::AttachedToWindow()
{
	Install(Window());
	Window()->SetKeyMenuBar(this);

	BMenu::AttachedToWindow();
}
//------------------------------------------------------------------------------
void BMenuBar::DetachedFromWindow()
{
	BMenu::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void BMenuBar::MessageReceived(BMessage *msg)
{
	BMenu::MessageReceived(msg);
}
//------------------------------------------------------------------------------
void BMenuBar::MouseDown(BPoint where)
{
	StealFocus();
	BMenu::MouseDown(where);
	
	// TODO: the real code should look like this:
	/*
	if (!Window()->IsActive())
	{
		Window()->Activate();
		Window()->UpdateIfNeeded();
	}
	
	StartMenuBar();
	*/
}
//------------------------------------------------------------------------------
void BMenuBar::WindowActivated(bool state)
{
	BView::WindowActivated(state);
}
//------------------------------------------------------------------------------
void BMenuBar::MouseUp(BPoint where)
{
	BMenu::MouseUp(where);
	RestoreFocus();
	
	// BView::MouseUp(where);
}
//------------------------------------------------------------------------------
void BMenuBar::FrameMoved(BPoint new_position)
{	
	BMenu::FrameMoved(new_position);
}
//------------------------------------------------------------------------------
void BMenuBar::FrameResized(float new_width, float new_height)
{
	BMenu::FrameResized(new_width, new_height);
}
//------------------------------------------------------------------------------
void BMenuBar::Show()
{
	BView::Show();
}
//------------------------------------------------------------------------------
void BMenuBar::Hide()
{
	BView::Hide();
}
//------------------------------------------------------------------------------
BHandler *BMenuBar::ResolveSpecifier(BMessage *msg, int32 index,
									 BMessage *specifier, int32 form,
									 const char *property)
{
	return BMenu::ResolveSpecifier(msg, index, specifier, form, property);
}
//------------------------------------------------------------------------------
status_t BMenuBar::GetSupportedSuites(BMessage *data)
{
	return BMenu::GetSupportedSuites(data);
}
//------------------------------------------------------------------------------
void BMenuBar::ResizeToPreferred()
{
	BMenu::ResizeToPreferred();
}
//------------------------------------------------------------------------------
void BMenuBar::GetPreferredSize(float *width, float *height)
{
	BMenu::GetPreferredSize(width, height);
}
//------------------------------------------------------------------------------
void BMenuBar::MakeFocus(bool state)
{
	BMenu::MakeFocus(state);
}
//------------------------------------------------------------------------------
void BMenuBar::AllAttached()
{
	BMenu::AllAttached();
}
//------------------------------------------------------------------------------
void BMenuBar::AllDetached()
{
	BMenu::AllDetached();
}
//------------------------------------------------------------------------------
status_t BMenuBar::Perform(perform_code d, void *arg)
{
	return BMenu::Perform(d, arg);
}
//------------------------------------------------------------------------------
void BMenuBar::_ReservedMenuBar1() {}
void BMenuBar::_ReservedMenuBar2() {}
void BMenuBar::_ReservedMenuBar3() {}
void BMenuBar::_ReservedMenuBar4() {}
//------------------------------------------------------------------------------
BMenuBar &BMenuBar::operator=(const BMenuBar &)
{
	return *this;
}
//------------------------------------------------------------------------------
void BMenuBar::StartMenuBar(int32 menuIndex, bool sticky, bool show_menu,
							BRect *special_rect)
{
	/*
	if (!Window())
		return;
		
	Window()->Lock();
	Window()->MenusBeginning();
	sem_id = create_sem(??, ??);
	set_menu_sem(BWindow, sem);
	fTrackingPID = spawn_thread(TrackTask, "menu_tracking?", B_NORMAL_PRIORITY, ??);
	Window()->Unlock();
	*/
}
//------------------------------------------------------------------------------
long BMenuBar::TrackTask(void *arg)
{
	return -1;
}
//------------------------------------------------------------------------------
BMenuItem *BMenuBar::Track(int32 *action, int32 startIndex, bool showMenu)
{
	return NULL;
}
//------------------------------------------------------------------------------
void BMenuBar::StealFocus()
{
	//fPrevFocusToken = _get_object_token_(Window()->CurrentFocus());
	MakeFocus();
}
//------------------------------------------------------------------------------
void BMenuBar::RestoreFocus()
{
	//fPrevFocusToken
}
//------------------------------------------------------------------------------
void BMenuBar::InitData(menu_layout layout)
{
	SetItemMargins(8, 2, 8, 2);
	SetIgnoreHidden(true);
}
//------------------------------------------------------------------------------
