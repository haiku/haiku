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

	virtual void AttachedToWindow();
	virtual void FrameResized(float, float);
	virtual void Pulse();
	virtual void MouseDown(BPoint where);
	virtual void MessageReceived(BMessage* msg);
	virtual void Draw(BRect updateRect);

private:
	bool fSizeChanged;
	FractalEngine* fFractalEngine;
	bool fOwnBitmap;
	BBitmap* fDisplayBitmap;
	double fLocationX;
	double fLocationY;
	double fSize;

	void RedrawFractal();
};


FractalView::FractalView()
	:
	BView(NULL, B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED),
	fSizeChanged(false),
	fFractalEngine(NULL),
	fOwnBitmap(false),
	fDisplayBitmap(NULL),
	fLocationX(0),
	fLocationY(0),
	fSize(0.005)
{
}


FractalView::~FractalView()
{
	if (fOwnBitmap)
		delete fDisplayBitmap;
}


void FractalView::AttachedToWindow()
{
	fFractalEngine = new FractalEngine(this, Window());
	fFractalEngine->Run();
	BMessage msg(FractalEngine::MSG_RESIZE);
	msg.AddUInt16("width", 641);
	msg.AddUInt16("height", 462);
	fFractalEngine->PostMessage(&msg);
	RedrawFractal();
}


void FractalView::FrameResized(float, float)
{
	fSizeChanged = true;
}


void FractalView::Pulse()
{
	if (!fSizeChanged)
		return;
	BMessage msg(FractalEngine::MSG_RESIZE);
	msg.AddUInt16("width", (uint16)Frame().Width() + 1);
	msg.AddUInt16("height", (uint16)Frame().Height() + 1);
	fFractalEngine->PostMessage(&msg);
	// The renderer will create new bitmaps, so we own the bitmap now
	fOwnBitmap = true;
	fSizeChanged = false;
	RedrawFractal();
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


void FractalView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case FractalEngine::MSG_RENDER_COMPLETE:
		if (fOwnBitmap) {
			fOwnBitmap = false;
			delete fDisplayBitmap;
		}
		fDisplayBitmap = NULL; // In case the following line fails
		msg->FindPointer("bitmap", (void**)&fDisplayBitmap);
		Invalidate();
		break;

	default:
		BView::MessageReceived(msg);
		break;
	}
}


void FractalView::RedrawFractal()
{
	BMessage message(FractalEngine::MSG_RENDER);
	message.AddDouble("locationX", fLocationX);
	message.AddDouble("locationY", fLocationY);
	message.AddDouble("size", fSize);
	fFractalEngine->PostMessage(&message);
}


void FractalView::Draw(BRect updateRect)
{
	DrawBitmap(fDisplayBitmap, updateRect, updateRect);
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
	SetPulseRate(250000); // pulse twice per second

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
