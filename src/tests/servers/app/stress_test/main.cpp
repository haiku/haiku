// main.cpp

#include <Application.h>
#include <MessageRunner.h>
#include <Window.h>

enum {
	TEST_MANY_WINDOWS = 0,
	TEST_SINGLE_WINDOW,
};

class TestApp : public BApplication {
 public:
						TestApp(uint32 testMode);
	virtual				~TestApp();

	virtual	void		ReadyToRun();
	virtual	void		MessageReceived(BMessage* message);

 private:
	BMessageRunner*		fPulse;
	BRect				fFrame;
	BRect				fScreenFrame;
	BWindow*			fWindow;
	uint32				fTestMode;
};


class TestWindow : public BWindow {
 public:
						TestWindow(BRect frame);
	virtual				~TestWindow();

 private:
	BMessageRunner*		fPulse;
};


TestApp::TestApp(uint32 testMode)
	: BApplication("application/x.vnd-Haiku.stress-test"),
	  fPulse(NULL),
	  fFrame(10.0, 30.0, 150.0, 100.0),
	  fScreenFrame(0.0, 0.0, 640.0, 480.0),
	  fTestMode(testMode)
{
}


TestApp::~TestApp()
{
	delete fPulse;
}


void
TestApp::ReadyToRun()
{
	fPulse = new BMessageRunner(be_app_messenger, new BMessage('tick'), 100L);
}


void
TestApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case 'tick':
			switch (fTestMode) {
				case TEST_MANY_WINDOWS:
					fFrame.OffsetBy(10.0, 0.0);
					if (fFrame.right > fScreenFrame.right) {
						// next row
						fFrame.OffsetTo(10.0, fFrame.top + 10.0);
					}
					if (fFrame.bottom > fScreenFrame.bottom) {
						// back to top
						fFrame.OffsetTo(10.0, 30.0);
					}
					new TestWindow(fFrame);
					break;
				case TEST_SINGLE_WINDOW:
					if (fWindow) {
						fWindow->Lock();
						fWindow->Quit();
					}
					fWindow = new BWindow(fFrame, "Test", B_TITLED_WINDOW, 0);
					fWindow->Show();
					break;
				default:
					PostMessage(B_QUIT_REQUESTED);
					break;
			}
			break;
		default:
			BApplication::MessageReceived(message);
	}
}


TestWindow::TestWindow(BRect frame)
	: BWindow(frame, "Test", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	Show();

	BMessenger self(this);
	BMessage message(B_QUIT_REQUESTED);
	fPulse = new BMessageRunner(self, &message, 10000000, 1);
}


TestWindow::~TestWindow()
{
	delete fPulse;
}



// main
int
main(int argc, char** argv)
{
	uint32 testMode = TEST_SINGLE_WINDOW;

	if (argc > 1) {
		if (strcmp(argv[1], "-many") == 0)
			testMode = TEST_MANY_WINDOWS;
		else if (strcmp(argv[1], "-single") == 0)
			testMode = TEST_SINGLE_WINDOW;
	}

	TestApp app(testMode);
	app.Run();

	return 0;
}
