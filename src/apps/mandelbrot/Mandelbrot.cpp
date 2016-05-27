/*
 * Copyright 2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */


#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <MenuBar.h>
#include <LayoutBuilder.h>
#include <View.h>
#include <Window.h>

#include "FractalEngine.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MandelbrotWindow"


// #pragma mark - FractalView


class FractalView : public BView {
public:
	FractalView();
	~FractalView();

	virtual void FrameResized(float, float)
		{ RedrawFractal(); }
	virtual void MouseDown(BPoint where);
	virtual void Draw(BRect updateRect);

private:
	BBitmap* fBitmap;
	double fLocationX;
	double fLocationY;
	double fSize;

	void RedrawFractal(uint16 width = 0, uint16 height = 0);
};


FractalView::FractalView()
	:
	BView(NULL, B_WILL_DRAW),
	fBitmap(NULL),
	fLocationX(0),
	fLocationY(0),
	fSize(0.005)
{
	RedrawFractal(641, 462);
}


FractalView::~FractalView()
{
	delete fBitmap;
}


void FractalView::MouseDown(BPoint where)
{
	uint32 buttons;
	GetMouse(&where, &buttons);

	BRect frame = Frame();
	fLocationX = ((where.x - frame.Width() / 2) * fSize + fLocationX);
	fLocationY = ((where.y - frame.Height() / 2) * -fSize + fLocationY);
	if (buttons & B_PRIMARY_MOUSE_BUTTON)
		fSize /= 2;
	else
		fSize *= 2;
	RedrawFractal();
}


void FractalView::RedrawFractal(uint16 width, uint16 height)
{
	delete fBitmap;
	if (width == 0)
		width = (uint16)Frame().Width();
	if (height == 0)
		height = (uint16)Frame().Height();
	fBitmap = FractalEngine(width, height, fLocationX, fLocationY, fSize);
	Invalidate();
}


void FractalView::Draw(BRect updateRect)
{
	DrawBitmap(fBitmap, updateRect, updateRect);
}


// #pragma mark - MandelbrotWindow


class MandelbrotWindow : public BWindow
{
public:

				MandelbrotWindow(BRect frame);
				~MandelbrotWindow() {}

	virtual void MessageReceived(BMessage* msg);
	virtual bool QuitRequested();

private:
		FractalView* fFractalView;
};


MandelbrotWindow::MandelbrotWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Mandelbrot"), B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL, 0L),
	fFractalView(new FractalView)
{
	BMenuBar* menuBar = new BMenuBar("MenuBar");
	BLayoutBuilder::Menu<>(menuBar)
		.AddMenu(B_TRANSLATE("File"))
			.AddItem(B_TRANSLATE("Quit"), B_QUIT_REQUESTED, 'Q')
		.End()
	.End();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0)
		.Add(menuBar)
		.Add(fFractalView)
	.End();
}


void
MandelbrotWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	default:
		BWindow::MessageReceived(msg);
		break;
	}
}


bool
MandelbrotWindow::QuitRequested()
{
	if (BWindow::QuitRequested()) {
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}
	return false;
}


// #pragma mark - MandelbrotApp


class MandelbrotApp : public BApplication
{
public:
				MandelbrotApp()
					: BApplication("application/x-vnd.Haiku-Mandelbrot") {}

		void	ReadyToRun();
		bool	QuitRequested() { return true; }
};


void
MandelbrotApp::ReadyToRun()
{
	MandelbrotWindow* wind = new MandelbrotWindow(BRect(0, 0, 640, 480));
	wind->CenterOnScreen();
	wind->Show();
}


int
main(int argc, char* argv[])
{
	MandelbrotApp().Run();
	return 0;
}
