#include "SSAwindow.h"
#include "Locker.h"
#include "View.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

// This is the BDirectWindow subclass that rendering occurs in.
// A view is added to it so that BView based screensavers will work.
SSAwindow::SSAwindow(BRect frame) : BDirectWindow(frame, "ScreenSaver Window", 
				B_BORDERED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE) , fSaver(NULL) 
{
	frame.OffsetTo(0,0);
	fView=new BView(frame,"ScreenSaver View",B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild (fView);
}


SSAwindow::~SSAwindow() 
{
	Hide();
//	Sync();
}


bool 
SSAwindow::QuitRequested(void) 
{
	return true;
}

void 
SSAwindow::DirectConnected(direct_buffer_info *info) 
{
	if (fSaver)
		fSaver->DirectConnected(info);
}
