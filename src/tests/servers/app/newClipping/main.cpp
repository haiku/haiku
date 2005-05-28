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
#include "WinBorder.h"

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
		BRect(50, 50, 800, 650),
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

	rgb_color	c;

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
//	Layer	*lay1
//		= new Layer(BRect(20,20,300,220), "lay1", B_FOLLOW_NONE, 0, c);
//		= new WinBorder(BRect(20,20,300,220), "lay1", B_FOLLOW_NONE, 0, c);
//	topLayer->AddLayer(lay1);
// ------
	WinBorder	*wb1 = new WinBorder(BRect(20,20,300,220), "wb1", B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE, c);
	topLayer->AddLayer(wb1);
		// same size as wb1
	Layer	*lay1 = new Layer(BRect(0,0,280,200), "lay1", B_FOLLOW_NONE, 0, c);
	wb1->AddLayer(lay1);
// ------
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

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	Layer	*lay12 = new Layer(BRect(170,20,290,150), "lay21",
			B_FOLLOW_NONE,
			B_FULL_UPDATE_ON_RESIZE, c);
	lay1->AddLayer(lay12);

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	Layer	*lay13 = new Layer(BRect(20,20,100,100), "lay31",
			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
			0, c);
	lay12->AddLayer(lay13);


//---------
	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	WinBorder	*wb2 = new WinBorder(BRect(280,120,600,420), "wb2", B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE, c);
	topLayer->AddLayer(wb2);
	Layer	*lay21 = new Layer(wb2->Bounds().OffsetToCopy(0,0), "lay21", B_FOLLOW_NONE, 0, c);
	wb2->AddLayer(lay21);

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	Layer	*lay22 = new Layer(BRect(20,20,150,150), "lay22",
			B_FOLLOW_NONE,
			0, c);
	lay21->AddLayer(lay22);

//---------

	BRegion		temp;
	wb1->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, wb1);

	wb2->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, wb2);

	wind->Lock();
	fView->Invalidate();
	wind->Unlock();

	snooze(2000000);

	wb1->Hide();

	snooze(2000000);

	wb1->Show();

/*

	snooze(2000000);

	lay2->MoveBy(25,35);

	snooze(2000000);

	lay2->ResizeBy(45,55);

	snooze(2000000);

	lay2->ResizeBy(-45, -55);


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
