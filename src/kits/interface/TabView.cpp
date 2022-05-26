/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Jérôme Duval (korli@users.berlios.de)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Artur Wyszynski
 *		Rene Gollent (rene@gollent.com)
 */


#include <TabView.h>
#include <TabViewPrivate.h>

#include <new>

#include <math.h>
#include <string.h>

#include <CardLayout.h>
#include <ControlLook.h>
#include <GroupLayout.h>
#include <LayoutUtils.h>
#include <List.h>
#include <Message.h>
#include <PropertyInfo.h>
#include <Rect.h>
#include <Region.h>
#include <String.h>
#include <Window.h>

#include <binary_compatibility/Support.h>


static property_info sPropertyList[] = {
	{
		"Selection",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_INT32_TYPE }
	},

	{ 0 }
};


BTab::BTab(BView* contentsView)
	:
	fEnabled(true),
	fSelected(false),
	fFocus(false),
	fView(contentsView),
	fTabView(NULL)
{
}


BTab::BTab(BMessage* archive)
	:
	BArchivable(archive),
	fSelected(false),
	fFocus(false),
	fView(NULL),
	fTabView(NULL)
{
	bool disable;

	if (archive->FindBool("_disable", &disable) != B_OK)
		SetEnabled(true);
	else
		SetEnabled(!disable);
}


BTab::~BTab()
{
	if (fView == NULL)
		return;

	if (fSelected)
		fView->RemoveSelf();

	delete fView;
}


BArchivable*
BTab::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BTab"))
		return new BTab(archive);

	return NULL;
}


status_t
BTab::Archive(BMessage* data, bool deep) const
{
	status_t result = BArchivable::Archive(data, deep);
	if (result != B_OK)
		return result;

	if (!fEnabled)
		result = data->AddBool("_disable", false);

	return result;
}


status_t
BTab::Perform(uint32 d, void* arg)
{
	return BArchivable::Perform(d, arg);
}


const char*
BTab::Label() const
{
	if (fView != NULL)
		return fView->Name();
	else
		return NULL;
}


void
BTab::SetLabel(const char* label)
{
	if (label == NULL || fView == NULL)
		return;

	fView->SetName(label);

	if (fTabView != NULL)
		fTabView->Invalidate();
}


bool
BTab::IsSelected() const
{
	return fSelected;
}


void
BTab::Select(BView* owner)
{
	fSelected = true;

	if (owner == NULL || fView == NULL)
		return;

	// NOTE: Views are not added/removed, if there is layout,
	// they are made visible/invisible in that case.
	if (owner->GetLayout() == NULL && fView->Parent() == NULL)
		owner->AddChild(fView);
}


void
BTab::Deselect()
{
	if (fView != NULL) {
		// NOTE: Views are not added/removed, if there is layout,
		// they are made visible/invisible in that case.
		bool removeView = false;
		BView* container = fView->Parent();
		if (container != NULL)
			removeView =
				dynamic_cast<BCardLayout*>(container->GetLayout()) == NULL;
		if (removeView)
			fView->RemoveSelf();
	}

	fSelected = false;
}


void
BTab::SetEnabled(bool enable)
{
	fEnabled = enable;
}


bool
BTab::IsEnabled() const
{
	return fEnabled;
}


void
BTab::MakeFocus(bool focus)
{
	fFocus = focus;
}


bool
BTab::IsFocus() const
{
	return fFocus;
}


void
BTab::SetView(BView* view)
{
	if (view == NULL || fView == view)
		return;

	if (fView != NULL) {
		fView->RemoveSelf();
		delete fView;
	}
	fView = view;

	if (fTabView != NULL && fSelected) {
		Select(fTabView->ContainerView());
		fTabView->Invalidate();
	}
}


BView*
BTab::View() const
{
	return fView;
}


void
BTab::DrawFocusMark(BView* owner, BRect frame)
{
	float width = owner->StringWidth(Label());

	owner->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));

	float offset = IsSelected() ? 3 : 2;
	switch (fTabView->TabSide()) {
		case BTabView::kTopSide:
			owner->StrokeLine(BPoint((frame.left + frame.right - width) / 2.0,
					frame.bottom - offset),
				BPoint((frame.left + frame.right + width) / 2.0,
					frame.bottom - offset));
			break;
		case BTabView::kBottomSide:
			owner->StrokeLine(BPoint((frame.left + frame.right - width) / 2.0,
					frame.top + offset),
				BPoint((frame.left + frame.right + width) / 2.0,
					frame.top + offset));
			break;
		case BTabView::kLeftSide:
			owner->StrokeLine(BPoint(frame.right - offset,
					(frame.top + frame.bottom - width) / 2.0),
				BPoint(frame.right - offset,
					(frame.top + frame.bottom + width) / 2.0));
			break;
		case BTabView::kRightSide:
			owner->StrokeLine(BPoint(frame.left + offset,
					(frame.top + frame.bottom - width) / 2.0),
				BPoint(frame.left + offset,
					(frame.top + frame.bottom + width) / 2.0));
			break;
	}
}


