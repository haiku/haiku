/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <ScrollView.h>

#include <Application.h>
#include <Window.h>
#include <Box.h>

#include <stdio.h>


#define TESTS


class Window : public BWindow {
	public:
		Window();
		virtual ~Window();
		
		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 580, 580), "Scroll-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE)
{
	const bool horiz[] = {false, true, false, true};
	const bool vert[] = {false, false, true, true};
	const border_style border[] = {B_NO_BORDER, B_PLAIN_BORDER, B_FANCY_BORDER, B_FANCY_BORDER};
	BRect rect(1, 1, 119, 119);
	BRect frame = rect.InsetByCopy(20, 20);

	for (int32 i = 0; i < 16; i++) {
		if (i > 0 && (i % 4) == 0)
			rect.OffsetBy(-480, 120);

		BBox *box = new BBox(rect);
		AddChild(box);

		BView *view = new BView(frame, "main view", B_FOLLOW_ALL, B_WILL_DRAW);
		BView *inner = new BView(frame.OffsetToCopy(B_ORIGIN).InsetBySelf(2, 2),
								"inner", B_FOLLOW_ALL, B_WILL_DRAW);
		inner->SetViewColor(200, 42, 42);
		view->AddChild(inner);

		BScrollView *scroller = new BScrollView("scroller", view, B_FOLLOW_ALL,
			0, horiz[i % 4], vert[i % 4], border[i / 4]);
		if (i >= 12)
			scroller->SetBorderHighlighted(true);
		box->AddChild(scroller);

#ifdef TESTS
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

		if (i % 4 == 3)
			scroller->SetBorder(B_FANCY_BORDER);
#endif

		rect.OffsetBy(120, 0);
	}

#ifdef TESTS
	printf("sizeof = %lu\n", sizeof(BScrollView));
#endif
}


Window::~Window()
{
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
	: BApplication("application/x-vnd.obos-test")
{
}


void
Application::ReadyToRun(void)
{
	Window *window = new Window();
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

