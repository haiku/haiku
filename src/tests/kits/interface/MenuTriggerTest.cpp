/*
 * Copyright 2007, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Window.h>

#include <stdio.h>


class Window : public BWindow {
	public:
		Window();
		virtual ~Window();
		
		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 400, 400), "Menu Trigger Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BMenuBar* bar = new BMenuBar(BRect(0, 0, 10, 10), "menuBar");
	AddChild(bar);

	BMenu* menu = new BMenu("File");
	menu->AddItem(new BMenuItem("Bart", NULL));
	menu->AddItem(new BMenuItem("bart", NULL));
	menu->AddItem(new BMenuItem("bart", NULL));
	menu->AddItem(new BMenuItem("Bart", NULL));
	menu->AddItem(new BMenuItem("BART", NULL));
	menu->AddItem(new BMenuItem("bärt", NULL));
	menu->AddItem(new BMenuItem("bärst", NULL));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED)));
	bar->AddItem(menu);

	menu = new BMenu("Edit");
	menu->AddItem(new BMenuItem("1 a", NULL));
	menu->AddItem(new BMenuItem("2 a", NULL));
	menu->AddItem(new BMenuItem("3 a", NULL));
	menu->AddItem(new BMenuItem("3 a", NULL));
	menu->AddItem(new BMenuItem("3 aöa", NULL));
	bar->AddItem(menu);

	menu = new BMenu("Extra");
	BMenuItem* item = new BMenuItem("\xe3\x81\x82 haiku", NULL);
	item->SetTrigger('h');
	menu->AddItem(item);
	bar->AddItem(menu);
}

Window::~Window()
{
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
	: BApplication("application/x-vnd.haiku-view_state")
{
}


void
Application::ReadyToRun(void)
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
