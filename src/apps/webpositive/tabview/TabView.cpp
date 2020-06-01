/*
 * Copyright (C) 2010 Rene Gollent <rene@gollent.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TabView.h"

#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <CardLayout.h>
#include <ControlLook.h>
#include <GroupView.h>
#include <SpaceLayoutItem.h>
#include <TabView.h>
#include <Window.h>

#include "TabContainerView.h"


// #pragma mark - TabView


TabView::TabView()
	:
	fContainerView(NULL),
	fLayoutItem(new TabLayoutItem(this)),
	fLabel()
{
}


TabView::~TabView()
{
	// The layout item is deleted for us by the layout which contains it.
	if (fContainerView == NULL)
		delete fLayoutItem;
}


BSize
TabView::MinSize()
{
	BSize size(MaxSize());
	size.width = 60.0f;
	return size;
}


BSize
TabView::PreferredSize()
{
	return MaxSize();
}


BSize
TabView::MaxSize()
{
	float extra = be_control_look->DefaultLabelSpacing();
	float labelWidth = 300.0f;
	return BSize(labelWidth, _LabelHeight() + extra);
}


void
TabView::Draw(BRect updateRect)
{
	BRect frame(fLayoutItem->Frame());
	frame.right++;
	frame.bottom++;

	int32 index = fContainerView->IndexOf(this);

	// make room for tail of last tab
	bool isLast = index == fContainerView->LastTabIndex();
	if (isLast)
		frame.right -= 2;

	DrawBackground(fContainerView, frame, updateRect);

	bool isFront = index == fContainerView->SelectedTabIndex();
	if (isFront)
		frame.top += 3.0f;
	else
		frame.top += 6.0f;

	float spacing = be_control_look->DefaultLabelSpacing();
	frame.InsetBy(spacing, spacing / 2);
	DrawContents(fContainerView, frame, updateRect);
}


void
TabView::DrawBackground(BView* owner, BRect frame, const BRect& updateRect)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 flags = 0;
	uint32 borders = BControlLook::B_TOP_BORDER
		| BControlLook::B_BOTTOM_BORDER;

	int32 index = fContainerView->IndexOf(this);
	int32 selected = fContainerView->SelectedTabIndex();
	int32 first = fContainerView->FirstTabIndex();
	int32 last = fContainerView->LastTabIndex();

	if (index == selected) {
		be_control_look->DrawActiveTab(owner, frame, updateRect, base, flags,
			borders, BControlLook::B_TOP_BORDER, index, selected, first, last);
	} else {
		be_control_look->DrawInactiveTab(owner, frame, updateRect, base, flags,
			borders, BControlLook::B_TOP_BORDER, index, selected, first, last);
	}
}


void
TabView::DrawContents(BView* owner, BRect frame, const BRect& updateRect)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	be_control_look->DrawLabel(owner, fLabel.String(), frame, updateRect,
		base, 0, BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
}


void
TabView::MouseDown(BPoint where, uint32 buttons)
{
	fContainerView->SelectTab(this);
}


void
TabView::MouseUp(BPoint where)
{
}


void
TabView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
}


void
TabView::Update()
{
	fLayoutItem->InvalidateContainer();
}


void
TabView::SetContainerView(TabContainerView* containerView)
{
	fContainerView = containerView;
}


TabContainerView*
TabView::ContainerView() const
{
	return fContainerView;
}


BLayoutItem*
TabView::LayoutItem() const
{
	return fLayoutItem;
}


void
TabView::SetLabel(const char* label)
{
	if (fLabel == label)
		return;

	fLabel = label;
	fLayoutItem->InvalidateLayout();
}


const BString&
TabView::Label() const
{
	return fLabel;
}


BRect
TabView::Frame() const
{
	return fLayoutItem->Frame();
}


float
TabView::_LabelHeight() const
{
	font_height fontHeight;
	fContainerView->GetFontHeight(&fontHeight);
	return ceilf(fontHeight.ascent) + ceilf(fontHeight.descent);
}


// #pragma mark - TabLayoutItem


TabLayoutItem::TabLayoutItem(TabView* parent)
	:
	fParent(parent),
	fVisible(true)
{
}


bool
TabLayoutItem::IsVisible()
{
	return fVisible;
}


void
TabLayoutItem::SetVisible(bool visible)
{
	if (fVisible == visible)
		return;

	fVisible = visible;

	InvalidateContainer();
	fParent->ContainerView()->InvalidateLayout();
}


BRect
TabLayoutItem::Frame()
{
	return fFrame;
}


void
TabLayoutItem::SetFrame(BRect frame)
{
	BRect dirty = fFrame;
	fFrame = frame;
	dirty = dirty | fFrame;
	InvalidateContainer(dirty);
}


BView*
TabLayoutItem::View()
{
	return NULL;
}


BSize
TabLayoutItem::BaseMinSize()
{
	return fParent->MinSize();
}


BSize
TabLayoutItem::BaseMaxSize()
{
	return fParent->MaxSize();
}


BSize
TabLayoutItem::BasePreferredSize()
{
	return fParent->PreferredSize();
}


BAlignment
TabLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


TabView*
TabLayoutItem::Parent() const
{
	return fParent;
}


void
TabLayoutItem::InvalidateContainer()
{
	InvalidateContainer(Frame());
}


void
TabLayoutItem::InvalidateContainer(BRect frame)
{
	// Invalidate more than necessary, to help the TabContainerView
	// redraw the parts outside any tabs... need 2px
	frame.bottom += 2;
	frame.right += 2;
	fParent->ContainerView()->Invalidate(frame);
}
