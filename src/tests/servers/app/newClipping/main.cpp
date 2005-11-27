#include <OS.h>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Region.h>
#include <Rect.h>
#include <String.h>

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
			void		AddWindow(BRect frame, const char* name);
			void		test2();
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
	fView->MakeFocus(true);
}

clsMainWindow::~clsMainWindow()
{
}

bool clsMainWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return BWindow::QuitRequested();
}


// AddWindow
void
clsMainWindow::AddWindow(BRect frame, const char* name)
{
	Layer* topLayer = fView->topLayer;

	WinBorder* window = new WinBorder(frame, name, B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE,
									  (rgb_color){ 255, 203, 0, 255 });
	topLayer->AddLayer(window);

	// add a coupld children
	frame.OffsetTo(B_ORIGIN);
	frame.InsetBy(5.0, 5.0);
	if (frame.IsValid()) {
		Layer* layer1 = new Layer(frame, "View 1", B_FOLLOW_ALL,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 180, 180, 180, 255 });

		frame.OffsetTo(B_ORIGIN);
		frame.InsetBy(15.0, 15.0);
		if (frame.IsValid()) {

			BRect f = frame;
			f.bottom = f.top + f.Height() / 3 - 3;
			f.right = f.left + f.Width() / 3 - 3;

			// top row of views
			Layer* layer;
			layer = new Layer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_TOP,
							  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddLayer(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new Layer(f, "View", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
							  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddLayer(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new Layer(f, "View", B_FOLLOW_RIGHT | B_FOLLOW_TOP,
							  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddLayer(layer);

			// middle row of views
			f = frame;
			f.bottom = f.top + f.Height() / 3 - 3;
			f.right = f.left + f.Width() / 3 - 3;
			f.OffsetBy(0, f.Height() + 6);

			layer = new Layer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,
							  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddLayer(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new Layer(f, "View", B_FOLLOW_ALL,
							  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddLayer(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new Layer(f, "View", B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM,
							  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddLayer(layer);

			// bottom row of views
			f = frame;
			f.bottom = f.top + f.Height() / 3 - 3;
			f.right = f.left + f.Width() / 3 - 3;
			f.OffsetBy(0, 2 * f.Height() + 12);

			layer = new Layer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
							  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddLayer(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new Layer(f, "View", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
							  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddLayer(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new Layer(f, "View", B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM,
							  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddLayer(layer);
		}

		window->AddLayer(layer1);
	}

	BRegion		temp;
	window->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, window);
}

// test2
void
clsMainWindow::test2()
{
	BRect frame(20, 20, 330, 230);
	for (int32 i = 0; i < 20; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}
/*
	frame.Set(10, 80, 320, 290);
	for (int32 i = 20; i < 40; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}

	frame.Set(20, 140, 330, 230);
	for (int32 i = 40; i < 60; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}

	frame.Set(20, 200, 330, 230);
	for (int32 i = 60; i < 80; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}

	frame.Set(20, 260, 330, 230);
// 99 windows are ok, the 100th looper does not run anymore,
// I guess I'm hitting a BeOS limit (max loopers per app?)
	for (int32 i = 80; i < 99; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}*/

	fView->RequestRedraw();
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
	Layer	*lay1 = new Layer(BRect(0,0,280,200), "lay1", B_FOLLOW_ALL, 0, c);
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
	Layer	*lay12 = new Layer(BRect(170,20,290,150), "lay12",
			B_FOLLOW_LEFT | B_FOLLOW_TOP,
//			B_FOLLOW_NONE,
			0, c);
	lay1->AddLayer(lay12);

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	Layer	*lay13 = new Layer(BRect(20,20,100,100), "lay13",
//			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
			B_FOLLOW_RIGHT | B_FOLLOW_TOP,
//			B_FOLLOW_LEFT | B_FOLLOW_V_CENTER,
//			B_FOLLOW_H_CENTER | B_FOLLOW_TOP,
			0, c);
	lay12->AddLayer(lay13);

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	Layer	*lay102 = new Layer(BRect(200,20,420,250), "lay102",
//			B_FOLLOW_NONE,
			B_FOLLOW_TOP_BOTTOM,
//			B_FULL_UPDATE_ON_RESIZE, c);
			0, c);
	lay1->AddLayer(lay102);

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	Layer	*lay103 = new Layer(BRect(0,0,100,50), "lay103",
			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
//			B_FOLLOW_LEFT | B_FOLLOW_TOP,
			0, c);
	lay102->AddLayer(lay103);

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	Layer	*lay104 = new Layer(BRect(0,51,100,130), "lay104",
//			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
			B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM,
			0, c);
	lay102->AddLayer(lay104);

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	Layer	*lay106 = new Layer(BRect(0,140,100, 200), "lay104",
//			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
			B_FOLLOW_RIGHT | B_FOLLOW_TOP,
			0, c);
	lay102->AddLayer(lay106);

	c.red = rand()/256;
	c.green = rand()/256;
	c.blue = rand()/256;
	Layer	*lay105 = new Layer(BRect(105,51,150,130), "lay104",
//			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
			B_FOLLOW_H_CENTER | B_FOLLOW_BOTTOM,
			0, c);
	lay102->AddLayer(lay105);

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


	WinBorder	*wb3 = new WinBorder(BRect(20,20,300,220), "wb3", B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE, c);
	topLayer->AddLayer(wb3);
		// same size as wb1
	lay1 = new Layer(BRect(0,0,280,200), "lay1", B_FOLLOW_ALL, 0, c);
	wb3->AddLayer(lay1);

	WinBorder	*wb4 = new WinBorder(BRect(20,20,300,220), "wb4", B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE, c);
	topLayer->AddLayer(wb4);
		// same size as wb1
	lay1 = new Layer(BRect(0,0,280,200), "lay1", B_FOLLOW_ALL, 0, c);
	wb4->AddLayer(lay1);

	WinBorder	*wb5 = new WinBorder(BRect(20,20,300,220), "wb5", B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE, c);
	topLayer->AddLayer(wb5);
		// same size as wb1
	lay1 = new Layer(BRect(0,0,280,200), "lay1", B_FOLLOW_ALL, 0, c);
	wb5->AddLayer(lay1);

	WinBorder	*wb6 = new WinBorder(BRect(20,20,300,220), "wb6", B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE, c);
	topLayer->AddLayer(wb6);
		// same size as wb1
	lay1 = new Layer(BRect(0,0,280,200), "lay1", B_FOLLOW_ALL, 0, c);
	wb6->AddLayer(lay1);

	WinBorder	*wb7 = new WinBorder(BRect(20,20,300,220), "wb7", B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE, c);
	topLayer->AddLayer(wb7);
		// same size as wb1
	lay1 = new Layer(BRect(0,0,280,200), "lay1", B_FOLLOW_ALL, 0, c);
	wb7->AddLayer(lay1);

	WinBorder	*wb8 = new WinBorder(BRect(20,20,300,220), "wb8", B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE, c);
	topLayer->AddLayer(wb8);
		// same size as wb1
	lay1 = new Layer(BRect(0,0,280,200), "lay1", B_FOLLOW_ALL, 0, c);
	wb8->AddLayer(lay1);

	WinBorder	*wb9 = new WinBorder(BRect(20,20,300,220), "wb9", B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE, c);
	topLayer->AddLayer(wb9);
		// same size as wb1
	lay1 = new Layer(BRect(0,0,280,200), "lay1", B_FOLLOW_ALL, 0, c);
	wb9->AddLayer(lay1);

	WinBorder	*wb10 = new WinBorder(BRect(20,20,300,220), "wb10", B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE, c);
	topLayer->AddLayer(wb10);
		// same size as wb1
	lay1 = new Layer(BRect(0,0,280,200), "lay1", B_FOLLOW_ALL, 0, c);
	wb10->AddLayer(lay1);

//---------

	BRegion		temp;
	wb1->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, wb1);

	wb2->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, wb2);

	wb3->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, wb3);

	wb4->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, wb4);

	wb5->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, wb5);

	wb6->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, wb6);

	wb7->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, wb7);

	wb8->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, wb8);

	wb9->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, wb9);

	wb10->GetWantedRegion(temp);
	topLayer->RebuildVisibleRegions(temp, wb10);


	fView->RequestRedraw();

	snooze(1000000);

	wb1->Hide();

	snooze(1000000);

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

	win->test2();

	be_app->Run();
	delete be_app;
	return 0;
}
