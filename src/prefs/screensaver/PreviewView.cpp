#include "PreviewView.h"
#include "Constants.h"
#include <Rect.h>
#include <Point.h>
#include <Shape.h>
#include <iostream>
#include <ScreenSaver.h>
#include <Errors.h>
#include <Screen.h>
#include <String.h>

inline BPoint scaleDirect(float x, float y,BRect area) {
	return BPoint(area.Width()*x+area.left,area.Height()*y+area.top);
}

inline BRect scaleDirect (float x1,float x2,float y1,float y2,BRect area) {
	return BRect(area.Width()*x1+area.left,area.Height()*y1+area.top, area.Width()*x2+area.left,area.Height()*y2+area.top);
}

float sampleX[]= {0,.05,.15,.7,.725,.8,.825,.85,.950,1.0};
float sampleY[]= {0,.05,.90,.95,.966,.975,1.0};
inline BPoint scale2(int x, int y,BRect area) { return scaleDirect(sampleX[x],sampleY[y],area); }
inline BRect scale2(int x1, int x2, int y1, int y2,BRect area) { return scaleDirect(sampleX[x1],sampleX[x2],sampleY[y1],sampleY[y2],area); }

PreviewView::PreviewView(BRect frame, const char *name,ScreenSaverPrefs *prefp)
: BView (frame,name,B_FOLLOW_NONE,B_WILL_DRAW),
  saver (NULL),configView(NULL),sst(NULL),threadID(0),prefPtr(prefp) {
	SetViewColor(216,216,216);
} 

PreviewView::~PreviewView() {
} 

void PreviewView::SetScreenSaver(BString name) {
	if (threadID)
		kill_thread(threadID);
	if (sst)
		delete sst;
	if (configView) {
		RemoveChild(configView);
		delete configView;
		}

	configView=new BView(scale2(1,8,1,2,Bounds()),"previewArea",B_FOLLOW_NONE,B_WILL_DRAW);
	configView->SetViewColor(0,0,0);
	AddChild(configView);

	sst=new ScreenSaverThread(Window(),configView,prefPtr);
	saver=sst->LoadAddOn();
	threadID=spawn_thread(threadFunc,"ScreenSaverRenderer",0,sst);
	resume_thread(threadID); 
}

void PreviewView::Draw(BRect update) {
	SetViewColor(216,216,216);

	SetHighColor(184,184,184);
	FillRoundRect(scale2(0,9,0,3,Bounds()),4,4);// Outer shape
	FillRoundRect(scale2(2,7,3,6,Bounds()),2,2);// control console outline

	SetHighColor(96,96,96);
	StrokeRoundRect(scale2(2,7,3,6,Bounds()),2,2);// control console outline
	StrokeRoundRect(scale2(0,9,0,3,Bounds()),4,4); // Outline outer shape
	SetHighColor(black);
//	FillRoundRect(scale2(1,8,1,2,Bounds()),4,4);// Screen itself

	SetHighColor(184,184,184);
	BRect outerShape=scale2(2,7,2,6,Bounds());
	outerShape.InsetBy(1,1);
	FillRoundRect(outerShape,4,4);// Outer shape

	SetHighColor(0,255,0);
	FillRect(scale2(3,4,4,5,Bounds()));
	SetHighColor(96,96,96);
	FillRect(scale2(5,6,4,5,Bounds()));
}
