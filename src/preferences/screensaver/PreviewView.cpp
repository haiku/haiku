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

static float sampleX[]= {0,.05,.15,.7,.725,.8,.825,.85,.950,1.0};
static float sampleY[]= {0,.05,.90,.95,.966,.975,1.0};
inline BPoint 
scale2(int x, int y,BRect area) 
{ 
	return scaleDirect(sampleX[x],sampleY[y],area); 
}


inline BRect 
scale2(int x1, int x2, int y1, int y2,BRect area) 
{ 
	return scaleDirect(sampleX[x1],sampleX[x2],sampleY[y1],sampleY[y2],area); 
}


PreviewView::PreviewView(BRect frame, const char *name,ScreenSaverPrefs *prefp)
: BView (frame,name,B_FOLLOW_NONE,B_WILL_DRAW),
  fSaver (NULL),fConfigView(NULL),fSst(NULL),fThreadID(-1),fPrefPtr(prefp) 
{
	SetViewColor(216,216,216);
} 


PreviewView::~PreviewView() 
{
	if (fThreadID>=0)
		kill_thread(fThreadID);
} 


void 
PreviewView::SetScreenSaver(BString name) 
{
	if (fThreadID>=0) {
		kill_thread(fThreadID);
		fThreadID=-1;
	}
	if (fSst)
		delete fSst;
	if (fConfigView) {
		RemoveChild(fConfigView);
		delete fConfigView;
	}

	fConfigView=new BView(scale2(1,8,1,2,Bounds()),"previewArea",B_FOLLOW_NONE,B_WILL_DRAW);
	fConfigView->SetViewColor(0,0,0);
	AddChild(fConfigView);

	fSst=new ScreenSaverThread(Window(),fConfigView,fPrefPtr);
	fSaver=fSst->LoadAddOn();
	if (fSaver) {
		fThreadID=spawn_thread(threadFunc,"ScreenSaverRenderer",0,fSst);
		resume_thread(fThreadID); 
	}
	
}

void 
PreviewView::Draw(BRect update) 
{
	SetViewColor(216,216,216);

	SetHighColor(184,184,184);
	FillRoundRect(scale2(0,9,0,3,Bounds()),4,4);// Outer shape
	FillRoundRect(scale2(2,7,3,6,Bounds()),2,2);// control console outline

	SetHighColor(96,96,96);
	StrokeRoundRect(scale2(2,7,3,6,Bounds()),2,2);// control console outline
	StrokeRoundRect(scale2(0,9,0,3,Bounds()),4,4); // Outline outer shape
	SetHighColor(kBlack);

	SetHighColor(184,184,184);
	BRect outerShape=scale2(2,7,2,6,Bounds());
	outerShape.InsetBy(1,1);
	FillRoundRect(outerShape,4,4);// Outer shape

	SetHighColor(0,255,0);
	FillRect(scale2(3,4,4,5,Bounds()));
	SetHighColor(96,96,96);
	FillRect(scale2(5,6,4,5,Bounds()));
}
