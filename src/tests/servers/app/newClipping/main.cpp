#include <OS.h>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Region.h>
#include <Rect.h>

#include <stdio.h>
#include <stdlib.h>

#include "MyView.h"
#include "Layer.h"

#define ApplicationSignature "application/x-vnd.generic-SkeletonApplication"

BWindow *wind = NULL;

class clsApp
:
	public BApplication
{
public:
	clsApp();
	~clsApp();
	virtual void ReadyToRun();
};

class clsMainWindow
:
	public BWindow
{
public:
						clsMainWindow(const char *uWindowTitle);
						~clsMainWindow();

	virtual bool		QuitRequested();
			void		test1();
private:
			MyView		*fView;
};

clsApp::clsApp() : BApplication(ApplicationSignature)
{
	srand(real_time_clock_usecs());
}

clsApp::~clsApp()
{
}

void clsApp::ReadyToRun()
{
}


clsMainWindow::clsMainWindow(const char *uWindowTitle)
:
	BWindow(
		BRect(50, 50, 500, 450),
		uWindowTitle,
		B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		0	)
{
	wind = this;
	fView = new MyView(Bounds(), "emu", B_FOLLOW_ALL, B_WILL_DRAW);
	AddChild(fView);
}

clsMainWindow::~clsMainWindow()
{
}

bool clsMainWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return BWindow::QuitRequested();
}

void clsMainWindow::test1()
{
	Layer		*topLayer = fView->topLayer;
//	Layer		*parent;

	rgb_color	c;
	BRect		temp;

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	Layer	*lay1 = new Layer(BRect(20,20,300,220), "lay1", B_FOLLOW_NONE, 0, c);
	topLayer->AddLayer(lay1);

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	Layer	*lay2 = new Layer(BRect(20,20,150,150), "lay2",
			B_FOLLOW_NONE,
			B_FULL_UPDATE_ON_RESIZE, c);
	lay1->AddLayer(lay2);

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	Layer	*lay3 = new Layer(BRect(20,20,100,100), "lay3",
			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
			0, c);
	lay2->AddLayer(lay3);

	temp	= lay1->Bounds();
	lay1->ConvertToScreen2(&temp);
	topLayer->RebuildVisibleRegions(BRegion(temp), lay1);

	wind->Lock();
	fView->Invalidate();
	wind->Unlock();


	snooze(2000000);

	lay2->MoveBy(25,35);

	snooze(2000000);

	lay2->ResizeBy(45,55);

	snooze(2000000);

	lay2->ResizeBy(-45, -55);

/*
	snooze(2000000);

	lay1->ScrollBy(0,50);

	snooze(2000000);

	lay2->Hide();

	snooze(2000000);

	lay2->Show();

	snooze(2000000);

	lay1->Invalidate(BRect(0,0,500,500));
*/
}

int main()
{
	new clsApp();
	clsMainWindow	*win = new clsMainWindow("clipping");
	win->Show();

	win->test1();

	be_app->Run();
	delete be_app;
	return 0;
}