void
BTab::DrawLabel(BView* owner, BRect frame)
{
	float rotation = 0.0f;
	BPoint center(frame.left + frame.Width() / 2,
		frame.top + frame.Height() / 2);
	switch (fTabView->TabSide()) {
		case BTabView::kTopSide:
		case BTabView::kBottomSide:
			rotation = 0.0f;
			break;
		case BTabView::kLeftSide:
			rotation = 270.0f;
			break;
		case BTabView::kRightSide:
			rotation = 90.0f;
			break;
	}

	if (rotation != 0.0f) {
		// DrawLabel doesn't allow rendering rotated text
		// rotate frame first and BAffineTransform will handle the rotation
		// we can't give "unrotated" frame because it comes from
		// BTabView::TabFrame and it is also used to handle mouse clicks
		BRect originalFrame(frame);
		frame.top = center.y - originalFrame.Width() / 2;
		frame.bottom = center.y + originalFrame.Width() / 2;
		frame.left = center.x - originalFrame.Height() / 2;
		frame.right = center.x + originalFrame.Height() / 2;
	}

	BAffineTransform transform;
	transform.RotateBy(center, rotation * M_PI / 180.0f);
	owner->SetTransform(transform);
	be_control_look->DrawLabel(owner, Label(), frame, frame,
		ui_color(B_PANEL_BACKGROUND_COLOR),
		IsEnabled() ? 0 : BControlLook::B_DISABLED,
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
	owner->SetTransform(BAffineTransform());
}


void
BTab::DrawTab(BView* owner, BRect frame, tab_position, bool)
{
	if (fTabView == NULL)
		return;

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 flags = 0;
	uint32 borders = _Borders(owner, frame);

	int32 index = fTabView->IndexOf(this);
	int32 selected = fTabView->Selection();
	int32 first = 0;
	int32 last = fTabView->CountTabs() - 1;

	if (index == selected) {
		be_control_look->DrawActiveTab(owner, frame, frame, base, flags,
			borders, fTabView->TabSide(), index, selected, first, last);
	} else {
		be_control_look->DrawInactiveTab(owner, frame, frame, base, flags,
			borders, fTabView->TabSide(), index, selected, first, last);
	}

	DrawLabel(owner, frame);
}


//	#pragma mark - BTab private methods


uint32
BTab::_Borders(BView* owner, BRect frame)
{
	uint32 borders = 0;
	if (owner == NULL || fTabView == NULL)
		return borders;

	if (fTabView->TabSide() == BTabView::kTopSide
		|| fTabView->TabSide() == BTabView::kBottomSide) {
		borders = BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER;

		if (frame.left == owner->Bounds().left)
			borders |= BControlLook::B_LEFT_BORDER;

		if (frame.right == owner->Bounds().right)
			borders |= BControlLook::B_RIGHT_BORDER;
	} else if (fTabView->TabSide() == BTabView::kLeftSide
		|| fTabView->TabSide() == BTabView::kRightSide) {
		borders = BControlLook::B_LEFT_BORDER | BControlLook::B_RIGHT_BORDER;

		if (frame.top == owner->Bounds().top)
			borders |= BControlLook::B_TOP_BORDER;

		if (frame.bottom == owner->Bounds().bottom)
			borders |= BControlLook::B_BOTTOM_BORDER;
	}

	return borders;
}


//	#pragma mark - FBC padding and private methods


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

BTab &BTab::operator=(const BTab &)
{
	// this is private and not functional, but exported
	return *this;
}


//	#pragma mark - BTabView


BTabView::BTabView(const char* name, button_width width, uint32 flags)
	:
	BView(name, flags)
{
	_InitObject(true, width);
}


BTabView::BTabView(BRect frame, const char* name, button_width width,
	uint32 resizeMask, uint32 flags)
	:
	BView(frame, name, resizeMask, flags)
{
	_InitObject(false, width);
}


BTabView::~BTabView()
{
	for (int32 i = 0; i < CountTabs(); i++)
		delete TabAt(i);

	delete fTabList;
}


BTabView::BTabView(BMessage* archive)
	:
	BView(BUnarchiver::PrepareArchive(archive)),
	fTabList(new BList),
	fContainerView(NULL),
	fFocus(-1)
{
	BUnarchiver unarchiver(archive);

	int16 width;
	if (archive->FindInt16("_but_width", &width) == B_OK)
		fTabWidthSetting = (button_width)width;
	else
		fTabWidthSetting = B_WIDTH_AS_USUAL;

	if (archive->FindFloat("_high", &fTabHeight) != B_OK) {
		font_height fh;
		GetFontHeight(&fh);
		fTabHeight = ceilf(fh.ascent + fh.descent + fh.leading + 8.0f);
	}

	if (archive->FindInt32("_sel", &fSelection) != B_OK)
		fSelection = -1;

	if (archive->FindInt32("_border_style", (int32*)&fBorderStyle) != B_OK)
		fBorderStyle = B_FANCY_BORDER;

	if (archive->FindInt32("_TabSide", (int32*)&fTabSide) != B_OK)
		fTabSide = kTopSide;

	int32 i = 0;
	BMessage tabMsg;

	if (BUnarchiver::IsArchiveManaged(archive)) {
		int32 tabCount;
		archive->GetInfo("_l_items", NULL, &tabCount);
		for (int32 i = 0; i < tabCount; i++) {
			unarchiver.EnsureUnarchived("_l_items", i);
			unarchiver.EnsureUnarchived("_view_list", i);
		}
		return;
	}

	fContainerView = ChildAt(0);
	_InitContainerView(Flags() & B_SUPPORTS_LAYOUT);

	while (archive->FindMessage("_l_items", i, &tabMsg) == B_OK) {
		BArchivable* archivedTab = instantiate_object(&tabMsg);

		if (archivedTab) {
			BTab* tab = dynamic_cast<BTab*>(archivedTab);

			BMessage viewMsg;
			if (archive->FindMessage("_view_list", i, &viewMsg) == B_OK) {
				BArchivable* archivedView = instantiate_object(&viewMsg);
				if (archivedView)
					AddTab(dynamic_cast<BView*>(archivedView), tab);
			}
		}

		tabMsg.MakeEmpty();
		i++;
	}
}


BArchivable*
BTabView::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "BTabView"))
		return new BTabView(archive);

	return NULL;
}


