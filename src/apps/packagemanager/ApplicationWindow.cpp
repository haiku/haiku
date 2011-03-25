/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2010 Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 *
 * Distributed under the terms of the MIT licence.
 */

#include "ApplicationWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Roster.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>

#include "ApplicationView.h"


enum {
	INIT = 'init',
};


class ApplicationsContainerView : public BGroupView {
public:
	ApplicationsContainerView()
		:
		BGroupView(B_VERTICAL, 0.0)
	{
		SetFlags(Flags() | B_PULSE_NEEDED);
		SetViewColor(245, 245, 245);
		AddChild(BSpaceLayoutItem::CreateGlue());
	}

	virtual BSize MinSize()
	{
		BSize minSize = BGroupView::MinSize();
		return BSize(minSize.width, 80);
	}

protected:
	virtual void DoLayout()
	{
		BGroupView::DoLayout();
		if (BScrollBar* scrollBar = ScrollBar(B_VERTICAL)) {
			BSize minSize = BGroupView::MinSize();
			float height = Bounds().Height();
			float max = minSize.height - height;
			scrollBar->SetRange(0, max);
			if (minSize.height > 0)
				scrollBar->SetProportion(height / minSize.height);
			else
				scrollBar->SetProportion(1);
		}
	}
};


class ApplicationContainerScrollView : public BScrollView {
public:
	ApplicationContainerScrollView(BView* target)
		:
		BScrollView("Applications scroll view", target, 0, false, true,
			B_NO_BORDER)
	{
	}

protected:
	virtual void DoLayout()
	{
		BScrollView::DoLayout();
		// Tweak scroll bar layout to hide part of the frame for better looks.
		BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
		scrollBar->MoveBy(1, -1);
		scrollBar->ResizeBy(0, 2);
		Target()->ResizeBy(1, 0);
		// Set the scroll steps
		if (BView* item = Target()->ChildAt(0)) {
			scrollBar->SetSteps(item->MinSize().height + 1,
				item->MinSize().height + 1);
		}
	}
};


// #pragma mark -


ApplicationWindow::ApplicationWindow(BRect frame, bool visible)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("PackageManager"),
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE),
	fMinimizeOnClose(false)
{
	SetPulseRate(1000000);

	SetLayout(new BGroupLayout(B_VERTICAL, 0.0));

	ApplicationsContainerView* downloadsGroupView = new ApplicationsContainerView();
	fApplicationViewsLayout = downloadsGroupView->GroupLayout();

	BMenuBar* menuBar = new BMenuBar("Menu bar");
	BMenu* menu = new BMenu("Actions");
	menu->AddItem(new BMenuItem("Apply changes",
		new BMessage('NADA')));
	menu->AddItem(new BMenuItem("Exit", new BMessage(B_QUIT_REQUESTED)));
	menuBar->AddItem(menu);

	fApplicationsScrollView = new ApplicationContainerScrollView(downloadsGroupView);

	fDiscardButton = new BButton("Revert",
		new BMessage('NADA'));
	fDiscardButton->SetEnabled(false);

	fApplyChangesButton = new BButton("Apply",
		new BMessage('NADA'));
	fApplyChangesButton->SetEnabled(false);

	const float spacing = be_control_look->DefaultItemSpacing();

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0.0)
		.Add(menuBar)
		.Add(fApplicationsScrollView)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, spacing)
			.AddGlue()
			.Add(fApplyChangesButton)
			.Add(fDiscardButton)
			.SetInsets(12, 5, 12, 5)
		)
	);

	PostMessage(INIT);

	if (!visible)
		Hide();
	Show();
}


ApplicationWindow::~ApplicationWindow()
{
}


void
ApplicationWindow::DispatchMessage(BMessage* message, BHandler* target)
{
	// We need to intercept mouse down events inside the area of download
	// progress views (regardless of whether they have children at the click),
	// so that they may display a context menu.
	BPoint where;
	int32 buttons;
	if (message->what == B_MOUSE_DOWN
		&& message->FindPoint("screen_where", &where) == B_OK
		&& message->FindInt32("buttons", &buttons) == B_OK
		&& (buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		for (int32 i = fApplicationViewsLayout->CountItems() - 1;
				BLayoutItem* item = fApplicationViewsLayout->ItemAt(i); i--) {
			ApplicationView* view = dynamic_cast<ApplicationView*>(
				item->View());
			if (!view)
				continue;
			BPoint viewWhere(where);
			view->ConvertFromScreen(&viewWhere);
			if (view->Bounds().Contains(viewWhere)) {
				view->ShowContextMenu(where);
				return;
			}
		}
	}
	BWindow::DispatchMessage(message, target);
}


void
ApplicationWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case INIT:
		{
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
ApplicationWindow::QuitRequested()
{
	if (fMinimizeOnClose) {
		if (!IsMinimized())
			Minimize(true);
	} else {
		if (!IsHidden())
			Hide();
	}
	return false;
}


void
ApplicationWindow::SetMinimizeOnClose(bool minimize)
{
	if (Lock()) {
		fMinimizeOnClose = minimize;
		Unlock();
	}
}


// #pragma mark - private


void
ApplicationWindow::AddCategory(const char* name, const char* icon,
	const char* description)
{
	ApplicationView* category = new ApplicationView(name, icon, description);
	fApplicationViewsLayout->AddView(0, category);
}


void
ApplicationWindow::AddApplication(const BMessage* info)
{
	ApplicationView* app = new ApplicationView(info);
	fApplicationViewsLayout->AddView(0, app);
}


void
ApplicationWindow::_ValidateButtonStatus()
{
	int32 finishedCount = 0;
	int32 missingCount = 0;
	for (int32 i = fApplicationViewsLayout->CountItems() - 1;
			BLayoutItem* item = fApplicationViewsLayout->ItemAt(i); i--) {
		ApplicationView* view = dynamic_cast<ApplicationView*>(
			item->View());
		if (!view)
			continue;
	}
	fDiscardButton->SetEnabled(finishedCount > 0);
	fApplyChangesButton->SetEnabled(missingCount > 0);
}

