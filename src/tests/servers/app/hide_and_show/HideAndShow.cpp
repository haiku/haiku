/*
 * Copyright 2005-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <stdio.h>

#include <Application.h>
#include <MessageRunner.h>
#include <TextView.h>
#include <Window.h>


static const uint32 kMsgToggleShow = 'tgsh';

class Window : public BWindow {
public:
							Window();
	virtual					~Window();

	virtual	void			MessageReceived(BMessage* message);
	virtual void			FrameResized(float width, float height);
	virtual	bool			QuitRequested();

private:
	BMessageRunner*			fRunner;
};


Window::Window()
	: BWindow(BRect(100, 100, 400, 400), "HideAndShow-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BRect rect(Bounds());
	rect.left += 20;
	rect.right -= 20;
	rect.top = 100;
	rect.bottom = 200;
	BTextView* view = new BTextView(Bounds(), "", rect, B_FOLLOW_ALL,
		B_FRAME_EVENTS | B_WILL_DRAW);
	view->MakeEditable(false);
	view->SetAlignment(B_ALIGN_CENTER);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	view->SetText("The window will be hidden and shown every 2 seconds.");
	AddChild(view);

	BMessage showAndHide(kMsgToggleShow);
	fRunner = new BMessageRunner(this, &showAndHide, 2000000);
}

Window::~Window()
{
	delete fRunner;
}


void
Window::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgToggleShow:
			if (IsHidden())
				Show();
			else
				Hide();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
Window::FrameResized(float width, float height)
{
	BTextView* view = dynamic_cast<BTextView*>(ChildAt(0));
	if (view == NULL)
		return;

	BRect rect = Bounds();
	rect.left += 20;
	rect.right -= 20;
	rect.top = 100;
	rect.bottom = 200;
	view->SetTextRect(rect);
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

	virtual	void			ReadyToRun();
};


Application::Application()
	: BApplication("application/x-vnd.haiku-hide_and_show")
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
