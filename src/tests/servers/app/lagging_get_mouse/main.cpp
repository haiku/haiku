// main.cpp

#include <stdio.h>

#include <Application.h>
#include <View.h>
#include <Window.h>

class TestView : public BView {

 public:
					TestView(BRect frame, const char* name,
							 uint32 resizeFlags, uint32 flags);

	virtual	void	Draw(BRect updateRect);
	virtual	void	MouseDown(BPoint where);

 private:
			BList	fMouseSamples;
};

// constructor
TestView::TestView(BRect frame, const char* name,
		uint32 resizeFlags, uint32 flags)
	: BView(frame, name, resizeFlags, flags),
	  fMouseSamples(128)
{
}

// Draw
void
TestView::Draw(BRect updateRect)
{
	int32 count = fMouseSamples.CountItems();
	if (count > 0) {
		BPoint* p = (BPoint*)fMouseSamples.ItemAtFast(0);
		MovePenTo(*p);
	}

	for (int32 i = 0; i < count; i++) {
		BPoint* p = (BPoint*)fMouseSamples.ItemAtFast(i);
		StrokeLine(*p);
	}
}

// MouseDown
void
TestView::MouseDown(BPoint where)
{
	// clear previous stroke
	int32 count = fMouseSamples.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (BPoint*)fMouseSamples.ItemAtFast(i);
	fMouseSamples.MakeEmpty();
	FillRect(Bounds(), B_SOLID_LOW);

	// sample new stroke
	uint32 buttons;
	GetMouse(&where, &buttons);
	MovePenTo(where);
	while (buttons) {

		StrokeLine(where);
		fMouseSamples.AddItem(new BPoint(where));

		snooze(20000);
		GetMouse(&where, &buttons);
	}
}


// main
int
main(int argc, char** argv)
{
	BApplication app("application/x.vnd-Haiku.BitmapBounds");

	BRect frame(50.0, 50.0, 300.0, 250.0);
	BWindow* window = new BWindow(frame, "Bitmap Bounds", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BView* view = new TestView(window->Bounds(), "test",
		B_FOLLOW_ALL, B_WILL_DRAW);
	window->AddChild(view);

	window->Show();

	app.Run();

	return 0;
}
