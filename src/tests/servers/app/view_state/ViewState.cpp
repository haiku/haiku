/*
 * Copyright 2005-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>
#include <Window.h>
#include <View.h>

#include <stdio.h>


class View : public BView {
public:
							View(BRect rect);
	virtual					~View();

	virtual void			Draw(BRect updateRect);
};


class Window : public BWindow {
public:
							Window();
	virtual					~Window();

	virtual bool			QuitRequested();
};


class Application : public BApplication {
public:
							Application();

	virtual void			ReadyToRun();
};


View::View(BRect rect)
	:
	BView(rect, "view state", B_FOLLOW_ALL, B_WILL_DRAW)
{
}


View::~View()
{
}


void
View::Draw(BRect updateRect)
{
	SetHighColor(100, 100, 100);
	StrokeRect(Bounds());

	// TODO: for now, we only test scaling functionality

	SetHighColor(42, 42, 242);
	StrokeRect(BRect(5, 5, 10, 10));
#ifdef __HAIKU__
	printf("scale 1: %g\n", Scale());
#endif

	SetScale(2.0);
	StrokeRect(BRect(5, 5, 10, 10));
#ifdef __HAIKU__
	printf("scale 2: %g\n", Scale());
#endif

	SetHighColor(42, 242, 42);
	PushState();
	StrokeRect(BRect(6, 6, 11, 11));
#ifdef __HAIKU__
	printf("scale 3: %g\n", Scale());
#endif

	SetHighColor(242, 42, 42);
	SetScale(2.0);
	StrokeRect(BRect(5, 5, 10, 10));
#ifdef __HAIKU__
	printf("scale 4: %g\n", Scale());
#endif

	PopState();
	SetScale(1.0);
}


//	#pragma mark -


Window::Window()
	:
	BWindow(BRect(100, 100, 400, 400), "ViewState-Test", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS)
{
	BView* view = new View(BRect(10, 10, 290, 290));
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


Application::Application()
	:
	BApplication("application/x-vnd.haiku-view_state")
{
}


void
Application::ReadyToRun()
{
	Window* window = new Window();
	window->Show();
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	Application app;
	app.Run();

	return 0;
}
