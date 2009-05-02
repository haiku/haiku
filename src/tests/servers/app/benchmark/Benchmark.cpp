/*
 * Copyright (C) 2008-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Screen.h>

#include "DrawingModeToString.h"
#include "TestWindow.h"

// tests
#include "HorizontalLineTest.h"
#include "RandomLineTest.h"
#include "StringTest.h"
#include "VerticalLineTest.h"


struct test_info {
	const char*				name;
	Test*					(*create)();
};

const test_info kTestInfos[] = {
	{ "HorizontalLines",	HorizontalLineTest::CreateTest },
	{ "RandomLines",		RandomLineTest::CreateTest },
	{ "Strings",			StringTest::CreateTest },
	{ "VerticalLines",		VerticalLineTest::CreateTest },
	{ NULL, NULL }
};


class Benchmark : public BApplication {
public:
	Benchmark(Test* test, drawing_mode mode, bool clipping)
		: BApplication("application/x-vnd.haiku-benchmark"),
		  fTest(test),
		  fTestWindow(NULL),
		  fDrawingMode(mode),
		  fUseClipping(clipping)
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

		fTestWindow = new TestWindow(frame, fTest, fDrawingMode,
			fUseClipping, BMessenger(this));
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
				fTestWindow->Lock();
				fTest->PrintResults(fTestWindow->View());
				fTestWindow->Unlock();
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
	drawing_mode	fDrawingMode;
	bool			fUseClipping;
};


static void
print_test_list(bool error)
{
	FILE* out = (error ? stderr : stdout);

	fprintf(out, "available tests:\n");

	for (int32 i = 0; kTestInfos[i].name; i++)
		fprintf(out, "  %s\n", kTestInfos[i].name);
}


int
main(int argc, char** argv)
{
	// get test name
	const char* testName;
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <test name>\n", argv[0]);
		print_test_list(true);
		exit(1);
	}
	// skip program name
	argc--;
	argv++;

	testName = argv[0];
	bool clipping = false;
	drawing_mode mode = B_OP_COPY;

	while (argc > 0) {
		drawing_mode possibleMode;
		if (strcmp(argv[0], "--clipping") == 0 || strcmp(argv[0], "-c") == 0) {
			clipping = true;
		} else if (ToDrawingMode(argv[0], possibleMode)) {
			mode = possibleMode;
		}
		argc--;
		argv++;
	}


	// find and create the test
	Test* test = NULL;
	try {
		for (int32 i = 0; kTestInfos[i].name; i++) {
			if (strcmp(testName, kTestInfos[i].name) == 0) {
				test = (kTestInfos[i].create)();
				break;
			}
		}
	} catch (std::bad_alloc) {
		fprintf(stderr, "Insufficient memory to create the test. Sorry.\n");
		exit(1);
	}

	if (test == NULL) {
		fprintf(stderr, "Error: Invalid test name: \"%s\"\n", testName);
		print_test_list(true);
		exit(1);
	}

	Benchmark app(test, mode, clipping);
	app.Run();
	return 0;
}
