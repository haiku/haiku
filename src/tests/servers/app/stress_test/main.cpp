// main.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <MessageRunner.h>
#include <Window.h>


int32 gMaxCount = 0;

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
	  fWindow(NULL),
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
	BMessage message('tick');
	fPulse = new BMessageRunner(be_app_messenger, &message, 100L);
}


void
TestApp::MessageReceived(BMessage* message)
{
	static int32 count = 0;
	if (gMaxCount != 0 && ++count > gMaxCount)
		PostMessage(B_QUIT_REQUESTED);

	switch (message->what) {
		case 'tick':
			switch (fTestMode) {
				case TEST_MANY_WINDOWS:
				{
					fFrame.OffsetBy(10.0, 0.0);
					if (fFrame.right > fScreenFrame.right) {
						// next row
						fFrame.OffsetTo(10.0, fFrame.top + 10.0);
					}
					if (fFrame.bottom > fScreenFrame.bottom) {
						// back to top
						fFrame.OffsetTo(10.0, 30.0);
					}
					int32 action = CountWindows() > 1 ? rand() % 10 : 0;
					switch (action) {
						case 0:	// new
							new TestWindow(fFrame);
							break;
						case 1:	// move
						{
							BWindow* window = WindowAt(rand() % CountWindows());
							if (window->Lock()) {
								if (window->IsHidden())
									window->Show();
								window->MoveBy(23, 19);
								window->Unlock();
							}
							break;
						}
						case 2:	// hide
						{
							BWindow* window = WindowAt(rand() % CountWindows());
							if (window->Lock()) {
								if (!window->IsHidden())
									window->Hide();
								window->Unlock();
							}
							break;
						}
						case 3:	// activate
						{
							BWindow* window = WindowAt(rand() % CountWindows());
							if (window->Lock()) {
								if (window->IsHidden())
									window->Show();
								window->Activate();
								window->Unlock();
							}
							break;
						}
						case 4:	// change workspace
						{
							BWindow* window = WindowAt(rand() % CountWindows());
							if (window->Lock()) {
								if (window->IsHidden())
									window->Show();
								window->SetWorkspaces(1 << (rand() % 4));
								window->Unlock();
							}
							break;
						}
						case 5:	// minimize
						{
							BWindow* window = WindowAt(rand() % CountWindows());
							if (window->Lock()) {
								if (window->IsHidden())
									window->Show();
								window->Minimize(true);
								window->Unlock();
							}
							break;
						}
						case 6:	// change size
						{
							BWindow* window = WindowAt(rand() % CountWindows());
							if (window->Lock()) {
								if (window->IsHidden())
									window->Show();
								window->ResizeBy(1, 2);
								window->Unlock();
							}
							break;
						}
						case 7:	// set title
						{
							BWindow* window = WindowAt(rand() % CountWindows());
							if (window->Lock()) {
								if (window->IsHidden())
									window->Show();
								char title[256];
								snprintf(title, sizeof(title), "Title %d",
									rand() % 100);
								window->SetTitle(title);
								window->Unlock();
							}
							break;
						}
						case 8:	// set look
						{
							BWindow* window = WindowAt(rand() % CountWindows());
							if (window->Lock()) {
								if (window->IsHidden())
									window->Show();
								window_look looks[] = {
									B_DOCUMENT_WINDOW_LOOK,
									B_MODAL_WINDOW_LOOK,
									B_FLOATING_WINDOW_LOOK,
								};
								window->SetLook(looks[rand() % 3]);
								window->Unlock();
							}
							break;
						}
						case 9:	// set feel
						{
							BWindow* window = WindowAt(rand() % CountWindows());
							if (window->Lock()) {
								if (window->IsHidden())
									window->Show();
								window_feel feels[] = {
									B_NORMAL_WINDOW_FEEL,
									B_FLOATING_APP_WINDOW_FEEL,
									B_MODAL_APP_WINDOW_FEEL,
								};
								window->SetFeel(feels[rand() % 3]);
								window->Unlock();
							}
							break;
						}
					}
					break;
				}

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
	fPulse = new BMessageRunner(self, &message, 100000000LL, 1);

	if (Thread() < B_OK)
		Quit();
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
	if (argc > 2) {
		gMaxCount = atol(argv[2]);
	}

	TestApp app(testMode);
	app.Run();

	return 0;
}
