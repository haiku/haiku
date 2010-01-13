/*
 * Copyright 2005-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>
#include <Window.h>

#include <WindowPrivate.h>

#include <stdio.h>
#include <string.h>


const uint32 kMsgUpdateLook = 'uplk';
const uint32 kMsgUpdateFeel = 'upfe';
const uint32 kMsgUpdateFlags = 'upfl';

const uint32 kMsgAddWindow = 'adwn';
const uint32 kMsgAddSubsetWindow = 'adsw';


int32 gNormalWindowCount = 0;


class Window : public BWindow {
	public:
		Window(BRect frame, window_look look, window_feel feel);
		virtual ~Window();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

	private:
		BMessage* AddWindowMessage(window_look look, window_feel feel);
		BString TitleForFeel(window_feel feel);
		void _UpdateFlagsMenuLabel();

		BMenuField*	fFlagsField;
};


Window::Window(BRect frame, window_look look, window_feel feel)
	: BWindow(frame, TitleForFeel(feel).String(), look, feel,
			B_ASYNCHRONOUS_CONTROLS)
{
	BRect rect(Bounds());
	BView *view = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	if (!IsModal() && !IsFloating())
		gNormalWindowCount++;

	float labelWidth = view->StringWidth("Flags:") + 5.f;

	BPopUpMenu* menu = new BPopUpMenu("looks");
	const struct { const char* name; int32 look; } looks[] = {
		{"Titled", B_TITLED_WINDOW_LOOK}, {"Document", B_DOCUMENT_WINDOW_LOOK},
		{"Floating", B_FLOATING_WINDOW_LOOK}, {"Modal", B_MODAL_WINDOW_LOOK},
		{"Bordered", B_BORDERED_WINDOW_LOOK}, {"No Border", B_NO_BORDER_WINDOW_LOOK},
		{"Left Titled", kLeftTitledWindowLook}, {"Desktop", kDesktopWindowLook}
	};
	for (uint32 i = 0; i < sizeof(looks) / sizeof(looks[0]); i++) {
		BMessage* message = new BMessage(kMsgUpdateLook);
		message->AddInt32("look", looks[i].look);
		BMenuItem* item = new BMenuItem(looks[i].name, message);
		if (looks[i].look == (int32)Look())
			item->SetMarked(true);

		menu->AddItem(item);
	}

	rect.InsetBy(10, 10);
	BMenuField* menuField = new BMenuField(rect, "look", "Look:", menu);
	menuField->ResizeToPreferred();
	menuField->SetDivider(labelWidth);
	view->AddChild(menuField);

	menu = new BPopUpMenu("feels");
	const struct { const char* name; int32 feel; } feels[] = {
		{"Normal", B_NORMAL_WINDOW_FEEL},
		{"Modal Subset", B_MODAL_SUBSET_WINDOW_FEEL},
		{"App Modal", B_MODAL_APP_WINDOW_FEEL},
		{"All Modal", B_MODAL_ALL_WINDOW_FEEL},
		{"Floating Subset", B_FLOATING_SUBSET_WINDOW_FEEL},
		{"App Floating", B_FLOATING_APP_WINDOW_FEEL},
		{"All Floating", B_FLOATING_ALL_WINDOW_FEEL},
		{"Menu", kMenuWindowFeel},
		{"WindowScreen", kWindowScreenFeel},
		{"Desktop", kDesktopWindowFeel},
	};
	for (uint32 i = 0; i < sizeof(feels) / sizeof(feels[0]); i++) {
		BMessage* message = new BMessage(kMsgUpdateFeel);
		message->AddInt32("feel", feels[i].feel);
		BMenuItem* item = new BMenuItem(feels[i].name, message);
		if (feels[i].feel == (int32)Feel())
			item->SetMarked(true);

		menu->AddItem(item);
	}

	rect.OffsetBy(0, menuField->Bounds().Height() + 10);
	menuField = new BMenuField(rect, "feel", "Feel:", menu);
	menuField->ResizeToPreferred();
	menuField->SetDivider(labelWidth);
	view->AddChild(menuField);

	menu = new BPopUpMenu("none", false, false);
	const struct { const char* name; uint32 flag; } flags[] = {
		{"Not Zoomable", B_NOT_ZOOMABLE},
		{"Not Closable", B_NOT_CLOSABLE},
		{"Not Movable", B_NOT_MOVABLE},
		{"Not Horizontally Resizable", B_NOT_H_RESIZABLE},
		{"Not Vertically Resizable", B_NOT_V_RESIZABLE},
		{"Outline Resize", B_OUTLINE_RESIZE},
		{"Accept First Click", B_WILL_ACCEPT_FIRST_CLICK},
		{"Not Anchored On Activate", B_NOT_ANCHORED_ON_ACTIVATE},
		{"Avoid Front", B_AVOID_FRONT},
#if defined(__HAIKU__) || defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST)
		{"Same Position In All Workspaces", B_SAME_POSITION_IN_ALL_WORKSPACES},
#endif
	};
	for (uint32 i = 0; i < sizeof(flags) / sizeof(flags[0]); i++) {
		BMessage* message = new BMessage(kMsgUpdateFlags);
		message->AddInt32("flag", flags[i].flag);
		BMenuItem* item = new BMenuItem(flags[i].name, message);

		menu->AddItem(item);
	}

	rect.OffsetBy(0, menuField->Bounds().Height() + 10);
	fFlagsField = new BMenuField(rect, "flags", "Flags:", menu);
	fFlagsField->ResizeToPreferred();
	fFlagsField->SetDivider(labelWidth);
	view->AddChild(fFlagsField);

	// normal

	rect.OffsetBy(0, menuField->Bounds().Height() + 10);
	BButton* button = new BButton(rect, "normal", "Add Normal Window",
		AddWindowMessage(B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL),
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	float width, height;
	button->GetPreferredSize(&width, &height);
	button->ResizeTo(rect.Width(), height);
	view->AddChild(button);

	// modal

	rect = button->Frame();
	rect.OffsetBy(0, rect.Height() + 5);
	button = new BButton(rect, "modal_subset", "Add Modal Subset",
		AddWindowMessage(B_MODAL_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL),
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	view->AddChild(button);

	rect.OffsetBy(0, rect.Height() + 5);
	button = new BButton(rect, "app_modal", "Add Application Modal",
		AddWindowMessage(B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL),
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	view->AddChild(button);

	rect.OffsetBy(0, rect.Height() + 5);
	button = new BButton(rect, "all_modal", "Add All Modal",
		AddWindowMessage(B_MODAL_WINDOW_LOOK, B_MODAL_ALL_WINDOW_FEEL),
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	view->AddChild(button);

	// floating

	rect = button->Frame();
	rect.OffsetBy(0, rect.Height() + 5);
	button = new BButton(rect, "floating_subset", "Add Floating Subset",
		AddWindowMessage(B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL),
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	view->AddChild(button);

	rect.OffsetBy(0, rect.Height() + 5);
	button = new BButton(rect, "app_floating", "Add Application Floating",
		AddWindowMessage(B_FLOATING_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL),
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	view->AddChild(button);

	rect.OffsetBy(0, rect.Height() + 5);
	button = new BButton(rect, "all_floating", "Add All Floating",
		AddWindowMessage(B_FLOATING_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL),
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	view->AddChild(button);

	// close

	rect.OffsetBy(0, rect.Height() + 15);
	button = new BButton(rect, "close", "Close Window",
		new BMessage(B_QUIT_REQUESTED), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	button->ResizeToPreferred();
	button->MoveTo((rect.Width() - button->Bounds().Width()) / 2, rect.top);
	view->AddChild(button);

	ResizeTo(Bounds().Width(), button->Frame().bottom + 10);
}


Window::~Window()
{
}


void
Window::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUpdateLook:
		{
			int32 look;
			if (message->FindInt32("look", &look) != B_OK)
				break;

			SetLook((window_look)look);
			break;
		}

		case kMsgUpdateFeel:
		{
			int32 feel;
			if (message->FindInt32("feel", &feel) != B_OK)
				break;

			if (!IsModal() && !IsFloating())
				gNormalWindowCount--;

			SetFeel((window_feel)feel);
			SetTitle(TitleForFeel((window_feel)feel).String());

			if (!IsModal() && !IsFloating())
				gNormalWindowCount++;
			break;
		}

		case kMsgUpdateFlags:
		{
			uint32 flag;
			if (message->FindInt32("flag", (int32*)&flag) != B_OK)
				break;

			BMenuItem* item;
			if (message->FindPointer("source", (void**)&item) != B_OK)
				break;

			item->SetMarked(!item->IsMarked());

			uint32 flags = Flags();
			if (item->IsMarked())
				flags |= flag;
			else
				flags &= ~flag;

			SetFlags(flags);
			_UpdateFlagsMenuLabel();
			break;
		}

		case kMsgAddWindow:
		case kMsgAddSubsetWindow:
		{
			int32 look, feel;
			if (message->FindInt32("look", &look) != B_OK
				|| message->FindInt32("feel", &feel) != B_OK)
				break;

			BWindow* window = new Window(Frame().OffsetByCopy(20, 20),
				(window_look)look, (window_feel)feel);

			if (message->what == kMsgAddSubsetWindow) {
				status_t status = window->AddToSubset(this);
				if (status != B_OK) {
					char text[512];
					snprintf(text, sizeof(text),
						"Window could not be added to subset:\n\n\%s", strerror(status));
					(new BAlert("Alert", text, "OK"))->Go(NULL);

					delete window;
					break;
				}
			}

			window->Show();
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


bool
Window::QuitRequested()
{
	if (!IsModal() && !IsFloating())
		gNormalWindowCount--;

	if (gNormalWindowCount < 1)
		be_app->PostMessage(B_QUIT_REQUESTED);

	return true;
}


BMessage*
Window::AddWindowMessage(window_look look, window_feel feel)
{
	BMessage* message = new BMessage(kMsgAddWindow);

	if (feel == B_FLOATING_SUBSET_WINDOW_FEEL
		|| feel == B_MODAL_SUBSET_WINDOW_FEEL)
		message->what = kMsgAddSubsetWindow;

	message->AddInt32("look", look);
	message->AddInt32("feel", feel);

	return message;
}


BString
Window::TitleForFeel(window_feel feel)
{
	BString title = "Look&Feel - ";

	switch ((uint32)feel) {
		case B_NORMAL_WINDOW_FEEL:
			title += "Normal";
			break;

		// modal feels

		case B_MODAL_SUBSET_WINDOW_FEEL:
			title += "Modal Subset";
			break;
		case B_MODAL_APP_WINDOW_FEEL:
			title += "Application Modal";
			break;
		case B_MODAL_ALL_WINDOW_FEEL:
			title += "All Modal";
			break;

		// floating feels

		case B_FLOATING_SUBSET_WINDOW_FEEL:
			title += "Floating Subset";
			break;
		case B_FLOATING_APP_WINDOW_FEEL:
			title += "Application Floating";
			break;
		case B_FLOATING_ALL_WINDOW_FEEL:
			title += "All Floating";
			break;

		// special/private feels

		case kMenuWindowFeel:
			title += "Menu";
			break;
		case kWindowScreenFeel:
			title += "WindowScreen";
			break;
		case kDesktopWindowFeel:
			title += "Desktop";
			break;
	}

	return title;
}


void
Window::_UpdateFlagsMenuLabel()
{
	BString label;
	BMenu* menu = fFlagsField->Menu();

	for (int32 i = 0; i < menu->CountItems(); i++) {
		BMenuItem* item = menu->ItemAt(i);

		if (item->IsMarked()) {
			if (label != "")
				label += " + ";
			label += item->Label();
		}
	}

	if (label == "")
		label = "none";

	menu->Superitem()->SetLabel(label.String());
}


//	#pragma mark -


class Application : public BApplication {
	public:
		Application();

		virtual void ReadyToRun();
};


Application::Application()
	: BApplication("application/x-vnd.haiku-looknfeel")
{
}


void
Application::ReadyToRun()
{
	Window *window = new Window(BRect(100, 100, 400, 420),
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL);
	window->Show();
}


//	#pragma mark -


int 
main(int argc, char **argv)
{
	Application app;// app;

	app.Run();
	return 0;
}