status_t
BTabView::Archive(BMessage* archive, bool deep) const
{
	BArchiver archiver(archive);

	status_t result = BView::Archive(archive, deep);

	if (result == B_OK)
		result = archive->AddInt16("_but_width", fTabWidthSetting);
	if (result == B_OK)
		result = archive->AddFloat("_high", fTabHeight);
	if (result == B_OK)
		result = archive->AddInt32("_sel", fSelection);
	if (result == B_OK && fBorderStyle != B_FANCY_BORDER)
		result = archive->AddInt32("_border_style", fBorderStyle);
	if (result == B_OK && fTabSide != kTopSide)
		result = archive->AddInt32("_TabSide", fTabSide);

	if (result == B_OK && deep) {
		for (int32 i = 0; i < CountTabs(); i++) {
			BTab* tab = TabAt(i);

			if ((result = archiver.AddArchivable("_l_items", tab, deep))
					!= B_OK) {
				break;
			}
			result = archiver.AddArchivable("_view_list", tab->View(), deep);
		}
	}

	return archiver.Finish(result);
}


status_t
BTabView::AllUnarchived(const BMessage* archive)
{
	status_t err = BView::AllUnarchived(archive);
	if (err != B_OK)
		return err;

	fContainerView = ChildAt(0);
	_InitContainerView(Flags() & B_SUPPORTS_LAYOUT);

	BUnarchiver unarchiver(archive);

	int32 tabCount;
	archive->GetInfo("_l_items", NULL, &tabCount);
	for (int32 i = 0; i < tabCount && err == B_OK; i++) {
		BTab* tab;
		err = unarchiver.FindObject("_l_items", i, tab);
		if (err == B_OK && tab) {
			BView* view;
			if ((err = unarchiver.FindObject("_view_list", i,
				BUnarchiver::B_DONT_ASSUME_OWNERSHIP, view)) != B_OK)
				break;

			tab->SetView(view);
			fTabList->AddItem(tab);
		}
	}

	if (err == B_OK)
		Select(fSelection);

	return err;
}


status_t
BTabView::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_ALL_UNARCHIVED:
		{
			perform_data_all_unarchived* data
				= (perform_data_all_unarchived*)_data;

			data->return_value = BTabView::AllUnarchived(data->archive);
			return B_OK;
		}
	}

	return BView::Perform(code, _data);
}


