// main.cpp

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <View.h>
#include <Window.h>

#include "bitmap.h"

enum {
	MSG_RESET	= 'rset',	
	MSG_TICK	= 'tick',
};

#define SPEED 2.0

// random_number_between
float
random_number_between(float v1, float v2)
{
	if (v1 < v2)
		return v1 + fmod(rand() / 1000.0, (v2 - v1));
	else if (v2 < v1)
		return v2 + fmod(rand() / 1000.0, (v1 - v2));
	return v1;
}

// TestView
class TestView : public BView {

 public:
					TestView(BRect frame, const char* name,
							 uint32 resizeFlags, uint32 flags);

	virtual	void	AttachedToWindow();
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

	struct point {
		double x;
		double y;
		double direction_x;
		double direction_y;
		double velocity_x;
		double velocity_y;
	};
	struct color_cycle {
		uint8 value;
		double direction;
	};

			void	_FillBitmap(point* polygon);
			void	_InitPolygon(const BRect& b, point* polygon) const;
			void	_InitColor(color_cycle* color) const;
			void	_MorphPolygon(const BRect& b, point* polygon);
			void	_MorphColor(color_cycle* color);

	BBitmap*		fBitmap;
	BView*			fOffscreenView;
	BMessageRunner*	fTicker;
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

	point			fPolygon[4];
	color_cycle		fColor[3];
};

// constructor
TestView::TestView(BRect frame, const char* name,
				   uint32 resizeFlags, uint32 flags)
	: BView(frame, name, resizeFlags, flags),
//	  fBitmap(new BBitmap(BRect(0, 0, kBitmapWidth - 1, kBitmapHeight -1), 0, kBitmapFormat)),
//	  fBitmap(new BBitmap(BRect(0, 0, 32 - 1, 8 - 1), 0, B_CMAP8)),
//	  fBitmap(new BBitmap(BRect(0, 0, 32 - 1, 8 - 1), 0, B_GRAY8)),
	  fBitmap(new BBitmap(BRect(0, 0, 199, 99), B_RGB32, true)),
//	  fBitmap(new BBitmap(BRect(0, 0, 639, 479), B_RGB32, true)),
//	  fBitmap(new BBitmap(BRect(0, 0, 639, 479), B_CMAP8, true)),
//	  fBitmap(new BBitmap(BRect(0, 0, 199, 99), B_CMAP8, true)),
//	  fBitmap(new BBitmap(BRect(0, 0, 199, 99), B_GRAY8, true)),
	  fOffscreenView(new BView(fBitmap->Bounds(), "Offscreen view",
							   B_FOLLOW_ALL, B_WILL_DRAW | B_SUBPIXEL_PRECISE)),
	  fTicker(NULL),
	  fBitmapRect(),
	  fTracking(TRACKING_NONE),
	  fLastMousePos(-1.0, -1.0)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
//	uint32 size = min_c((uint32)fBitmap->BitsLength(), sizeof(kBitmapBits));
//	memcpy(fBitmap->Bits(), kBitmapBits, size);
/*	uint8* bits = (uint8*)fBitmap->Bits();
	uint32 width = fBitmap->Bounds().IntegerWidth() + 1;
	uint32 height = fBitmap->Bounds().IntegerHeight() + 1;
	uint32 bpr = fBitmap->BytesPerRow();
printf("width: %ld, height: %ld, bpr: %ld\n", width, height, bpr);
	int32 index = 0;
	for (uint32 y = 0; y < height; y++) {
		uint8* h = bits;
		for (uint32 x = 0; x < width; x++) {
			*h = index++;
			h++;
		}
		bits += bpr;
	}
BRect a(0.0, 10.0, 20.0, 10.0);
BRect b(0.0, 10.0, 10.0, 30.0);
printf("Intersects: %d\n", a.Intersects(b));*/
	if (fBitmap->Lock()) {
		fBitmap->AddChild(fOffscreenView);
		fOffscreenView->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
		fBitmap->Unlock();
	}

	srand((long int)system_time());
	_InitPolygon(fBitmap->Bounds(), fPolygon);
	_InitColor(fColor);

	_ResetRect();
}

