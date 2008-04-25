// main.cpp

#include <stdio.h>

#include <Application.h>
#include <Region.h>
#include <View.h>
#include <Window.h>

class TestView1 : public BView {

 public:
	TestView1(BRect frame, const char* name, uint32 resizeFlags, uint32 flags)
		: BView(frame, name, resizeFlags, flags)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetHighColor(ViewColor());
	}

	virtual void Draw(BRect updateRect)
	{
		BRegion region;
		region.Include(BRect(20, 20, 40, 40));
		region.Include(BRect(30, 30, 80, 80));
		ConstrainClippingRegion(&region);

		SetHighColor(255, 0, 0, 255);
		FillRect(BRect(0, 0, 100, 100));

		ConstrainClippingRegion(NULL);

		SetHighColor(0, 0, 0, 255);
		StrokeLine(BPoint(2, 2), BPoint(80, 80));
	}
};

class TestView2 : public BView {

 public:
	TestView2(BRect frame, const char* name, uint32 resizeFlags, uint32 flags)
		: BView(frame, name, resizeFlags, flags)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetHighColor(ViewColor());
	}

	virtual void AttachedToWindow()
	{
		SetOrigin(20, 20);
	}

	virtual void Draw(BRect updateRect)
	{
//		SetOrigin(20, 20);

		BRegion region;
		region.Include(BRect(20, 20, 40, 40));
		region.Include(BRect(30, 30, 80, 80));
		ConstrainClippingRegion(&region);

		SetHighColor(255, 128, 0, 255);
		FillRect(BRect(0, 0, 100, 100));

		ConstrainClippingRegion(NULL);

		SetHighColor(0, 0, 0, 255);
		StrokeLine(BPoint(2, 2), BPoint(80, 80));
	}
};

class TestView3 : public BView {

 public:
	TestView3(BRect frame, const char* name, uint32 resizeFlags, uint32 flags)
		: BView(frame, name, resizeFlags, flags)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetHighColor(ViewColor());
	}

	virtual void AttachedToWindow()
	{
		SetOrigin(20, 20);
	}

	virtual void Draw(BRect updateRect)
	{
		BRegion region;
		region.Include(BRect(20, 20, 40, 40));
		region.Include(BRect(30, 30, 80, 80));
		ConstrainClippingRegion(&region);

		SetHighColor(55, 255, 128, 255);
		FillRect(BRect(0, 0, 100, 100));

		PushState();
			SetOrigin(15, 15);
	
			ConstrainClippingRegion(&region);
	
			SetHighColor(155, 255, 128, 255);
			FillRect(BRect(0, 0, 100, 100));
	
//			ConstrainClippingRegion(NULL);
	
			SetHighColor(0, 0, 0, 255);
			StrokeLine(BPoint(2, 2), BPoint(80, 80));
			SetHighColor(255, 0, 0, 255);
			StrokeLine(BPoint(2, 2), BPoint(4, 2));
		PopState();

		SetHighColor(0, 0, 0, 255);
		StrokeLine(BPoint(4, 2), BPoint(82, 80));
	}
};

class TestView4 : public BView {

 public:
	TestView4(BRect frame, const char* name, uint32 resizeFlags, uint32 flags)
		: BView(frame, name, resizeFlags, flags)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetHighColor(ViewColor());
	}

	virtual void AttachedToWindow()
	{
		SetOrigin(20, 20);
	}

	virtual void Draw(BRect updateRect)
	{
		BRegion region;
		region.Include(BRect(20, 20, 40, 40));
		region.Include(BRect(30, 30, 140, 140));
		ConstrainClippingRegion(&region);

		SetHighColor(55, 255, 128, 255);
		FillRect(BRect(0, 0, 200, 200));

		// NOTE: This exposes broken behavior of the ZETA
		// (probably R5 too) app_server. The new origin
		// is not taken into account
		PushState();

		ConstrainClippingRegion(&region);
		SetOrigin(15, 15);
		SetScale(1.5);

		SetHighColor(155, 255, 128, 255);
		FillRect(BRect(0, 0, 200, 200));

		ConstrainClippingRegion(NULL);

		SetHighColor(0, 0, 0, 255);
		SetDrawingMode(B_OP_OVER);
		DrawString("Text is scaled.", BPoint(20, 30));

		SetScale(1.2);
		DrawString("Text is scaled.", BPoint(20, 30));

		StrokeLine(BPoint(2, 2), BPoint(80, 80));

		PopState();

		SetHighColor(0, 0, 0, 255);
		StrokeLine(BPoint(4, 2), BPoint(82, 80));
	}
};

