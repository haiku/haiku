/*
 * Copyright 2019, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Distributed under terms of the MIT license.
 */


#include <Application.h>
#include <Bitmap.h>
#include <View.h>
#include <Window.h>

#include <assert.h>


class BitmapView: public BView
{
	public:
		BitmapView(BBitmap* bitmap)
			: BView(bitmap->Bounds(), "test view", B_FOLLOW_LEFT_TOP, B_WILL_DRAW)
			, fBitmap(bitmap)
		{
		}

		~BitmapView()
		{
			delete fBitmap;
		}

		void Draw(BRect updateRect)
		{
			DrawBitmap(fBitmap);
		}

	private:
		BBitmap* fBitmap;
};


int
main(void)
{
	BApplication app("application/Haiku-BitmapTest");

	BWindow* window = new BWindow(BRect(10, 10, 100, 100),
		"Bitmap drawing test", B_DOCUMENT_WINDOW, B_QUIT_ON_WINDOW_CLOSE);
	window->Show();

	BBitmap* bitmap = new BBitmap(BRect(0, 0, 24, 24), B_GRAY1);

	// Bitmap is 25 pixels wide, which rounds up to 4 pixels
	// The last byte only has one bit used, and 7 bits of padding
	assert(bitmap->BytesPerRow() == 4);

	// This was extracted from letter_a.pbm and should look mostly like a
	// black "A" letter on a white background (confirmed BeOS behavior)
	const unsigned char data[] = {
		0, 0, 0, 0,
		0, 8, 0, 0,
		0, 0x1c, 0, 0,
		0, 0x3e, 0, 0,
		0, 0x7e, 0, 0,
		0, 0xFF, 0, 0,
		0, 0xE7, 0, 0,
		0, 0xC3, 0, 0,
		1, 0xC3, 0x80, 0,
		1, 0x81, 0x80, 0,
		3, 0x81, 0xC0, 0,
		3, 0xFF, 0xC0, 0,
		7, 0xFF, 0xE0, 0,
		7, 0xFF, 0xE0, 0,
		7, 0x81, 0xE0, 0,
		0x0F, 0, 0x0F, 0,
		0x0F, 0, 0x0F, 0,
		0x1F, 0, 0xF8, 0,
		0x1E, 0, 0x78, 0,
		0x1C, 0, 0x38, 0,
		0x3C, 0, 0x3C, 0,
		0x3C, 0, 0x3C, 0,
		0x38, 0, 0x0E, 0,
		0x78, 0, 0x0F, 0,
		0, 0, 0, 0
	};
	bitmap->SetBits(data, sizeof(data), 0, B_GRAY1);

	BView* view = new BitmapView(bitmap);
	window->AddChild(view);

	app.Run();
	return 0;
}