void
BTabView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (fSelection < 0 && CountTabs() > 0)
		Select(0);
}


void
BTabView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BTabView::AllAttached()
{
	BView::AllAttached();
}


void
BTabView::AllDetached()
{
	BView::AllDetached();
}


// #pragma mark -


void
BTabView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			BMessage reply(B_REPLY);
			bool handled = false;

			BMessage specifier;
			int32 index;
			int32 form;
			const char* property;
			if (message->GetCurrentSpecifier(&index, &specifier, &form,
					&property) == B_OK) {
				if (strcmp(property, "Selection") == 0) {
					if (message->what == B_GET_PROPERTY) {
						reply.AddInt32("result", fSelection);
						handled = true;
					} else {
						// B_GET_PROPERTY
						int32 selection;
						if (message->FindInt32("data", &selection) == B_OK) {
							Select(selection);
							reply.AddInt32("error", B_OK);
							handled = true;
						}
					}
				}
			}

			if (handled)
				message->SendReply(&reply);
			else
				BView::MessageReceived(message);
			break;
		}

#if 0
		// TODO this would be annoying as-is, but maybe it makes sense with
		// a modifier or using only deltaX (not the main mouse wheel)
		case B_MOUSE_WHEEL_CHANGED:
		{
			float deltaX = 0.0f;
			float deltaY = 0.0f;
			message->FindFloat("be:wheel_delta_x", &deltaX);
			message->FindFloat("be:wheel_delta_y", &deltaY);

			if (deltaX == 0.0f && deltaY == 0.0f)
				return;

			if (deltaY == 0.0f)
				deltaY = deltaX;

			int32 selection = Selection();
			int32 numTabs = CountTabs();
			if (deltaY > 0  && selection < numTabs - 1) {
				// move to the right tab.
				Select(Selection() + 1);
			} else if (deltaY < 0 && selection > 0 && numTabs > 1) {
				// move to the left tab.
				Select(selection - 1);
			}
			break;
		}
#endif

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
BTabView::KeyDown(const char* bytes, int32 numBytes)
{
	if (IsHidden())
		return;

	switch (bytes[0]) {
		case B_DOWN_ARROW:
		case B_LEFT_ARROW: {
			int32 focus = fFocus - 1;
			if (focus < 0)
				focus = CountTabs() - 1;
			SetFocusTab(focus, true);
			break;
		}

		case B_UP_ARROW:
		case B_RIGHT_ARROW: {
			int32 focus = fFocus + 1;
			if (focus >= CountTabs())
				focus = 0;
			SetFocusTab(focus, true);
			break;
		}

		case B_RETURN:
		case B_SPACE:
			Select(FocusTab());
			break;

		default:
			BView::KeyDown(bytes, numBytes);
	}
}


void
BTabView::MouseDown(BPoint where)
{
	// Which button is pressed?
	uint32 buttons = 0;
	BMessage* currentMessage = Window()->CurrentMessage();
	if (currentMessage != NULL) {
		currentMessage->FindInt32("buttons", (int32*)&buttons);
	}

	int32 selection = Selection();
	int32 numTabs = CountTabs();
	if (buttons & B_MOUSE_BUTTON(4)) {
		// The "back" mouse button moves to previous tab
		if (selection > 0 && numTabs > 1)
			Select(Selection() - 1);
	} else if (buttons & B_MOUSE_BUTTON(5)) {
		// The "forward" mouse button moves to next tab
		if (selection < numTabs - 1)
			Select(Selection() + 1);
	} else {
		// Other buttons are used to select a tab by clicking directly on it
		for (int32 i = 0; i < CountTabs(); i++) {
			if (TabFrame(i).Contains(where)
					&& i != Selection()) {
				Select(i);
				return;
			}
		}
	}

	BView::MouseDown(where);
}


void
BTabView::MouseUp(BPoint where)
{
	BView::MouseUp(where);
}


void
BTabView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	BView::MouseMoved(where, transit, dragMessage);
}


void
BTabView::Pulse()
{
	BView::Pulse();
}


