#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Message.h>
#include <Picture.h>
#include <ScrollView.h>
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
			void				Test4(BRect updateRect);
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
	// If all went well, this should cover the first line of text and the
	// 200x20 rectangle below it.
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

	FillEllipse(BRect(10, 10, 40, 40));

	PopState();
	EndPicture();

	ClipToInversePicture(&clipper, BPoint(10, 50));

	FillRect(BRect(10, 50, 60, 100));

}


void
TestView::Test3(BRect updateRect)
{
	// Check that the clipping can be undone.
	ClipToPicture(NULL);

	// Paint the whole unclipped region green.
	// If all went well, this should cover the whole window.
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
TestView::Test4(BRect updateRect)
{
	// Test multiple clipping pictures with Push/PopState()
	
	// First clipping is a circle
	BPicture clipper;
	BeginPicture(&clipper);
	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	FillEllipse(BRect(70, 50, 120, 100));

	PopState();
	EndPicture();

	ClipToPicture(&clipper);

	// This should push the first clipping
	PushState();

	// Second clipping is another circle, offset to the right.
	BPicture clipper2;
	BeginPicture(&clipper2);
	PushState();

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	FillEllipse(BRect(100, 50, 150, 100));

	PopState();
	EndPicture();
	ClipToPicture(&clipper2);

	// This should only paint the intersection of the two circles.
	// No other part of the circles should ever be visible.
	FillRect(BRect(70, 50, 150, 100));

	// ... and back to clipping only the first circle.
	PopState();

}


void
TestView::Draw(BRect updateRect)
{
	Test1(updateRect);
	Test2(updateRect);

	// Paint the whole unclipped region blue.
	// If all went well, this should cover the whole window, except the
	// clipped-out circle
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

	Test3(updateRect);
	Test4(updateRect);

	SetOrigin(5,55);
	Test2(updateRect);

	SetOrigin(50,55);
	SetScale(2);
	Test2(updateRect);
	SetScale(1);
}


// #pragma mark -


int
main(int argc, char** argv)
{
	BApplication app(kAppSignature);

	BWindow* window = new BWindow(BRect(50.0, 50.0, 300.0, 250.0),
		"ClipToPicture Test", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BRect targetRect = window->Bounds();
	targetRect.right -= B_V_SCROLL_BAR_WIDTH;
	targetRect.bottom -= B_H_SCROLL_BAR_HEIGHT;
	BView* view = new TestView(targetRect, "test", B_FOLLOW_ALL,
		B_WILL_DRAW);
	BScrollView* scroll = new BScrollView("scroll", view, B_FOLLOW_ALL, 0,
		true, true);
	window->AddChild(scroll);

	window->Show();

	app.Run();
	return 0;
}
