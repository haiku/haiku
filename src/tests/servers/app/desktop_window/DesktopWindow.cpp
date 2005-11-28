/*
 * Copyright 2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>
#include <Window.h>
#include <View.h>

#include <WindowPrivate.h>

#include <stdio.h>


class View : public BView {
	public:
		View(BRect rect);
		virtual ~View();

		virtual void Draw(BRect updateRect);
};


View::View(BRect rect)
	: BView(rect, "desktop view", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(100, 100, 100);
	SetHighColor(0, 0, 0);
	SetLowColor(ViewColor());
}


View::~View()
{
}


void
View::Draw(BRect updateRect)
{
	MovePenTo(20, 30);
	DrawString("Desktop Window");
}


//	#pragma mark -


class Window : public BWindow {
	public:
		Window();
		virtual ~Window();
		
		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 400, 400), "DesktopWindow-Test",
			(window_look)kDesktopWindowLook, (window_feel)kDesktopWindowFeel,
			B_ASYNCHRONOUS_CONTROLS)
{
	BView *view = new View(Bounds());
	AddChild(view);
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

		virtual void ReadyToRun();
};


Application::Application()
	: BApplication("application/x-vnd.haiku-desktop_window")
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
