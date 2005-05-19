// main.cpp

#include <stdio.h>
#include <string.h>

#include "Application.h"
#include "Bitmap.h"
#include "Button.h"
#include "Message.h"
#include "View.h"
#include "Window.h"

#include "bitmap.h"

enum {
	MSG_RESET	= 'rset',	
};

class TestView : public BView {

 public:
					TestView(BRect frame, const char* name,
							  uint32 resizeFlags, uint32 flags)
						: BView(frame, name, resizeFlags, flags),
						  fBitmap(new BBitmap(BRect(0, 0, kBitmapWidth - 1, kBitmapHeight -1),
						  					  0, kBitmapFormat)),
						  fBitmapRect(),
						  fTracking(TRACKING_NONE),
						  fLastMousePos(-1.0, -1.0)
					{
						SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
						SetLowColor(ViewColor());
						uint32 size = min_c((uint32)fBitmap->BitsLength(), sizeof(kBitmapBits));
						memcpy(fBitmap->Bits(), kBitmapBits, size);
						_ResetRect();
					}

	virtual	void	MessageReceived(BMessage* message);

	virtual	void	Draw(BRect updateRect);

	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseUp(BPoint where);
	virtual	void	MouseMoved(BPoint where, uint32 transit,
							   const BMessage* dragMessage);

 private:
			void	_ResetRect();

	BBitmap*		fBitmap;
	BRect			fBitmapRect;

	enum {
		TRACKING_NONE = 0,

		TRACKING_LEFT,
		TRACKING_RIGHT,
		TRACKING_TOP,
		TRACKING_BOTTOM,

		TRACKING_LEFT_TOP,
		TRACKING_RIGHT_TOP,
		TRACKING_LEFT_BOTTOM,
		TRACKING_RIGHT_BOTTOM,

		TRACKING_ALL
	};

	uint32			fTracking;
	BPoint			fLastMousePos;
};

// MessageReceived
void
TestView::MessageReceived(BMessage* message)
{
	if (message->what == MSG_RESET) {
		BRect old = fBitmapRect;
		_ResetRect();
		Invalidate(old | fBitmapRect);
	} else
		BView::MessageReceived(message);
}

// Draw
void
TestView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_COPY);
	DrawBitmap(fBitmap, fBitmap->Bounds(), fBitmapRect);

	// text
	SetDrawingMode(B_OP_OVER);
	SetHighColor(0, 0, 0, 255);
	const char* message = "Click and drag to move the bitmap!";
	DrawString(message, BPoint(20.0, 30.0));
}

// MouseDown
void
TestView::MouseDown(BPoint where)
{
	fTracking = TRACKING_ALL;
	fLastMousePos = where;
}

// MouseUp
void
TestView::MouseUp(BPoint where)
{
	fTracking = TRACKING_NONE;
}

// MouseMoved
void
TestView::MouseMoved(BPoint where, uint32 transit,
					 const BMessage* dragMessage)
{
	if (fTracking > TRACKING_NONE) {
		switch (fTracking) {
			case TRACKING_ALL:
			default:
				BRect old = fBitmapRect;
				fBitmapRect.OffsetBy(where - fLastMousePos);
				fLastMousePos = where;
				Invalidate(old | fBitmapRect);
				break;
		}
	}
}

// _ResetRect
void
TestView::_ResetRect()
{
	fBitmapRect = fBitmap->Bounds();
	fBitmapRect.OffsetBy(floorf((Bounds().Width() - fBitmapRect.Width()) / 2.0 + 0.5),
						 floorf((Bounds().Height() - fBitmapRect.Height()) / 2.0 + 0.5));
}

// show_window
void
show_window(BRect frame, const char* name)
{
	BWindow* window = new BWindow(frame, name,
								  B_TITLED_WINDOW,
								  B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BView* view = new TestView(window->Bounds(), "test", B_FOLLOW_ALL,
							   B_WILL_DRAW/* | B_FULL_UPDATE_ON_RESIZE*/);

	window->AddChild(view);
	BRect b(0.0, 0.0, 60.0, 15.0);
	b.OffsetTo(5.0, view->Bounds().bottom - (b.Height() + 15.0));
	BButton* control = new BButton(b, "button", "Reset", new BMessage(MSG_RESET));
	view->AddChild(control);
	control->SetTarget(view);

	window->Show();
}

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x.vnd-Haiku.BitmapDrawing");

	BRect frame(50.0, 50.0, 300.0, 250.0);
	show_window(frame, "Bitmap Drawing");

	app->Run();

	delete app;
	return 0;
}
