/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <MessageRunner.h>
#include <StatusBar.h>
#include <Window.h>

#include <stdio.h>


const uint32 kMsgUpdate = 'updt';

class Window : public BWindow {
	public:
		Window();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

	private:
		BMessageRunner* fUpdater;
		BStatusBar*	fStatusBar;
};


Window::Window()
	: BWindow(BRect(100, 100, 520, 200), "StatusBar-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BView* main = new BView(Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	main->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(main);

	BRect rect(20, 10, 400, 30);
	fStatusBar = new BStatusBar(rect, NULL, "label", "trailing label");
	fStatusBar->SetResizingMode(B_FOLLOW_LEFT_RIGHT);
	//fStatusBar->ResizeToPreferred();
	float width, height;
	fStatusBar->GetPreferredSize(&width, &height);
	fStatusBar->ResizeTo(rect.Width(), height);
	fStatusBar->SetMaxValue(50.0f);
	main->AddChild(fStatusBar);

	BMessage update(kMsgUpdate);
	fUpdater = new BMessageRunner(this, &update, 10000LL);
}


void
Window::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUpdate:
		{
			char buffer[100];
			snprintf(buffer, sizeof(buffer), "%ld ", (int32)fStatusBar->CurrentValue());
			fStatusBar->Update(1, fStatusBar->CurrentValue() > 25 ? " updated!" : NULL, buffer);

			if (fStatusBar->CurrentValue() >= fStatusBar->MaxValue()) {
#if 1
				fStatusBar->Reset("-", "????");
#else
				fStatusBar->Reset();
				fStatusBar->SetText("-");
				fStatusBar->SetTrailingText("????");
#endif
				fStatusBar->SetMaxValue(50.0);
			}
		}

		default:
			BWindow::MessageReceived(message);
	}
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	delete fUpdater;
	return true;
}


//	#pragma mark -


class Application : public BApplication {
	public:
		Application();

		virtual void ReadyToRun(void);
};


Application::Application()
	: BApplication("application/x-vnd.haiku-test")
{
}


void
Application::ReadyToRun(void)
{
	BWindow *window = new Window();
	window->Show();
}


//	#pragma mark -


int 
main(int argc, char **argv)
{
	Application app;

	app.Run();
	return 0;
}

