/*
 * Preview.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <View.h>
#include "Preview.h"

#ifdef USE_PREVIEW_FOR_DEBUG

class PreviewView : public BView {
public:
	PreviewView(BRect, BBitmap *);
	void Draw(BRect);
private:
	BBitmap *fBitmap;
};

PreviewView::PreviewView(BRect frame, BBitmap *bitmap)
	: BView(frame, "preview", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fBitmap = bitmap;
}

void PreviewView::Draw(BRect)
{
	DrawBitmap(fBitmap);
} 

PreviewWindow::PreviewWindow(BRect frame, const char *title, BBitmap *bitmap)
	: BWindow(frame, title, B_TITLED_WINDOW, 0)
{
	AddChild(new PreviewView(Bounds(), bitmap));
	fSemaphore = create_sem(0, "PreviewSem");
}

bool PreviewWindow::QuitRequested()
{
	release_sem(fSemaphore);
	return true;
}

int PreviewWindow::Go()
{
	Show();
	acquire_sem(fSemaphore);
	delete_sem(fSemaphore);
	Lock();
	Quit();
	return 0;
}

#endif	/* USE_PREVIEW_FOR_DEBUG */
