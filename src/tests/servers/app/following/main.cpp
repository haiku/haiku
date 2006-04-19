// main.cpp

#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Message.h>
#include <Button.h>
#include <View.h>
#include <Window.h>

enum {
	TRACKING_NONE = 0,
	TRACKING_ALL,
	TRACKING_RIGHT,
	TRACKING_BOTTOM,
	TRACKING_RIGHT_BOTTOM,
};

class TestView : public BView {

 public:
					TestView(BRect frame, const char* name,
							  uint32 resizeFlags, uint32 flags)
						: BView(frame, name, resizeFlags, flags),
						  fTracking(TRACKING_NONE),
						  fLastMousePos(0.0, 0.0)
					{
						rgb_color color;
						color.red = rand() / 256;
						color.green = rand() / 256;
						color.blue = rand() / 256;
						color.alpha = 255;
						SetViewColor(color);
						SetLowColor(color);
					}

	virtual	void	Draw(BRect updateRect);

	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseUp(BPoint where);
	virtual	void	MouseMoved(BPoint where, uint32 transit,
							   const BMessage* dragMessage);

 private:
			uint32	fTracking;

			BPoint	fLastMousePos;
};

// Draw
void
TestView::Draw(BRect updateRect)
{
	// text
	SetHighColor(0, 0, 0, 255);
	const char* message = "Click and drag to move this view!";
	DrawString(message, BPoint(20.0, 30.0));

	BRect r(Bounds());
	r.right -= 15.0;
	r.bottom -= 15.0;
	StrokeLine(r.RightTop(), BPoint(r.right, Bounds().bottom));
	StrokeLine(r.LeftBottom(), BPoint(Bounds().right, r.bottom));
}

// MouseDown
void
TestView::MouseDown(BPoint where)
{
	BRect r(Bounds());
	r.right -= 15.0;
	r.bottom -= 15.0;
	if (r.Contains(where))
		fTracking = TRACKING_ALL;
	else if (r.bottom < where.y && r.right < where.x)
		fTracking = TRACKING_RIGHT_BOTTOM;
	else if (r.bottom < where.y)
		fTracking = TRACKING_BOTTOM;
	else if (r.right < where.x)
		fTracking = TRACKING_RIGHT;

	fLastMousePos = where;
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

// MouseUp
void
TestView::MouseUp(BPoint where)
{
	fTracking = TRACKING_NONE;
}

// MouseMoved
void
TestView::MouseMoved(BPoint where, uint32 transit,
					 const BMessage* dragMessage)
{
	BPoint offset = where - fLastMousePos;
	switch (fTracking) {
		case TRACKING_ALL:
			MoveBy(offset.x, offset.y);
			// fLastMousePos stays fixed
			break;
		case TRACKING_RIGHT:
			ResizeBy(offset.x, 0.0);
			fLastMousePos = where;
			break;
		case TRACKING_BOTTOM:
			ResizeBy(0.0, offset.y);
			fLastMousePos = where;
			break;
		case TRACKING_RIGHT_BOTTOM:
			ResizeBy(offset.x, offset.y);
			fLastMousePos = where;
			break;
	}
}



// show_window
void
show_window(BRect frame, const char* name)
{
	BWindow* window = new BWindow(frame, name,
								  B_TITLED_WINDOW,
								  B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BView* view = new TestView(window->Bounds(), "test 1", B_FOLLOW_ALL,
							   B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);

	window->AddChild(view);

	BRect bounds = view->Bounds();
	bounds.InsetBy(20, 20);
	BView* view1 = new TestView(bounds, "test 2", B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM,
								B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	view->AddChild(view1);

	bounds = view1->Bounds();
	bounds.InsetBy(20, 20);
	BView* view2 = new TestView(bounds, "test 3", B_FOLLOW_NONE,
								B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	view1->AddChild(view2);


	window->Show();
}

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x.vnd-Haiku.Following");

	BRect frame(50.0, 50.0, 300.0, 250.0);
	show_window(frame, "Following Test");

	app->Run();

	delete app;
	return 0;
}
