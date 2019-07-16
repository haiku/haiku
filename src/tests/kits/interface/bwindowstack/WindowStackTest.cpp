/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "WindowStackTest.h"

#include <Alert.h>
#include <Application.h>
#include <ControlLook.h>
#include <Roster.h>
#include <String.h>
#include <Window.h>

#include <WindowStack.h>


const int32 kGetWindows = '&GeW';
const int32 kAddWindow = '&AdW';
const int32 kRemoveWindow = '&ReW';


WindowListItem::WindowListItem(const char* text, BWindow* window)
	:
	BStringItem(text),
	fWindow(window)
{
	
}


MainView::MainView()
	:
	BBox("MainView")
{
	fStackedWindowsLabel = new BStringView("label", "Stacked windows:");
	fStackedWindowsList = new BListView;
	fGetWindowsButton = new BButton("Get Windows", new BMessage(kGetWindows));
	fAddWindowButton = new BButton("Add Window", new BMessage(kAddWindow));
	fRemoveWindowButton = new BButton("Remove Window",
		new BMessage(kRemoveWindow));

	float spacing = be_control_look->DefaultItemSpacing();
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, spacing)
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(fStackedWindowsLabel)
			.AddGlue()
		.End()
			.Add(fStackedWindowsList)
		.AddGroup(B_HORIZONTAL, spacing)
			.AddGlue()
			.Add(fGetWindowsButton)
			.Add(fRemoveWindowButton)
			.Add(fAddWindowButton)
		.End()
		//.SetInsets(spacing, spacing, spacing, spacing)
	);

}


void
MainView::AttachedToWindow()
{
	fGetWindowsButton->SetTarget(this);
	fAddWindowButton->SetTarget(this);
	fRemoveWindowButton->SetTarget(this);
}


void
MainView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kGetWindows:
		{
			BWindowStack windowStack(Window());
			/*BString string;
			string << windowStack.CountWindows();
			BAlert* alert = new BAlert("title", "Count: ", string.String());
			alert->Go();*/
			int32 stackWindowCount = windowStack.CountWindows();
			fStackedWindowsList->MakeEmpty();
			for (int i = 0; i < stackWindowCount; i++) {
				BString result;

				BMessenger messenger;//(NULL, Window());
				windowStack.WindowAt(i, messenger);
				
				// don't deadlock
				if (!messenger.IsTargetLocal()) {
					BMessage message(B_GET_PROPERTY);
					message.AddSpecifier("Title");
					BMessage reply;

					messenger.SendMessage(&message, &reply);
					reply.FindString("result", &result);
				}
				else
					result = Window()->Title();

				fStackedWindowsList->AddItem(new BStringItem(
					result.String()));
			}
			break;
		}

		case kAddWindow:
		{
			app_info appInfo;
			if (be_app->GetAppInfo(&appInfo) != B_OK)
				break;

			team_id team;
			BRoster roster;
			//roster.Launch("application/x-vnd.windowstack_test", (BMessage*)NULL,
			//	&team);
			roster.Launch(&appInfo.ref, (BMessage*)NULL,
				&team);

			BMessage message(B_GET_PROPERTY);
			message.AddSpecifier("Window", int32(0));
			BMessage reply;
			BMessenger appMessenger(NULL, team);
			appMessenger.SendMessage(&message, &reply);

			BMessenger window;
			reply.FindMessenger("result", &window);
			int32 error = 0;
			reply.FindInt32("error", &error);

			BWindowStack windowStack(Window());
			if (windowStack.HasWindow(window)) {
				BAlert* alert = new BAlert("API Error",
					"Window on stack but should not be there!", "OK");
				alert->Go();
			}
			windowStack.AddWindow(window);
			if (!windowStack.HasWindow(window)) {
				BAlert* alert = new BAlert("API Error",
					"Window not on stack but should be there!", "OK");
				alert->Go();
			}
			break;
		}

		case kRemoveWindow:
		{
			BWindowStack windowStack(Window());
			BMessenger messenger;
			windowStack.WindowAt(0, messenger);
			windowStack.RemoveWindow(messenger);
			break;
		}
	}

	BView::MessageReceived(message);
}


int main()
{
	BApplication app("application/x-vnd.windowstack_test");
	BWindow *window = new BWindow(BRect(100, 100, 500, 300),
		"BWindowStackTest", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE);
	window->SetLayout(new BGroupLayout(B_VERTICAL));
	window->AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(new MainView)
		.SetInsets(10, 10, 10, 10)
	);
	
	window->Show();	
	app.Run();
}
