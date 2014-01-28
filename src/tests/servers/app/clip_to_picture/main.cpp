#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Message.h>
#include <Picture.h>
#include <View.h>
#include <Window.h>


static const char* kAppSignature = "application/x.vnd-Haiku.ClipToPicture";


class TestView : public BView {
public:
								TestView(BRect frame, const char* name,
									uint32 resizeFlags, uint32 flags);

	virtual	void				Draw(BRect updateRect);

private:
			void				Test1(BRect updateRect);
			void				Test2(BRect updateRect);
			void				Test3(BRect updateRect);
};


TestView::TestView(BRect frame, const char* name, uint32 resizeFlags,
		uint32 flags)
	:
	BView(frame, name, resizeFlags, flags)
{
}


void
TestView::Test1(BRect updateRect)
{
	BPicture clipper;

	// Test antialiased clipping to a text
	BeginPicture(&clipper);
	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	DrawString("Hello World! - Clipping", BPoint(10, 10));
	FillRect(BRect(0, 20, 200, 40));

	PopState();
	EndPicture();

	ClipToPicture(&clipper);

	// This string is inside the clipping rectangle. It is completely drawn.
	DrawString("Hello World! - Clipped", BPoint(10, 30));
	// This rect is above the clipping string. Only the glyphs are painted.
	FillRect(BRect(0, 0, 200, 20));

	// Paint the whole unclipped region red.
	PushState();
	SetDrawingMode(B_OP_ALPHA);
	rgb_color color;
	color.red = 255;
	color.green = 0;
	color.blue = 0;
	color.alpha = 64;
	SetHighColor(color);
	FillRect(Bounds());
	PopState();
}


void
TestView::Test2(BRect updateRect)
{
	BPicture clipper;

	// Test inverse clipping: a round hole in a rect
	BeginPicture(&clipper);
	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	FillEllipse(BRect(20, 60, 50, 90));

	PopState();
	EndPicture();

	ClipToInversePicture(&clipper);

	FillRect(BRect(10, 50, 60, 100));

	// Paint the whole unclipped region blue.
	PushState();
	SetDrawingMode(B_OP_ALPHA);
	rgb_color color;
	color.red = 0;
	color.green = 0;
	color.blue = 255;
	color.alpha = 64;
	SetHighColor(color);
	FillRect(Bounds());
	PopState();
}


void
TestView::Test3(BRect updateRect)
{
	// Check that the clipping can be undone.
	ClipToPicture(NULL);

	// Paint the whole unclipped region green.
	PushState();
	SetDrawingMode(B_OP_ALPHA);
	rgb_color color;
	color.red = 0;
	color.green = 255;
	color.blue = 0;
	color.alpha = 64;
	SetHighColor(color);
	FillRect(Bounds());
	PopState();
}


void
TestView::Draw(BRect updateRect)
{
	Test1(updateRect);
	Test2(updateRect);
	Test3(updateRect);
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
