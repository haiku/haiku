#include <Application.h>
#include <Bitmap.h>
#include <Region.h>
#include <View.h>
#include <Window.h>
#include <stdio.h>


class ClippingView : public BView {
public:
							ClippingView(BRect frame);

virtual	void				Draw(BRect updateRect);
virtual	void				MouseDown(BPoint where);
virtual	void				KeyDown(const char *bytes, int32 numBytes);

private:
		BWindow *			fFloatingWindow;
};


class ClippingWindow : public BWindow {
public:
							ClippingWindow(BRect frame);

private:
		ClippingView *		fView;
};


class ClippingApp : public BApplication {
public:
							ClippingApp();

private:
		ClippingWindow *	fWindow;
};


ClippingApp::ClippingApp()
	:	BApplication("application/x.vnd-Haiku.ClippingPlusRedraw")
{
	fWindow = new ClippingWindow(BRect(200, 200, 500, 400));
	fWindow->Show();
}


ClippingWindow::ClippingWindow(BRect frame)
	:	BWindow(frame, "Window", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
{
	fView = new ClippingView(frame.OffsetToSelf(0, 0));
	AddChild(fView);
	fView->MakeFocus();
}


ClippingView::ClippingView(BRect frame)
	:	BView(frame, "View", B_FOLLOW_ALL, B_WILL_DRAW),
		fFloatingWindow(NULL)
{
}


void
ClippingView::Draw(BRect updateRect)
{
	printf("got draw with update rect: %f, %f, %f, %f\n",
		updateRect.left, updateRect.top, updateRect.right, updateRect.bottom);

	SetHighColor(0, 255, 0);
	FillRect(Bounds(), B_SOLID_HIGH);
}


void
ClippingView::MouseDown(BPoint where)
{
	if (fFloatingWindow == NULL) {
		BPoint leftTop = ConvertToScreen(BPoint(50, 50));
		fFloatingWindow = new BWindow(BRect(leftTop, leftTop + BPoint(100, 50)),
			"Floating", B_FLOATING_WINDOW, B_AVOID_FOCUS);
		fFloatingWindow->Show();
	} else {
		fFloatingWindow->Lock();
		fFloatingWindow->Quit();
		fFloatingWindow = NULL;
	}
}


void
ClippingView::KeyDown(const char *bytes, int32 numBytes)
{
	SetHighColor(0, 0, 255);
	FillRect(Bounds(), B_SOLID_HIGH);

	BRegion region(BRect(200, 100, 250, 150));
	ConstrainClippingRegion(&region);

	SetHighColor(255, 0, 0);
	FillRect(Bounds(), B_SOLID_HIGH);
}


int
main(int argc, const char *argv[])
{
	ClippingApp *app = new ClippingApp();
	app->Run();
	delete app;
	return 0;
}
