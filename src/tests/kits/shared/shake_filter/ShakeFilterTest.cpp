/*
 * Copyright 2009, Alexandre Deckner <alex@zappotek.com>
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <ShakeTrackingFilter.h>
#include <Window.h>
#include <View.h>

#include <stdio.h>


static const uint32 kMsgShake = 'Shke';


class View : public BView {
public:
						View(BRect rect, const char* name, uint32 followMode);
		virtual			~View();

		virtual	void	AttachedToWindow();
		virtual	void	MessageReceived(BMessage* msg);
};


View::View(BRect rect, const char* name, uint32 followMode)
	:
	BView(rect, name, followMode, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetHighColor(255, 0, 0);
}


View::~View()
{
}


void
View::AttachedToWindow()
{
	AddFilter(new ShakeTrackingFilter(this, kMsgShake, 3, 500000));
}


void
View::MessageReceived(BMessage* msg)
{
	if (msg->what == kMsgShake) {
		uint32 count = 0;
		msg->FindUInt32("count", &count);
		printf("Shake! %lu\n", count);
	} else {
		BView::MessageReceived(msg);
	}
}


//	#pragma mark -


int
main(int argc, char **argv)
{
	BApplication app("application/x-vnd.haiku-ShakeFilterTest");

	BWindow* window = new BWindow(BRect(100, 100, 400, 400),
		"ShakeFilter-Test", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BRect frame = window->Bounds();

	window->AddChild(new View(frame, "L ", B_FOLLOW_ALL));
	window->Show();

	app.Run();
	return 0;
}
