// main.cpp

#include <stdio.h>

#include "Application.h"
#include "TextControl.h"
#include "View.h"
#include "Window.h"

class HelloView : public BView {
 public:
					HelloView(BRect frame, const char* name,
							  uint32 resizeFlags, uint32 flags)
						: BView(frame, name, resizeFlags, flags)
					{ }

	virtual	void	Draw(BRect updateRect)
					{
//				printf("HelloView::Draw()\n");
//				updateRect.PrintToStream();

//						SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
//						FillRect(updateRect);
						BRect r(Bounds());
//				r.PrintToStream();

						const char* message = "Hello World!";
						float width = StringWidth(message);
						BPoint p(r.left + r.Width() / 2.0 - width / 2.0,
								 r.top + r.Height() / 2.0);

						SetHighColor(255, 0, 0, 128);
						DrawString(message, p);
					}
};


// show_window
void
show_window(BRect frame, const char* name)
{
	BWindow* window = new BWindow(frame, name,
								  B_TITLED_WINDOW,
								  B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BView* view = new HelloView(window->Bounds(), "test", B_FOLLOW_ALL,
								B_WILL_DRAW/* | B_FULL_UPDATE_ON_RESIZE*/);

	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	window->AddChild(view);
/*	BRect b(view->Bounds());
	b.InsetBy(10.0, 20.0);
	BTextControl* control = new BTextControl(b, "tc", "Text", "Enter Text Here", NULL);
	view->AddChild(control);*/

	window->Show();
}

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x.vnd-Haiku.windows_test");

	BRect frame(50.0, 50.0, 200.0, 150.0);
	show_window(frame, "Window #1");

	frame.Set(80.0, 100.0, 250.0, 200.0);
	show_window(frame, "Window #2");

	app->Run();

	delete app;
	return 0;
}
