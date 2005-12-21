// main.cpp

#include <stdio.h>

#include "Application.h"
#include "Message.h"
#include "Button.h"
#include "View.h"
#include "Window.h"

enum {
	MSG_RESET = 'rset'
};

class TestView : public BView {

 public:
					TestView(BRect frame, const char* name,
							  uint32 resizeFlags, uint32 flags)
						: BView(frame, name, resizeFlags, flags),
						  fTracking(false),
						  fLastMousePos(0.0, 0.0)
					{
						SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
						SetLowColor(ViewColor());
					}

	virtual	void	MessageReceived(BMessage* message);

	virtual	void	Draw(BRect updateRect);

	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseUp(BPoint where);
	virtual	void	MouseMoved(BPoint where, uint32 transit,
							   const BMessage* dragMessage);

 private:
			bool	fTracking;

			BPoint	fLastMousePos;
};

// MessageReceived
void
TestView::MessageReceived(BMessage* message)
{
	if (message->what == MSG_RESET)
		ScrollTo(0.0, 0.0);
	else
		BView::MessageReceived(message);
}

// Draw
void
TestView::Draw(BRect updateRect)
{
	// ellipses
	SetHighColor(200, 90, 0, 100);
	StrokeEllipse(BPoint(80.0, 50.0), 70.0, 40.0);

	SetDrawingMode(B_OP_ALPHA);

	SetPenSize(2.0);
	SetHighColor(20, 40, 180, 150);
	StrokeEllipse(BPoint(230.0, 90.0), 14.0, 60.0);

	SetPenSize(5.0);
	SetHighColor(60, 20, 110, 100);
	StrokeEllipse(BPoint(100.0, 180.0), 35.0, 25.0);

	// text
	SetHighColor(0, 0, 0, 255);
	SetDrawingMode(B_OP_OVER);
	const char* message = "Click and drag to scroll this view!";
	DrawString(message, BPoint(0.0, 30.0));
}

// MouseDown
void
TestView::MouseDown(BPoint where)
{
	fTracking = true;
	fLastMousePos = where;
	SetMouseEventMask(B_POINTER_EVENTS);
}

// MouseUp
void
TestView::MouseUp(BPoint where)
{
	fTracking = false;
}

// MouseMoved
void
TestView::MouseMoved(BPoint where, uint32 transit,
					 const BMessage* dragMessage)
{
	if (fTracking) {
		BPoint offset = fLastMousePos - where;
		ScrollBy(offset.x, offset.y);
		fLastMousePos = where + offset;
	}
}



// show_window
void
show_window(BRect frame, const char* name)
{
	BWindow* window = new BWindow(frame, name,
								  B_TITLED_WINDOW,
								  B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BView* view = new TestView(window->Bounds(), "test", B_FOLLOW_ALL,
							   B_WILL_DRAW/* | B_FULL_UPDATE_ON_RESIZE*/);

	window->AddChild(view);
	BRect b(0.0, 0.0, 60.0, 15.0);
	b.OffsetTo(5.0, view->Bounds().bottom - (b.Height() + 15.0));
	BButton* control = new BButton(b, "button", "Reset", new BMessage(MSG_RESET));
	view->AddChild(control);
	control->SetTarget(view);

	window->Show();
}

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x.vnd-Haiku.Scrolling");

	BRect frame(50.0, 50.0, 300.0, 250.0);
	show_window(frame, "Scrolling Test");

	app->Run();

	delete app;
	return 0;
}
