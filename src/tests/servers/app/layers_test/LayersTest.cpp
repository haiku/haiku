/*
 * Copyright 2016, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Kacper Kasper, kacperkasper@gmail.com
 */


#include <Application.h>
#include <GradientRadial.h>
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

		virtual void Draw(BRect);
};

View::View(BRect rect)
	: BView(rect, "gradients", B_FOLLOW_ALL, B_WILL_DRAW)
{
}


View::~View()
{
}


void
View::Draw(BRect)
{
	BGradientRadial g(BPoint(50, 50), 50.0);
	g.AddColor((rgb_color) { 0, 0, 0, 255 }, 255.0);
	g.AddColor((rgb_color) { 255, 255, 255, 0 }, 0.0);
	SetHighColor(0, 0, 0);
	BeginLayer(255);
	// 1, 1
	FillRect(BRect(10, 10, 90, 90));
	// 2, 1
	FillRect(BRect(100, 10, 190, 90), g);
	EndLayer();
	BeginLayer(100);
	// 1, 2
	FillRect(BRect(10, 100, 90, 190));
	// 2, 2
	FillRect(BRect(100, 100, 190, 190), g);
	EndLayer();
}


//	#pragma mark -


class Window : public BWindow {
	public:
		Window();
		virtual ~Window();

		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 300, 300), "Layers-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
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
	: BApplication("application/x-vnd.haiku-layers_test")
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
	Application app;

	app.Run();
	return 0;
}