// AttachedToWindow
void
TestView::AttachedToWindow()
{
	BMessenger mess(this, Window());
	BMessage msg(MSG_TICK);
	fTicker = new BMessageRunner(mess, &msg, 40000LL);
}

// MessageReceived
void
TestView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_RESET: {
			BRect old = fBitmapRect;
			_ResetRect();
			_InvalidateBitmapRect(old | fBitmapRect);
			break;
		}
		case MSG_TICK:
			_MorphPolygon(fBitmap->Bounds(), fPolygon);
			_MorphColor(fColor);
			_FillBitmap(fPolygon);
			Invalidate(fBitmapRect);
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}

// Draw
void
TestView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_ALPHA);
	DrawBitmap(fBitmap, fBitmap->Bounds(), fBitmapRect);

	SetDrawingMode(B_OP_COPY);
	// background arround bitmap
	BRect topOfBitmap(updateRect.left, updateRect.top, updateRect.right, fBitmapRect.top - 1);
	if (topOfBitmap.IsValid())
		FillRect(topOfBitmap, B_SOLID_LOW);

	BRect leftOfBitmap(updateRect.left, fBitmapRect.top, fBitmapRect.left - 1, fBitmapRect.bottom);
	if (leftOfBitmap.IsValid())
		FillRect(leftOfBitmap, B_SOLID_LOW);

	BRect rightOfBitmap(fBitmapRect.right + 1, fBitmapRect.top, updateRect.right, fBitmapRect.bottom);
	if (rightOfBitmap.IsValid())
		FillRect(rightOfBitmap, B_SOLID_LOW);

	BRect bottomOfBitmap(updateRect.left, fBitmapRect.bottom + 1, updateRect.right, updateRect.bottom);
	if (bottomOfBitmap.IsValid())
		FillRect(bottomOfBitmap, B_SOLID_LOW);

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

// hit_test
bool
hit_test(BPoint where, BPoint p)
{
	BRect r(p, p);
	r.InsetBy(-5.0, -5.0);
	return r.Contains(where);
}

