// main.cpp

#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <View.h>
#include <Window.h>

class TestView : public BView {

 public:
					TestView(BRect frame, const char* name,
							 uint32 resizeFlags, uint32 flags);

	virtual	void	Draw(BRect updateRect);

 private:
	BBitmap*		fBitmap;
};

//#define LEFT_OFFSET 0
//#define TOP_OFFSET 0
#define LEFT_OFFSET 30
#define TOP_OFFSET 30

// constructor
TestView::TestView(BRect frame, const char* name,
				   uint32 resizeFlags, uint32 flags)
	: BView(frame, name, resizeFlags, flags),

	  fBitmap(new BBitmap(BRect(0 + LEFT_OFFSET,
	  							0 + TOP_OFFSET,
	  							99 + LEFT_OFFSET,
	  							99 + TOP_OFFSET),
	  					  B_BITMAP_CLEAR_TO_WHITE,
	  					  B_RGB32))
{
	SetViewColor(216, 216, 216);

	uint8* bits = (uint8*)fBitmap->Bits();
	uint32 width = fBitmap->Bounds().IntegerWidth() + 1;
	uint32 height = fBitmap->Bounds().IntegerHeight() + 1;
	uint32 bpr = fBitmap->BytesPerRow();

	// top row -> red
	uint8* b = bits;
	for (uint32 x = 0; x < width; x++) {
		b[0] = 0;
		b[1] = 0;
		b += 4;
	}

	// left/right edge pixels -> red
	b = bits + bpr;
	for (uint32 y = 1; y < height - 1; y++) {
		b[0] = 0;
		b[1] = 0;
		b[(width - 1) * 4 + 0] = 0;
		b[(width - 1) * 4 + 1] = 0;
		b += bpr;
	}

	// bottom row -> red
	for (uint32 x = 0; x < width; x++) {
		b[0] = 0;
		b[1] = 0;
		b += 4;
	}
}

// Draw
void
TestView::Draw(BRect updateRect)
{
//	DrawBitmap(BBitmap*, BRect source, BRect destination);
//
//  BeBook:
//  If a source rectangle is given, only that part of the
//  bitmap image is drawn. Otherwise, the entire bitmap
//  is placed in the view. The source rectangle is stated
//  in the internal coordinates of the BBitmap object.

// Test 1:
	// if the above was true, then we should see the left
	// top area of the bitmap...
//	BRect view(0, 0, 50, 50);
//	BRect bitmap = view.OffsetByCopy(fBitmap->Bounds().LeftTop());
//
//	DrawBitmap(fBitmap, bitmap, view);

// Test 2:
	// if the above was true, we should simply see the entire
	// bitmap at the left top corner of the view
	BRect bitmap = fBitmap->Bounds();
	BRect view = bitmap.OffsetToCopy(B_ORIGIN);

	DrawBitmap(fBitmap, bitmap, view);
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
