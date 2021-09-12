#include <Application.h>
#include <GridView.h>
#include <LayoutBuilder.h>
#include <Picture.h>
#include <ScrollView.h>
#include <Shape.h>
#include <View.h>
#include <Window.h>


typedef void (*clipper)(BView*, BRect, bool);
typedef void (*test)(BView*, clipper);

static const BRect bigRect(-1000,-1000, 1000,1000);
static const BRect baseRect(-25,-10, 25,10);
static const BRect corner(-25,-10, -15,0);
static const BRect crossRect(-5,-35, 5,40);


static void
rectClipper(BView* view, BRect rect, bool inverse)
{
	if (inverse)
		view->ClipToInverseRect(rect);
	else
		view->ClipToRect(rect);
}

static void
pictureClipper(BView* view, BRect rect, bool inverse)
{
	BPicture p;
	view->BeginPicture(&p);
	view->FillEllipse(rect);
	view->SetHighColor(0, 0, 0, 0);
	view->FillEllipse(rect.InsetByCopy(rect.Width()/4, rect.Height()/4));
	view->EndPicture();

	if (inverse)
		view->ClipToInversePicture(&p);
	else
		view->ClipToPicture(&p);
}

static void
shapeClipper(BView* view, BRect rect, bool inverse)
{
	BShape s;
	s.MoveTo(rect.LeftTop());
	s.LineTo(rect.RightBottom());
	s.ArcTo(rect.Width()/4, rect.Height()/4, 0, false, true, rect.RightTop());
	s.LineTo(rect.LeftBottom());
	s.Close();

	if (inverse)
		view->ClipToInverseShape(&s);
	else
		view->ClipToShape(&s);
}


static void
testBase(BView* view, clipper clip, bool inverse)
{
	clip(view, baseRect, inverse);
	view->SetHighColor(255,0,0);
	view->FillRect(bigRect);
}

static void
testBaseDirect(BView* view, clipper clip)
{
	testBase(view, clip, false);
}

static void
testBaseInverse(BView* view, clipper clip)
{
	testBase(view, clip, true);
}

static void
testCross(BView* view, clipper clip, bool inverse)
{
	clip(view, crossRect, inverse);
	view->SetHighColor(0,0,255);
	view->FillRect(bigRect);
}

static void
testCorner(BView* view, clipper clip, bool inverse)
{
	clip(view, corner, inverse);
	view->SetHighColor(0,255,0);
	view->FillRect(bigRect);
}

static void
testCross00(BView* view, clipper clip)
{
	testBase(view, clip, false);
	testCross(view, clip, false);
}

static void
testCross01(BView* view, clipper clip)
{
	testBase(view, clip, false);
	testCross(view, clip, true);
}

static void
testCross10(BView* view, clipper clip)
{
	testBase(view, clip, true);
	testCross(view, clip, false);
}

static void
testCross11(BView* view, clipper clip)
{
	testBase(view, clip, true);
	testCross(view, clip, true);
}

static void
test3000(BView* view, clipper clip)
{
	testBase(view, clip, false);
	testCorner(view, clip, false);
	testCross(view, clip, false);
}

static void
test3001(BView* view, clipper clip)
{
	testBase(view, clip, false);
	testCorner(view, clip, false);
	testCross(view, clip, true);
}

static void
test3010(BView* view, clipper clip)
{
	testBase(view, clip, false);
	testCorner(view, clip, true);
	testCross(view, clip, false);
}

static void
test3011(BView* view, clipper clip)
{
	testBase(view, clip, false);
	testCorner(view, clip, true);
	testCross(view, clip, true);
}

static void
test3100(BView* view, clipper clip)
{
	testBase(view, clip, true);
	testCorner(view, clip, false);
	testCross(view, clip, false);
}

static void
test3101(BView* view, clipper clip)
{
	testBase(view, clip, true);
	testCorner(view, clip, false);
	testCross(view, clip, true);
}

static void
test3110(BView* view, clipper clip)
{
	testBase(view, clip, true);
	testCorner(view, clip, true);
	testCross(view, clip, false);
}

static void
test3111(BView* view, clipper clip)
{
	testBase(view, clip, true);
	testCorner(view, clip, true);
	testCross(view, clip, true);
}

static const test tests[] = {
	testBaseDirect, testBaseInverse,
	testCross00, testCross01, testCross10, testCross11,
	test3000, test3001, test3010, test3011,
	test3100, test3101, test3110, test3111
};


class View : public BView {
public:
								View(const char* name, test, clipper);
			void				Draw(BRect);
			void				AttachedToWindow();

private:
			test				fTest;
			clipper				fClipper;
};

View::View(const char* name, test fTest, clipper fClipper)
	:
	BView(BRect(0,0, 100,100), name, 0, B_WILL_DRAW),
	fTest(fTest),
	fClipper(fClipper)
{
}

void
View::AttachedToWindow()
{
	BView::AttachedToWindow();
	TranslateBy(50, 50);
}

void
View::Draw(BRect)
{
	fTest(this, fClipper);
}


class App : public BApplication {
public:
								App();
};

App::App()
	:
	BApplication("application/x-vnd.Haiku-Test_inverse_clip")
{
	BWindow* window = new BWindow(BRect(100,100, 800,400), "clip test",
		B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE);

	BGridView* grid = new BGridView();
	grid->SetResizingMode(B_FOLLOW_ALL_SIDES);

	BLayoutBuilder::Grid<> layout(grid);
	layout.SetInsets(B_USE_DEFAULT_SPACING);

	int testsCount = B_COUNT_OF(tests);
	for (int i = 0; i < testsCount; i++) {
		layout.Add(new View("region", tests[i], rectClipper), 0, i);
		layout.Add(new View("rotate", tests[i], rectClipper), 1, i);
			// This one changes from Region to Shape clipping
		layout.Add(new View("picture", tests[i], pictureClipper), 2, i);
		layout.Add(new View("rotate", tests[i], pictureClipper), 3, i);
		layout.Add(new View("shape", tests[i], shapeClipper), 4, i);
		layout.Add(new View("rotate", tests[i], shapeClipper), 5, i);
	}

	BScrollView* scroll = new BScrollView("scroll", grid, B_FOLLOW_ALL_SIDES,
		0, true, true, B_NO_BORDER);
	window->AddChild(scroll);
	BRect bounds(window->Bounds());
	scroll->ResizeTo(bounds.Width(), bounds.Height());

	BView* rotate;
	while ((rotate = window->FindView("rotate")) != NULL) {
		rotate->RotateBy(0.78);
		rotate->SetName("rotated");
	}

	window->Show();
}


int
main()
{
	App app;
	app.Run();
	return 0;
}
