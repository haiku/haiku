
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <OS.h>
#include <Window.h>
#include <WindowStack.h>


static BRect* sFrames = NULL;
static uint32 sNumFrames = 0;


class TestApp : public BApplication {
public:
								TestApp(uint32 numWindows);
	virtual						~TestApp();

private:
			void				_CreateFrames(uint32 numWindows);
			int32				_WindowCreator();
			static int32		_ThreadStarter(void* data);

private:
			uint32				fFrameNum;
			uint32				fNumWindows;
			uint32				fMaxWindows;
};


class TestWindow : public BWindow {
public:
						TestWindow(BRect frame);
	virtual				~TestWindow();

	virtual	void		DispatchMessage(BMessage* message, BHandler* handler);
};


TestApp::TestApp(uint32 numWindows)
	:
	BApplication("application/x.vnd-Haiku.stack-tile"),
	fNumWindows(0),
	fMaxWindows(numWindows)
{
	_CreateFrames(numWindows);
}


TestApp::~TestApp()
{
	delete[] sFrames;
}


void
TestApp::_CreateFrames(uint32 numWindows)
{
	BWindowStack* stack = NULL;
	while (fNumWindows < fMaxWindows) {
		if (fFrameNum >= sNumFrames)
			fFrameNum = 0;

		BWindow* window = new TestWindow(BRect(20, 20, 300, 200));

		if (!stack) stack = new BWindowStack(window);
		else stack->AddWindow(window);

		window->Show();
		fNumWindows++;
	}
}


TestWindow::TestWindow(BRect frame)
	:
	BWindow(frame, "Test", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
}


TestWindow::~TestWindow()
{
}


void
TestWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	BWindow::DispatchMessage(message, handler);

	int a = rand();
	char buf[32];
	sprintf(buf, "%d", a);
	SetTitle(buf);
}


int
main(int argc, char** argv)
{
	uint32 numWindows = 2;
	if (argc > 1)
		numWindows = atoi(argv[1]);

	TestApp app(numWindows);
	app.Run();

	return 0;
}
