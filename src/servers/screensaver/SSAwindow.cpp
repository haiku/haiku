#include "SSAwindow.h"
#include "Locker.h"
#include "View.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

// This is the BDirectWindow subclass that rendering occurs in.
// A view is added to it so that BView based screensavers will work.
SSAwindow::SSAwindow(BRect frame,BScreenSaver *saverIn) : BDirectWindow(frame, "ScreenSaver Window", 
				B_BORDERED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE) {
	frame.OffsetTo(0,0);
	view=new BView(frame,"ScreenSaver View",B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild (view);
	saver=saverIn;
	}

SSAwindow::~SSAwindow() {
	Hide();
	Sync();
	}

bool SSAwindow::QuitRequested(void) {
	return true;
	}

void SSAwindow::DirectConnected(direct_buffer_info *info) {
	int i=info->buffer_state;
	/*
	printf ("direct connected; bufState=%d; bits = %x, bpr=%d, bottom=%d\n",i,info->bits,info->bytes_per_row,info->window_bounds.bottom);
	if (!(i&B_DIRECT_STOP))  {
		printf ("ready to clear; bits = %x, bpr=%d, bottom=%d\n",info->bits,info->bytes_per_row,info->window_bounds.bottom);
		memset(info->bits,0,info->bytes_per_row*info->window_bounds.bottom);
		}
	*/
//	printf ("direct connected; bufState=%d; bits = %x, bpr=%d, bottom=%d\n",i,info->bits,info->bytes_per_row,info->window_bounds.bottom);
	if (saver)
		saver->DirectConnected(info);
//	printf ("direct connected ending!\n");
	}
