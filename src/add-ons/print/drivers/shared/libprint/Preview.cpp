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
	BBitmap *__bitmap;
};

PreviewView::PreviewView(BRect frame, BBitmap *bitmap)
	: BView(frame, "preview", B_FOLLOW_ALL, B_WILL_DRAW)
{
	__bitmap = bitmap;
}

void PreviewView::Draw(BRect)
{
	DrawBitmap(__bitmap);
} 

PreviewWindow::PreviewWindow(BRect frame, const char *title, BBitmap *bitmap)
	: BWindow(frame, title, B_TITLED_WINDOW, 0)
{
	AddChild(new PreviewView(Bounds(), bitmap));
	__semaphore = create_sem(0, "PreviewSem");
}

bool PreviewWindow::QuitRequested()
{
	release_sem(__semaphore);
	return true;
}

int PreviewWindow::Go()
{
	Show();
	acquire_sem(__semaphore);
	delete_sem(__semaphore);
	Lock();
	Quit();
	return 0;
}

#endif	/* USE_PREVIEW_FOR_DEBUG */
