#include <Application.h>
#include <Window.h>
#include <Box.h>


class View : public BView {
	public:
		View(BRect frame);

		virtual void FrameMoved(BPoint location);
};


View::View(BRect frame)
	: BView(frame, NULL, B_FOLLOW_NONE, B_FRAME_EVENTS)
{
	MoveBy(5, 5);
}


void
View::FrameMoved(BPoint point)
{
	point.PrintToStream();
}


//	#pragma mark -


class Window : public BWindow {
	public:
		Window();
		virtual ~Window();

		virtual bool QuitRequested();
		virtual void FrameResized(float width, float height);
};


Window::Window()
	: BWindow(BRect(100, 100, 101, 101), "Test", /*B_TITLED_WINDOW*/ B_DOCUMENT_WINDOW
	/*B_BORDERED_WINDOW*/, B_NOT_ZOOMABLE /*| B_NOT_RESIZABLE*/)
{
	Show();

	snooze(500000);	// .5 secs

	Bounds().PrintToStream();
	ResizeTo(55, 55);
	Bounds().PrintToStream();

	snooze(500000);	// .5 secs

	SetSizeLimits(5, 500, 5, 500);
	ResizeTo(5, 5);
	Bounds().PrintToStream();

	snooze(500000);	// .5 secs

	SetSizeLimits(80, 500, 80, 500);	
	Bounds().PrintToStream();
}


Window::~Window()
{
}


void
Window::FrameResized(float width, float height)
{
	CurrentMessage()->PrintToStream();
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

		virtual void ReadyToRun(void);
};


Application::Application()
	: BApplication("application/x-vnd.haiku.resizelimits-test")
{
	Window *window = new Window();
	window->Show();
}


void
Application::ReadyToRun(void)
{
}


int 
main(int argc, char **argv)
{
	Application app;
	app.Run();

	return 0;
}
