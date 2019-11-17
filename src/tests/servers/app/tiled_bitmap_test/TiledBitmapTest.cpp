/*
 * Copyright 2019, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Kacper Kasper, kacperkasper@gmail.com
 */


#include <Application.h>
#include <Bitmap.h>
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
	BBitmap* fBitmap;
};

View::View(BRect rect)
	: BView(rect, "tiledBitmaps", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(make_color(255, 0, 0));
	fBitmap = new BBitmap(BRect(0, 0, 5, 5), B_RGBA32);
	uint32 data[] = {
		0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000,
		0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,
		0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,
		0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,
		0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,
		0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000,
	};
	/*uint32 data[] = {
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};*/
	fBitmap->SetBits((void*)data, sizeof(data), 0, B_RGBA32);
}


View::~View()
{
	delete fBitmap;
}


void
View::Draw(BRect)
{
	DrawTiledBitmap(fBitmap, BRect(10, 10, 60, 60), BPoint(1, 1));
	DrawTiledBitmap(fBitmap, BRect(10, 60, 60, 110), BPoint(3, 3));
	DrawTiledBitmap(fBitmap, BRect(10, 110, 60, 160));
	DrawBitmap(fBitmap, fBitmap->Bounds(), BRect(60, 10, 110, 60));
}


//	#pragma mark -


class Window : public BWindow {
	public:
		Window();
		virtual ~Window();

		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 300, 300), "TiledBitmap-Test",
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
	: BApplication("application/x-vnd.haiku-tiled_bitmap_test")
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
