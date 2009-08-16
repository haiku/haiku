/*
 * Copyright 2009, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Button.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ScrollView.h>
#include <String.h>
#include <TextControl.h>
#include <TextView.h>
#include <Window.h>

#include <stdio.h>


const static uint32 kMsgAlignLeft = 'alle';
const static uint32 kMsgAlignCenter = 'alce';
const static uint32 kMsgAlignRight = 'alri';


class Window : public BWindow {
	public:
		Window();

		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *message);

	private:
		BTextControl* fTextControl;
		BTextView* fTextView;
};


//	#pragma mark -


Window::Window()
	: BWindow(BRect(100, 100, 800, 500), "TextView-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	fTextControl = new BTextControl("text-contr-O",
		"a single line of text - (c) Conglom-O", NULL);
	fTextView = new BTextView("text-O");
	BScrollView* scrollView = new BScrollView("scroll-O", fTextView, 0, true,
		true, B_FANCY_BORDER);

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(fTextControl)
		.Add(scrollView)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.Add(new BButton("Align Left", new BMessage(kMsgAlignLeft)))
			.AddGlue()
			.Add(new BButton("Align Center", new BMessage(kMsgAlignCenter)))
			.AddGlue()
			.Add(new BButton("Align Right", new BMessage(kMsgAlignRight)))
		)
		.SetInsets(5, 5, 5, 5)
	);

	// generate some lines of content
	const int32 kLineCount = 10;
	const int32 kLineNoSize = 6;
	BString line = ": just some text here - nothing special to see\n";
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
	fTextView->SetInsets(2,2,2,2);
	fTextView->SetText(content.String());
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
Window::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgAlignLeft:
			fTextControl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_LEFT);
			fTextView->SetAlignment(B_ALIGN_LEFT);
			break;

		case kMsgAlignCenter:
			fTextControl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_CENTER);
			fTextView->SetAlignment(B_ALIGN_CENTER);
			break;

		case kMsgAlignRight:
			fTextControl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
			fTextView->SetAlignment(B_ALIGN_RIGHT);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
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

	const int kExpectedTextViewSize = 356;
	if (sizeof(BTextView) != kExpectedTextViewSize) {
		fprintf(stderr, "sizeof(BTextView) is %ld instead of %d!\n",
			sizeof(BTextView), kExpectedTextViewSize);
		return 1;
	}

	app.Run();
	return 0;
}

