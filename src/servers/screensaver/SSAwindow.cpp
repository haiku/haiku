#include "SSAwindow.h"
#include "Locker.h"
#include "View.h"
#include <memory.h>
#include <stdlib.h>

extern void callDirectConnected(direct_buffer_info *);
extern BView *view;

// This is the BDirectWindow subclass that rendering occurs in.
// A view is added to it so that BView based screensavers will work.
SSAwindow::SSAwindow(BRect frame) : BDirectWindow(frame, "ScreenSaver Window", 
				B_TITLED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE) 
	{
	frame.OffsetTo(0.0,0.0);
	view=new BView(frame,"ScreenSaver View",B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild (view);
	view->Frame().PrintToStream();
	view->SetOrigin(0.0,0.0);

	// SetFullScreen(true);
	}

SSAwindow::~SSAwindow() 
	{
	int32 result;
	
	Hide();
	Sync();
	}

bool SSAwindow::QuitRequested(void)
	{
	return true;
	}

void SSAwindow::DirectConnected(direct_buffer_info *info) 
	{
	callDirectConnected(info);
	}
