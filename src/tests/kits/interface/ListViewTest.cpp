/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <ListView.h>
#include <StringItem.h>
#include <Window.h>

#include <stdio.h>


class ColorfulItem: public BStringItem
{
	public:
						ColorfulItem(const char* label, rgb_color color);
		virtual	void	DrawItem(BView* owner, BRect frame,
							bool complete = false);

	private:
		rgb_color fColor;
};


ColorfulItem::ColorfulItem(const char* label, rgb_color color)
	:
	BStringItem(label),
	fColor(color)
{
}


void
ColorfulItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	owner->SetHighColor(fColor);
	BStringItem::DrawItem(owner, frame, complete);
}


//	#pragma mark -


class Window : public BWindow {
	public:
		Window();

		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 520, 430), "ListView-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BRect rect(20, 10, 200, 300);
	BListView *listView = new BListView(rect, "list");

	listView->AddItem(new BStringItem("normal item"));
	listView->AddItem(new ColorfulItem("green item", make_color(0, 255, 0)));
	listView->AddItem(new BStringItem("normal item"));

	AddChild(listView);
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark -


class Application : public BApplication {
	public:
		Application();

		virtual void ReadyToRun(void);
};


Application::Application()
	: BApplication("application/x-vnd.obos-test")
{
}


void
Application::ReadyToRun(void)
{
	BWindow *window = new Window();
	window->Show();
}


//	#pragma mark -


int 
main(int argc, char **argv)
{
	Application app;

	app.Run();
	return 0;
}