void
BTabView::Select(int32 index)
{
	if (index == Selection())
		return;

	if (index < 0 || index >= CountTabs())
		index = Selection();

	BTab* tab = TabAt(Selection());

	if (tab)
		tab->Deselect();

	tab = TabAt(index);
	if (tab != NULL && fContainerView != NULL) {
		if (index == 0)
			fTabOffset = 0.0f;

		tab->Select(fContainerView);
		fSelection = index;

		// make the view visible through the layout if there is one
		BCardLayout* layout
			= dynamic_cast<BCardLayout*>(fContainerView->GetLayout());
		if (layout != NULL)
			layout->SetVisibleItem(index);
	}

	Invalidate();

	if (index != 0 && !Bounds().Contains(TabFrame(index))){
		if (!Bounds().Contains(TabFrame(index).LeftTop()))
			fTabOffset += TabFrame(index).left - Bounds().left - 20.0f;
		else
			fTabOffset += TabFrame(index).right - Bounds().right + 20.0f;

		Invalidate();
	}

	SetFocusTab(index, true);
}


int32
BTabView::Selection() const
{
	return fSelection;
}


void
BTabView::WindowActivated(bool active)
{
	BView::WindowActivated(active);

	if (IsFocus())
		Invalidate();
}


void
BTabView::MakeFocus(bool focus)
{
	BView::MakeFocus(focus);

	SetFocusTab(Selection(), focus);
}


void
BTabView::SetFocusTab(int32 tab, bool focus)
{
	if (tab >= CountTabs())
		tab = 0;

	if (tab < 0)
		tab = CountTabs() - 1;

	if (focus) {
		if (tab == fFocus)
			return;

		if (fFocus != -1){
			if (TabAt (fFocus) != NULL)
				TabAt(fFocus)->MakeFocus(false);
			Invalidate(TabFrame(fFocus));
		}
		if (TabAt(tab) != NULL){
			TabAt(tab)->MakeFocus(true);
			Invalidate(TabFrame(tab));
			fFocus = tab;
		}
	} else if (fFocus != -1) {
		TabAt(fFocus)->MakeFocus(false);
		Invalidate(TabFrame(fFocus));
		fFocus = -1;
	}
}


int32
BTabView::FocusTab() const
{
	return fFocus;
}


void
BTabView::Draw(BRect updateRect)
{
	DrawTabs();
	DrawBox(TabFrame(fSelection));

	if (IsFocus() && fFocus != -1)
		TabAt(fFocus)->DrawFocusMark(this, TabFrame(fFocus));
}


BRect
BTabView::DrawTabs()
{
	BRect bounds(Bounds());
	BRect tabFrame(bounds);
	uint32 borders = 0;
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	// set tabFrame to area around tabs
	if (fTabSide == kTopSide || fTabSide == kBottomSide) {
		if (fTabSide == kTopSide)
			tabFrame.bottom = fTabHeight;
		else
			tabFrame.top = tabFrame.bottom - fTabHeight;
	} else if (fTabSide == kLeftSide || fTabSide == kRightSide) {
		if (fTabSide == kLeftSide)
			tabFrame.right = fTabHeight;
		else
			tabFrame.left = tabFrame.right - fTabHeight;
	}

	// draw frame behind tabs
	be_control_look->DrawTabFrame(this, tabFrame, bounds, base, 0,
		borders, fBorderStyle, fTabSide);

	// draw the tabs on top of the tab frame
	BRect activeTabFrame;
	int32 tabCount = CountTabs();
	for (int32 i = 0; i < tabCount; i++) {
		BRect tabFrame = TabFrame(i);
		if (i == fSelection)
			activeTabFrame = tabFrame;

		TabAt(i)->DrawTab(this, tabFrame,
			i == fSelection ? B_TAB_FRONT
				: (i == 0) ? B_TAB_FIRST : B_TAB_ANY,
			i != fSelection - 1);
	}

	BRect tabsBounds;
	float last = 0.0f;
	float lastTab = 0.0f;
	if (fTabSide == kTopSide || fTabSide == kBottomSide) {
		lastTab = TabFrame(tabCount - 1).right;
		last = tabFrame.right;
		tabsBounds.left = tabsBounds.right = lastTab;
		borders = BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER;
	} else if (fTabSide == kLeftSide || fTabSide == kRightSide) {
		lastTab = TabFrame(tabCount - 1).bottom;
		last = tabFrame.bottom;
		tabsBounds.top = tabsBounds.bottom = lastTab;
		borders = BControlLook::B_LEFT_BORDER | BControlLook::B_RIGHT_BORDER;
	}

	if (lastTab < last) {
		// draw a 1px right border on the last tab
		be_control_look->DrawInactiveTab(this, tabsBounds, tabsBounds, base, 0,
			borders, fTabSide);
	}

	return fSelection < CountTabs() ? TabFrame(fSelection) : BRect();
}


