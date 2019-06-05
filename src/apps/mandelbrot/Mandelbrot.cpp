/*
 * Copyright 2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 *		kerwizzy
 */


#include <AboutWindow.h>
#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <MenuBar.h>
#include <LayoutBuilder.h>
#include <View.h>
#include <Window.h>

#include <algorithm>

#include "FractalEngine.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MandelbrotWindow"

#define MANDELBROT_VIEW_REFRESH_FPS 10

// #pragma mark - FractalView

//#define TRACE_MANDELBROT_VIEW
#ifdef TRACE_MANDELBROT_VIEW
#	include <stdio.h>
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...)
#endif


class FractalView : public BView {
public:
	FractalView();
	~FractalView();

	virtual void AttachedToWindow();
	virtual void FrameResized(float, float);
	virtual void Pulse();

	virtual void MouseDown(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 mode, const BMessage*);
	virtual void MouseUp(BPoint where);

	virtual void MessageReceived(BMessage* msg);
	virtual void Draw(BRect updateRect);

			void ResetPosition();
			void SetLocationFromFrame(double frameX, double frameY);
			void ZoomFractal(double originX, double originY, double zoomFactor);
			void ZoomFractalFromFrame(double frameOriginX, double frameOriginY,
				double zoomFactor);
			void ImportBitsAndInvalidate();
			void RedrawFractal();
			void UpdateSize();
			void CreateDisplayBitmap(uint16 width, uint16 height);
			FractalEngine* fFractalEngine;

private:
			BRect GetDragFrame();

	BPoint fSelectStart;
	BPoint fSelectEnd;
	bool fSelecting;
	uint32 fMouseButtons;

	BBitmap* fDisplayBitmap;

	double fLocationX;
	double fLocationY;
	double fSize;
};


FractalView::FractalView()
	:
	BView(NULL, B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED),
	fFractalEngine(NULL),
	fSelecting(false),
	fDisplayBitmap(NULL),
	fLocationX(0),
	fLocationY(0),
	fSize(0.005)
{
	SetHighColor(make_color(255, 255, 255, 255));
}


FractalView::~FractalView()
{
	delete fDisplayBitmap;
}


void FractalView::ResetPosition()
{
	fLocationX = 0;
	fLocationY = 0;
	fSize = 0.005;
}


void FractalView::AttachedToWindow()
{
	fFractalEngine = new FractalEngine(this, Window());
	fFractalEngine->Run();
	TRACE("Attached to window\n");
}


void FractalView::FrameResized(float, float)
{
	TRACE("Frame Resize\n");
	UpdateSize();
}


void FractalView::UpdateSize()
{
	TRACE("Update Size\n");
	BMessage msg(FractalEngine::MSG_RESIZE);

	uint16 width = (uint16)Frame().Width()+1;
	uint16 height = (uint16)Frame().Height()+1;

	msg.AddUInt16("width", width);
	msg.AddUInt16("height", height);

	CreateDisplayBitmap(width,height);

	msg.AddPointer("bitmap",fDisplayBitmap);

	fFractalEngine->PostMessage(&msg); // Create the new buffer
}


void FractalView::CreateDisplayBitmap(uint16 width,uint16 height)
{
	delete fDisplayBitmap;
	fDisplayBitmap = NULL;
	TRACE("width %u height %u\n",width,height);
	BRect rect(0, 0, width, height);
	fDisplayBitmap = new BBitmap(rect, B_RGB24);
}


BRect FractalView::GetDragFrame()
{
	BRect dragZone = BRect(std::min(fSelectStart.x, fSelectEnd.x),
		std::min(fSelectStart.y, fSelectEnd.y),
		std::max(fSelectStart.x, fSelectEnd.x),
		std::max(fSelectStart.y, fSelectEnd.y)),
		frame = Frame();
	float width = dragZone.Width(),
		height = width * (frame.Height() / frame.Width());

	float x1 = fSelectStart.x, y1 = fSelectStart.y,	x2, y2;
	if (fSelectStart.x < fSelectEnd.x)
		x2 = x1 + width;
	else
		x2 = x1 - width;
	if (fSelectStart.y < fSelectEnd.y)
		y2 = y1 + height;
	else
		y2 = y1 - height;
	return BRect(x1, y1, x2, y2);
}


void FractalView::MouseDown(BPoint where)
{
	fSelecting = true;
	fSelectStart = where;
	fMouseButtons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", (int32*)&fMouseButtons);
}


void FractalView::MouseMoved(BPoint where, uint32 mode, const BMessage*)
{
	if (fSelecting) {
		fSelectEnd = where;
		Invalidate();
	}
}


void FractalView::SetLocationFromFrame(double frameX,double frameY)
{
	BRect frame = Frame();

	fLocationX = ((frameX - frame.Width() / 2) * fSize + fLocationX);
	fLocationY = ((frameY - frame.Height() / 2) * -fSize + fLocationY);
		// -fSize because is in raster coordinates (y swapped)
}


void FractalView::ZoomFractalFromFrame(double frameOriginX, double frameOriginY,
	double zoomFactor)
{
	BRect frame = Frame();

	ZoomFractal((frameOriginX - frame.Width() / 2) * fSize + fLocationX,
		 (frameOriginY - frame.Height() / 2) * -fSize + fLocationY,
		 zoomFactor);
}


void FractalView::ZoomFractal(double originX, double originY, double zoomFactor)
{
	double deltaX = originX - fLocationX;
	double deltaY = originY - fLocationY;

	TRACE("oX %g oY %g zoom %g\n", originX, originY, zoomFactor);

	deltaX /= zoomFactor;
	deltaY /= zoomFactor;

	fLocationX = originX - deltaX;
	fLocationY = originY - deltaY;
	fSize /= zoomFactor;
}


