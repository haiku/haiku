#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Message.h>
#include <Shape.h>
#include <View.h>
#include <Window.h>


static const char* kAppSignature = "application/x.vnd-Haiku.DrawStringOffsets";


class TestView : public BView {
public:
								TestView(BRect frame, const char* name,
									uint32 resizeFlags, uint32 flags);

	virtual	void				Draw(BRect updateRect);
};


TestView::TestView(BRect frame, const char* name, uint32 resizeFlags,
		uint32 flags)
	:
	BView(frame, name, resizeFlags, flags)
{
}


void
TestView::Draw(BRect updateRect)
{
	BPoint offsets[5];
	offsets[0].x = 10;
	offsets[0].y = 10;
	offsets[1].x = 15;
	offsets[1].y = 10;
	offsets[2].x = 20;
	offsets[2].y = 18;
	offsets[3].x = 23;
	offsets[3].y = 12;
	offsets[4].x = 30;
	offsets[4].y = 10;

	DrawString("Hello", offsets, 5);
}


// #pragma mark -


int
main(int argc, char** argv)
{
	BApplication app(kAppSignature);

	BWindow* window = new BWindow(BRect(50.0, 50.0, 300.0, 250.0),
		"DrawString() with offsets", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BView* view = new TestView(window->Bounds(), "test", B_FOLLOW_ALL,
		B_WILL_DRAW);
	window->AddChild(view);

	window->Show();

	app.Run();
	return 0;
}