void
BTabView::DrawBox(BRect selectedTabRect)
{
	BRect rect(Bounds());
	uint32 bordersToDraw = BControlLook::B_ALL_BORDERS;
	switch (fTabSide) {
		case kTopSide:
			bordersToDraw &= ~BControlLook::B_TOP_BORDER;
			rect.top = fTabHeight;
			break;
		case kBottomSide:
			bordersToDraw &= ~BControlLook::B_BOTTOM_BORDER;
			rect.bottom -= fTabHeight;
			break;
		case kLeftSide:
			bordersToDraw &= ~BControlLook::B_LEFT_BORDER;
			rect.left = fTabHeight;
			break;
		case kRightSide:
			bordersToDraw &= ~BControlLook::B_RIGHT_BORDER;
			rect.right -= fTabHeight;
			break;
	}

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	if (fBorderStyle == B_FANCY_BORDER)
		be_control_look->DrawGroupFrame(this, rect, rect, base, bordersToDraw);
	else if (fBorderStyle == B_PLAIN_BORDER) {
		be_control_look->DrawBorder(this, rect, rect, base, B_PLAIN_BORDER,
			0, bordersToDraw);
	} else
		; // B_NO_BORDER draws no box
}


BRect
BTabView::TabFrame(int32 index) const
{
	if (index >= CountTabs() || index < 0)
		return BRect();

	float width = 100.0f;
	float height = fTabHeight;
	float offset = BControlLook::ComposeSpacing(B_USE_WINDOW_SPACING);
	BRect bounds(Bounds());

	switch (fTabWidthSetting) {
		case B_WIDTH_FROM_LABEL:
		{
			float x = 0.0f;
			for (int32 i = 0; i < index; i++){
				x += StringWidth(TabAt(i)->Label()) + 20.0f;
			}

			switch (fTabSide) {
				case kTopSide:
					return BRect(offset + x, 0.0f,
						offset + x + StringWidth(TabAt(index)->Label()) + 20.0f,
						height);
				case kBottomSide:
					return BRect(offset + x, bounds.bottom - height,
						offset + x + StringWidth(TabAt(index)->Label()) + 20.0f,
						bounds.bottom);
				case kLeftSide:
					return BRect(0.0f, offset + x, height, offset + x
						+ StringWidth(TabAt(index)->Label()) + 20.0f);
				case kRightSide:
					return BRect(bounds.right - height, offset + x,
						bounds.right, offset + x
							+ StringWidth(TabAt(index)->Label()) + 20.0f);
				default:
					return BRect();
			}
		}

		case B_WIDTH_FROM_WIDEST:
			width = 0.0;
			for (int32 i = 0; i < CountTabs(); i++) {
				float tabWidth = StringWidth(TabAt(i)->Label()) + 20.0f;
				if (tabWidth > width)
					width = tabWidth;
			}
			// fall through

		case B_WIDTH_AS_USUAL:
		default:
			switch (fTabSide) {
				case kTopSide:
					return BRect(offset + index * width, 0.0f,
						offset + index * width + width, height);
				case kBottomSide:
					return BRect(offset + index * width, bounds.bottom - height,
						offset + index * width + width, bounds.bottom);
				case kLeftSide:
					return BRect(0.0f, offset + index * width, height,
						offset + index * width + width);
				case kRightSide:
					return BRect(bounds.right - height, offset + index * width,
						bounds.right, offset + index * width + width);
				default:
					return BRect();
			}
	}
}


void
BTabView::SetFlags(uint32 flags)
{
	BView::SetFlags(flags);
}


void
BTabView::SetResizingMode(uint32 mode)
{
	BView::SetResizingMode(mode);
}


// #pragma mark -


void
BTabView::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


void
BTabView::GetPreferredSize(float* _width, float* _height)
{
	BView::GetPreferredSize(_width, _height);
}


BSize
BTabView::MinSize()
{
	BSize size;
	if (GetLayout())
		size = GetLayout()->MinSize();
	else {
		size = _TabsMinSize();
		BSize containerSize = fContainerView->MinSize();
		containerSize.width += 2 * _BorderWidth();
		containerSize.height += 2 * _BorderWidth();
		if (containerSize.width > size.width)
			size.width = containerSize.width;
		size.height += containerSize.height;
	}
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
BTabView::MaxSize()
{
	BSize size;
	if (GetLayout())
		size = GetLayout()->MaxSize();
	else {
		size = _TabsMinSize();
		BSize containerSize = fContainerView->MaxSize();
		containerSize.width += 2 * _BorderWidth();
		containerSize.height += 2 * _BorderWidth();
		if (containerSize.width > size.width)
			size.width = containerSize.width;
		size.height += containerSize.height;
	}
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


BSize
BTabView::PreferredSize()
{
	BSize size;
	if (GetLayout() != NULL)
		size = GetLayout()->PreferredSize();
	else {
		size = _TabsMinSize();
		BSize containerSize = fContainerView->PreferredSize();
		containerSize.width += 2 * _BorderWidth();
		containerSize.height += 2 * _BorderWidth();
		if (containerSize.width > size.width)
			size.width = containerSize.width;
		size.height += containerSize.height;
	}
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
}


void
BTabView::FrameMoved(BPoint newPosition)
{
	BView::FrameMoved(newPosition);
}


void
BTabView::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);
}


