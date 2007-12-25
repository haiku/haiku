
#include <stdio.h>

#include <Application.h>
#include <View.h>
#include <Window.h>

/*-----------------------------------------------------------------------------

OBSERVATION on R5 behaviour:

* The hook function DrawAfterChildren is not called at all if the
  view flags don't include B_DRAW_ON_CHILDREN.

* If the view flags include B_DRAW_ON_CHILDREN, then any drawing commands
  executed in Draw() AND DrawAfterChildren() will paint on top of children.

* The background of a view with the B_DRAW_ON_CHILDREN flag set will not
  be painted by the app_server when child views change position and areas
  in the parent view are "exposed". If the expose events have other reasons,
  the background is painted as usual.

* The app_server side background painting of child views does not occur
  after the Draw() hook of the parent view with B_DRAW_ON_CHILDREN has been
  called. So while DrawAfterChildren() may be called after the Draw() hooks
  of all children have been called, the background has been painted earlier.

* It looks like the background painting inside app_server of a view with
  B_DRAW_ON_CHILDREN paints over the background of any children, though
  the background of the children is later painted too. Therefor, if a child
  has B_TRANSPARENT_COLOR background, the background of the parent with
  B_DRAW_ON_CHILDREN stays visible in the area of that child.

* Both Draw() and DrawAfterChildren() appear to push their own graphics
  states onto the state stack.


CONCLUSION:

It looks like the B_DRAW_ON_CHILDREN flag causes two effects:

* The local view clipping region inside the app_server simply ignores
  any children, this effects any drawing commands, those from Draw()
  and those from DrawAfterChildren()

* The DrawAfterChildren() hook is called after the children have drawn,
  so that the user may move all drawing functions there which he does not
  wish to have painted over by children.

That areas exposed by moving child views are not repainted could
be considered a bug of the R5 implementation.

-----------------------------------------------------------------------------*/



class TestView : public BView {
public:
	TestView(BRect frame);
	~TestView();

	virtual	void Draw(BRect updateRect);
	virtual void DrawAfterChildren(BRect updateRect);
};


TestView::TestView(BRect frame)
	: BView(frame, "TestView", B_FOLLOW_ALL,
		B_WILL_DRAW | B_DRAW_ON_CHILDREN | B_FULL_UPDATE_ON_RESIZE)
{
	SetViewColor(200, 220, 255);
}


TestView::~TestView()
{
}


void
TestView::Draw(BRect updateRect)
{
	printf("Draw(BRect(%.1f, %.1f, %.1f, %.1f))\n",
		updateRect.left, updateRect.top, updateRect.right, updateRect.bottom);

	printf("pensize: %.2f\n", PenSize());

	SetHighColor(0, 0, 255);
	StrokeLine(Bounds().LeftBottom(), Bounds().RightTop());

	SetPenSize(5);
}

void
TestView::DrawAfterChildren(BRect updateRect)
{
	printf("DrawAfterChildren(BRect(%.1f, %.1f, %.1f, %.1f))\n",
		updateRect.left, updateRect.top, updateRect.right, updateRect.bottom);

	printf("pensize: %.2f\n", PenSize());

	SetHighColor(255, 0, 0);
	StrokeLine(Bounds().LeftTop(), Bounds().RightBottom());
	Sync();

	SetPenSize(7);
}


//	#pragma mark -

class ChildView : public BView {
public:
	ChildView(BRect frame, const char* name, rgb_color viewColor);
	~ChildView();

	virtual	void Draw(BRect updateRect);
};


ChildView::ChildView(BRect frame, const char* name, rgb_color viewColor)
	: BView(frame, name, B_FOLLOW_ALL, 0)
{
	SetLowColor(200, 200, 200);
	SetViewColor(viewColor);
	if (*(int32*)&viewColor == *(int32*)&B_TRANSPARENT_COLOR)
		SetFlags(Flags() | B_WILL_DRAW);
}


ChildView::~ChildView()
{
}


void
ChildView::Draw(BRect updateRect)
{
	FillRect(updateRect, B_SOLID_LOW);
}


// #pragma mark -


int
main(int argc, char** argv)
{
	BApplication app("application/x-vnd.Haiku-DrawAfterChildren");

	BRect frame(100, 100, 700, 400);
	BWindow* window = new BWindow(frame, "Window",
		B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE);

	frame.OffsetTo(B_ORIGIN);
	TestView* view = new TestView(frame);
	window->AddChild(view);

	frame.InsetBy(20, 20);
	frame.right = frame.left + frame.Width() / 2 - 10;
	BView* child = new ChildView(frame, "child 1",
		(rgb_color){ 200, 200, 200, 255 });
	view->AddChild(child);

	frame.OffsetBy(frame.Width() + 20, 0);
	child = new ChildView(frame, "child 2", B_TRANSPARENT_COLOR);
	view->AddChild(child);

	window->Show();

	app.Run();
	return 0;
}