void FractalView::MouseUp(BPoint where)
{
	BRect frame = Frame();
	fSelecting = false;
	if (fabs(fSelectStart.x - where.x) > 4) {
		fSelectEnd = where;
		BRect dragFrame = GetDragFrame();
		BPoint lt = dragFrame.LeftTop();
		float centerX = lt.x + dragFrame.Width() / 2,
			centerY = lt.y + dragFrame.Height() / 2;

		SetLocationFromFrame(centerX, centerY);
		fSize = std::fabs((dragFrame.Width() * fSize) / frame.Width());
	} else {
		if (fMouseButtons & B_PRIMARY_MOUSE_BUTTON) {
			SetLocationFromFrame(where.x, where.y);
			ZoomFractal(fLocationX, fLocationY, 2);
		} else {
			ZoomFractal(fLocationX, fLocationY, 0.5);
		}
	}
	RedrawFractal();
}


void FractalView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case B_MOUSE_WHEEL_CHANGED: {
		float change = msg->FindFloat("be:wheel_delta_y");
		BPoint where;
		GetMouse(&where, NULL);
		double zoomFactor;
		if (change < 0)
			zoomFactor = 3.0/2.0;
		else
			zoomFactor = 2.0/3.0;
		ZoomFractalFromFrame(where.x, where.y, zoomFactor);

		RedrawFractal();
		break;
	}

	case FractalEngine::MSG_BUFFER_CREATED:
		TRACE("Got buffer created msg.\n");

		ImportBitsAndInvalidate();
		RedrawFractal();
		break;

	case FractalEngine::MSG_RENDER_COMPLETE:
		TRACE("Got render complete msg.\n");

		Window()->SetPulseRate(0);
		ImportBitsAndInvalidate();
		break;

	default:
		BView::MessageReceived(msg);
		break;
	}
}


void FractalView::Pulse()
{
	ImportBitsAndInvalidate();
}


void FractalView::ImportBitsAndInvalidate()
{
	TRACE("Importing bits...\n");

	fFractalEngine->WriteToBitmap(fDisplayBitmap);
	Invalidate();
}


void FractalView::RedrawFractal()
{
	Window()->SetPulseRate(1000000 / MANDELBROT_VIEW_REFRESH_FPS);
	BMessage message(FractalEngine::MSG_RENDER);
	message.AddDouble("locationX", fLocationX);
	message.AddDouble("locationY", fLocationY);
	message.AddDouble("size", fSize);
	fFractalEngine->PostMessage(&message);
}


void FractalView::Draw(BRect updateRect)
{
	DrawBitmap(fDisplayBitmap, updateRect, updateRect);
	if (fSelecting)
		StrokeRect(GetDragFrame());
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
		MSG_ITER_16384,

		MSG_SUBSAMPLING_1,
		MSG_SUBSAMPLING_2,
		MSG_SUBSAMPLING_3,
		MSG_SUBSAMPLING_4
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
	BMenuBar* menuBar = new BMenuBar("MenuBar");
	BMenu* setMenu;
	BMenu* paletteMenu;
	BMenu* iterMenu;
	BMenu* subsamplingMenu;
	BLayoutBuilder::Menu<>(menuBar)
		.AddMenu(B_TRANSLATE("File"))
			.AddItem(B_TRANSLATE("About"), B_ABOUT_REQUESTED)
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
		.AddMenu(B_TRANSLATE("Subsampling"))
			.GetMenu(subsamplingMenu)
			.AddItem(B_TRANSLATE("1 (none)"), MSG_SUBSAMPLING_1)
			.AddItem("4", MSG_SUBSAMPLING_2)
			.AddItem("9", MSG_SUBSAMPLING_3)
			.AddItem("16", MSG_SUBSAMPLING_4)
		.End()
	.End();
	setMenu->SetRadioMode(true);
	setMenu->FindItem(MSG_MANDELBROT_SET)->SetMarked(true);
	paletteMenu->SetRadioMode(true);
	paletteMenu->FindItem(MSG_ROYAL_PALETTE)->SetMarked(true);
	iterMenu->SetRadioMode(true);
	iterMenu->FindItem(MSG_ITER_1024)->SetMarked(true);
	subsamplingMenu->SetRadioMode(true);
	subsamplingMenu->FindItem(MSG_SUBSAMPLING_2)->SetMarked(true);

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
		fFractalView->ResetPosition(); \
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
#define HANDLE_SUBSAMPLING(uiwhat, id) \
	case uiwhat: { \
		BMessage msg(FractalEngine::MSG_SET_SUBSAMPLING); \
		msg.AddUInt8("subsampling", id); \
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

	HANDLE_SUBSAMPLING(MSG_SUBSAMPLING_1, 1)
	HANDLE_SUBSAMPLING(MSG_SUBSAMPLING_2, 2)
	HANDLE_SUBSAMPLING(MSG_SUBSAMPLING_3, 3)
	HANDLE_SUBSAMPLING(MSG_SUBSAMPLING_4, 4)

	case B_ABOUT_REQUESTED: {
		BAboutWindow* wind = new BAboutWindow("Mandelbrot",
			"application/x-vnd.Haiku-Mandelbrot");

		const char* authors[] = {
			"Augustin Cavalier <waddlesplash>",
			"kerwizzy",
			NULL
		};
		wind->AddCopyright(2016, "Haiku, Inc.");
		wind->AddAuthors(authors);
		wind->Show();
		break;
	}

	default:
		BWindow::MessageReceived(msg);
		break;
	}
}
#undef HANDLE_SET
#undef HANDLE_PALETTE
#undef HANDLE_ITER
#undef HANDLE_SUBSAMPLING


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