// #pragma mark -


BHandler*
BTabView::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	BPropertyInfo propInfo(sPropertyList);

	if (propInfo.FindMatch(message, 0, specifier, what, property) >= B_OK)
		return this;

	return BView::ResolveSpecifier(message, index, specifier, what, property);
}


status_t
BTabView::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/vnd.Be-tab-view");

	BPropertyInfo propInfo(sPropertyList);
	message->AddFlat("messages", &propInfo);

	return BView::GetSupportedSuites(message);
}


// #pragma mark -


void
BTabView::AddTab(BView* target, BTab* tab)
{
	if (tab == NULL)
		tab = new BTab(target);
	else
		tab->SetView(target);

	if (fContainerView->GetLayout())
		fContainerView->GetLayout()->AddView(CountTabs(), target);

	fTabList->AddItem(tab);
	BTab::Private(tab).SetTabView(this);

	// When we haven't had a any tabs before, but are already attached to the
	// window, select this one.
	if (CountTabs() == 1 && Window() != NULL)
		Select(0);
}


BTab*
BTabView::RemoveTab(int32 index)
{
	if (index < 0 || index >= CountTabs())
		return NULL;

	BTab* tab = (BTab*)fTabList->RemoveItem(index);
	if (tab == NULL)
		return NULL;

	tab->Deselect();
	BTab::Private(tab).SetTabView(NULL);

	if (fContainerView->GetLayout())
		fContainerView->GetLayout()->RemoveItem(index);

	if (CountTabs() == 0)
		fFocus = -1;
	else if (index <= fSelection)
		Select(fSelection - 1);

	if (fFocus >= 0) {
		if (fFocus == CountTabs() - 1 || CountTabs() == 0)
			SetFocusTab(fFocus, false);
		else
			SetFocusTab(fFocus, true);
	}

	return tab;
}


BTab*
BTabView::TabAt(int32 index) const
{
	return (BTab*)fTabList->ItemAt(index);
}


void
BTabView::SetTabWidth(button_width width)
{
	fTabWidthSetting = width;

	Invalidate();
}


button_width
BTabView::TabWidth() const
{
	return fTabWidthSetting;
}


void
BTabView::SetTabHeight(float height)
{
	if (fTabHeight == height)
		return;

	fTabHeight = height;
	_LayoutContainerView(GetLayout() != NULL);

	Invalidate();
}


float
BTabView::TabHeight() const
{
	return fTabHeight;
}


void
BTabView::SetBorder(border_style borderStyle)
{
	if (fBorderStyle == borderStyle)
		return;

	fBorderStyle = borderStyle;

	_LayoutContainerView((Flags() & B_SUPPORTS_LAYOUT) != 0);
}


border_style
BTabView::Border() const
{
	return fBorderStyle;
}


void
BTabView::SetTabSide(tab_side tabSide)
{
	if (fTabSide == tabSide)
		return;

	fTabSide = tabSide;
	_LayoutContainerView(Flags() & B_SUPPORTS_LAYOUT);
}


BTabView::tab_side
BTabView::TabSide() const
{
	return fTabSide;
}


BView*
BTabView::ContainerView() const
{
	return fContainerView;
}


int32
BTabView::CountTabs() const
{
	return fTabList->CountItems();
}


BView*
BTabView::ViewForTab(int32 tabIndex) const
{
	BTab* tab = TabAt(tabIndex);
	if (tab != NULL)
		return tab->View();

	return NULL;
}


int32
BTabView::IndexOf(BTab* tab) const
{
	if (tab != NULL) {
		int32 tabCount = CountTabs();
		for (int32 index = 0; index < tabCount; index++) {
			if (TabAt(index) == tab)
				return index;
		}
	}

	return -1;
}


void
BTabView::_InitObject(bool layouted, button_width width)
{
	fTabList = new BList;

	fTabWidthSetting = width;
	fSelection = -1;
	fFocus = -1;
	fTabOffset = 0.0f;
	fBorderStyle = B_FANCY_BORDER;
	fTabSide = kTopSide;

	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_PANEL_BACKGROUND_COLOR);

	font_height fh;
	GetFontHeight(&fh);
	fTabHeight = ceilf(fh.ascent + fh.descent + fh.leading + 8.0f);

	fContainerView = NULL;
	_InitContainerView(layouted);
}


