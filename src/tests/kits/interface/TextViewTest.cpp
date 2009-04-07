/*
 * Copyright 2009, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Window.h>
#include <ScrollView.h>
#include <String.h>
#include <TextView.h>

#include <stdio.h>


class Window : public BWindow {
	public:
		Window();

		virtual bool QuitRequested();
};


//	#pragma mark -


Window::Window()
	: BWindow(BRect(100, 100, 800, 500), "TextView-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BRect rect = Bounds().InsetByCopy(20, 20);

	BTextView* textView = new BTextView(rect, "text-o-mo",
		rect.OffsetToCopy(0, 0), B_FOLLOW_ALL);

	BScrollView* scrollView = new BScrollView("scroll-o-mo", textView,
		B_FOLLOW_ALL_SIDES, 0, true, true);
	AddChild(scrollView);

	printf("starting to prepare content ... [%Ld]\n", system_time());

	// generate a million lines of content
	const int32 kLineCount = 1000000;
	const int32 kLineNoSize = 6;
	BString line
		= ":	you should see a pretty large text in this textview	...\n";
	BString format = BString("%*d") << line;
	BString content;
	int32 lineLength = line.Length() + kLineNoSize;
	int32 contentLength = lineLength * kLineCount;
	char* currLine = content.LockBuffer(contentLength);
	if (currLine) {
		int32 lineNo = 0;
		for ( ; lineNo < kLineCount; currLine += lineLength)
			sprintf(currLine, format.String(), kLineNoSize, lineNo++);
		content.UnlockBuffer(contentLength);
	}
	printf("setting content ... [%Ld]\n", system_time());
	textView->SetText(content.String());
	printf("done. [%Ld]\n", system_time());
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

