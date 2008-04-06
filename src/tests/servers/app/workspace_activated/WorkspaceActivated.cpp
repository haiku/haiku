/*
 * Copyright 2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>
#include <String.h>
#include <Window.h>
#include <View.h>

#include <stdio.h>


class Application : public BApplication {
public:
	Application();

	virtual void ReadyToRun();
};

class Window : public BWindow {
public:
	Window();
	virtual ~Window();

	virtual void WorkspaceActivated(int32 workspace, bool active);
	virtual bool QuitRequested();
};

class View : public BView {
public:
	View(BRect rect);
	virtual ~View();

	virtual void Draw(BRect updateRect);
};


View::View(BRect rect)
	: BView(rect, "workspace", B_FOLLOW_ALL, B_WILL_DRAW)
{
}


View::~View()
{
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

	BString current;
	current << current_workspace();

	DrawString(current.String());
}


//	#pragma mark -


Window::Window()
	: BWindow(BRect(100, 100, 400, 400), "WorkspaceActivated-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BView *view = new View(BRect(10, 10, 290, 290));
	AddChild(view);
}


Window::~Window()
{
}


void
Window::WorkspaceActivated(int32 workspace, bool active)
{
	printf("Workspace %ld (%ld), active %d\n",
		workspace, current_workspace(), active);
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark -


Application::Application()
	: BApplication("application/x-vnd.haiku-workspace_activated")
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
main(int argc, char** argv)
{
	Application app;

	app.Run();
	return 0;
}