void
BTabView::_InitContainerView(bool layouted)
{
	bool needsLayout = false;
	bool createdContainer = false;
	if (layouted) {
		if (GetLayout() == NULL) {
			SetLayout(new(std::nothrow) BGroupLayout(B_HORIZONTAL));
			needsLayout = true;
		}

		if (fContainerView == NULL) {
			fContainerView = new BView("view container", B_WILL_DRAW);
			fContainerView->SetLayout(new(std::nothrow) BCardLayout());
			createdContainer = true;
		}
	} else if (fContainerView == NULL) {
		fContainerView = new BView(Bounds(), "view container", B_FOLLOW_ALL,
			B_WILL_DRAW);
		createdContainer = true;
	}

	if (needsLayout || createdContainer)
		_LayoutContainerView(layouted);

	if (createdContainer) {
		fContainerView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		fContainerView->SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
		AddChild(fContainerView);
	}
}


BSize
BTabView::_TabsMinSize() const
{
	BSize size(0.0f, TabHeight());
	int32 count = min_c(2, CountTabs());
	for (int32 i = 0; i < count; i++) {
		BRect frame = TabFrame(i);
		size.width += frame.Width();
	}

	if (count < CountTabs()) {
		// TODO: Add size for yet to be implemented buttons that allow
		// "scrolling" the displayed tabs left/right.
	}

	return size;
}


float
BTabView::_BorderWidth() const
{
	switch (fBorderStyle) {
		default:
		case B_FANCY_BORDER:
			return 3.0f;

		case B_PLAIN_BORDER:
			return 1.0f;

		case B_NO_BORDER:
			return 0.0f;
	}
}


void
BTabView::_LayoutContainerView(bool layouted)
{
	float borderWidth = _BorderWidth();
	if (layouted) {
		float topBorderOffset;
		switch (fBorderStyle) {
			default:
			case B_FANCY_BORDER:
				topBorderOffset = 1.0f;
				break;

			case B_PLAIN_BORDER:
				topBorderOffset = 0.0f;
				break;

			case B_NO_BORDER:
				topBorderOffset = -1.0f;
				break;
		}
		BGroupLayout* layout = dynamic_cast<BGroupLayout*>(GetLayout());
		if (layout != NULL) {
			float inset = borderWidth + TabHeight() - topBorderOffset;
			switch (fTabSide) {
				case kTopSide:
					layout->SetInsets(borderWidth, inset, borderWidth,
						borderWidth);
					break;
				case kBottomSide:
					layout->SetInsets(borderWidth, borderWidth, borderWidth,
						inset);
					break;
				case kLeftSide:
					layout->SetInsets(inset, borderWidth, borderWidth,
						borderWidth);
					break;
				case kRightSide:
					layout->SetInsets(borderWidth, borderWidth, inset,
						borderWidth);
					break;
			}
		}
	} else {
		BRect bounds = Bounds();
		switch (fTabSide) {
			case kTopSide:
				bounds.top += TabHeight();
				break;
			case kBottomSide:
				bounds.bottom -= TabHeight();
				break;
			case kLeftSide:
				bounds.left += TabHeight();
				break;
			case kRightSide:
				bounds.right -= TabHeight();
				break;
		}
		bounds.InsetBy(borderWidth, borderWidth);

		fContainerView->MoveTo(bounds.left, bounds.top);
		fContainerView->ResizeTo(bounds.Width(), bounds.Height());
	}
}


// #pragma mark - FBC and forbidden


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


BTabView::BTabView(const BTabView& tabView)
	: BView(tabView)
{
	// this is private and not functional, but exported
}


BTabView&
BTabView::operator=(const BTabView&)
{
	// this is private and not functional, but exported
	return *this;
}

//	#pragma mark - binary compatibility


extern "C" void
B_IF_GCC_2(_ReservedTabView1__8BTabView, _ZN8BTabView17_ReservedTabView1Ev)(
	BTabView* tabView, border_style borderStyle)
{
	tabView->BTabView::SetBorder(borderStyle);
}

extern "C" void
B_IF_GCC_2(_ReservedTabView2__8BTabView, _ZN8BTabView17_ReservedTabView2Ev)(
	BTabView* tabView, BTabView::tab_side tabSide)
{
	tabView->BTabView::SetTabSide(tabSide);
}
