
#include <Application.h>
#include <View.h>
#include <Window.h>


class MouseView : public BView {
public:
	MouseView(BRect frame, bool noHistory);
	~MouseView();

	virtual void AttachedToWindow();
	virtual void MouseMoved(BPoint point, uint32 transit,
		const BMessage *message);
	virtual void MouseDown(BPoint point);
	virtual void MouseUp(BPoint point);

private:
	bool	fNoHistory;
};


MouseView::MouseView(BRect frame, bool noHistory)
	: BView(frame, "MouseView", B_FOLLOW_ALL, B_WILL_DRAW),
	fNoHistory(noHistory)
{
	SetViewColor(255, 255, 200);
	if (noHistory)
		SetHighColor(200, 0, 0);
	else
		SetHighColor(0, 200, 0);
}


MouseView::~MouseView()
{
}


void
MouseView::AttachedToWindow()
{
	if (fNoHistory)
		SetEventMask(0, B_NO_POINTER_HISTORY);
}


void
MouseView::MouseDown(BPoint point)
{
	SetMouseEventMask(0, B_NO_POINTER_HISTORY);
	SetHighColor(0, 0, 200);
}


void
MouseView::MouseUp(BPoint point)
{
	if (fNoHistory)
		SetHighColor(200, 0, 0);
	else
		SetHighColor(0, 200, 0);
}


void
MouseView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	FillRect(BRect(point - BPoint(1, 1), point + BPoint(1, 1)));
	snooze(25000);
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	BApplication app("application/x-vnd.Simon-NoPointerHistory");

	BWindow* window = new BWindow(BRect(100, 100, 700, 400), "Window",
		B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE);
	window->AddChild(new MouseView(BRect(10, 10, 295, 290), true));
	window->AddChild(new MouseView(BRect(305, 10, 590, 290), false));
	window->Show();

	app.Run();
	return 0;
}

