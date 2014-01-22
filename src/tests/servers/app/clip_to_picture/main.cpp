#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Message.h>
#include <Picture.h>
#include <View.h>
#include <Window.h>


static const char* kAppSignature = "application/x.vnd-Haiku.ShapeTest";


class TestView : public BView {
public:
								TestView(BRect frame, const char* name,
									uint32 resizeFlags, uint32 flags);
			void				AttachedToWindow();

	virtual	void				Draw(BRect updateRect);
};


TestView::TestView(BRect frame, const char* name, uint32 resizeFlags,
		uint32 flags)
	:
	BView(frame, name, resizeFlags, flags)
{
}


void
TestView::AttachedToWindow()
{
	BPicture picture;

	BeginPicture(&picture);
	DrawString("Hello World! - Clipping", BPoint(10, 10));

	FillRect(BRect(0, 20, 400, 40));
	EndPicture();

	ClipToPicture(&picture);
}


void
TestView::Draw(BRect updateRect)
{
	SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR)); // opaque black.
	DrawString("Hello World! - Clipped", BPoint(10, 30));
	FillRect(BRect(0, 0, 200, 20));
}


// #pragma mark -


int
main(int argc, char** argv)
{
	BApplication app(kAppSignature);

	BWindow* window = new BWindow(BRect(50.0, 50.0, 300.0, 250.0),
		"ClipToPicture Test", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BView* view = new TestView(window->Bounds(), "test", B_FOLLOW_ALL,
		B_WILL_DRAW);
	window->AddChild(view);

	window->Show();

	app.Run();
	return 0;
}
