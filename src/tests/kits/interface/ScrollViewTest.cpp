/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <ScrollView.h>

#include <Application.h>
#include <Window.h>
#include <Box.h>

#include <stdio.h>


//#define ARCHIVE_TEST
//#define CHANGE_BORDER_TEST
//#define SIZE_TEST

uint32 gNumWindows = 0;


class Window : public BWindow {
	public:
		Window();

		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 580, 580), "Scroll-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	const bool horiz[] = {false, true, false, true};
	const bool vert[] = {false, false, true, true};
	const border_style border[] = {B_NO_BORDER, B_PLAIN_BORDER, B_FANCY_BORDER, B_FANCY_BORDER};
	BRect rect(1, 1, 119, 119);
	BRect frame = rect.InsetByCopy(20, 20);

	for (int32 i = 0; i < 16; i++) {
		if (i > 0 && (i % 4) == 0)
			rect.OffsetBy(-480, 120);

		int32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP;
		if (i % 4 == 3) {
			if (i == 15)
				resizingMode = B_FOLLOW_ALL;
			else
				resizingMode = B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP;
		} else if (i > 11)
			resizingMode = B_FOLLOW_TOP_BOTTOM | B_FOLLOW_LEFT;

		BBox *box = new BBox(rect, "box", resizingMode);
		AddChild(box);

		BView *view = new BView(frame, "main view", B_FOLLOW_ALL, B_WILL_DRAW);
		view->SetViewColor(200, 200, 21);
		BView *inner = new BView(frame.OffsetToCopy(B_ORIGIN).InsetBySelf(3, 3),
								"inner", B_FOLLOW_ALL, B_WILL_DRAW);
		inner->SetViewColor(200, 42, 42);
		view->AddChild(inner);

		BScrollView *scroller = new BScrollView("scroller", view, resizingMode,
			0, horiz[i % 4], vert[i % 4], border[i / 4]);
		if (i >= 12)
			scroller->SetBorderHighlighted(true);
		box->AddChild(scroller);

#ifdef ARCHIVE_TEST
		if (i == 7) {
			BMessage archive;
			if (scroller->Archive(&archive/*, false*/) == B_OK) {
				puts("BScrollView archived:");
				archive.PrintToStream();
			} else
				puts("archiving failed!");

			BScrollView *second = (BScrollView *)BScrollView::Instantiate(&archive);
			archive.MakeEmpty();
			if (second != NULL && second->Archive(&archive) == B_OK) {
				puts("2. BScrollView archived:");
				archive.PrintToStream();
			}
		}
#endif

#ifdef CHANGE_BORDER_TEST
		if (i % 4 == 1)
			scroller->SetBorder(B_FANCY_BORDER);
		else if (i % 4 == 2)
			scroller->SetBorder(B_PLAIN_BORDER);
		else if (i % 4 == 3)
			scroller->SetBorder(B_NO_BORDER);
#endif

		rect.OffsetBy(120, 0);
	}

#ifdef SIZE_TEST
	printf("sizeof = %lu (R5 == 212)\n", sizeof(BScrollView));
#endif

	gNumWindows++;
}


bool
Window::QuitRequested()
{
	if (--gNumWindows <= 0)
		be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark -


class KnobWindow : public BWindow {
	public:
		KnobWindow(bool horiz, bool vert);

		virtual bool QuitRequested();
};


KnobWindow::KnobWindow(bool horiz, bool vert)
	: BWindow(BRect(200, 200, 400, 400), "Scroll Knob",
			B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BRect frame = Bounds();

	if (horiz) {
		MoveBy(50, 50);
		frame.bottom -= B_H_SCROLL_BAR_HEIGHT;
	}
	if (vert) {
		MoveBy(100, 100);
		frame.right -= B_V_SCROLL_BAR_WIDTH;
	}

	BView *view = new BView(frame, "main view", B_FOLLOW_ALL, B_WILL_DRAW);
	BScrollView *scroller = new BScrollView("scroller", view, B_FOLLOW_ALL, 0, horiz, vert);
	AddChild(scroller);

	// check the SetTarget() functionality

	BView *outer = new BView(frame, "main view", B_FOLLOW_ALL, B_WILL_DRAW);
	outer->SetViewColor(200, 200, 21);
	BView *inner = new BView(frame.OffsetToCopy(B_ORIGIN).InsetBySelf(3, 3),
						"inner", B_FOLLOW_ALL, B_WILL_DRAW);
	inner->SetViewColor(200, 42, 42);
	outer->AddChild(inner);

	scroller->RemoveChild(view);
		// works with and without it

	scroller->SetTarget(NULL);
	scroller->SetTarget(outer);
	delete view;

	gNumWindows++;
}


bool 
KnobWindow::QuitRequested()
{
	if (--gNumWindows <= 0)
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
	: BApplication("application/x-vnd.obos-test")
{
}


void
Application::ReadyToRun(void)
{
	BWindow *window = new Window();
	window->Show();

	window = new KnobWindow(true, false);
	window->Show();

	window = new KnobWindow(false, true);
	window->Show();

	window = new KnobWindow(true, true);
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

