#include <Application.h>
#include <String.h>
#include <View.h>
#include <Window.h>

class View : public BView {
public:
	View(BRect bounds)
		: BView(bounds, "test", B_FOLLOW_ALL, B_WILL_DRAW)
	{
	}

	virtual void Draw(BRect updateRect)
	{
		BFont font;
		GetFont(&font);

		const BRect bounds = Bounds();
		const BString text = "The quick brown fox jumps over the lazy dog!";

		float size = 2.0f;
		float y = 30.0f;

		while (size < 48 && y - size < bounds.bottom) {
			font.SetSize(size);
			SetFont(&font);

			BString textAndSize = text;
			textAndSize << "  (" << size << ")";

			DrawString(textAndSize, BPoint(30.0f, y));

			y = (int)(y + size * 1.2f);
			size *= 1.08;
		}
	}
};

class Window : public BWindow {
public:
	Window()
		: BWindow(BRect(30, 30, 600, 500), "Text rendering test",
			B_DOCUMENT_WINDOW,
			B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
	{
		AddChild(new View(Bounds()));
	}

};

class Application : public BApplication {
public:
	Application()
		:BApplication("application/x-vnd.haiku.text-rendering-test")
	{
	}

	virtual void ReadyToRun()
	{
		(new Window())->Show();
	}
};


int
main()
{
	Application app;
	app.Run();
	return 0;
}
