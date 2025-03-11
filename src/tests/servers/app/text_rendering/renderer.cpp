#include <Application.h>
#include <CheckBox.h>
#include <GridView.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <Shape.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>


#define NO_AA(x)		(((x) & 1) != 0)
#define DECORATION(x)	(((x) & 2) != 0)
#define USE_OFFSETS(x)	(((x) & 4) != 0)
#define VECTOR(x)		(((x) & 8) != 0)

#define CHANGE_AA 'chaa'

static const BString kText = "De fฝbug ";
	// 'ฝ' is a character needing a fallback font

extern void set_subpixel_antialiasing(bool subpix);
extern status_t get_subpixel_antialiasing(bool* subpix);


BString
test_description(int test)
{
	BString description;
	if (NO_AA(test))
		description << "No antialias\n";
	else
		description << "Antialias\n";
	if (DECORATION(test))
		description << "With line\n";
	else
		description << "No line\n";
	if (USE_OFFSETS(test))
		description << "Offsets\n";
	else
		description << "Start point\n";
	if (VECTOR(test))
		description << "Vector";
	else
		description << "Scan";
	return description;
}


class View : public BView {
public:
	View(int clip, int test)
		:
		BView(BRect(0, 0, 40, 50), "", 0, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
		fClip(clip)
	{
		BFont font;
		GetFont(&font);
		font.SetSize(28);
			// Greater than 30 forces vector data
		if (NO_AA(test))
			font.SetFlags(B_DISABLE_ANTIALIASING);
		if (DECORATION(test))
			font.SetFace(font.Face() | B_STRIKEOUT_FACE);
		SetFont(&font);
		fOffsets = USE_OFFSETS(test);
		fVector = VECTOR(test);
	}

	void Draw(BRect)
	{
		BRect clipRect = Bounds().InsetByCopy(10, 5);
		BShape s;

		switch (fClip) {
			case 1:
				ClipToRect(clipRect);
				break;
			case 2:
				s.MoveTo(clipRect.LeftTop());
				s.LineTo(clipRect.RightBottom());
				s.LineTo(clipRect.RightTop());
				s.LineTo(clipRect.LeftBottom());
				s.Close();
				ClipToShape(&s);
				break;
		}
		SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
		FillRect(Bounds().InsetByCopy(1, 1), B_SOLID_LOW);

		if (fVector)
			RotateBy(0.09);

		if (fOffsets) {
			int len = kText.CountChars();
			BPoint offsets[len];
			for (int i = 0; i < len; i++)
				offsets[i].Set(3 + 18 * i, 40);
			DrawString(kText, offsets, len);
		} else {
			DrawString(kText, BPoint(3, 40));
		}
	}

private:
			int		fClip;
			bool	fOffsets;
			bool	fVector;
};


class App : public BApplication {
public:
	App()
		:
		BApplication("application/x-vnd.haiku.text-renderer-test")
	{
		BWindow* window = new BWindow(BRect(100, 100, 640, 400), "Text renderer test",
			B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE);

		BGridView* grid = new BGridView();
		grid->SetResizingMode(B_FOLLOW_ALL_SIDES);

		BLayoutBuilder::Grid<> layout(grid);
		layout.SetInsets(B_USE_DEFAULT_SPACING);

		fSubpixelAA = new BCheckBox("Subpixel AA", new BMessage(CHANGE_AA));
		bool subpixel;
		get_subpixel_antialiasing(&subpixel);
		fSubpixelAA->SetValue(subpixel);
		fSubpixelAA->SetTarget(this);
		layout.Add(fSubpixelAA, 0, 0);

		for (int test = 0; test < 16; test++) {
			layout.Add(new BStringView("", test_description(test)), 0, test + 1);

			for (int clip = 0; clip <= 2; clip++)
				layout.Add(new View(clip, test), clip + 1, test + 1);
		}

		BScrollView* scroll
			= new BScrollView("scroll", grid, B_FOLLOW_ALL_SIDES, 0, false, true, B_NO_BORDER);
		window->AddChild(scroll);
		scroll->ResizeTo(window->Bounds().Size());
		window->Show();
	}

	void MessageReceived(BMessage* message)
	{
		if (message->what == CHANGE_AA)
			set_subpixel_antialiasing(fSubpixelAA->Value());
		else
			BApplication::MessageReceived(message);
	}

private:
			BControl*		fSubpixelAA;
};


int
main(int argc, char** argv)
{
	App app;
	app.Run();
	return 0;
}
