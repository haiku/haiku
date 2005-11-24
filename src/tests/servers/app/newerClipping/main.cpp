
#include <Application.h>
#include <Window.h>
#include <View.h>

#include <stdio.h>
#include <stdlib.h>

#include "Desktop.h"
#include "DrawingEngine.h"
#include "ViewLayer.h"
#include "WindowLayer.h"

class App : public BApplication {
 public:
						App();
						~App();

	virtual void		ReadyToRun();
};

class Window : public BWindow {
 public:
						Window(const char* title);
						~Window();

			void		AddWindow(BRect frame, const char* name);
			void		Test();
 private:
			DrawView*	fView;
			Desktop*	fDesktop;
};

App::App()
	: BApplication("application/x-vnd.stippi.ClippingTest")
{
	srand(real_time_clock_usecs());
}

App::~App()
{
}

void
App::ReadyToRun()
{
	Window* win = new Window("clipping");
	win->Show();

	win->Test();
}

Window::Window(const char* title)
	: BWindow(BRect(50, 50, 800, 650), title,
			  B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			  B_QUIT_ON_WINDOW_CLOSE | B_ASYNCHRONOUS_CONTROLS)
{
	fView = new DrawView(Bounds());
	fDesktop = new Desktop(fView);
	fDesktop->Run();
	AddChild(fView);
	fView->MakeFocus(true);
}

Window::~Window()
{
	fDesktop->Lock();
	fDesktop->Quit();
}

// AddWindow
void
Window::AddWindow(BRect frame, const char* name)
{
	WindowLayer* window = new WindowLayer(frame, name,
										  fDesktop->GetDrawingEngine(),
										  fDesktop);
	window->Run();

	BMessage message(MSG_ADD_WINDOW);
	message.AddPointer("window", (void*)window);
	fDesktop->PostMessage(&message);
}

// Test
void
Window::Test()
{
	AddWindow(BRect(20, 20, 80, 80), "Window 1");
	AddWindow(BRect(60, 60, 220, 180), "Window 2");
	AddWindow(BRect(120, 160, 500, 380), "Window 3");
	AddWindow(BRect(40, 210, 400, 280), "Window 4");
	AddWindow(BRect(180, 410, 400, 680), "Window 5");
	AddWindow(BRect(30, 350, 100, 440), "Window 6");
	AddWindow(BRect(80, 10, 200, 120), "Window 7");
	AddWindow(BRect(480, 40, 670, 320), "Window 8");
	AddWindow(BRect(250, 500, 310, 600), "Window 9");
	AddWindow(BRect(130, 450, 230, 500), "Window 10");
}

// main
int
main(int argc, const char* argv[])
{
	App app;
	app.Run();
	return 0;
}
