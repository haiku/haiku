/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ActivityWindow.h"

#include <stdio.h>

#include <Application.h>
#include <Catalog.h>
#include <File.h>
#include <FindDirectory.h>
#ifdef __HAIKU__
#include <GroupLayout.h>
#endif
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Roster.h>

#include "ActivityMonitor.h"
#include "ActivityView.h"
#include "DataSource.h"
#include "SettingsWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ActivityWindow"

static const uint32 kMsgAddView = 'advw';
static const uint32 kMsgShowSettings = 'shst';


ActivityWindow::ActivityWindow()
	:
	BWindow(BRect(100, 100, 500, 350), B_TRANSLATE_SYSTEM_NAME("ActivityMonitor"),
	B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
	BMessage settings;
	_LoadSettings(settings);

	BRect frame;
	if (settings.FindRect("window frame", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}

#ifdef __HAIKU__
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL, 0);
	SetLayout(layout);

	// create GUI

	BMenuBar* menuBar = new BMenuBar("menu");
	layout->AddView(menuBar);

	fLayout = new BGroupLayout(B_VERTICAL);
	float inset = ceilf(be_plain_font->Size() * 0.7);
	fLayout->SetInsets(inset, inset, inset, inset);
	fLayout->SetSpacing(inset);

	BView* top = new BView("top", 0, fLayout);
	top->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	layout->AddView(top);

	BMessage viewState;
	int32 count = 0;
	for (int32 i = 0; settings.FindMessage("activity view", i, &viewState)
			== B_OK; i++) {
		ActivityView* view = new ActivityView("ActivityMonitor", &viewState);
		fLayout->AddItem(view->CreateHistoryLayoutItem());
		fLayout->AddItem(view->CreateLegendLayoutItem());
		count++;
	}
	if (count == 0) {
		// Add default views (memory & CPU usage)
		_AddDefaultView();
		_AddDefaultView();
	}
#else	// !__HAIKU__
	BView *layout = new BView(Bounds(), "topmost", B_FOLLOW_NONE, 0);
	AddChild(layout);

	// create GUI
	BRect mbRect(Bounds());
	mbRect.bottom = 10;
	BMenuBar* menuBar = new BMenuBar(mbRect, "menu");
	layout->AddChild(menuBar);

	BRect topRect(Bounds());
	topRect.top = menuBar->Bounds().bottom + 1;

	BView* top = new BView(topRect, "top", B_FOLLOW_ALL, 0);
	top->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	layout->AddChild(top);

	BMessage viewState;
	int32 count = 0;
	ActivityView *aview;
	BRect rect;
	for (int32 i = 0; settings.FindMessage("activity view", i, &viewState)
			== B_OK; i++) {
		aview = new ActivityView("ActivityMonitor", &viewState);
		if (!rect.IsValid())
			rect = aview->Bounds();
		else
			rect.OffsetBySelf(0.0, aview->Bounds().Height());
		top->AddChild(aview);
		count++;
	}
	if (count == 0)
		top->AddChild(new ActivityView("ActivityMonitor", NULL));

#endif
	// add menu

	// "File" menu
	BMenu* menu = new BMenu(B_TRANSLATE("File"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Add graph"),
		new BMessage(kMsgAddView)));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	menu->SetTargetForItems(this);
	menuBar->AddItem(menu);

	// "Settings" menu
	menu = new BMenu(B_TRANSLATE("Settings"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
		new BMessage(kMsgShowSettings)));
	menu->SetTargetForItems(this);
	menuBar->AddItem(menu);
}


ActivityWindow::~ActivityWindow()
{
}