// hit_test
bool
hit_test(BPoint where, BPoint a, BPoint b)
{
	BRect r(a, b);
	if (a.x == b.x)
		r.InsetBy(-3.0, 0.0);
	else
		r.InsetBy(0.0, -3.0);
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

// _FillBitmap
void
TestView::_FillBitmap(point* polygon)
{
	if (fBitmap->Lock()) {
		fOffscreenView->SetDrawingMode(B_OP_COPY);
		fOffscreenView->SetHighColor(0, 0, 0, 30);
		fOffscreenView->FillRect(fOffscreenView->Bounds());

		fOffscreenView->SetDrawingMode(B_OP_ALPHA);
		fOffscreenView->SetHighColor(fColor[0].value,
									 fColor[1].value,
									 fColor[2].value,
									 30);
		fOffscreenView->SetPenSize(4);
		fOffscreenView->SetLineMode(B_BUTT_CAP, B_ROUND_JOIN);

		BPoint pointList[4];
		pointList[0].x = polygon[0].x;
		pointList[0].y = polygon[0].y;
		pointList[1].x = polygon[1].x;
		pointList[1].y = polygon[1].y;
		pointList[2].x = polygon[2].x;
		pointList[2].y = polygon[2].y;
		pointList[3].x = polygon[3].x;
		pointList[3].y = polygon[3].y;

		fOffscreenView->StrokePolygon(pointList, 4);

		fOffscreenView->Sync();
		fBitmap->Unlock();
	}
}

// _InitPolygon
void
TestView::_InitPolygon(const BRect& b, point* polygon) const
{
	polygon[0].x = b.left;
	polygon[0].y = b.top;
	polygon[0].direction_x = random_number_between(-SPEED, SPEED);
	polygon[0].direction_y = random_number_between(-SPEED, SPEED);
	polygon[0].velocity_x = 0.0;
	polygon[0].velocity_y = 0.0;
	polygon[1].x = b.right;
	polygon[1].y = b.top;
	polygon[1].direction_x = random_number_between(-SPEED, SPEED);
	polygon[1].direction_y = random_number_between(-SPEED, SPEED);
	polygon[1].velocity_x = 0.0;
	polygon[1].velocity_y = 0.0;
	polygon[2].x = b.right;
	polygon[2].y = b.bottom;
	polygon[2].direction_x = random_number_between(-SPEED, SPEED);
	polygon[2].direction_y = random_number_between(-SPEED, SPEED);
	polygon[2].velocity_x = 0.0;
	polygon[2].velocity_y = 0.0;
	polygon[3].x = b.left;
	polygon[3].y = b.bottom;
	polygon[3].direction_x = random_number_between(-SPEED, SPEED);
	polygon[3].direction_y = random_number_between(-SPEED, SPEED);
	polygon[3].velocity_x = 0.0;
	polygon[3].velocity_y = 0.0;
}

// _InitColor
void
TestView::_InitColor(color_cycle* color) const
{
	color[0].value = 0;
	color[0].direction = random_number_between(-SPEED * 4, SPEED * 4);
	color[1].value = 0;
	color[1].direction = random_number_between(-SPEED * 4, SPEED * 4);
	color[2].value = 0;
	color[2].direction = random_number_between(-SPEED * 4, SPEED * 4);
}

// morph
inline void
morph(double* value, double* direction, double* velocity, double min, double max)
{
	*value += *velocity;

	// flip direction if necessary
	if (*value < min && *direction < 0.0) {
		*direction = -*direction;
	} else if (*value > max && *direction > 0.0) {
		*direction = -*direction;
	}

	// accelerate velocity
	if (*direction < 0.0) {
		if (*velocity > *direction)
			*velocity += *direction / 10.0;
		// truncate velocity
		if (*velocity < *direction)
			*velocity = *direction;
	} else {
		if (*velocity < *direction)
			*velocity += *direction / 10.0;
		// truncate velocity
		if (*velocity > *direction)
			*velocity = *direction;
	}
}

// morph
inline void
morph(uint8* value, double* direction)
{
	int32 v = (int32)(*value + *direction);
	if (v < 0) {
		v = 0;
		*direction = -*direction;
	} else if (v > 255) {
		v = 255;
		*direction = -*direction;
	}
	*value = (uint8)v;
}

// _MorphPolygon
void
TestView::_MorphPolygon(const BRect& b, point* polygon)
{
	morph(&polygon[0].x, &polygon[0].direction_x, &polygon[0].velocity_x, b.left, b.right);
	morph(&polygon[1].x, &polygon[1].direction_x, &polygon[1].velocity_x, b.left, b.right);
	morph(&polygon[2].x, &polygon[2].direction_x, &polygon[2].velocity_x, b.left, b.right);
	morph(&polygon[3].x, &polygon[3].direction_x, &polygon[3].velocity_x, b.left, b.right);
	morph(&polygon[0].y, &polygon[0].direction_y, &polygon[0].velocity_y, b.top, b.bottom);
	morph(&polygon[1].y, &polygon[1].direction_y, &polygon[1].velocity_y, b.top, b.bottom);
	morph(&polygon[2].y, &polygon[2].direction_y, &polygon[2].velocity_y, b.top, b.bottom);
	morph(&polygon[3].y, &polygon[3].direction_y, &polygon[3].velocity_y, b.top, b.bottom);
}

// _MorphColor
void
TestView::_MorphColor(color_cycle* color)
{
	morph(&color[0].value, &color[0].direction);
	morph(&color[1].value, &color[1].direction);
	morph(&color[2].value, &color[2].direction);
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
	control->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	window->Show();
}

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x.vnd-Haiku.BitmapDrawing");

//	BRect frame(10.0, 30.0, 790.0, 590.0);
	BRect frame(10.0, 30.0, 330.0, 220.0);
	show_window(frame, "BitmapDrawing");

	app->Run();

	delete app;
	return 0;
}
