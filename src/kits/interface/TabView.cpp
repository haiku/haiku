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
//	File Name:		TabView.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BTab creates individual "tabs" that can be assigned
//                  to specific views.
//                  BTabView provides the framework for containing and
//                  managing groups of BTab objects.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <TabView.h>
#include <Message.h>
#include <List.h>
#include <Rect.h>
#include <Errors.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BTab::BTab(BView *tabView)
	:	fEnabled(true),
		fSelected(false),
		fFocus(false),
		fView(tabView)
{
}
//------------------------------------------------------------------------------
BTab::~BTab()
{
	if (fView->Window() != NULL)
	{
		fView->RemoveSelf();

		delete fView;
	}
}
//------------------------------------------------------------------------------
BTab::BTab(BMessage *archive)
	:	fSelected(false),
		fFocus(false),
		fView(NULL)
{
	if (archive->FindBool("_disable", &fEnabled) != B_OK)
		fEnabled = true;
	else
		fEnabled = !fEnabled;
}
//------------------------------------------------------------------------------
BArchivable *BTab::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BTab"))
		return new BTab(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BTab::Archive(BMessage *archive, bool deep) const
{
	status_t err = BArchivable::Archive(archive, deep);

	if (err != B_OK)
		return err;

	if ( !fEnabled )
		err = archive->AddBool("_disable", false);

	return err;
}
//------------------------------------------------------------------------------
status_t BTab::Perform(uint32 d, void *arg)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
const char *BTab::Label() const
{
	if (fView)
		return fView->Name();
	else
		return NULL;
}
//------------------------------------------------------------------------------
void BTab::SetLabel(const char *label)
{
	if (fView)
		fView->SetName(label);
}
//------------------------------------------------------------------------------
bool BTab::IsSelected() const
{
	return fSelected;
}
//------------------------------------------------------------------------------
void BTab::Select(BView *owner)
{	
	dynamic_cast<BTabView*>(owner)->ContainerView()->AddChild(fView);

	fSelected = true;
}
//------------------------------------------------------------------------------
void BTab::Deselect()
{
	fView->RemoveSelf();

	fSelected = false;
}
//------------------------------------------------------------------------------
void BTab::SetEnabled(bool enabled)
{
	fEnabled = enabled;
}
//------------------------------------------------------------------------------
bool BTab::IsEnabled() const
{
	return fEnabled;
}
//------------------------------------------------------------------------------
void BTab::MakeFocus(bool inFocus)
{
	fFocus = inFocus;
}
//------------------------------------------------------------------------------
bool BTab::IsFocus() const
{
	return fFocus;
}
//------------------------------------------------------------------------------
void BTab::SetView(BView *view)
{
	// TODO: do we need to remove the previous view and add the new one if
	// selected?
	fView = view;
}
//------------------------------------------------------------------------------
BView *BTab::View() const
{
	return fView;
}
//------------------------------------------------------------------------------
void BTab::DrawFocusMark(BView *owner, BRect frame)
{
	float width = owner->StringWidth(Label());

	owner->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	owner->StrokeLine(BPoint(frame.left + frame.Width() * 0.5f - width * 0.5f, frame.bottom),
		BPoint(frame.left + frame.Width() * 0.5f + width * 0.5f, frame.bottom));
}
//------------------------------------------------------------------------------
void BTab::DrawLabel(BView *owner, BRect frame)
{
	const char *label = Label();

	if (label)
	{
		owner->SetHighColor(0, 0, 0);
		owner->DrawString(label, BPoint(frame.left + frame.Width() * 0.5f -
			owner->StringWidth(label) * 0.5f, frame.bottom - 4.0f - 2.0f));
	}
}
//------------------------------------------------------------------------------
void BTab::DrawTab(BView *owner, BRect frame, tab_position position, bool full)
{
	// TODO: write a decent DrawTab method, this one is from the Be Book
	
	// Save the original colors
	rgb_color hi = owner->HighColor();
	rgb_color lo = owner->LowColor();

	rgb_color kGray = ui_color(B_PANEL_BACKGROUND_COLOR),
		kBlack = tint_color(kGray, B_DARKEN_MAX_TINT),
		kDarkGray = tint_color(kGray, B_DARKEN_4_TINT),
		kMedGray = tint_color(kGray, B_DARKEN_2_TINT),
		kWhite = tint_color(kGray, B_LIGHTEN_MAX_TINT);

	// Draw the label by calling DrawLabel() 
	owner->SetHighColor(kBlack); 
	owner->SetLowColor(kGray); 
	DrawLabel(owner, frame); 

	// Start a line array to draw the tab -- 
	// this is faster than drawing the lines 
	// one by one. 
	owner->BeginLineArray(7); 

	// Do the bottom left corner, visible 
	// only on the frontmost tab. 
	if (position != B_TAB_ANY) { 
		owner->AddLine(BPoint(frame.left, frame.bottom), 
			BPoint(frame.left+3, frame.bottom-3), kWhite); 
	} 

	// Left wall -- always drawn 
	owner->AddLine(BPoint(frame.left+4, frame.bottom-4), 
		BPoint(frame.left+4, frame.top), kWhite); 

	// Top -- always drawn 
	owner->AddLine(BPoint(frame.left+5, frame.top), 
		BPoint(frame.right-5, frame.top), kWhite); 

	// Right wall -- always drawn. Has a nice bevel. 
		owner->AddLine(BPoint(frame.right-4, frame.top), 
			BPoint(frame.right-4, frame.bottom-4), kDarkGray); 
		owner->AddLine(BPoint(frame.right-5, frame.top), 
			BPoint(frame.right-5, frame.bottom-4), kMedGray); 

	// Bottom-right corner, only visible if the tab 
	// is either frontmost or the rightmost tab. 
	if (full) { 
		owner->AddLine(BPoint(frame.right-3, frame.bottom-3), 
			BPoint(frame.right, frame.bottom), kDarkGray); 
		owner->AddLine(BPoint(frame.right-4, frame.bottom-3), 
			BPoint(frame.right-1, frame.bottom), kMedGray); 
	} 

	owner->EndLineArray(); 

	owner->SetHighColor(hi); 
	owner->SetLowColor(lo);
}
//------------------------------------------------------------------------------
void BTab::_ReservedTab1() {}
void BTab::_ReservedTab2() {}
void BTab::_ReservedTab3() {}
void BTab::_ReservedTab4() {}
void BTab::_ReservedTab5() {}
void BTab::_ReservedTab6() {}
void BTab::_ReservedTab7() {}
void BTab::_ReservedTab8() {}
void BTab::_ReservedTab9() {}
void BTab::_ReservedTab10() {}
void BTab::_ReservedTab11() {}
void BTab::_ReservedTab12() {}
//------------------------------------------------------------------------------
BTab &BTab::operator=(const BTab &)
{
	return *this;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
BTabView::BTabView(BRect frame, const char *name, button_width width, 
		uint32 resizingMode, uint32 flags)
	:	BView(frame, name, resizingMode, flags),
		fTabWidthSetting(width),
		fTabWidth(0.0f),
		fSelection(-1),
		fInitialSelection(-1),
		fFocus(-1)
{
	fTabList = new BList;

	//font_height fh;
	//GetFontHeight(&fh);
	//fTabHeight = fh.ascent + fh.descent + 8.0f;
	fTabHeight = 20.0f;

	BRect bounds = Bounds();

	bounds.top += fTabHeight + 1.0f;
	bounds.InsetBy ( 2.0f, 2.0f );

	fContainerView = new BView(bounds, "_fContainerView", B_FOLLOW_ALL,
		B_FRAME_EVENTS);
	AddChild(fContainerView);
}
//------------------------------------------------------------------------------
BTabView::~BTabView()
{
	for (int32 i = 0; i < CountTabs(); i++)
		delete TabAt(i);

	delete fTabList;
}
//------------------------------------------------------------------------------
BTabView::BTabView(BMessage *archive)
	:	BView(archive),
		fInitialSelection(-1),
		fFocus(-1)
{
	fTabList = new BList;

	if (archive->FindFloat("_high", &fTabHeight) != B_OK)
	{
		//font_height fh;
		//GetFontHeight(&fh);
		//fTabHeight = fh.ascent + fh.descent + 8.0f;
		fTabHeight = 20.0f;
	}
	
	if (archive->FindInt16("_but_width", (int16*)&fTabWidthSetting) != B_OK)
		fTabWidthSetting = B_WIDTH_AS_USUAL;
		
	if (archive->FindInt32("_sel", &fSelection) != B_OK)
		fSelection = -1;

	int32 i = 0;
	BMessage msg;

	while (archive->FindMessage("_l_items", i++, &msg) == B_OK)
	{
		BTab *tab = dynamic_cast<BTab*>(instantiate_object(&msg));
		
		if (tab)
			AddTab(NULL, tab);
	}

	BRect bounds = Bounds();

	bounds.top += fTabHeight + 1.0f;
	bounds.InsetBy ( 2.0f, 2.0f );

	fContainerView = new BView(bounds, "_fContainerView", B_FOLLOW_ALL,
		B_FRAME_EVENTS);
	AddChild(fContainerView);
}
//------------------------------------------------------------------------------
BArchivable *BTabView::Instantiate(BMessage *archive)
{
	if ( validate_instantiation(archive, "BTabView"))
		return new BTabView(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BTabView::Archive(BMessage *archive, bool deep) const
{
	status_t err = BView::Archive(archive, deep);

	if (err != B_OK)
		return err;

	err = archive->AddFloat("_high", fTabHeight);

	if (err != B_OK)
		return err;

	if (fTabHeight != B_WIDTH_AS_USUAL)
	{
		err = archive->AddInt16("_but_width", fTabWidthSetting);
		
		if (err != B_OK)
			return err;
	}

	if (fSelection != -1)
	{
		err = archive->AddInt32("_sel", fSelection);
		
		if (err != B_OK)
			return err;
	}

	if (deep)
	{
		BMessage tabArchive;
		
		for (int32 i = 0; i < CountTabs(); i++)
			if ((err = TabAt(i)->Archive(&tabArchive, deep)) == B_OK)
				archive->AddMessage("_l_items", &tabArchive);
			else
				return err;
	}

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BTabView::Perform(perform_code d, void *arg)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BTabView::WindowActivated(bool active)
{
	BView::WindowActivated(active);

	DrawTabs();
}
//------------------------------------------------------------------------------
void BTabView::AttachedToWindow()
{
	BView::AttachedToWindow();

	// TODO: check if this color is set in the BeOS implementation
	if (Parent())
      SetViewColor(Parent()->ViewColor ());

	Select(0);
}
//------------------------------------------------------------------------------
void BTabView::AllAttached()
{
	BView::AllAttached();
}
//------------------------------------------------------------------------------
void BTabView::AllDetached()
{
	BView::AllDetached();
}
//------------------------------------------------------------------------------
void BTabView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void BTabView::MessageReceived(BMessage *message)
{
	BView::MessageReceived(message);
}
//------------------------------------------------------------------------------
void BTabView::FrameMoved(BPoint newLocation)
{
	BView::FrameMoved(newLocation);
}
//------------------------------------------------------------------------------
void BTabView::FrameResized(float width,float height)
{
	BView::FrameResized(width, height);
}
//------------------------------------------------------------------------------
void BTabView::KeyDown(const char *bytes, int32 numBytes)
{
	switch (bytes[0])
	{
		case B_DOWN_ARROW:
		case B_LEFT_ARROW:
		{
			SetFocusTab((fFocus - 1) % CountTabs(), true);
			break;
		}
		case B_UP_ARROW:
		case B_RIGHT_ARROW:
		{
			SetFocusTab((fFocus + 1) % CountTabs(), true);
			break;
		}
		case B_RETURN:
		case B_SPACE:
		{
			Select(FocusTab());
			break;
		}
		default:
			BView::KeyDown(bytes, numBytes);
	}
}
//------------------------------------------------------------------------------
void BTabView::MouseDown(BPoint point)
{
	if (point.y <= fTabHeight)
	{
		for (int32 i = 0; i < CountTabs(); i++)
		{
			if (TabFrame(i).Contains(point))
			{
				Select(i);
				return;
			}
		}
	}
	else
		BView::MouseDown(point);
}
//------------------------------------------------------------------------------
void BTabView::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}
//------------------------------------------------------------------------------
void BTabView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BView::MouseMoved(point, transit, message);
}
//------------------------------------------------------------------------------
void BTabView::Pulse()
{
	BView::Pulse();
}
//------------------------------------------------------------------------------
void BTabView::Select(int32 tab)
{
	if (tab < 0 || tab >= CountTabs())
		return;

	if (tab == fSelection)
		return;

	if (fSelection != -1)
		TabAt(fSelection)->Deselect();

	if (tab != -1)
		TabAt(tab)->Select(this);

	fSelection = tab;

	Invalidate();
}
//------------------------------------------------------------------------------
int32 BTabView::Selection() const
{
	return fSelection;
}
//------------------------------------------------------------------------------
void BTabView::MakeFocus(bool focused)
{
	BView::MakeFocus(focused);

	SetFocusTab(Selection(), true);
}
//------------------------------------------------------------------------------
void BTabView::SetFocusTab(int32 tab, bool focused)
{
	if (tab < 0 || tab >= CountTabs())
		return;

	if (focused)
	{
		if (tab == fFocus)
			return;

		if (fFocus != -1)
			TabAt (fFocus)->MakeFocus(false);

		TabAt(tab)->MakeFocus(true);
		fFocus = tab;
	}
	else if (fFocus != -1)
	{
		TabAt (fFocus)->MakeFocus(false);
		fFocus = -1;
	}

	Invalidate();
}
//------------------------------------------------------------------------------
int32 BTabView::FocusTab() const
{
	return fFocus;
}
//------------------------------------------------------------------------------
void BTabView::Draw(BRect updateRect)
{
	DrawBox(DrawTabs());

	if (IsFocus() && fFocus != -1)
		TabAt(fFocus)->DrawFocusMark(this, TabFrame(fFocus));
}
//------------------------------------------------------------------------------
BRect BTabView::DrawTabs()
{
	for(int32 i = 0; i < CountTabs(); i++)
		TabAt(i)->DrawTab(this, TabFrame(i),
			(i == fSelection) ? B_TAB_FRONT : (i == 0) ? B_TAB_FIRST : B_TAB_ANY,
			(i + 1 != fSelection));

	return TabFrame(fSelection);
}
//------------------------------------------------------------------------------
void BTabView::DrawBox(BRect selTabRect)
{
	BRect rect = Bounds();

	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_LIGHTEN_MAX_TINT));

	StrokeLine(BPoint(0, selTabRect.bottom + 1), BPoint(0, rect.bottom));

	if (selTabRect.left > 0.0f)
		StrokeLine(BPoint ( 0, selTabRect.bottom + 1),
			BPoint(selTabRect.left, selTabRect.bottom + 1));

	if (selTabRect.right < rect.right - 2.0f)
		StrokeLine(BPoint(selTabRect.right, selTabRect.bottom + 1),
			BPoint(rect.right - 2, selTabRect.bottom + 1 ));

	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_2_TINT));

	StrokeLine(BPoint(rect.left + 1, rect.bottom - 1),
		BPoint(rect.right - 1, rect.bottom - 1));

	StrokeLine(BPoint(rect.right - 1, selTabRect.bottom + 1),
		BPoint(rect.right - 1, rect.bottom - 2));

	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_4_TINT));

	StrokeLine(BPoint ( rect.left + 1, rect.bottom ),
		BPoint(rect.right - 1, rect.bottom ) );

	StrokeLine(BPoint(rect.right, selTabRect.bottom + 1),
		BPoint(rect.right, rect.bottom));
}
//------------------------------------------------------------------------------
BRect BTabView::TabFrame(int32 tab_index) const
{
	return BRect((float)(tab_index * 94), 0, (float)(tab_index * 94 + 100),
		fTabHeight);
}
//------------------------------------------------------------------------------
void BTabView::SetFlags(uint32 flags)
{
	BView::SetFlags(flags);
}
//------------------------------------------------------------------------------
void BTabView::SetResizingMode(uint32 mode)
{
	BView::SetResizingMode(mode);
}
//------------------------------------------------------------------------------
void BTabView::GetPreferredSize(float *width, float *height)
{
	*width = Bounds().Width();
	*height = Bounds().Height();
}
//------------------------------------------------------------------------------
void BTabView::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	BView::ResizeTo(width, height);
}
//------------------------------------------------------------------------------
BHandler *BTabView::ResolveSpecifier(BMessage *message, int32 index,
							BMessage *specifier, int32 what, const char *property)
{
	return BView::ResolveSpecifier(message, index, specifier, what, property);
}
//------------------------------------------------------------------------------
status_t BTabView::GetSupportedSuites(BMessage *message)
{
	return BView::GetSupportedSuites(message);
}
//------------------------------------------------------------------------------
void BTabView::AddTab(BView *target, BTab *tab)
{
	if (tab == NULL)
		tab = new BTab(target);
	else
		tab->SetView(target);

	fTabList->AddItem(tab);

	// TODO: check if we need to select the new tab
	if (fSelection == -1)
		Select(0);

	Invalidate();
}
//------------------------------------------------------------------------------
BTab *BTabView::RemoveTab(int32 tab_index)
{
	// TODO: check if we need to change the selection and focus
	if (fSelection == CountTabs() - 1)
		Select(fSelection - 1);
	
	if (fFocus == CountTabs() - 1)
		SetFocusTab(fFocus, false);
	else
		SetFocusTab(fFocus, true);

	BTab *tab = (BTab*)fTabList->RemoveItem(tab_index);

	Invalidate();

	return tab;
}
//------------------------------------------------------------------------------
BTab *BTabView::TabAt(int32 tab_index) const
{
	return (BTab*)fTabList->ItemAt(tab_index);
}
//------------------------------------------------------------------------------
void BTabView::SetTabWidth(button_width width)
{
	fTabWidthSetting = width;

	Invalidate();
}
//------------------------------------------------------------------------------
button_width BTabView::TabWidth() const
{
	return fTabWidthSetting;
}
//------------------------------------------------------------------------------
void BTabView::SetTabHeight(float height)
{
	if (fTabHeight == height)
		return;

	fContainerView->ResizeBy(0.0f, height - fTabHeight);

	fTabHeight = height;

	Invalidate();
}
//------------------------------------------------------------------------------
float BTabView::TabHeight() const
{
	return fTabHeight;
}
//------------------------------------------------------------------------------
BView *BTabView::ContainerView() const
{
	return fContainerView;
}
//------------------------------------------------------------------------------
int32 BTabView::CountTabs() const
{
	return fTabList->CountItems();
}
//------------------------------------------------------------------------------
BView *BTabView::ViewForTab(int32 tabIndex) const
{
	BTab *tab = TabAt(tabIndex);

	if (tab)
		return tab->View();
	else
		return NULL;
}
//------------------------------------------------------------------------------
void BTabView::_InitObject()
{
}
//------------------------------------------------------------------------------
void BTabView::_ReservedTabView1() {}
void BTabView::_ReservedTabView2() {}
void BTabView::_ReservedTabView3() {}
void BTabView::_ReservedTabView4() {}
void BTabView::_ReservedTabView5() {}
void BTabView::_ReservedTabView6() {}
void BTabView::_ReservedTabView7() {}
void BTabView::_ReservedTabView8() {}
void BTabView::_ReservedTabView9() {}
void BTabView::_ReservedTabView10() {}
void BTabView::_ReservedTabView11() {}
void BTabView::_ReservedTabView12() {}
//------------------------------------------------------------------------------
BTabView::BTabView(const BTabView &tabView)
	:	BView(tabView)
{
}
//------------------------------------------------------------------------------
BTabView &BTabView::operator=(const BTabView &)
{
	return *this;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
