/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Window.h>
#include <View.h>

#include <stdio.h>


class View : public BView {
public:
						View(BRect rect, const char* name, uint32 followMode,
							uint8 red, uint8 green, uint8 blue);
		virtual			~View();

		virtual void	Draw(BRect updateRect);

		virtual	void	MouseDown(BPoint where);
		virtual	void	MouseMoved(BPoint where, uint32 transit,
							const BMessage* dragMessage);

private:
				void	_SetTransit(uint32 transit);

				uint32	fLastTransit;
};


View::View(BRect rect, const char* name, uint32 followMode,
		uint8 red, uint8 green, uint8 blue)
	: BView(rect, name, followMode, B_WILL_DRAW),
	  fLastTransit(B_OUTSIDE_VIEW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetHighColor(red, green, blue);
}


View::~View()
{
}


void
View::Draw(BRect updateRect)
{
	if (fLastTransit == B_INSIDE_VIEW || fLastTransit == B_ENTERED_VIEW)
		FillRect(updateRect);
}


void
View::MouseDown(BPoint where)
{
	uint32 buttons;
	do {
		GetMouse(&where, &buttons);
	} while (buttons != 0);
}


void
View::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	_SetTransit(transit);
}


void
View::_SetTransit(uint32 transit)
{
	if (transit == fLastTransit)
		return;

	switch (transit) {
		case B_ENTERED_VIEW:
			if (fLastTransit == B_INSIDE_VIEW)
				printf("%s, B_ENTERED_VIEW, but already inside!\n", Name());
			break;

		case B_EXITED_VIEW:
			if (fLastTransit == B_OUTSIDE_VIEW)
				printf("%s, B_EXITED_VIEW, but already outside!\n", Name());
			break;

		case B_INSIDE_VIEW:
			if (fLastTransit == B_OUTSIDE_VIEW)
				printf("%s, B_INSIDE_VIEW, but never entered!\n", Name());
			if (fLastTransit == B_EXITED_VIEW)
				printf("%s, B_INSIDE_VIEW, but just exited!\n", Name());
			break;

		case B_OUTSIDE_VIEW:
			if (fLastTransit == B_INSIDE_VIEW)
				printf("%s, B_OUTSIDE_VIEW, but never exited!\n", Name());
			if (fLastTransit == B_ENTERED_VIEW)
				printf("%s, B_OUTSIDE_VIEW, but just entered!\n", Name());
			break;
	}

	fLastTransit = transit;
	Invalidate();
}


//	#pragma mark -


int 
main(int argc, char **argv)
{
	BApplication app("application/x-vnd.haiku-view_transit");

	BWindow* window = new BWindow(BRect(100, 100, 400, 400),
		"ViewTransit-Test", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BRect frame = window->Bounds();
	frame.right /= 2;
	window->AddChild(new View(frame, "L ", B_FOLLOW_ALL, 255, 0, 0));
	frame.left = frame.right + 1;
	frame.right = window->Bounds().right;
	View* view = new View(frame, "R", B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT,
		0, 255, 0);
	window->AddChild(view);
	view->SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);

	window->Show();

	app.Run();
	return 0;
}
