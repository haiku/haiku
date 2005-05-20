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
			void	_InvalidateBitmapRect(BRect r);
			void	_DrawCross(BPoint where, rgb_color c);

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
		_InvalidateBitmapRect(old | fBitmapRect);
	} else
		BView::MessageReceived(message);
}

// Draw
void
TestView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_COPY);
	DrawBitmap(fBitmap, fBitmap->Bounds(), fBitmapRect);

	// indicate the frame to see any errors in the drawing code
	rgb_color red = (rgb_color){ 255, 0, 0, 255 };
	_DrawCross(fBitmapRect.LeftTop() + BPoint(-1.0, -1.0), red);
	_DrawCross(fBitmapRect.RightTop() + BPoint(1.0, -1.0), red);
	_DrawCross(fBitmapRect.LeftBottom() + BPoint(-1.0, 1.0), red);
	_DrawCross(fBitmapRect.RightBottom() + BPoint(1.0, 1.0), red);

	// text
	SetDrawingMode(B_OP_ALPHA);
	const char* message = "Click and drag to move and resize the bitmap!";
	BPoint textPos(20.0, 30.0);
	SetHighColor(255, 255, 255, 180);
	DrawString(message, textPos);
	SetHighColor(0, 0, 0, 180);
	DrawString(message, textPos + BPoint(-1.0, -1.0));
}

bool
hit_test(BPoint where, BPoint p)
{
	BRect r(p, p);
	r.InsetBy(-7.0, -7.0);
	return r.Contains(where);
}

bool
hit_test(BPoint where, BPoint a, BPoint b)
{
	BRect r(a, b);
	if (a.x == b.x)
		r.InsetBy(-7.0, 0.0);
	else
		r.InsetBy(0.0, -7.0);
	return r.Contains(where);
}

// MouseDown
void
TestView::MouseDown(BPoint where)
{
	fTracking = TRACKING_NONE;

	// check if we hit a corner
	if (hit_test(where, fBitmapRect.LeftTop()))
		fTracking = TRACKING_LEFT_TOP;
	else if (hit_test(where, fBitmapRect.RightTop()))
		fTracking = TRACKING_RIGHT_TOP;
	else if (hit_test(where, fBitmapRect.LeftBottom()))
		fTracking = TRACKING_LEFT_BOTTOM;
	else if (hit_test(where, fBitmapRect.RightBottom()))
		fTracking = TRACKING_RIGHT_BOTTOM;
	// check if we hit a side
	else if (hit_test(where, fBitmapRect.LeftTop(), fBitmapRect.RightTop()))
		fTracking = TRACKING_TOP;
	else if (hit_test(where, fBitmapRect.LeftTop(), fBitmapRect.LeftBottom()))
		fTracking = TRACKING_LEFT;
	else if (hit_test(where, fBitmapRect.RightTop(), fBitmapRect.RightBottom()))
		fTracking = TRACKING_RIGHT;
	else if (hit_test(where, fBitmapRect.LeftBottom(), fBitmapRect.RightBottom()))
		fTracking = TRACKING_BOTTOM;
	// check if we hit inside the rect
	else if (fBitmapRect.Contains(where))
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
		BRect old = fBitmapRect;
		BPoint offset = where - fLastMousePos;
		switch (fTracking) {
			case TRACKING_LEFT_TOP:
				fBitmapRect.Set(fBitmapRect.left + offset.x,
								fBitmapRect.top + offset.y,
								fBitmapRect.right,
								fBitmapRect.bottom);
				break;
			case TRACKING_RIGHT_BOTTOM:
				fBitmapRect.Set(fBitmapRect.left,
								fBitmapRect.top,
								fBitmapRect.right + offset.x,
								fBitmapRect.bottom + offset.y);
				break;
			case TRACKING_LEFT_BOTTOM:
				fBitmapRect.Set(fBitmapRect.left + offset.x,
								fBitmapRect.top,
								fBitmapRect.right,
								fBitmapRect.bottom + offset.y);
				break;
			case TRACKING_RIGHT_TOP:
				fBitmapRect.Set(fBitmapRect.left,
								fBitmapRect.top + offset.y,
								fBitmapRect.right + offset.x,
								fBitmapRect.bottom);
				break;
			case TRACKING_LEFT:
				fBitmapRect.Set(fBitmapRect.left + offset.x,
								fBitmapRect.top,
								fBitmapRect.right,
								fBitmapRect.bottom);
				break;
			case TRACKING_TOP:
				fBitmapRect.Set(fBitmapRect.left,
								fBitmapRect.top + offset.y,
								fBitmapRect.right,
								fBitmapRect.bottom);
				break;
			case TRACKING_RIGHT:
				fBitmapRect.Set(fBitmapRect.left,
								fBitmapRect.top,
								fBitmapRect.right + offset.x,
								fBitmapRect.bottom);
				break;
			case TRACKING_BOTTOM:
				fBitmapRect.Set(fBitmapRect.left,
								fBitmapRect.top,
								fBitmapRect.right,
								fBitmapRect.bottom + offset.y);
				break;
			case TRACKING_ALL:
			default:
				fBitmapRect.OffsetBy(offset);
				break;
		}
		fLastMousePos = where;
		if (old != fBitmapRect)
			_InvalidateBitmapRect(old | fBitmapRect);
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

// _InvalidateBitmapRect
void
TestView::_InvalidateBitmapRect(BRect r)
{
	r.InsetBy(-4.0, -4.0);
	Invalidate(r);
}

// _DrawCross
void
TestView::_DrawCross(BPoint where, rgb_color c)
{
	BeginLineArray(4);
		AddLine(BPoint(where.x, where.y - 3),
				BPoint(where.x, where.y - 1), c);
		AddLine(BPoint(where.x, where.y + 1),
				BPoint(where.x, where.y + 3), c);
		AddLine(BPoint(where.x - 3, where.y),
				BPoint(where.x - 1, where.y), c);
		AddLine(BPoint(where.x + 1, where.y),
				BPoint(where.x + 3, where.y), c);
	EndLineArray();
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
	BButton* control = new BButton(b, "button", "Reset", new BMessage(MSG_RESET),
								   B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	view->AddChild(control);
	control->SetTarget(view);

	window->Show();
}

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x.vnd-Haiku.BitmapDrawing");

	BRect frame(50.0, 50.0, 400.0, 250.0);
	show_window(frame, "Bitmap Drawing");

	app->Run();

	delete app;
	return 0;
}
