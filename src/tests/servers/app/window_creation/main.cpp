
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Box.h>
#include <OS.h>
#include <Screen.h>
#include <Window.h>


static BRect* sFrames = NULL;
static uint32 sNumFrames = 0;


class TestApp : public BApplication {
public:
								TestApp(uint32 numWindows, bool views);
	virtual						~TestApp();

	virtual	void				ReadyToRun();

private:
			void				_CreateFrames(uint32 numWindows);
			int32				_WindowCreator();
			static int32		_ThreadStarter(void* data);

private:
			uint32				fFrameNum;
			BRect				fScreenFrame;
			uint32				fNumWindows;
			uint32				fMaxWindows;
			bool				fTestViews;
};


class TestWindow : public BWindow {
public:
						TestWindow(BRect frame, bool createView);
	virtual				~TestWindow();
};


TestApp::TestApp(uint32 numWindows, bool views)
	:
	BApplication("application/x.vnd-Haiku.window-creation"),
	fScreenFrame(),
	fNumWindows(0),
	fMaxWindows(numWindows),
	fTestViews(views)

{
	fScreenFrame = BScreen().Frame();
	_CreateFrames(numWindows);
}


TestApp::~TestApp()
{
	delete[] sFrames;
}


void
TestApp::ReadyToRun()
{
	thread_id thread = spawn_thread(_ThreadStarter, "Window creator",
		B_NORMAL_PRIORITY, this);
	resume_thread(thread);		
}


void
TestApp::_CreateFrames(uint32 numWindows)
{
	BRect frame(0, 0, 50, 50);
	uint32 numHorizontal = (fScreenFrame.IntegerWidth() + 1)
		/ (frame.IntegerWidth() + 1);
	uint32 numVertical = (fScreenFrame.IntegerHeight() + 1)
		/ (frame.IntegerHeight() + 1);
	sNumFrames = numHorizontal * numVertical;
	sFrames = new BRect[sNumFrames];
	for (uint32 i = 0; i < sNumFrames; i++) {
		sFrames[i] = frame;	
		frame.OffsetBy(50, 0);
		if (!fScreenFrame.Contains(frame))
			frame.OffsetTo(0, frame.bottom + 1);
	}
}


int32
TestApp::_WindowCreator()
{
	bigtime_t startTime = system_time();
	
	while (fNumWindows < fMaxWindows) {
		if (fFrameNum >= sNumFrames)
			fFrameNum = 0;

		BWindow* window = new TestWindow(sFrames[fFrameNum++], fTestViews);
		window->Show();
		fNumWindows++;
	}

	bigtime_t endTime = system_time();
	
	printf("Test completed. %ld windows created in %lld usecs.\n", fNumWindows,
		endTime - startTime);

	PostMessage(B_QUIT_REQUESTED);
	return B_OK;
}


int32
TestApp::_ThreadStarter(void* data)
{
	return static_cast<TestApp*>(data)->_WindowCreator();
}


TestWindow::TestWindow(BRect frame, bool views)
	:
	BWindow(frame, "Test", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	if (views)
		AddChild(new BBox(Bounds())); 
}


TestWindow::~TestWindow()
{
}


int
main(int argc, char** argv)
{
	uint32 numWindows = 10;
	bool testViews = false;
	if (argc > 1)
		numWindows = atoi(argv[1]);

	if (argc > 2) {
		if (!strcmp(argv[2], "views"))
			testViews = true;
	}

	TestApp app(numWindows, testViews);
	app.Run();

	return 0;
}
