/*
 * Copyright 2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>


bool gAvoid = true;


class View : public BView {
	public:
		View(BRect rect);
		virtual ~View();

		virtual void AttachedToWindow();

		virtual void KeyDown(const char* bytes, int32 numBytes);
		virtual void KeyUp(const char* bytes, int32 numBytes);

		virtual void MouseDown(BPoint where);
		virtual void MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage);

		virtual void Draw(BRect updateRect);
		virtual void Pulse();

	private:
		BString	fChar;
		bool	fPressed;
};


View::View(BRect rect)
	: BView(rect, "desktop view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	SetViewColor(255, 255, 255);
	SetHighColor(0, 0, 0);
	SetLowColor(ViewColor());
}


View::~View()
{
}


void
View::AttachedToWindow()
{
	MakeFocus(true);
	KeyDown("<>", 2);
	fPressed = false;
}


void
View::KeyDown(const char* bytes, int32 numBytes)
{
	fChar = bytes;
	fPressed = true;
	SetHighColor(0, 0, 0);
	Window()->SetPulseRate(200000);
	Invalidate();
}


void
View::KeyUp(const char* bytes, int32 numBytes)
{
	fPressed = false;
	Invalidate();
}


void
View::MouseDown(BPoint where)
{
	SetHighColor(0, 250, 0);
	Invalidate();
	Window()->SetPulseRate(150000);
}


void
View::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (transit == B_ENTERED_VIEW)
		SetHighColor(0, 150, 150);
	else
		SetHighColor(0, 0, 150);

	Invalidate();
	Window()->SetPulseRate(10000);
}


void
View::Draw(BRect updateRect)
{
	BFont font;
	font.SetSize(200);

	font_height fontHeight;
	font.GetHeight(&fontHeight);

	SetFont(&font);
	MovePenTo(20, ceilf(20 + fontHeight.ascent));

	DrawString(fChar.String());
}


void
View::Pulse()
{
	if (fPressed)
		return;

	rgb_color color = HighColor();
	SetHighColor(tint_color(color, B_LIGHTEN_2_TINT));
	Invalidate();

	if (color.red > 253)
		Window()->SetPulseRate(0);
}


//	#pragma mark -


class Window : public BWindow {
	public:
		Window();
		virtual ~Window();
		
		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 400, 400), "AvoidFocus-Test",
			B_TITLED_WINDOW, (gAvoid ? B_AVOID_FOCUS : 0) | B_ASYNCHRONOUS_CONTROLS)
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
	: BApplication("application/x-vnd.haiku-avoid_focus")
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
	if (argc > 1 && (!strcmp(argv[1], "false") || (isdigit(argv[1][0]) && atoi(argv[1]) == 0)))
		gAvoid = false;

	Application app;

	app.Run();
	return 0;
}
