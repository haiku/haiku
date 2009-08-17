/*
 * Copyright 2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <stdlib.h>

#include <Application.h>
#include <MessageRunner.h>
#include <Window.h>
#include <View.h>

#include <WindowPrivate.h>


class ShowInvalidationView : public BView {
public:
							ShowInvalidationView(BRect rect);
	virtual					~ShowInvalidationView();

	virtual void			Draw(BRect updateRect);
};


class ShowingWindow : public BWindow {
public:
							ShowingWindow();
};


class ChangingWindow : public BWindow {
public:
							ChangingWindow();
	virtual					~ChangingWindow();

	virtual void			MessageReceived(BMessage* message);

private:
			BMessageRunner*	fRunner;
};


class Application : public BApplication {
public:
							Application();

	virtual void			ReadyToRun();
};


ShowInvalidationView::ShowInvalidationView(BRect rect)
	:
	BView(rect, "show invalidation", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}


ShowInvalidationView::~ShowInvalidationView()
{
}


void
ShowInvalidationView::Draw(BRect updateRect)
{
	SetHighColor(rand() % 256, rand() % 256, rand() % 256);
	FillRect(updateRect);
}


//	#pragma mark -


ShowingWindow::ShowingWindow()
	:
	BWindow(BRect(150, 150, 450, 450), "WindowInvalidation-Test",
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
	BView* view = new ShowInvalidationView(Bounds());
	AddChild(view);
}


//	#pragma mark -


ChangingWindow::ChangingWindow()
	:
	BWindow(BRect(150, 150, 400, 400), "WindowInvalidation-Test",
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
	BMessage message('actn');
	fRunner = new BMessageRunner(this, &message, 25000);
}


ChangingWindow::~ChangingWindow()
{
	delete fRunner;
}


void
ChangingWindow::MessageReceived(BMessage* message)
{
	if (message->what == 'actn') {
		switch (rand() % 4) {
			case 0:
			{
				// resize window
				BRect bounds;
				do {
					bounds = Bounds();
					bounds.right += rand() % 21 - 10;
					bounds.bottom += rand() % 21 - 10;
				} while (bounds.Width() > 400 || bounds.Height() > 400
					|| bounds.Width() < 50 || bounds.Height() < 50);

				ResizeTo(bounds.Width() + 1, bounds.Height() + 1);
				break;
			}

			case 1:
			{
				// move window
				BPoint leftTop;
				do {
					leftTop = Frame().LeftTop();
					leftTop.x += rand() % 21 - 10;
					leftTop.y += rand() % 21 - 10;
				} while (!BRect(100, 100, 200, 200).Contains(leftTop));

				MoveTo(leftTop);
				break;
			}

			case 2:
			{
				// set title
				static const char* kChoices[]
					= {"Window", "Invalidation", "Test", "Hooray"};

				SetTitle(kChoices[rand() % (sizeof(kChoices) / sizeof(char*))]);
				break;
			}

			case 3:
			{
				// change look
				static const window_look kLooks[]
					= {B_TITLED_WINDOW_LOOK, B_DOCUMENT_WINDOW_LOOK,
						B_FLOATING_WINDOW_LOOK, kLeftTitledWindowLook};

				SetLook(kLooks[rand() % (sizeof(kLooks) / sizeof(kLooks[0]))]);
				break;
			}
		}
	} else
		BWindow::MessageReceived(message);
}


//	#pragma mark -


Application::Application()
	:
	BApplication("application/x-vnd.haiku-view_state")
{
}


void
Application::ReadyToRun()
{
	BWindow* window = new ChangingWindow();
	window->Show();

	window = new ShowingWindow();
	window->Show();
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	Application app;
	app.Run();

	return 0;
}
