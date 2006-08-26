// main.cpp

#include <stdio.h>

#include "Application.h"
#include "Message.h"
#include "Button.h"
#include "View.h"
#include "Window.h"

#define MSG_COPY_BITS 'cbts'

class TestView : public BView {

	enum {
		TRACKING_NONE,
		TRACKING_SOURCE,
		TRACKING_DEST
	};

 public:
					TestView(BRect frame, const char* name,
							  uint32 resizeFlags, uint32 flags)
						: BView(frame, name, resizeFlags, flags),
						  fTracking(TRACKING_NONE),
						  fCopyBitsJustCalled(false)
					{
						fSourceRect.Set(frame.left, frame.top,
									    (frame.left + frame.right) / 2,
									    frame.bottom);
						fDestRect.Set((frame.left + frame.right) / 2,
									  frame.top,
									  frame.right, frame.bottom);
						fSourceRect.InsetBy(10.0, 10.0);
						fDestRect.InsetBy(10.0, 10.0);

						SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
						SetLowColor(ViewColor());
					}

	virtual	void	MessageReceived(BMessage* message);

	virtual	void	Draw(BRect updateRect);

	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseUp(BPoint where);
	virtual	void	MouseMoved(BPoint where, uint32 transit,
							   const BMessage* dragMessage);

 private:
			void	_TrackMouse(BPoint where);

			BRect	fSourceRect;
			BRect	fDestRect;

			uint32	fTracking;

			bool	fCopyBitsJustCalled;
};

// MessageReceived
void
TestView::MessageReceived(BMessage* message)
{
	if (message->what == MSG_COPY_BITS) {
		printf("MSG_COPY_BITS\n");
		fSourceRect.PrintToStream();
		fDestRect.PrintToStream();
		CopyBits(fSourceRect, fDestRect);
		fCopyBitsJustCalled = true;
	} else
		BView::MessageReceived(message);
}

// Draw
void
TestView::Draw(BRect updateRect)
{
	if (fCopyBitsJustCalled) {
		printf("TestView::Draw(%.1f, %.1f, %.1f, %.1f) after CopyBits()\n",
			   updateRect.left, updateRect.top, updateRect.right, updateRect.bottom);
		fCopyBitsJustCalled = false;
	}

	// background
//	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
//	FillRect(updateRect);

	BRect r(Bounds());

	// draw some pattern with lines
	float width = r.Width();
	float height = r.Height();
	int32 lineCount = 20;
	SetPenSize(2.0);
	for (int32 i = 0; i < lineCount; i++) {
		SetHighColor(255, (255 / lineCount) * i, 255 - (255 / lineCount) * i);
		StrokeLine(BPoint(r.left + (width / lineCount) * i, r.top),
				   BPoint(r.left, r.top + (height / lineCount) * i));
		StrokeLine(BPoint(r.right - (width / lineCount) * i, r.bottom),
				   BPoint(r.right, r.bottom - (height / lineCount) * i));
	}
	StrokeLine(BPoint(r.left, r.bottom), BPoint(r.right, r.top));

	// source and dest rect
	SetPenSize(1.0);

	SetHighColor(0, 255, 0, 255);
	StrokeRect(fSourceRect);

	SetHighColor(0, 0, 255, 255);
	StrokeRect(fDestRect);

	// text
	SetHighColor(128, 0, 50, 255);

	const char* message = "Left-Click and drag";
	width = StringWidth(message);
	BPoint p(r.left + r.Width() / 2.0 - width / 2.0,
			 r.top + r.Height() / 2.0 - 50.0);

	DrawString(message, p);

	message = "to set source rect!";
	width = StringWidth(message);
	p.x = r.left + r.Width() / 2.0 - width / 2.0;
	p.y += 20;

	DrawString(message, p);

	message = "Right-Click and drag";
	width = StringWidth(message);
	p.x = r.left + r.Width() / 2.0 - width / 2.0;
	p.y += 30.0;

	DrawString(message, p);

	message = "to set destination rect!";
	width = StringWidth(message);
	p.x = r.left + r.Width() / 2.0 - width / 2.0;
	p.y += 20;

	DrawString(message, p);
}

// MouseDown
void
TestView::MouseDown(BPoint where)
{
	BMessage* message = Window()->CurrentMessage();
	uint32 buttons;
	if (message && message->FindInt32("buttons", (int32*)&buttons) >= B_OK) {
		if (buttons & B_PRIMARY_MOUSE_BUTTON) {
			fTracking = TRACKING_SOURCE;
			fSourceRect.left = where.x;
			fSourceRect.top = where.y;
		}
		if (buttons & B_SECONDARY_MOUSE_BUTTON) {
			fTracking = TRACKING_DEST;
			fDestRect.left = where.x;
			fDestRect.top = where.y;
		}
		Invalidate();
		_TrackMouse(where);
	}
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
		_TrackMouse(where);
	}
}

float
min4(float a, float b, float c, float d)
{
	return min_c(a, min_c(b, min_c(c, d)));
}

float
max4(float a, float b, float c, float d)
{
	return max_c(a, max_c(b, max_c(c, d)));
}

// _TrackMouse
void
TestView::_TrackMouse(BPoint where)
{
	BRect before;
	BRect after;
	bool invalidate = false;
	switch (fTracking) {
		case TRACKING_SOURCE:
			before = fSourceRect;
			fSourceRect.right = where.x;
			fSourceRect.bottom = where.y;
			after = fSourceRect;
			invalidate = true;
			break;
		case TRACKING_DEST:
			before = fDestRect;
			fDestRect.right = where.x;
			fDestRect.bottom = where.y;
			after = fDestRect;
			invalidate = true;
			break;
	}
	if (invalidate) {
		BRect dirty(min4(before.left, before.right, after.left, after.right),
					min4(before.top, before.bottom, after.top, after.bottom),
					max4(before.left, before.right, after.left, after.right),
					max4(before.top, before.bottom, after.top, after.bottom));
		Invalidate(dirty);
	}
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
	BRect b(0.0, 0.0, 50.0, 15.0);
	b.OffsetTo(5.0, view->Bounds().bottom - (b.Height() + 15.0));
	BButton* control = new BButton(b, "button", "Copy", new BMessage(MSG_COPY_BITS));
	view->AddChild(control);
	control->SetTarget(view);

	// test CopyBits() on top of children
	b = BRect(80, 130, 130, 160);
	BView* child = new BView(b, "some child", B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, 0);
	child->SetViewColor(255, 0, 0);
	view->AddChild(child);

	b = BRect(136, 127, 158, 140);
	child = new BView(b, "some other child", B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, 0);
	child->SetViewColor(255, 255, 0);
	view->AddChild(child);

	window->Show();
}

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x.vnd-Haiku.CopyBits");

	BRect frame(50.0, 50.0, 300.0, 250.0);
	show_window(frame, "CopyBits Test");

	app->Run();

	delete app;
	return 0;
}
