/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FileTypes.h"
#include "FileTypesWindow.h"

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <be_apps/Tracker/RecentItems.h>


FileTypesWindow::FileTypesWindow(BRect frame)
	: BWindow(frame, "FileTypes", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	// add the menu

	BMenuBar* menuBar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
	AddChild(menuBar);

	BMenu* menu = new BMenu("File");
	menu->AddItem(new BMenuItem("New Resource File" B_UTF8_ELLIPSIS,
		NULL, 'N', B_COMMAND_KEY));

	BMenu* recentsMenu = BRecentFilesList::NewFileListMenu("Open" B_UTF8_ELLIPSIS,
		NULL, NULL, be_app, 10, false, NULL, kSignature);
	BMenuItem* item = new BMenuItem(recentsMenu, new BMessage(kMsgOpenFilePanel));
	item->SetShortcut('O', B_COMMAND_KEY);
	menu->AddItem(item);
	menu->AddItem(new BMenuItem("Application Types" B_UTF8_ELLIPSIS,
		new BMessage));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("About FileTypes" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
		'Q', B_COMMAND_KEY));
	menu->SetTargetForItems(be_app);
	item->SetTarget(this);
	menuBar->AddItem(menu);

	// MIME Types list

	BRect rect = Bounds();
	rect.top = menuBar->Bounds().Height() + 1.0f;
	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	BButton* button = new BButton(rect, "add", "Add" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveTo(8.0f, topView->Bounds().bottom - 8.0f - button->Bounds().Height());
	topView->AddChild(button);

	rect = button->Frame();
	rect.OffsetBy(rect.Width() + 6.0f, 0.0f);
	button = new BButton(rect, "remove", "Remove", NULL, B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	topView->AddChild(button);

	rect.bottom = rect.top - 10.0f;
	rect.top = 10.0f;
	rect.left = 10.0f;
	rect.right -= B_V_SCROLL_BAR_WIDTH + 2.0f;
	BListView* listView = new BListView(rect, "listview", B_SINGLE_SELECTION_LIST,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	BScrollView* scrollView = new BScrollView("scrollview", listView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	topView->AddChild(scrollView);

	// "Icon" group

	BFont font(be_bold_font);
	float labelWidth = font.StringWidth("Icon");
	font_height fontHeight;
	font.GetHeight(&fontHeight);

	rect.left = rect.right + 12.0f + B_V_SCROLL_BAR_WIDTH;
	rect.right = rect.left + max_c(B_LARGE_ICON, labelWidth) + 32.0f;
	rect.bottom = rect.top + ceilf(fontHeight.ascent)
		+ max_c(B_LARGE_ICON, button->Bounds().Height() * 2.0f + 4.0f) + 12.0f;
	rect.top -= 2.0f;
	BBox* box = new BBox(rect);
	box->SetLabel("Icon");
	topView->AddChild(box);

	BRect innerRect = BRect(0.0f, 0.0f, B_LARGE_ICON - 1.0f, B_LARGE_ICON - 1.0f);
	innerRect.OffsetTo((rect.Width() - innerRect.Width()) / 2.0f,
		(rect.Height() - innerRect.Height()) / 2.0f);
	BBox* iconBox = new BBox(innerRect, "icon box", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_PLAIN_BORDER);
	box->AddChild(iconBox);

	// "File Extensions" group

	BRect rightRect(rect);
	rightRect.left = rect.right + 8.0f;
	rightRect.right = topView->Bounds().Width() - 8.0f;
	box = new BBox(rightRect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("File Extensions");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.left = innerRect.right - button->StringWidth("Remove") - 16.0f;
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	button = new BButton(innerRect, "add ext", "Add" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_RIGHT);
	box->AddChild(button);

	innerRect.OffsetBy(0, innerRect.Height() + 4.0f);
	button = new BButton(innerRect, "remove ext", "Remove", NULL, B_FOLLOW_RIGHT);
	box->AddChild(button);

	innerRect.right = innerRect.left - 10.0f - B_V_SCROLL_BAR_WIDTH;
	innerRect.left = 10.0f;
	innerRect.top = 8.0f + ceilf(fontHeight.ascent);
	innerRect.bottom -= 2.0f;
		// take scrollview border into account
	listView = new BListView(innerRect, "listview ext", B_SINGLE_SELECTION_LIST,
		B_FOLLOW_LEFT_RIGHT);
	scrollView = new BScrollView("scrollview ext", listView,
		B_FOLLOW_LEFT_RIGHT, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	box->AddChild(scrollView);

	// "Description" group

	rect.top = rect.bottom + 8.0f;
	rect.bottom = rect.top + ceilf(fontHeight.ascent) + 19.0f;
	rect.right = rightRect.right;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("Description");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	BTextControl* control = new BTextControl(innerRect, "internal", "Internal Name:", "",
		NULL, B_FOLLOW_LEFT_RIGHT);
	labelWidth = control->StringWidth(control->Label()) + 2.0f;
	control->SetDivider(labelWidth);
	control->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	control->SetEnabled(false);
	box->ResizeBy(0, control->Bounds().Height() * 2.0f);
	box->AddChild(control);

	innerRect.OffsetBy(0, control->Bounds().Height() + 5.0f);
	control = new BTextControl(innerRect, "type", "Type Name:", "",
		NULL, B_FOLLOW_LEFT_RIGHT);
	control->SetDivider(labelWidth);
	control->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	box->AddChild(control);

	// "Preferred Application" group

	rect = box->Frame();
	rect.top = rect.bottom + 8.0f;
	rect.bottom = rect.top + ceilf(fontHeight.ascent)
		+ button->Bounds().Height() + 14.0f;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("Preferred Application");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.left = innerRect.right - button->StringWidth("Same As" B_UTF8_ELLIPSIS) - 24.0f;
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	button = new BButton(innerRect, "same as", "Same As" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_RIGHT);
	box->AddChild(button);

	innerRect.OffsetBy(-innerRect.Width() - 6.0f, 0.0f);
	button = new BButton(innerRect, "select", "Select" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_RIGHT);
	box->AddChild(button);

	menu = new BPopUpMenu("preferred");
	menu->AddItem(item = new BMenuItem("None", NULL));
	item->SetMarked(true);

	innerRect.right = innerRect.left - 6.0f;
	innerRect.left = 8.0f;
	BMenuField* menuField = new BMenuField(innerRect, "preferred", NULL, menu,
		B_FOLLOW_LEFT_RIGHT);
	float width, height;
	menuField->GetPreferredSize(&width, &height);
	menuField->ResizeTo(innerRect.Width(), height);
	menuField->MoveBy(0.0f, (innerRect.Height() - height) / 2.0f);
	box->AddChild(menuField);

	// "Extra Attributes" group

	rect.top = rect.bottom + 8.0f;
	rect.bottom = topView->Bounds().Height() - 8.0f;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM);
	box->SetLabel("Extra Attributes");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.left = innerRect.right - button->StringWidth("Remove") - 16.0f;
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	button = new BButton(innerRect, "add attr", "Add" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_RIGHT);
	box->AddChild(button);

	innerRect.OffsetBy(0, innerRect.Height() + 4.0f);
	button = new BButton(innerRect, "remove attr", "Remove", NULL, B_FOLLOW_RIGHT);
	box->AddChild(button);

	innerRect.right = innerRect.left - 10.0f - B_V_SCROLL_BAR_WIDTH;
	innerRect.left = 10.0f;
	innerRect.top = 8.0f + ceilf(fontHeight.ascent);
	innerRect.bottom = box->Bounds().bottom - 10.0f;
		// take scrollview border into account
	listView = new BListView(innerRect, "listview attr", B_SINGLE_SELECTION_LIST,
		B_FOLLOW_ALL);
	scrollView = new BScrollView("scrollview attr", listView,
		B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	box->AddChild(scrollView);

	SetSizeLimits(rightRect.left + 72.0f + font.StringWidth("File Extensions"), 32767.0f,
		rect.top + 2.0f * button->Bounds().Height() + fontHeight.ascent + 32.0f
		+ menuBar->Bounds().Height(), 32767.0f);
}


FileTypesWindow::~FileTypesWindow()
{
}


void
FileTypesWindow::MessageReceived(BMessage* message)
{
	BWindow::MessageReceived(message);
}


bool
FileTypesWindow::QuitRequested()
{
	be_app->PostMessage(kMsgTypesWindowClosed);
	return true;
}


