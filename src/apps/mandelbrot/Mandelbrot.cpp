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

	FractalEngine* fFractalEngine;
	void RedrawFractal();

private:
	bool fSizeChanged;
	bool fOwnBitmap;
	BBitmap* fDisplayBitmap;
	double fLocationX;
	double fLocationY;
	double fSize;
};


FractalView::FractalView()
	:
	BView(NULL, B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED),
	fFractalEngine(NULL),
	fSizeChanged(false),
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
	enum {
		MSG_MANDELBROT_SET = 'MndW',
		MSG_BURNINGSHIP_SET,
		MSG_TRICORN_SET,
		MSG_JULIA_SET,
		MSG_ORBITTRAP_SET,
		MSG_MULTIBROT_SET,

		MSG_ROYAL_PALETTE,
		MSG_DEEPFROST_PALETTE,
		MSG_FROST_PALETTE,
		MSG_FIRE_PALETTE,
		MSG_MIDNIGHT_PALETTE,
		MSG_GRASSLAND_PALETTE,
		MSG_LIGHTNING_PALETTE,
		MSG_SPRING_PALETTE,
		MSG_HIGHCONTRAST_PALETTE,

		MSG_ITER_128,
		MSG_ITER_512,
		MSG_ITER_1024,
		MSG_ITER_4096,
		MSG_ITER_8192,
		MSG_ITER_12288,
		MSG_ITER_16384
	};
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
	BMenu* setMenu;
	BMenu* paletteMenu;
	BMenu* iterMenu;
	BLayoutBuilder::Menu<>(menuBar)
		.AddMenu(B_TRANSLATE("File"))
			.AddItem(B_TRANSLATE("Quit"), B_QUIT_REQUESTED, 'Q')
		.End()
		.AddMenu(B_TRANSLATE("Set"))
			.GetMenu(setMenu)
			.AddItem(B_TRANSLATE("Mandelbrot"), MSG_MANDELBROT_SET)
			.AddItem(B_TRANSLATE("Burning Ship"), MSG_BURNINGSHIP_SET)
			.AddItem(B_TRANSLATE("Tricorn"), MSG_TRICORN_SET)
			.AddItem(B_TRANSLATE("Julia"), MSG_JULIA_SET)
			.AddItem(B_TRANSLATE("Orbit Trap"), MSG_ORBITTRAP_SET)
			.AddItem(B_TRANSLATE("Multibrot"), MSG_MULTIBROT_SET)
		.End()
		.AddMenu(B_TRANSLATE("Palette"))
			.GetMenu(paletteMenu)
			.AddItem(B_TRANSLATE("Royal"), MSG_ROYAL_PALETTE)
			.AddItem(B_TRANSLATE("Deepfrost"), MSG_DEEPFROST_PALETTE)
			.AddItem(B_TRANSLATE("Frost"), MSG_FROST_PALETTE)
			.AddItem(B_TRANSLATE("Fire"), MSG_FIRE_PALETTE)
			.AddItem(B_TRANSLATE("Midnight"), MSG_MIDNIGHT_PALETTE)
			.AddItem(B_TRANSLATE("Grassland"), MSG_GRASSLAND_PALETTE)
			.AddItem(B_TRANSLATE("Lightning"), MSG_LIGHTNING_PALETTE)
			.AddItem(B_TRANSLATE("Spring"), MSG_SPRING_PALETTE)
			.AddItem(B_TRANSLATE("High contrast"), MSG_HIGHCONTRAST_PALETTE)
		.End()
		.AddMenu(B_TRANSLATE("Iterations"))
			.GetMenu(iterMenu)
			.AddItem("128", MSG_ITER_128)
			.AddItem("512", MSG_ITER_512)
			.AddItem("1024", MSG_ITER_1024)
			.AddItem("4096", MSG_ITER_4096)
			.AddItem("8192", MSG_ITER_8192)
			.AddItem("12288", MSG_ITER_12288)
			.AddItem("16384", MSG_ITER_16384)
		.End()
	.End();
	setMenu->SetRadioMode(true);
	setMenu->FindItem(MSG_MANDELBROT_SET)->SetMarked(true);
	paletteMenu->SetRadioMode(true);
	paletteMenu->FindItem(MSG_ROYAL_PALETTE)->SetMarked(true);
	iterMenu->SetRadioMode(true);
	iterMenu->FindItem(MSG_ITER_1024)->SetMarked(true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0)
		.Add(menuBar)
		.Add(fFractalView)
	.End();
}


#define HANDLE_SET(uiwhat, id) \
	case uiwhat: { \
		BMessage msg(FractalEngine::MSG_CHANGE_SET); \
		msg.AddUInt8("set", id); \
		fFractalView->fFractalEngine->PostMessage(&msg); \
		fFractalView->RedrawFractal(); \
		break; \
	}
#define HANDLE_PALETTE(uiwhat, id) \
	case uiwhat: { \
		BMessage msg(FractalEngine::MSG_SET_PALETTE); \
		msg.AddUInt8("palette", id); \
		fFractalView->fFractalEngine->PostMessage(&msg); \
		fFractalView->RedrawFractal(); \
		break; \
	}
#define HANDLE_ITER(uiwhat, id) \
	case uiwhat: { \
		BMessage msg(FractalEngine::MSG_SET_ITERATIONS); \
		msg.AddUInt16("iterations", id); \
		fFractalView->fFractalEngine->PostMessage(&msg); \
		fFractalView->RedrawFractal(); \
		break; \
	}
void
MandelbrotWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	HANDLE_SET(MSG_MANDELBROT_SET, 0)
	HANDLE_SET(MSG_BURNINGSHIP_SET, 1)
	HANDLE_SET(MSG_TRICORN_SET, 2)
	HANDLE_SET(MSG_JULIA_SET, 3)
	HANDLE_SET(MSG_ORBITTRAP_SET, 4)
	HANDLE_SET(MSG_MULTIBROT_SET, 5)

	HANDLE_PALETTE(MSG_ROYAL_PALETTE, 0)
	HANDLE_PALETTE(MSG_DEEPFROST_PALETTE, 1)
	HANDLE_PALETTE(MSG_FROST_PALETTE, 2)
	HANDLE_PALETTE(MSG_FIRE_PALETTE, 3)
	HANDLE_PALETTE(MSG_MIDNIGHT_PALETTE, 4)
	HANDLE_PALETTE(MSG_GRASSLAND_PALETTE, 5)
	HANDLE_PALETTE(MSG_LIGHTNING_PALETTE, 6)
	HANDLE_PALETTE(MSG_SPRING_PALETTE, 7)
	HANDLE_PALETTE(MSG_HIGHCONTRAST_PALETTE, 8)

	HANDLE_ITER(MSG_ITER_128, 128)
	HANDLE_ITER(MSG_ITER_512, 512)
	HANDLE_ITER(MSG_ITER_1024, 1024)
	HANDLE_ITER(MSG_ITER_4096, 4096)
	HANDLE_ITER(MSG_ITER_8192, 8192)
	HANDLE_ITER(MSG_ITER_12288, 12288)
	HANDLE_ITER(MSG_ITER_16384, 16384)

	default:
		BWindow::MessageReceived(msg);
		break;
	}
}
#undef HANDLE_SET
#undef HANDLE_PALETTE
#undef HANDLE_ITER


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
