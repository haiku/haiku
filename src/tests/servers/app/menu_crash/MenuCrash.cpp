/*
 * Copyright 2007, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <stdio.h>

#include <Application.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <View.h>
#include <Window.h>


class MenuItem : public BMenuItem {
	public:
		MenuItem(const char* name);
		virtual ~MenuItem();

		virtual void DrawContent();
};


MenuItem::MenuItem(const char* name)
	: BMenuItem(name, NULL)
{
}


MenuItem::~MenuItem()
{
}


void
MenuItem::DrawContent()
{
	*(uint32*)0 = 0;
}


//	#pragma mark -


class Window : public BWindow {
	public:
		Window();
		virtual ~Window();
		
		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 400, 400), "MenuCrash-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BMenuBar* menuBar = new BMenuBar(Bounds(), "menu");
	AddChild(menuBar);

	// add menu

	BMenu* menu = new BMenu("File");
	BMenu* crashMenu = new BMenu("Crash");
	crashMenu->AddItem(new MenuItem("Crash"));
	menu->AddItem(crashMenu);

	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
		'Q', B_COMMAND_KEY));
	menu->SetTargetForItems(this);
	menuBar->AddItem(menu);
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
	: BApplication("application/x-vnd.haiku-menu_crash")
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
	Application app;

	app.Run();
	return 0;
}