void
ActivityWindow::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		_MessageDropped(message);
		return;
	}

	switch (message->what) {
		case B_REFS_RECEIVED:
		case B_SIMPLE_DATA:
			_MessageDropped(message);
			break;

		case kMsgAddView:
		{
#ifdef __HAIKU__
			BView* firstView = fLayout->View()->ChildAt(0);

			_AddDefaultView();

			if (firstView != NULL)
				ResizeBy(0, firstView->Bounds().Height() + fLayout->Spacing());
#endif
			break;
		}

		case kMsgRemoveView:
		{
#ifdef __HAIKU__
			BView* view;
			if (message->FindPointer("view", (void**)&view) != B_OK)
				break;

			view->RemoveSelf();
			ResizeBy(0, -view->Bounds().Height() - fLayout->Spacing());
			delete view;
#endif
			break;
		}

		case kMsgShowSettings:
		{
			if (fSettingsWindow.IsValid()) {
				// Just bring the window to front (via scripting)
				BMessage toFront(B_SET_PROPERTY);
				toFront.AddSpecifier("Active");
				toFront.AddSpecifier("Window", B_TRANSLATE("Settings"));
				toFront.AddBool("data", true);
				fSettingsWindow.SendMessage(&toFront);
			} else {
				// Open new settings window
				BWindow* window = new SettingsWindow(this);
				window->Show();

				fSettingsWindow = window;
			}
			break;
		}

		case kMsgTimeIntervalUpdated:
			BroadcastToActivityViews(message);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
ActivityWindow::QuitRequested()
{
	_SaveSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


int32
ActivityWindow::ActivityViewCount() const
{
#ifdef __HAIKU__
	return fLayout->View()->CountChildren();
#else
	return 1;
#endif
}


ActivityView*
ActivityWindow::ActivityViewAt(int32 index) const
{
	return dynamic_cast<ActivityView*>(fLayout->View()->ChildAt(index));
}


void
ActivityWindow::BroadcastToActivityViews(BMessage* message, BView* exceptToView)
{
	BView* view;
	for (int32 i = 0; (view = ActivityViewAt(i)) != NULL; i++) {
		if (view != exceptToView)
			PostMessage(message, view);
	}
}


bigtime_t
ActivityWindow::RefreshInterval() const
{
	ActivityView* view = ActivityViewAt(0);
	if (view != 0)
		return view->RefreshInterval();

	return 100000;
}


status_t
ActivityWindow::_OpenSettings(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("ActivityMonitor settings");

	return file.SetTo(path.Path(), mode);
}


status_t
ActivityWindow::_LoadSettings(BMessage& settings)
{
	BFile file;
	status_t status = _OpenSettings(file, B_READ_ONLY);
	if (status < B_OK)
		return status;

	return settings.Unflatten(&file);
}


status_t
ActivityWindow::_SaveSettings()
{
	BFile file;
	status_t status = _OpenSettings(file, B_WRITE_ONLY | B_CREATE_FILE
		| B_ERASE_FILE);
	if (status < B_OK)
		return status;

	BMessage settings('actm');
	status = settings.AddRect("window frame", Frame());
	if (status != B_OK)
		return status;

#ifdef __HAIKU__
	BView* top = fLayout->View();
#else
	BView* top = ChildAt(0);
#endif
	int32 count = top->CountChildren();
	for (int32 i = 0; i < count; i++) {
		ActivityView* view = dynamic_cast<ActivityView*>(top->ChildAt(i));
		if (view == NULL)
			continue;

		BMessage* viewState = new BMessage;
		status = view->SaveState(*viewState);
		if (status == B_OK)
			status = settings.AddMessage("activity view", viewState);
		if (status != B_OK) {
			delete viewState;
			break;
		}
	}

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}


void
ActivityWindow::_AddDefaultView()
{
	BMessage settings;
	settings.AddInt64("refresh interval", RefreshInterval());

	ActivityView* view = new ActivityView("ActivityMonitor", &settings);

	switch (ActivityViewCount()) {
		case 0:
			// The first view defaults to memory usage
			view->AddDataSource(new UsedMemoryDataSource());
			view->AddDataSource(new CachedMemoryDataSource());
			break;
		case 2:
			// The third view defaults to network in/out
			view->AddDataSource(new NetworkUsageDataSource(true));
			view->AddDataSource(new NetworkUsageDataSource(false));
			break;
		case 1:
		default:
			// Everything beyond that defaults to a CPU usage view
			view->AddDataSource(new CPUUsageDataSource());
			break;
	}

	fLayout->AddItem(view->CreateHistoryLayoutItem());
	fLayout->AddItem(view->CreateLegendLayoutItem());
}


void
ActivityWindow::_MessageDropped(BMessage* message)
{
	entry_ref ref;
	if (message->FindRef("refs", &ref) != B_OK) {
		// TODO: If app, then launch it, and add ActivityView for this one?
	}
}

