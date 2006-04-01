/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>
#include <Cursor.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>


class View : public BView {
	public:
		View(BRect rect);
		virtual ~View();

		virtual void AttachedToWindow();
};


const uint8 kCursorData[68] = {
	16, 1, 8, 8,

	0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
	0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
	0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
	0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,

	0xff, 0xff, 0x80, 0x01, 0x80, 0x01, 0x8f, 0xf1,
	0x88, 0x11, 0x88, 0x11, 0x88, 0x11, 0x89, 0x91,
	0x89, 0x91, 0x88, 0x11, 0x88, 0x11, 0x88, 0x11,
	0x8f, 0xf1, 0x80, 0x01, 0x80, 0x01, 0xff, 0xff,
};

bool gHide = false;


View::View(BRect rect)
	: BView(rect, "desktop view", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(0, 150, 0);
}


View::~View()
{
}


void
View::AttachedToWindow()
{
	BCursor cursor(kCursorData);
	SetViewCursor(&cursor);
}


//	#pragma mark -


class Window : public BWindow {
	public:
		Window();
		virtual ~Window();
		
		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 400, 400), "Cursor-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BView *view = new View(Bounds().InsetByCopy(30, 30));
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
	: BApplication("application/x-vnd.haiku-cursor_test")
{
}


void
Application::ReadyToRun()
{
	Window *window = new Window();
	window->Show();

	if (gHide)
		HideCursor();
	else
		SetCursor(B_I_BEAM_CURSOR);
}


//	#pragma mark -


int 
main(int argc, char **argv)
{
	if (argc > 1 && !strcmp(argv[1], "hide"))
		gHide = true;

	Application app;

	app.Run();
	return 0;
}
