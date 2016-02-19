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

#include <binary_compatibility/Support.h>


using std::nothrow;


static property_info sPropertyList[] = {
	{
		"Selection",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_INT32_TYPE }
	},

	{}
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
	owner->StrokeLine(BPoint((frame.left + frame.right - width) / 2.0,
			frame.bottom - offset),
		BPoint((frame.left + frame.right + width) / 2.0,
			frame.bottom - offset));
}


void
BTab::DrawLabel(BView* owner, BRect frame)
{
	be_control_look->DrawLabel(owner, Label(), frame, frame,
		ui_color(B_PANEL_BACKGROUND_COLOR),
		IsEnabled() ? 0 : BPrivate::BControlLook::B_DISABLED,
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
}


void
BTab::DrawTab(BView* owner, BRect frame, tab_position position, bool full)
{
	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 borders = BControlLook::B_TOP_BORDER
		| BControlLook::B_BOTTOM_BORDER;

	if (frame.left == owner->Bounds().left)
		borders |= BControlLook::B_LEFT_BORDER;

	if (frame.right == owner->Bounds().right)
		borders |= BControlLook::B_RIGHT_BORDER;

	if (position == B_TAB_FRONT) {
		frame.bottom += 1;
		be_control_look->DrawActiveTab(owner, frame, frame, no_tint, 0,
			borders);
	} else {
		be_control_look->DrawInactiveTab(owner, frame, frame, no_tint, 0,
			borders);
	}

	DrawLabel(owner, frame);
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
		fTabHeight = fh.ascent + fh.descent + fh.leading + 8.0f;
	}

	if (archive->FindInt32("_sel", &fSelection) != B_OK)
		fSelection = -1;

	if (archive->FindInt32("_border_style", (int32*)&fBorderStyle) != B_OK)
		fBorderStyle = B_FANCY_BORDER;

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

	if (result == B_OK && deep) {
		for (int32 i = 0; i < CountTabs(); i++) {
			BTab* tab = TabAt(i);

			if ((result = archiver.AddArchivable("_l_items", tab, deep)) != B_OK)
				break;
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
			if (message->GetCurrentSpecifier(&index, &specifier, &form, &property) == B_OK) {
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
	if (where.y > fTabHeight)
		return;

	for (int32 i = 0; i < CountTabs(); i++) {
		if (TabFrame(i).Contains(where)
			&& i != Selection()) {
			Select(i);
			return;
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
	DrawBox(TabFrame(fSelection));
	DrawTabs();

	if (IsFocus() && fFocus != -1)
		TabAt(fFocus)->DrawFocusMark(this, TabFrame(fFocus));
}


BRect
BTabView::DrawTabs()
{
	float left = 0;

	for (int32 i = 0; i < CountTabs(); i++) {
		BRect tabFrame = TabFrame(i);
		TabAt(i)->DrawTab(this, tabFrame,
			i == fSelection ? B_TAB_FRONT : (i == 0) ? B_TAB_FIRST : B_TAB_ANY,
			i + 1 != fSelection);
		left = tabFrame.right;
	}

	BRect frame(Bounds());
	if (fBorderStyle == B_PLAIN_BORDER)
		frame.right += 1;
	else if (fBorderStyle == B_NO_BORDER)
		frame.right += 2;
	if (left < frame.right) {
		frame.left = left;
		frame.bottom = fTabHeight;
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		uint32 borders = BControlLook::B_TOP_BORDER
			| BControlLook::B_BOTTOM_BORDER | BControlLook::B_RIGHT_BORDER;
		if (left == 0)
			borders |= BControlLook::B_LEFT_BORDER;
		be_control_look->DrawInactiveTab(this, frame, frame, base, 0,
			borders);
	}
	if (fBorderStyle == B_NO_BORDER) {
		// Draw a small inactive area before first tab.
		frame = Bounds();
		frame.right = 0.0f;
			// one pixel wide
		frame.bottom = fTabHeight;
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		uint32 borders = BControlLook::B_TOP_BORDER
			| BControlLook::B_BOTTOM_BORDER;
		be_control_look->DrawInactiveTab(this, frame, frame, base, 0,
			borders);
	}

	if (fSelection < CountTabs())
		return TabFrame(fSelection);

	return BRect();
}


void
BTabView::DrawBox(BRect selTabRect)
{
	BRect rect(Bounds());
	rect.top = selTabRect.bottom;
	if (fBorderStyle != B_FANCY_BORDER)
		rect.top += 1.0f;
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	if (fBorderStyle == B_FANCY_BORDER)
		be_control_look->DrawGroupFrame(this, rect, rect, base);
	else {
		uint32 borders = BControlLook::B_TOP_BORDER;
		if (fBorderStyle == B_PLAIN_BORDER)
			borders = BControlLook::B_ALL_BORDERS;
		be_control_look->DrawBorder(this, rect, rect, base, B_PLAIN_BORDER,
			0, borders);
	}
}


BRect
BTabView::TabFrame(int32 index) const
{
	if (index >= CountTabs() || index < 0)
		return BRect();

	float width = 100.0f;
	float height = fTabHeight;
	float borderOffset = 0.0f;
	// Do not use 2.0f for B_NO_BORDER, that will look yet different
	// again (handled in DrawTabs()).
	if (fBorderStyle == B_PLAIN_BORDER)
		borderOffset = 1.0f;
	switch (fTabWidthSetting) {
		case B_WIDTH_FROM_LABEL:
		{
			float x = 0.0f;
			for (int32 i = 0; i < index; i++){
				x += StringWidth(TabAt(i)->Label()) + 20.0f;
			}

			return BRect(x - borderOffset, 0.0f,
				x + StringWidth(TabAt(index)->Label()) + 20.0f
					- borderOffset,
				height);
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
			return BRect(index * width - borderOffset, 0.0f,
				index * width + width - borderOffset, height);
	}

	// TODO: fix to remove "offset" in DrawTab and DrawLabel ...
	switch (fTabWidthSetting) {
		case B_WIDTH_FROM_LABEL:
		{
			float x = 6.0f;
			for (int32 i = 0; i < index; i++){
				x += StringWidth(TabAt(i)->Label()) + 20.0f;
			}

			return BRect(x - fTabOffset, 0.0f,
				x - fTabOffset + StringWidth(TabAt(index)->Label()) + 20.0f,
				fTabHeight);
		}

		case B_WIDTH_FROM_WIDEST:
		{
			float width = 0.0f;

			for (int32 i = 0; i < CountTabs(); i++) {
				float tabWidth = StringWidth(TabAt(i)->Label()) + 20.0f;
				if (tabWidth > width)
					width = tabWidth;
			}
			return BRect((6.0f + index * width) - fTabOffset, 0.0f,
				(6.0f + index * width + width) - fTabOffset, fTabHeight);
		}

		case B_WIDTH_AS_USUAL:
		default:
			return BRect((6.0f + index * 100.0f) - fTabOffset, 0.0f,
				(6.0f + index * 100.0f + 100.0f) - fTabOffset, fTabHeight);
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

	if (fFocus == CountTabs() - 1 || CountTabs() == 0)
		SetFocusTab(fFocus, false);
	else
		SetFocusTab(fFocus, true);

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
BTabView::SetBorder(border_style border)
{
	if (fBorderStyle == border)
		return;

	fBorderStyle = border;

	_LayoutContainerView((Flags() & B_SUPPORTS_LAYOUT) != 0);
}


border_style
BTabView::Border() const
{
	return fBorderStyle;
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


void
BTabView::_InitObject(bool layouted, button_width width)
{
	fTabList = new BList;

	fTabWidthSetting = width;
	fSelection = -1;
	fFocus = -1;
	fTabOffset = 0.0f;
	fBorderStyle = B_FANCY_BORDER;

	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_PANEL_BACKGROUND_COLOR);

	font_height fh;
	GetFontHeight(&fh);
	fTabHeight = fh.ascent + fh.descent + fh.leading + 8.0f;

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
			SetLayout(new(nothrow) BGroupLayout(B_HORIZONTAL));
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
			layout->SetInsets(borderWidth, borderWidth + TabHeight()
				- topBorderOffset, borderWidth, borderWidth);
		}
	} else {
		BRect bounds = Bounds();

		bounds.top += TabHeight();
		bounds.InsetBy(borderWidth, borderWidth);

		fContainerView->MoveTo(bounds.left, bounds.top);
		fContainerView->ResizeTo(bounds.Width(), bounds.Height());
	}
}


// #pragma mark - FBC and forbidden


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
	BTabView* tabView, border_style border)
{
	tabView->BTabView::SetBorder(border);
}