class TestView5 : public BView {

 public:
	TestView5(BRect frame, const char* name, uint32 resizeFlags, uint32 flags)
		: BView(frame, name, resizeFlags, flags)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetHighColor(ViewColor());
	}

	virtual void AttachedToWindow()
	{
	}

	virtual void Draw(BRect updateRect)
	{
		BRegion region;
		region.Include(BRect(0, 0, 40, 40));
		region.Include(BRect(30, 30, 140, 140));
		ConstrainClippingRegion(&region);

		SetHighColor(55, 255, 128, 255);
		FillRect(BRect(0, 0, 200, 200));

		PushState();
			ConstrainClippingRegion(&region);
			SetOrigin(50, 10);

			SetHighColor(155, 55, 128, 255);
			FillRect(BRect(0, 0, 200, 200));
		PopState();

		SetHighColor(255, 0, 0);
		StrokeLine(Bounds().LeftTop(), Bounds().RightBottom());
	}
};


class TestView6 : public BView {

 public:
	TestView6(BRect frame, const char* name, uint32 resizeFlags, uint32 flags)
		: BView(frame, name, resizeFlags, flags)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetHighColor(ViewColor());
	}

	virtual void AttachedToWindow()
	{
	}

	virtual void Draw(BRect updateRect)
	{
		BRegion region;
		region.Include(BRect(0, 0, 40, 40));
		region.Include(BRect(30, 30, 140, 140));
		ConstrainClippingRegion(&region);

		SetHighColor(55, 255, 128, 255);
		FillRect(BRect(0, 0, 200, 200));

		PushState();
			SetOrigin(50, 10);
			ConstrainClippingRegion(&region);

			SetHighColor(155, 55, 128, 255);
			FillRect(BRect(0, 0, 200, 200));
		PopState();

		SetHighColor(255, 0, 0);
		StrokeLine(Bounds().LeftTop(), Bounds().RightBottom());
	}
};

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication(
		"application/x.vnd-Haiku.ClippingRegion");

	BRect frame(50.0, 50.0, 300.0, 250.0);
	BWindow* window;
	BView* view;
	
//	window = new BWindow(frame, "Test1", B_TITLED_WINDOW,
//		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);
//
//	view = new TestView1(window->Bounds(), "test1", B_FOLLOW_ALL,
//		B_WILL_DRAW);
//	window->AddChild(view);
//	window->Show();
//
//	frame.OffsetBy(20, 20);
//	window = new BWindow(frame, "Test2", B_TITLED_WINDOW,
//		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);
//
//	view = new TestView2(window->Bounds(), "test2", B_FOLLOW_ALL,
//		B_WILL_DRAW);
//	window->AddChild(view);
//	window->Show();
//
//	frame.OffsetBy(20, 20);
//	window = new BWindow(frame, "Test3", B_TITLED_WINDOW,
//		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);
//
//	view = new TestView3(window->Bounds(), "test3", B_FOLLOW_ALL,
//		B_WILL_DRAW);
//	window->AddChild(view);
//	window->Show();
//
//	frame.OffsetBy(20, 20);
//	window = new BWindow(frame, "Test4", B_TITLED_WINDOW,
//		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);
//
//	view = new TestView4(window->Bounds(), "test4", B_FOLLOW_ALL,
//		B_WILL_DRAW);
//	window->AddChild(view);
//	window->Show();


	frame.OffsetBy(20, 20);
	window = new BWindow(frame, "Test5", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	view = new TestView5(window->Bounds(), "test5", B_FOLLOW_ALL,
		B_WILL_DRAW);
	window->AddChild(view);
	window->Show();

	frame.OffsetBy(20, 20);
	window = new BWindow(frame, "Test6", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	view = new TestView6(window->Bounds(), "test6", B_FOLLOW_ALL,
		B_WILL_DRAW);
	window->AddChild(view);
	window->Show();

	app->Run();

	delete app;
	return 0;
}
