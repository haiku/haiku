/*
 * Copyright 2005-2007, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>
#include <MessageRunner.h>
#include <Window.h>
#include <View.h>

#include <stdio.h>


static const uint32 kMsgUpdate = 'updt';


class View : public BView {
	public:
		View(BRect rect);
		virtual ~View();

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void Draw(BRect updateRect);
		virtual void KeyDown(const char* bytes, int32 numBytes);
		virtual void KeyUp(const char* bytes, int32 numBytes);
		virtual void MessageReceived(BMessage* message);
		virtual void MouseDown(BPoint where);

	private:
		void _Update();

		BMessageRunner* fRunner;
		char	fLastKey;
		bool	fPressed;
		uint8	fLastColor;
};

class Window : public BWindow {
	public:
		Window(int32 offset = 0);
		virtual ~Window();

		virtual bool QuitRequested();
};

class Application : public BApplication {
	public:
		Application();

		virtual void ReadyToRun();
};


View::View(BRect rect)
	: BView(rect, "lock focus", B_FOLLOW_ALL, B_WILL_DRAW),
	fRunner(NULL),
	fLastKey('\0'),
	fPressed(false),
	fLastColor(255)
{
}


View::~View()
{
}


void
View::AttachedToWindow()
{
	MakeFocus(this);

	BMessage update(kMsgUpdate);
	fRunner = new BMessageRunner(this, &update, 16667);

	BFont font;
	font.SetSize(72);
	SetFont(&font);
}


void
View::DetachedFromWindow()
{
	delete fRunner;
	fRunner = NULL;
}


void
View::MouseDown(BPoint where)
{
	SetMouseEventMask(0, B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS
		| B_NO_POINTER_HISTORY);

	::Window* window = new ::Window(100);
	window->Show();
}


void
View::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUpdate:
			_Update();
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
View::_Update()
{
	if (fPressed)
		return;

	if (fLastColor < 255) {
		fLastColor += 15;
		Invalidate();
	}
}


void
View::KeyUp(const char* bytes, int32 numBytes)
{
	fPressed = false;
}


void
View::KeyDown(const char* bytes, int32 numBytes)
{
	fLastKey = bytes[0];
	fLastColor = 0;
	fPressed = true;
	Invalidate();
}


void
View::Draw(BRect updateRect)
{
	SetHighColor(fLastColor, fLastColor, fLastColor);
	DrawString(&fLastKey, 1, BPoint(20, 70));
}


//	#pragma mark -


Window::Window(int32 offset)
	: BWindow(BRect(100 + offset, 100 + offset, 400 + offset, 400 + offset),
			"LockFocus-Test", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
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


Application::Application()
	: BApplication("application/x-vnd.haiku-lock_focus")
{
}


void
Application::ReadyToRun(void)
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
