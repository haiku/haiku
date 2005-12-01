
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

	virtual void		ReadyToRun();
};

class Window : public BWindow {
 public:
						Window(const char* title);
	virtual				~Window();

			void		AddWindow(BRect frame, const char* name);
			void		Test();
 private:
			DrawView*	fView;
			Desktop*	fDesktop;
};

// constructor
App::App()
	: BApplication("application/x-vnd.stippi.ClippingTest")
{
	srand(real_time_clock_usecs());
}

// ReadyToRun
void
App::ReadyToRun()
{
	Window* win = new Window("clipping");
	win->Show();

	win->Test();
}

// constructor
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

// destructor
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

	// add a coupld children
	frame.OffsetTo(B_ORIGIN);
	frame.InsetBy(5.0, 5.0);
	if (frame.IsValid()) {
		ViewLayer* layer1 = new ViewLayer(frame, "View 1",
										  B_FOLLOW_ALL,
										  B_FULL_UPDATE_ON_RESIZE, 
										  (rgb_color){ 180, 180, 180, 255 });

		frame.OffsetTo(B_ORIGIN);
		frame.InsetBy(15.0, 15.0);
		if (frame.IsValid()) {

			BRect f = frame;
			f.bottom = f.top + f.Height() / 3 - 3;
			f.right = f.left + f.Width() / 3 - 3;

			// top row of views
			ViewLayer* layer;
			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_TOP,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new ViewLayer(f, "View", B_FOLLOW_RIGHT | B_FOLLOW_TOP,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			// middle row of views
			f = frame;
			f.bottom = f.top + f.Height() / 3 - 3;
			f.right = f.left + f.Width() / 3 - 3;
			f.OffsetBy(0, f.Height() + 6);

			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new ViewLayer(f, "View", B_FOLLOW_ALL,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new ViewLayer(f, "View", B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			// bottom row of views
			f = frame;
			f.bottom = f.top + f.Height() / 3 - 3;
			f.right = f.left + f.Width() / 3 - 3;
			f.OffsetBy(0, 2 * f.Height() + 12);

			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			f.OffsetBy(f.Width() + 6, 0);

//			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			f.OffsetBy(f.Width() + 6, 0);

//			layer = new ViewLayer(f, "View", B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM,
			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);
		}

		window->AddChild(layer1);
	}

	window->Run();

	BMessage message(MSG_ADD_WINDOW);
	message.AddPointer("window", (void*)window);
	fDesktop->PostMessage(&message);
}

// Test
void
Window::Test()
{
	BRect frame(20, 20, 330, 230);
//	AddWindow(frame, "Window 1");
//	AddWindow(frame, "Window 2");
	for (int32 i = 0; i < 20; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}

/*	frame.Set(10, 80, 320, 290);
	for (int32 i = 20; i < 40; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}

	frame.Set(20, 140, 330, 230);
	for (int32 i = 40; i < 60; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}

	frame.Set(20, 200, 330, 230);
	for (int32 i = 60; i < 80; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}

	frame.Set(20, 260, 330, 230);
// 99 windows are ok, the 100th looper does not run anymore,
// I guess I'm hitting a BeOS limit (max loopers per app?)
	for (int32 i = 80; i < 99; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}*/
}

// main
int
main(int argc, const char* argv[])
{
	srand((long int)system_time());

	App app;
	app.Run();
	return 0;
}
