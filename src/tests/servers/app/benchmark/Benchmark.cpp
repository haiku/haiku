/*
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdio.h>

#include <Application.h>
#include <Screen.h>

#include "TestWindow.h"

// tests
#include "StringTest.h"

class Benchmark : public BApplication {
public:
	Benchmark()
		: BApplication("application/x-vnd.haiku-benchmark"),
		  fTest(new StringTest),
		  fTestWindow(NULL)
	{
	}

	~Benchmark()
	{
		delete fTest;
	}

	virtual	void ReadyToRun()
	{
		uint32 width = 500;
		uint32 height = 500;
		BScreen screen;
		BRect frame = screen.Frame();
		frame.left = (frame.left + frame.right - width) / 2;
		frame.top = (frame.top + frame.bottom - width) / 2;
		frame.right = frame.left + width - 1;
		frame.bottom = frame.top + height - 1;

		fTestWindow = new TestWindow(frame, fTest, B_OP_COPY,
			BMessenger(this));
	}

	virtual bool QuitRequested()
	{
		if (fTestWindow != NULL)
			fTestWindow->SetAllowedToQuit(true);
		return BApplication::QuitRequested();
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_TEST_CANCELED:
				printf("Test canceled early.\n");
				// fall through
			case MSG_TEST_FINISHED:
				fTest->PrintResults();
				PostMessage(B_QUIT_REQUESTED);
				break;
			default:
				BApplication::MessageReceived(message);
				break;
		}
	}

private:
	Test*			fTest;
	TestWindow*		fTestWindow;
};


// main
int
main(int argc, char** argv)
{
	Benchmark app;
	app.Run();
	return 0;
}
