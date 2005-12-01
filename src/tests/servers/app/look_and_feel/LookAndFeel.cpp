/*
 * Copyright 2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Window.h>

#include <stdio.h>


const uint32 kMsgUpdateLook = 'uplk';
const uint32 kMsgUpdateFlags = 'upfl';


class Window : public BWindow {
	public:
		Window();
		virtual ~Window();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();
	
	private:
		void _UpdateFlagsMenuLabel();

		BMenuField*	fFlagsField;
};


Window::Window()
	: BWindow(BRect(100, 100, 400, 400), "Look&Feel-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BRect rect(Bounds());
	BView *view = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	BPopUpMenu* menu = new BPopUpMenu("looks");
	const struct { const char* name; int32 look; } looks[] = {
		{"Titled", B_TITLED_WINDOW_LOOK}, {"Document", B_DOCUMENT_WINDOW_LOOK},
		{"Floating", B_FLOATING_WINDOW_LOOK}, {"Modal", B_MODAL_WINDOW_LOOK},
		{"Bordered", B_BORDERED_WINDOW_LOOK}, {"No Border", B_NO_BORDER_WINDOW_LOOK}
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
	menuField->SetDivider(menuField->StringWidth(menuField->Label()) + 5.0f);
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
		{"Not Anchored On Activate", B_NOT_ANCHORED_ON_ACTIVATE}
	};
	for (uint32 i = 0; i < sizeof(looks) / sizeof(looks[0]); i++) {
		BMessage* message = new BMessage(kMsgUpdateFlags);
		message->AddInt32("flag", flags[i].flag);
		BMenuItem* item = new BMenuItem(flags[i].name, message);

		menu->AddItem(item);
	}

	rect.OffsetBy(0, menuField->Bounds().Height() + 10);
	fFlagsField = new BMenuField(rect, "flags", "Flags:", menu);
	fFlagsField->ResizeToPreferred();
	fFlagsField->SetDivider(fFlagsField->StringWidth(fFlagsField->Label()) + 5.0f);
	view->AddChild(fFlagsField);
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

		default:
			BWindow::MessageReceived(message);
	}
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
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
	Window *window = new Window();
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
